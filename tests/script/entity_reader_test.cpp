// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/entity_reader.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/color.hpp"
#include "tg/core/document.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"
#include "tg/script/report.hpp"
#include "tg/script/text_format.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
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

TEST(ReadEntity, PolylineWidthsAreWrittenAndReadWithoutLoss) {
    const std::string definition =
        "LWPOLYLINE closed=yes width=0.5 vertex=0,0 bulge=1 "
        "start-width=1 end-width=2 vertex=10,0 end-width=3";
    const auto entity = read(definition);
    EXPECT_EQ(written(entity), definition);
    const auto& polyline = std::get<core::Polyline>(entity);
    EXPECT_EQ(polyline.constantWidth(), 0.5);
    EXPECT_EQ(polyline.vertices()[0].startWidth, 1);
    EXPECT_EQ(polyline.vertices()[1].endWidth, 3);
    for (const auto* invalid : {"LWPOLYLINE width=1 width=2", "LWPOLYLINE start-width=1",
                                "LWPOLYLINE vertex=0,0 start-width=1 start-width=2",
                                "LWPOLYLINE vertex=0,0 end-width=1 end-width=2",
                                "LWPOLYLINE vertex=0,0 end-width=nan", "LWPOLYLINE width=inf"}) {
        EXPECT_FALSE(failure(invalid).message.empty());
    }
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
             "LWPOLYLINE colour=red",
             "LWPOLYLINE vertex",
             "LWPOLYLINE =1",
             "LWPOLYLINE \"open",
         }) {
        const EntityError error = failure(text);
        EXPECT_FALSE(error.message.empty()) << text;
        EXPECT_FALSE(error.notImplemented) << text;
    }
}

std::string writtenRecord(const core::Entity& entity, core::RawGroups placement,
                          bool flipped = false) {
    core::EntityExtras extras;
    extras.placement = std::move(placement);
    extras.flippedExtrusion = flipped;
    std::string text;
    writeEntity(core::EntityRecord{{1}, entity, std::move(extras), 1}, text);
    return text;
}

// Quota, thickness and extrusion of stage 0.5 (docs/verifica/rapporto.md):
// only those different from their default, quotas first, before ~box.
TEST(WriteEntity, PlacementPropertiesOfARecordAsRead) {
    const core::Line line{{0, 0}, {10, 5}};
    EXPECT_EQ(writtenRecord(line, {{30, " 1.000"}, {31, "2"}}),
              "LINE start=0,0 end=10,5 start-z=1 end-z=2");
    EXPECT_EQ(writtenRecord(line, {{30, "0.0"}, {31, "0"}, {39, "0"}, {230, "1.0"}}),
              "LINE start=0,0 end=10,5");
    EXPECT_EQ(writtenRecord(core::Circle{{-5, 5}, 2},
                            {{30, "4"}, {210, "0"}, {220, "0"}, {230, "-1"}}, true),
              "CIRCLE center=-5,5 radius=2 z=4 extrusion=0,0,-1");
    const std::string arc =
        writtenRecord(core::Arc{{0, 0}, 1, 0, 1}, {{39, "-0.75"}, {210, "1e-12"}});
    EXPECT_TRUE(arc.ends_with(" thickness=-0.75 extrusion=1e-12,0,1")) << arc;
    core::Polyline polyline;
    polyline.addVertex({0, 0});
    EXPECT_EQ(writtenRecord(polyline, {{39, "2"}, {38, "3"}}),
              "LWPOLYLINE closed=no vertex=0,0 elevation=3 thickness=2");
    core::Text text;
    text.value = "A";
    const std::string line1 = writtenRecord(text, {{39, "0.5"}, {30, "1"}, {31, "1"}});
    EXPECT_NE(line1.find(" value=\"A\" z=1 alignment-z=1 thickness=0.5 ~box="), std::string::npos)
        << line1;
    // Without extras the record is written as the entity alone.
    std::string alone;
    writeEntity(core::EntityRecord{{1}, line}, alone);
    EXPECT_EQ(alone, written(line));
}

