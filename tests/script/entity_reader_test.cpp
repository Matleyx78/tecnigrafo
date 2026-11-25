// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/entity_reader.hpp"

#include "new_entities_format.hpp"
#include "text.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/color.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/hatch_patterns.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text.hpp"
#include "tg/script/report.hpp"
#include "tg/script/text_format.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
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

// ---------------------------------------------------------------------------
// The entity types of stage 0.8 (docs/tappe/0.8/PT9-rapporto-entita-nuove.md).
// Until the package of a type is merged readEntity() refuses it, so the
// lines are read back with the reader of the seven types.

std::variant<core::Entity, EntityError> readNew(std::string_view text) {
    const std::optional<std::vector<std::string_view>> tokens = detail::splitTokens(text);
    if (!tokens || tokens->empty()) {
        return EntityError{"no tokens"};
    }
    const std::vector<std::string_view> properties(tokens->begin() + 1, tokens->end());
    auto read = detail::readNewEntity(tokens->front(), properties);
    if (!read) {
        return EntityError{"not a type of the 0.8"};
    }
    return std::move(*read);
}

/// Writes `entity`, reads the line back and checks that it writes the same
/// line; returns the entity read.
core::Entity readsBack(const core::Entity& entity) {
    const std::string line = written(entity);
    std::variant<core::Entity, EntityError> read = readNew(line);
    if (const auto* error = std::get_if<EntityError>(&read)) {
        ADD_FAILURE() << line << ": " << error->message;
        return entity;
    }
    EXPECT_EQ(written(std::get<core::Entity>(read)), line);
    return std::get<core::Entity>(std::move(read));
}

core::Text textOf(std::string_view value) {
    core::Text text;
    text.position = {1.0, 2.0};
    text.height = 1.8;
    text.value = value;
    return text;
}

std::vector<core::Entity> entitiesOfStageEight() {
    std::vector<core::Entity> entities;
    entities.emplace_back(core::Solid{{{{0.0, 0.0}, {10.0, 0.0}, {0.0, 5.0}, {10.0, 5.0}}}});
    entities.emplace_back(core::Ellipse{{1.0, 2.0}, {10.0, 0.0}, 0.5, 0.0, core::kTwoPi});
    entities.emplace_back(core::Ellipse{{1.0, 2.0}, {0.0, -3.0}, 1.0, core::kPi, 0.25});

    core::MText mtext;
    mtext.width = 60.0;
    mtext.value = R"(Prima\Pseconda "citata")";
    entities.emplace_back(mtext);
    mtext.attachment = core::MTextAttachment::MiddleCenter;
    mtext.style = "Note";
    mtext.lineSpacing = 2.0;
    mtext.exactSpacing = true;
    mtext.directionByStyle = true;
    entities.emplace_back(mtext);

    entities.emplace_back(core::AttributeDefinition{textOf("N"), "TG_PIN1", "", {}});
    entities.emplace_back(
        core::AttributeDefinition{textOf(""), "TG_REF", "Riferimento", {true, false, true, true}});

    core::Insert insert;
    insert.block = "TG_EL_CONTACT_NO";
    insert.insertion = {10.0, 20.0};
    entities.emplace_back(insert);
    insert.scaleX = -1.0;
    insert.scaleY = 2.5;
    insert.rotation = core::kPi / 2.0;
    entities.emplace_back(insert);

    core::Dimension dimension;
    dimension.definition.linePoint = {50.0, 10.0};
    dimension.definition.point13 = {0.0, 0.0};
    dimension.definition.point14 = {50.0, 0.0};
    dimension.definition.point15 = {7.0, 8.0};
    dimension.definition.point16 = {3.0, 4.0};
    dimension.presentation.textPoint = {25.0, 10.625};
    dimension.value.measurement = 50.0;
    for (const core::DimensionKind kind :
         {core::DimensionKind::Linear, core::DimensionKind::Aligned, core::DimensionKind::Angular,
          core::DimensionKind::Radius, core::DimensionKind::Diameter, core::DimensionKind::Other}) {
        dimension.definition.kind = kind;
        entities.emplace_back(dimension);
    }
    dimension.definition.kind = core::DimensionKind::Other;
    dimension.definition.dxfType = 6;
    dimension.presentation.style = "Elettrico";
    dimension.presentation.textOverride = "<> mm";
    dimension.presentation.textRotation = core::kPi / 4.0;
    dimension.presentation.userTextPosition = true;
    dimension.presentation.overrides = {{"DIMASZ", "2"}, {"DIMTXT", "3.5"}};
    entities.emplace_back(dimension);

    core::Hatch hatch;
    hatch.pattern = "ANSI31";
    core::HatchLoop outer;
    outer.flags = 1U | 2U;
    outer.polyline =
        core::Polyline{{{{0.0, 0.0}, 0.5}, {{10.0, 0.0}, 0.0}, {{10.0, 10.0}, 0.0}}, true};
    core::HatchLoop hole;
    hole.flags = 16U | 4U;
    hole.edges = {{core::Arc{{5.0, 5.0}, 2.0, 0.0, core::kTwoPi}},
                  {core::Line{{0.0, 0.0}, {1.0, 1.0}}},
                  {core::Arc{{0.0, 0.0}, 1.0, core::kPi, core::kPi / 2.0}}};
    hatch.loops = {outer, hole};
    hatch.seeds = {{1.0, 1.0}, {2.0, 2.0}};
    entities.emplace_back(hatch);
    hatch.solid = true;
    hatch.doubled = true;
    hatch.angle = core::kPi / 6.0;
    hatch.scale = 0.5;
    hatch.islands = core::IslandStyle::Ignore;
    hatch.loops.clear();
    hatch.seeds.clear();
    entities.emplace_back(hatch);
    return entities;
}

