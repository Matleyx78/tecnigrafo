// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/entity_reader.hpp"

#include "tg/core/polyline.hpp"
#include "tg/script/report.hpp"
#include "tg/script/text_format.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

core::Entity read(std::string_view text) {
    std::variant<core::Entity, EntityError> entity = readEntity(text);
    if (const auto* error = std::get_if<EntityError>(&entity)) {
        ADD_FAILURE() << text << ": " << error->message;
        return core::Polyline{};
    }
    return std::get<core::Entity>(std::move(entity));
}

EntityError failure(std::string_view text) {
    std::variant<core::Entity, EntityError> entity = readEntity(text);
    if (!std::holds_alternative<EntityError>(entity)) {
        ADD_FAILURE() << "expected an error for: " << text;
        return {};
    }
    return std::get<EntityError>(std::move(entity));
}

std::string written(const core::Entity& entity) {
    std::string text;
    writeEntity(entity, text);
    return text;
}

TEST(WriteEntity, PolylineWithStraightAndArcSegments) {
    core::Polyline polyline;
    polyline.addVertex({0.0, 0.0});
    polyline.addVertex({10.0, 0.0}, 0.5);
    polyline.addVertex({20.0, -2.5});
    EXPECT_EQ(written(polyline),
              "LWPOLYLINE closed=no vertex=0,0 vertex=10,0 bulge=0.5 vertex=20,-2.5");
    polyline.setClosed(true);
    EXPECT_EQ(written(polyline),
              "LWPOLYLINE closed=yes vertex=0,0 vertex=10,0 bulge=0.5 vertex=20,-2.5");
    EXPECT_EQ(written(core::Polyline{}), "LWPOLYLINE closed=no");
    EXPECT_EQ(dxfType(polyline), "LWPOLYLINE");
}

TEST(ReadEntity, ReadsWhatTheReportWrites) {
    const core::Polyline expected{{{{0.0, 0.0}, 0.0}, {{10.0, 0.0}, 0.5}, {{20.0, -2.5}, -1.0}},
                                  true};
    EXPECT_EQ(read("LWPOLYLINE closed=yes vertex=0,0 vertex=10,0 bulge=0.5 vertex=20,-2.5 "
                   "bulge=-1"),
              core::Entity{expected});
}

/// Random numbers reproducible on every platform: std::mt19937 is fully
/// specified by the standard, its distributions are not.
class Numbers {
public:
    explicit Numbers(std::uint32_t seed) : m_engine(seed) {}

    double between(double low, double high) {
        return low + ((high - low) * static_cast<double>(m_engine()) / kRange);
    }

private:
    static constexpr double kRange = 4294967296.0;  // 2^32, mt19937's range

    std::mt19937 m_engine;
};

TEST(ReadEntity, ADefinitionWrittenAsInTheReportReadsBackWithoutLosses) {
    Numbers random(20261018);
    for (int i = 0; i < 100; ++i) {
        core::Polyline polyline;
        for (int vertex = 0; vertex < 1 + (i % 6); ++vertex) {
            const core::Vec2 point{random.between(-1e6, 1e6), random.between(-1e6, 1e6)};
            polyline.addVertex(point, vertex % 2 == 0 ? 0.0 : random.between(-2.0, 2.0));
        }
        polyline.setClosed(i % 3 == 0);
        const core::Entity entity{polyline};
        EXPECT_EQ(read(written(entity)), entity) << written(entity);
    }
}

core::Entity polyline(std::vector<core::PolylineVertex> vertices, bool closed) {
    return core::Polyline{std::move(vertices), closed};
}

TEST(ReadEntity, DefaultsMayBeLeftOut) {
    EXPECT_EQ(read("LWPOLYLINE"), core::Entity{core::Polyline{}});
    EXPECT_EQ(read("LWPOLYLINE vertex=1,2 vertex=3,4"),
              polyline({{{1.0, 2.0}, 0.0}, {{3.0, 4.0}, 0.0}}, false));
    // closed may come anywhere, once.
    EXPECT_EQ(read("LWPOLYLINE vertex=1,2 closed=yes"), polyline({{{1.0, 2.0}, 0.0}}, true));
}