TEST(ReadEntity, PlacementPropertiesAreCheckedAndAccepted) {
    EXPECT_EQ(read("LINE start=0,0 end=10,5 start-z=1 end-z=2 thickness=3 extrusion=0,0,-1"),
              (core::Entity{core::Line{{0, 0}, {10, 5}}}));
    EXPECT_EQ(read("CIRCLE center=-5,5 radius=2 z=4 extrusion=0,0,-1"),
              (core::Entity{core::Circle{{-5, 5}, 2}}));
    EXPECT_EQ(read("LWPOLYLINE closed=no vertex=0,0 elevation=3"),
              read("LWPOLYLINE closed=no vertex=0,0"));
    EXPECT_EQ(read("TEXT value=\"A\" position=0,0 z=1 alignment-z=1"),
              read("TEXT value=\"A\" position=0,0"));
    EXPECT_EQ(failure("LINE start=0,0 end=1,1 z=1").message, "unknown property 'z' of LINE");
    EXPECT_EQ(failure("CIRCLE center=0,0 radius=1 elevation=1").message,
              "unknown property 'elevation' of CIRCLE");
    EXPECT_EQ(failure("ARC center=0,0 radius=1 start-angle=0 end-angle=9 z=a").message,
              "z needs a number, found 'a'");
    EXPECT_EQ(failure("CIRCLE center=0,0 radius=1 extrusion=0,1").message,
              "extrusion needs a vector x,y,z, found '0,1'");
    EXPECT_EQ(failure("CIRCLE center=0,0 radius=1 thickness=1 thickness=2").message,
              "thickness is given more than once");
}

// ---------------------------------------------------------------------------
// Properties, layers and linetypes of stage 0.6 (package P5)

EntityDefinition definitionOf(std::string_view text) {
    std::variant<EntityDefinition, EntityError> read = readEntityDefinition(text);
    if (const auto* error = std::get_if<EntityError>(&read)) {
        ADD_FAILURE() << text << ": " << error->message;
        return {core::Line{}, {}, std::nullopt};
    }
    return std::get<EntityDefinition>(std::move(read));
}

TEST(ReadEntityDefinition, PropertiesInAnyOrderAfterTheGeometry) {
    const EntityDefinition definition = definitionOf(
        "LINE lineweight=0.5 start=0,0 layer=\"Muri esterni\" color=red end=1,1 "
        "ltscale=2 linetype=\"ACAD_ISO02W100\"");
    EXPECT_EQ(definition.entity, (core::Entity{core::Line{{0.0, 0.0}, {1.0, 1.0}}}));
    WrittenProperties expected;
    expected.layer = "Muri esterni";
    expected.color = core::Color::fromIndex(1);
    expected.linetype = "ACAD_ISO02W100";
    expected.linetypeScale = 2.0;
    expected.lineweight = 50;
    EXPECT_EQ(definition.properties, expected);
    EXPECT_FALSE(definition.linetypeGeneration.has_value());
    // readEntity() accepts them too and keeps the geometry only.
    EXPECT_EQ(read("LINE start=0,0 end=1,1 color=bylayer layer=\"0\""),
              (core::Entity{core::Line{{0.0, 0.0}, {1.0, 1.0}}}));
}

TEST(ReadEntityDefinition, EveryWayOfWritingAColorAndALineweight) {
    for (const auto& [text, color] : std::vector<std::pair<std::string_view, core::Color>>{
             {"bylayer", core::Color::byLayer()},
             {"256", core::Color::byLayer()},
             {"BYBLOCK", core::Color::byBlock()},
             {"0", core::Color::byBlock()},
             {"255", core::Color::fromIndex(255)},
             {"white", core::Color::fromIndex(7)},
             {"0,128,255", core::Color::fromRgb({0, 128, 255})},
         }) {
        const EntityDefinition definition =
            definitionOf("CIRCLE center=0,0 radius=1 color=" + std::string{text});
        EXPECT_EQ(definition.properties.color, color) << text;
    }
    for (const auto& [text, lineweight] : std::vector<std::pair<std::string_view, int>>{
             {"bylayer", core::lineweight::kByLayer},
             {"byblock", core::lineweight::kByBlock},
             {"default", core::lineweight::kDefault},
             {"0", 0},
             {"0.35mm", 35},
             {"2.11", 211},
         }) {
        const EntityDefinition definition =
            definitionOf("CIRCLE center=0,0 radius=1 lineweight=" + std::string{text});
        EXPECT_EQ(definition.properties.lineweight, lineweight) << text;
    }
}

TEST(ReadEntityDefinition, LinetypeGenerationOfAPolyline) {
    EXPECT_EQ(
        definitionOf("LWPOLYLINE closed=yes linetype-generation=yes vertex=0,0").linetypeGeneration,
        true);
    EXPECT_EQ(definitionOf("LWPOLYLINE linetype-generation=no").linetypeGeneration, false);
}