TEST(ReadNewEntity, EveryLineWrittenReadsBackTheSame) {
    for (const core::Entity& entity : entitiesOfStageEight()) {
        readsBack(entity);
    }
}

TEST(ReadNewEntity, LinesWithoutAnglesReadBackTheSameEntity) {
    const std::vector<core::Entity> entities = entitiesOfStageEight();
    for (const std::size_t index : {0U, 1U, 3U, 5U, 6U, 7U}) {
        EXPECT_EQ(readsBack(entities.at(index)), entities.at(index)) << written(entities.at(index));
    }
    // A whole circle of a boundary reads back as written: 0 to 2*pi.
    const core::Entity& hatch = entities.at(entities.size() - 2);
    const core::Entity read = readsBack(hatch);
    const auto& edge = std::get<core::Hatch>(read).loops.at(1).edges.at(0);
    EXPECT_EQ(std::get<core::Arc>(edge.curve), (core::Arc{{5.0, 5.0}, 2.0, 0.0, core::kTwoPi}));
}

TEST(ReadNewEntity, DefaultsMayBeLeftOut) {
    const auto entity = [](std::string_view text) {
        std::variant<core::Entity, EntityError> read = readNew(text);
        if (const auto* error = std::get_if<EntityError>(&read)) {
            ADD_FAILURE() << text << ": " << error->message;
            return core::Entity{core::Line{}};
        }
        return std::get<core::Entity>(std::move(read));
    };
    EXPECT_EQ(std::get<core::Ellipse>(entity("ELLIPSE center=0,0 major-axis=1,0 ratio=1")),
              (core::Ellipse{{0.0, 0.0}, {1.0, 0.0}, 1.0, 0.0, core::kTwoPi}));
    core::MText mtext;
    mtext.value = "A";
    EXPECT_EQ(std::get<core::MText>(entity(R"(MTEXT insertion=0,0 value="A")")), mtext);
    core::AttributeDefinition definition{textOf("N"), "T", "", {}};
    definition.text.position = {0.0, 0.0};
    definition.text.height = 2.5;
    EXPECT_EQ(std::get<core::AttributeDefinition>(
                  entity(R"(ATTDEF tag="T" position=0,0 value="N" ~box=whatever)")),
              definition);
    core::Insert insert;
    insert.block = "B";
    EXPECT_EQ(std::get<core::Insert>(entity(R"(INSERT block="B" insertion=0,0)")), insert);
    const auto angular = std::get<core::Dimension>(
        entity("DIMENSION kind=angular first=0,0;1,0 second=2,2;0,1 arc-point=3,3 "
               "text-point=1,1 measurement=90"));
    EXPECT_EQ(angular.definition.linePoint, (core::Vec2{2.0, 2.0}));
    EXPECT_EQ(angular.definition.point15, (core::Vec2{0.0, 1.0}));
    EXPECT_EQ(angular.definition.dxfType, 2);
    EXPECT_EQ(angular.presentation.style, "ISO-25");
    EXPECT_FALSE(angular.presentation.textRotation.has_value());
    core::Hatch hatch;
    hatch.pattern = "NET";
    // With the families -HATCH gives NET in a new drawing (PT23, point 5).
    hatch.patternLines = core::predefinedPatternLines("NET", 1.0, 0.0, core::kMetricMeasurement);
    EXPECT_EQ(std::get<core::Hatch>(entity(R"(HATCH pattern="NET")")), hatch);
}