TEST(ReadEntity, DerivedPropertiesAreIgnored) {
    EXPECT_EQ(read("LWPOLYLINE closed=no ~length=10 vertex=0,0 vertex=10,0"),
              polyline({{{0.0, 0.0}, 0.0}, {{10.0, 0.0}, 0.0}}, false));
}

TEST(ReadEntity, Errors) {
    for (const char* text : {
             "",
             "LWPOLYLINES",
             "lwpolyline",
             "LWPOLYLINE closed=maybe",
             "LWPOLYLINE closed=yes closed=no",
             "LWPOLYLINE vertex=1",
             "LWPOLYLINE vertex=1, 2",
             "LWPOLYLINE bulge=1 vertex=0,0",
             "LWPOLYLINE vertex=0,0 bulge=1 bulge=2",
             "LWPOLYLINE vertex=0,0 bulge=x",
             "LWPOLYLINE color=red",
             "LWPOLYLINE vertex",
             "LWPOLYLINE =1",
             "LWPOLYLINE \"open",
         }) {
        const EntityError error = failure(text);
        EXPECT_FALSE(error.message.empty()) << text;
        EXPECT_FALSE(error.notImplemented) << text;
    }
}

TEST(ReadEntity, TypesOfTheStageNotImplementedYet) {
    for (const char* text : {"LINE start=0,0 end=10,0", "CIRCLE center=0,0 radius=10",
                             "ARC center=0,0 radius=10 start-angle=0 end-angle=90",
                             "TEXT position=0,0 value=\"Hello\""}) {
        const EntityError error = failure(text);
        EXPECT_TRUE(error.notImplemented) << text;
    }
}

TEST(TextFormat, QuoteEscapesQuotesBackslashesAndControlCharacters) {
    EXPECT_EQ(quoteText("Hello world"), "\"Hello world\"");
    EXPECT_EQ(quoteText(""), "\"\"");
    EXPECT_EQ(quoteText("say \"hi\""), "\"say \\\"hi\\\"\"");
    EXPECT_EQ(quoteText("a\\b"), "\"a\\\\b\"");
    EXPECT_EQ(quoteText("tab\there"), "\"tab\\u{09}here\"");
    EXPECT_EQ(quoteText("\x7F"), "\"\\u{7F}\"");
    EXPECT_EQ(quoteText("è 100%%c ⌀"), "\"è 100%%c ⌀\"");  // UTF-8 as it is
}

TEST(TextFormat, UnquoteReadsWhatQuoteWrites) {
    for (const char* text : {"", "Hello world", "say \"hi\"", "a\\b", "tab\there", "\x01\x1F\x7F",
                             "è 100%%c ⌀", "  spaces  "}) {
        EXPECT_EQ(unquoteText(quoteText(text)), std::string{text}) << text;
    }
}

TEST(TextFormat, UnquoteAcceptsAnyCodePoint) {
    EXPECT_EQ(unquoteText(R"("\u{2300}")"), std::string{"\u2300"});
    EXPECT_EQ(unquoteText(R"("\u{d8}")"), std::string{"\u00D8"});
    EXPECT_EQ(unquoteText(R"("\u{1F600}")"), std::string{"\U0001F600"});
    EXPECT_EQ(unquoteText(R"("\u{41}B")"), std::string{"AB"});
}

TEST(TextFormat, UnquoteRejectsMalformedStrings) {
    for (const char* text : {"", R"(")", "abc", R"("abc)", R"(abc")", R"("a"b")", R"("\")",
                             R"("\x")", R"("\u{}")", R"("\u{110000}")", R"("\u{D800}")",
                             R"("\u{1234567}")", R"("\u{12")", R"("\u12")", R"("\u{G}")"}) {
        EXPECT_FALSE(unquoteText(text).has_value()) << text;
    }
}
}  // namespace
}  // namespace tg::script