TEST(ReadEntityDefinition, ErrorsOfTheProperties) {
    for (const std::string_view text : {
             "LINE start=0,0 end=1,1 layer=Muri",
             "LINE start=0,0 end=1,1 layer=\"\"",
             R"(LINE start=0,0 end=1,1 layer="A" layer="B")",
             "LINE start=0,0 end=1,1 linetype=Continuous",
             "LINE start=0,0 end=1,1 color=bylayr",
             "LINE start=0,0 end=1,1 color=256,0,0",
             "LINE start=0,0 end=1,1 color=1 color=2",
             "LINE start=0,0 end=1,1 ltscale=0",
             "LINE start=0,0 end=1,1 ltscale=-1",
             "LINE start=0,0 end=1,1 ltscale=x",
             "LINE start=0,0 end=1,1 lineweight=0.33",
             "LINE start=0,0 end=1,1 lineweight=-1",
             "LINE start=0,0 end=1,1 linetype-generation=yes",
             "LWPOLYLINE linetype-generation=maybe",
             "LWPOLYLINE linetype-generation=yes linetype-generation=no",
         }) {
        std::variant<EntityDefinition, EntityError> read = readEntityDefinition(text);
        ASSERT_TRUE(std::holds_alternative<EntityError>(read)) << text;
        EXPECT_FALSE(std::get<EntityError>(read).message.empty()) << text;
    }
}

TEST(ReadProperties, OnlyTheProperties) {
    std::variant<WrittenProperties, EntityError> read =
        readProperties("color=2 layer=\"Muri\" ltscale=0.5");
    ASSERT_TRUE(std::holds_alternative<WrittenProperties>(read));
    WrittenProperties expected;
    expected.layer = "Muri";
    expected.color = core::Color::fromIndex(2);
    expected.linetypeScale = 0.5;
    EXPECT_EQ(std::get<WrittenProperties>(read), expected);
    for (const std::string_view text : {"vertex=0,0", "layer", "color=2 color=3", "layer=\"A"}) {
        EXPECT_TRUE(std::holds_alternative<EntityError>(readProperties(text))) << text;
    }
}

TEST(ResolveProperties, NamesOfTheDocumentWithoutCaseAndDefaults) {
    core::Document document = core::Document::newDrawing();
    core::Layer walls;
    walls.name = "Muri";
    walls.linetype = document.continuousLinetype();
    const core::LayerId wallsId = document.addLayer(walls);

    // Left out: the defaults, never the current values.
    core::CurrentProperties current = document.current();
    current.layer = wallsId;
    current.color = core::Color::fromIndex(3);
    document.setCurrent(current);
    auto resolved = resolveProperties({}, document);
    ASSERT_TRUE(std::holds_alternative<core::EntityProperties>(resolved));
    core::EntityProperties defaults;
    defaults.layer = document.layerZero();
    defaults.linetype = document.byLayerLinetype();
    EXPECT_EQ(std::get<core::EntityProperties>(resolved), defaults);

    WrittenProperties written;
    written.layer = "MURI";
    written.linetype = "continuous";
    resolved = resolveProperties(written, document);
    ASSERT_TRUE(std::holds_alternative<core::EntityProperties>(resolved));
    EXPECT_EQ(std::get<core::EntityProperties>(resolved).layer, wallsId);
    EXPECT_EQ(std::get<core::EntityProperties>(resolved).linetype, document.continuousLinetype());

    // applyProperties() changes only what is written.
    WrittenProperties scale;
    scale.linetypeScale = 3.0;
    const auto applied = applyProperties(scale, document.current(), document);
    ASSERT_TRUE(std::holds_alternative<core::EntityProperties>(applied));
    core::CurrentProperties expected = document.current();
    expected.linetypeScale = 3.0;
    EXPECT_EQ(std::get<core::EntityProperties>(applied), expected);

    WrittenProperties unknown;
    unknown.layer = "Nessuno";
    EXPECT_TRUE(std::holds_alternative<EntityError>(resolveProperties(unknown, document)));
    unknown = {};
    unknown.linetype = "Nessuno";
    EXPECT_TRUE(std::holds_alternative<EntityError>(resolveProperties(unknown, document)));
}