TEST(ReadNewEntity, AHatchGetsTheFamiliesOfAGeneratedPattern) {
    // Decision D1 on #960 (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md,
    // point 5): the families of -HATCH in a new drawing, with origin 0,0.
    const auto families = [](std::string_view text) {
        std::variant<core::Entity, EntityError> read = readNew(text);
        if (const auto* error = std::get_if<EntityError>(&read)) {
            ADD_FAILURE() << text << ": " << error->message;
            return std::vector<core::PatternLine>{};
        }
        return std::get<core::Hatch>(std::get<core::Entity>(read)).patternLines;
    };
    const std::vector<core::PatternLine> ansi31 =
        families(R"(HATCH pattern="ANSI31" solid=no angle=0 scale=1)");
    ASSERT_EQ(ansi31.size(), 1U);
    EXPECT_NEAR(core::radiansToDegrees(ansi31[0].angle), 45.0, 1e-12);
    EXPECT_EQ(ansi31[0].origin, (core::Vec2{0.0, 0.0}));
    EXPECT_NEAR(ansi31[0].offset.x, -2.245064030267288, 1e-12);
    EXPECT_NEAR(ansi31[0].offset.y, 2.245064030267288, 1e-12);
    EXPECT_TRUE(ansi31[0].dashes.empty());

    const std::vector<core::PatternLine> net = families(R"(HATCH pattern="net" scale=2)");
    ASSERT_EQ(net.size(), 2U);
    EXPECT_NEAR(net[0].offset.y, 2 * 3.175, 1e-12);
    // An angle beyond a turn is reduced first, as -HATCH reduces it.
    EXPECT_EQ(families(R"(HATCH pattern="ANSI31" angle=390)"),
              core::predefinedPatternLines("ANSI31", 1.0, core::reducedDegreesToRadians(30),
                                           core::kMetricMeasurement));
    // A doubled pattern keeps them: -HATCH does not double.
    EXPECT_EQ(families(R"(HATCH pattern="NET" double=yes)"), families(R"(HATCH pattern="NET")"));

    EXPECT_TRUE(families(R"(HATCH pattern="SOLID" solid=yes)").empty());
    EXPECT_TRUE(families(R"(HATCH pattern="ANSI31" solid=yes)").empty());
    EXPECT_TRUE(families(R"(HATCH pattern="ANSI37")").empty());
    // The origin of the pattern is checked and left out, as the quota of
    // stage 0.5: an entity read here has no XDATA.
    EXPECT_EQ(families(R"(HATCH pattern="ANSI31" origin=10,0)"),
              families(R"(HATCH pattern="ANSI31")"));
}

TEST(ReadNewEntity, AValidLineIsRefusedUntilItsTypeIsSupported) {
    for (const core::Entity& entity : entitiesOfStageEight()) {
        if (core::isSupported(entity)) {
            continue;  // its package is merged: readEntity() reads it
        }
        const std::string line = written(entity);
        const EntityError error = failure(line);
        const std::string type = line.substr(0, line.find(' '));
        EXPECT_EQ(error.message, type + " is not supported yet.");
        EXPECT_TRUE(error.notImplemented) << line;
        // With the properties of stages 0.5 and 0.6 too.
        const EntityError withProperties = failure(line + R"( thickness=1 layer="0" color=1)");
        EXPECT_EQ(withProperties.message, type + " is not supported yet.");
    }
}

TEST(ReadNewEntity, ErrorsOfTheLines) {
    struct Case {
        std::string_view text;
        std::string_view message;
    };

    for (const Case& test : {
             Case{"SOLID vertex=0,0 vertex=1,0 vertex=0,1", "a SOLID has four vertices"},
             Case{"SOLID vertex=0,0 vertex=1,0 vertex=0,1 vertex=1,1 vertex=2,2",
                  "a SOLID has four vertices"},
             Case{"SOLID vertex=0,0 vertex=1,0 vertex=0,1 vertex=x",
                  "vertex needs a point x,y, found 'x'"},
             Case{"SOLID corner=0,0", "unknown property 'corner' of SOLID"},
             Case{"ELLIPSE center=0,0 major-axis=10,0", "missing property ratio of ELLIPSE"},
             Case{"ELLIPSE center=0,0 major-axis=10,0 ratio=0.5 start=0",
                  "start and end of an ELLIPSE are given together, or left out together"},
             Case{"ELLIPSE center=0,0 major-axis=10,0 ratio=2",
                  "ratio needs a number in (0, 1], found '2'"},
             Case{"ELLIPSE center=0,0 major-axis=10,0 ratio=0",
                  "ratio needs a number above 0, found '0'"},
             Case{"ELLIPSE center=0,0 major-axis=0,0 ratio=1",
                  "major-axis needs a vector other than 0,0"},
             Case{"ELLIPSE center=0,0 center=1,1 major-axis=1,0 ratio=1",
                  "center is given more than once"},
             Case{"MTEXT insertion=0,0", "missing property value of MTEXT"},
             Case{R"(MTEXT insertion=0,0 value="a" attachment=top)",
                  "attachment needs one of top-left, top-center, top-right, middle-left, "
                  "middle-center, middle-right, bottom-left, bottom-center, bottom-right, "
                  "found 'top'"},
             Case{R"(MTEXT insertion=0,0 value="a" width=-1)",
                  "width needs a number not below 0, found '-1'"},
             Case{R"(MTEXT insertion=0,0 value="a" line-spacing-style=double)",
                  "line-spacing-style needs one of at-least, exactly, found 'double'"},
             Case{R"(ATTDEF position=0,0 value="N")", "missing property tag of ATTDEF"},
             Case{R"(ATTDEF tag="T" value="N")", "missing property position of ATTDEF"},
             Case{R"(ATTDEF tag="T" position=0,0 value="N" size=1)",
                  "unknown property 'size' of ATTDEF"},
             Case{R"(ATTDEF tag="" position=0,0 value="N")", "tag of an ATTDEF cannot be empty"},
             Case{R"(ATTDEF tag="T" position=0,0 value="N" constant=si)",
                  "constant needs yes or no, found 'si'"},
             Case{"INSERT insertion=0,0", "missing property block of INSERT"},
             Case{R"(INSERT block="B" insertion=0,0 scale=0,1)",
                  "scale needs two numbers x,y other than 0, found '0,1'"},
             Case{"DIMENSION text-point=0,0 measurement=1", "missing property kind of DIMENSION"},
             Case{"DIMENSION kind=ordinate text-point=0,0 measurement=1",
                  "kind needs one of linear, aligned, angular, radius, diameter, other, "
                  "found 'ordinate'"},
             Case{"DIMENSION kind=radius center=0,0 text-point=0,0 measurement=1",
                  "missing property point of DIMENSION"},
             Case{"DIMENSION kind=radius center=0,0 point=1,0 measurement=1",
                  "missing property text-point of DIMENSION"},
             Case{"DIMENSION kind=radius center=0,0 point=1,0 origin1=0,0 text-point=0,0 "
                  "measurement=1",
                  "origin1 is not a property of DIMENSION kind=radius"},
             Case{"DIMENSION kind=aligned origin1=0,0 origin2=1,0 line-point=1,1 rotation=0 "
                  "text-point=0,0 measurement=1",
                  "rotation is not a property of DIMENSION kind=aligned"},
             Case{"DIMENSION kind=angular line-point=9,9 first=0,0;1,0 second=2,2;0,1 "
                  "arc-point=3,3 text-point=1,1 measurement=90",
                  "line-point of an angular DIMENSION is the first point of second"},
             Case{"DIMENSION kind=angular first=0,0 second=2,2;0,1 arc-point=3,3 "
                  "text-point=1,1 measurement=90",
                  "first needs two points x1,y1;x2,y2, found '0,0'"},
             Case{"DIMENSION kind=other type=9 text-point=0,0 measurement=1",
                  "type needs an integer from 0 to 7, found '9'"},
             Case{"DIMENSION kind=other type=0 text-point=0,0 measurement=1 overrides=DIMTXT",
                  "overrides needs NAME:value items separated by ',', found 'DIMTXT'"},
             Case{"DIMENSION kind=other type=0 text-point=0,0 measurement=1 "
                  "overrides=DIMTXT:1,DIMTXT:2",
                  "the override 'DIMTXT' is given more than once"},
             Case{"HATCH loop=polyline vertex=0,0", "missing property pattern of HATCH"},
             Case{R"(HATCH pattern="A" vertex=0,0)", "vertex belongs to a loop of polyline"},
             Case{R"(HATCH pattern="A" loop=edges vertex=0,0)",
                  "vertex belongs to a loop of polyline"},
             Case{R"(HATCH pattern="A" loop=polyline circle=0,0;1)",
                  "circle belongs to a loop of edges"},
             Case{R"(HATCH pattern="A" loop=external)",
                  "loop needs exactly one of polyline and edges"},
             Case{R"(HATCH pattern="A" loop=polyline+edges)",
                  "loop needs exactly one of polyline and edges"},
             Case{R"(HATCH pattern="A" loop=inner+edges)",
                  "loop needs signals among external, outermost, derived, polyline, edges "
                  "joined by '+', found 'inner'"},
             Case{R"(HATCH pattern="A" loop=edges circle=0,0;0)",
                  "circle needs cx,cy;r with r above 0, found '0,0;0'"},
             Case{R"(HATCH pattern="A" loop=edges arc=0,0;1;90;90)",
                  "arc needs cx,cy;r;a1;a2 with different angles in [0, 360), found "
                  "'0,0;1;90;90'"},
             Case{R"(HATCH pattern="A" islands=all)",
                  "islands needs one of normal, outer, ignore, found 'all'"},
             Case{R"(HATCH pattern="A" scale=0)", "scale needs a number above 0, found '0'"},
             Case{R"(ATTRIB owner=300 position=0,0 value="N" tag="T")",
                  "ATTRIB belongs to an INSERT: it is not defined alone"},
         }) {
        const EntityError error = failure(test.text);
        EXPECT_EQ(error.message, test.message) << test.text;
        EXPECT_FALSE(error.notImplemented) << test.text;
    }
}