TEST(ReadLayerDefinition, EveryValueAndTheDefaults) {
    auto read = readLayerDefinition(
        "LAYER name=\"Muri esterni\" color=1 linetype=\"ACAD_ISO02W100\" lineweight=0.5 off frozen "
        "locked noplot");
    ASSERT_TRUE(std::holds_alternative<LayerDefinition>(read));
    EXPECT_EQ(std::get<LayerDefinition>(read),
              (LayerDefinition{"Muri esterni", core::Color::fromIndex(1), "ACAD_ISO02W100", 50,
                               true, true, true, true}));
    // Left out: the values of a new layer 0, in any order.
    read = readLayerDefinition("LAYER locked name=\"A\"");
    ASSERT_TRUE(std::holds_alternative<LayerDefinition>(read));
    EXPECT_EQ(std::get<LayerDefinition>(read),
              (LayerDefinition{"A", core::Color::fromIndex(7), "Continuous",
                               core::lineweight::kDefault, false, false, true, false}));
    read = readLayerDefinition("LAYER name=\"B\" color=10,20,30 lineweight=default");
    ASSERT_TRUE(std::holds_alternative<LayerDefinition>(read));
    EXPECT_EQ(std::get<LayerDefinition>(read).color, core::Color::fromRgb({10, 20, 30}));
}

TEST(ReadLayerDefinition, Errors) {
    for (const std::string_view text : {
             "LAYER",
             "LAYER color=1",
             "LAYER name=A",
             "LAYER name=\"a/b\"",
             "LAYER name=\" a\"",
             R"(LAYER name="A" name="B")",
             "LAYER name=\"A\" color=bylayer",
             "LAYER name=\"A\" color=0",
             R"(LAYER name="A" linetype="ByLayer")",
             R"(LAYER name="A" linetype="byblock")",
             "LAYER name=\"A\" lineweight=bylayer",
             "LAYER name=\"A\" lineweight=0.33",
             "LAYER name=\"A\" off off",
             "LAYER name=\"A\" on",
             "LAYER name=\"A\" ltscale=2",
             "LTYPE name=\"A\"",
         }) {
        EXPECT_TRUE(std::holds_alternative<EntityError>(readLayerDefinition(text))) << text;
    }
}

TEST(ReadLinetypeDefinition, PatternDescriptionAndStandardNames) {
    auto read =
        readLinetypeDefinition(R"(LTYPE name="Tratteggio" pattern=5,-2,0,-2 description="mio")");
    ASSERT_TRUE(std::holds_alternative<core::LinetypeDefinition>(read));
    const auto& dashed = std::get<core::LinetypeDefinition>(read);
    EXPECT_EQ(dashed.name, "Tratteggio");
    EXPECT_EQ(dashed.description, "mio");
    EXPECT_EQ(dashed.pattern.elements, (std::vector<double>{5.0, -2.0, 0.0, -2.0}));

    read = readLinetypeDefinition("LTYPE name=\"Pieno\" pattern=continuous");
    ASSERT_TRUE(std::holds_alternative<core::LinetypeDefinition>(read));
    EXPECT_TRUE(std::get<core::LinetypeDefinition>(read).pattern.continuous());
    EXPECT_EQ(std::get<core::LinetypeDefinition>(read).description, "");

    // A standard linetype by its name alone, in any case; the name is kept.
    for (const std::string_view name : {"ACAD_ISO02W100", "acad_iso10w100"}) {
        read = readLinetypeDefinition("LTYPE name=\"" + std::string{name} + "\"");
        ASSERT_TRUE(std::holds_alternative<core::LinetypeDefinition>(read)) << name;
        const auto& standard = std::get<core::LinetypeDefinition>(read);
        const std::optional<core::LinetypeDefinition> expected = core::standardLinetype(name);
        ASSERT_TRUE(expected.has_value());
        EXPECT_EQ(standard.name, name);
        EXPECT_EQ(standard.pattern, expected.value_or(core::LinetypeDefinition{}).pattern);
        EXPECT_EQ(standard.description, expected.value_or(core::LinetypeDefinition{}).description);
    }
}

TEST(ReadLinetypeDefinition, Errors) {
    for (const std::string_view text : {
             "LTYPE",
             "LTYPE pattern=1,-1",
             "LTYPE name=\"Mio\"",
             "LTYPE name=\"Mio\" pattern=",
             "LTYPE name=\"Mio\" pattern=5,,2",
             "LTYPE name=\"Mio\" pattern=a",
             "LTYPE name=\"Mio\" pattern=1 pattern=2",
             "LTYPE name=\"Mio\" pattern=1 description=mio",
             "LTYPE name=\"Mio\" pattern=1 off",
             "LAYER name=\"Mio\"",
         }) {
        EXPECT_TRUE(std::holds_alternative<EntityError>(readLinetypeDefinition(text))) << text;
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