// --- DIMENSION definitions (docs/tappe/0.8/PT23-quote-lineari.md, point 9) ---

/// The dimension a definition makes: regenerated as a command makes it.
core::Dimension dimensionRead(std::string_view text) {
    const core::Entity entity = read(text);
    if (!std::holds_alternative<core::Dimension>(entity)) {
        ADD_FAILURE() << text;
        return {};
    }
    return std::get<core::Dimension>(entity);
}

/// The count of the records of each type of the graphics: lines, solids, texts.
std::vector<std::size_t> shapesOf(const core::Dimension& dimension) {
    std::vector<std::size_t> counts(3, 0);
    for (const core::EntityRecord& record : dimension.graphics) {
        if (std::holds_alternative<core::Line>(record.entity)) {
            ++counts[0];
        } else if (std::holds_alternative<core::Solid>(record.entity)) {
            ++counts[1];
        } else if (std::holds_alternative<core::MText>(record.entity)) {
            ++counts[2];
        }
    }
    return counts;
}

std::string textOf(const core::Dimension& dimension) {
    for (const core::EntityRecord& record : dimension.graphics) {
        if (const auto* text = std::get_if<core::MText>(&record.entity)) {
            return text->value;
        }
    }
    return {};
}

TEST(ReadDimension, ALinearDefinitionIsRegenerated) {
    const core::Dimension dimension = dimensionRead(
        "DIMENSION kind=linear origin1=0,0 origin2=40,0 line-point=40,10 "
        "text-point=0,0 measurement=0");
    EXPECT_EQ(dimension.value.measurement, 40.0);
    EXPECT_EQ(textOf(dimension), "40");
    EXPECT_EQ(shapesOf(dimension), (std::vector<std::size_t>{3, 2, 1}));
    const auto& extension = std::get<core::Line>(dimension.graphics.at(0).entity);
    EXPECT_EQ(extension, (core::Line{{0.0, 0.625}, {0.0, 11.25}}));
}

TEST(ReadDimension, AnAlignedDefinitionAndItsOverridesAreRegenerated) {
    const core::Dimension dimension = dimensionRead(
        "DIMENSION kind=aligned origin1=0,0 origin2=24,18 line-point=18,26 "
        "text-point=0,0 measurement=0 overrides=DIMASZ:2,DIMDEC:1,DIMZIN:0");
    EXPECT_EQ(dimension.value.measurement, 30.0);
    EXPECT_EQ(textOf(dimension), "30,0");
    EXPECT_EQ(shapesOf(dimension), (std::vector<std::size_t>{3, 2, 1}));
    // The arrow is DIMASZ long, along the line from its tip at the foot.
    const auto& arrow = std::get<core::Solid>(dimension.graphics.at(3).entity);
    const core::Vec2 base = (arrow.vertices[0] + arrow.vertices[1]) * 0.5;
    EXPECT_NEAR(core::distance(base, arrow.vertices[2]), 2.0, 1e-12);
}

TEST(ReadDimension, ZeroMeasurementIsValidAndCoincidentPointsAreNot) {
    const core::Dimension zero = dimensionRead(
        "DIMENSION kind=linear origin1=0,0 origin2=0,10 line-point=0,20 "
        "text-point=0,0 measurement=5");
    EXPECT_EQ(zero.value.measurement, 0.0);
    EXPECT_EQ(textOf(zero), "0");
    const EntityError coincident = failure(
        "DIMENSION kind=aligned origin1=1,1 origin2=1,1 line-point=5,5 "
        "text-point=0,0 measurement=0");
    EXPECT_EQ(coincident.message, "Dimension cannot be regenerated by Tecnigrafo.");
    EXPECT_FALSE(coincident.notImplemented);
}

TEST(ReadDimension, KindsNotRealizedYetAndOther) {
    // Until PT24, the kinds it realizes wait as types not supported yet.
    for (const std::string_view text : {
             "DIMENSION kind=angular first=0,0;1,0 second=0,0;0,1 arc-point=3,3 "
             "text-point=1,1 measurement=90",
             "DIMENSION kind=radius center=0,0 point=1,0 text-point=1,0 measurement=1",
             "DIMENSION kind=diameter point1=-1,0 point2=1,0 text-point=1,0 measurement=2",
         }) {
        std::variant<core::Entity, EntityError> entity = readEntity(text);
        if (std::holds_alternative<core::Entity>(entity)) {
            continue;  // PT24 is merged
        }
        const auto& error = std::get<EntityError>(entity);
        EXPECT_EQ(error.message, "DIMENSION is not supported yet.") << text;
        EXPECT_TRUE(error.notImplemented) << text;
    }
    const EntityError other = failure("DIMENSION kind=other type=6 text-point=0,0 measurement=1");
    EXPECT_EQ(other.message, "Dimension cannot be regenerated by Tecnigrafo.");
    EXPECT_FALSE(other.notImplemented);
}

}  // namespace
}  // namespace tg::script
