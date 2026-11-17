// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/report.hpp"

#include "new_entities_format.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/color.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text.hpp"
#include "tg/edit/document_file.hpp"
#include "tg/edit/editor.hpp"
#include "tg/script/entity_reader.hpp"
#include "tg/script/number_format.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// Lines, circles, arcs and texts in the format of docs/verifica/rapporto.md: written
// by writeEntity(), read back by readEntity().
namespace tg::script {
namespace {

std::string written(const core::Entity& entity) {
    std::string text;
    writeEntity(entity, text);
    return text;
}

core::Entity read(std::string_view text) {
    std::variant<core::Entity, EntityError> entity = readEntity(text);
    if (const auto* error = std::get_if<EntityError>(&entity)) {
        ADD_FAILURE() << text << ": " << error->message;
        return core::Line{};
    }
    return std::get<core::Entity>(entity);
}

EntityError failure(std::string_view text) {
    std::variant<core::Entity, EntityError> entity = readEntity(text);
    if (!std::holds_alternative<EntityError>(entity)) {
        ADD_FAILURE() << "expected an error for: " << text;
        return {};
    }
    return std::get<EntityError>(entity);
}

TEST(WriteEntity, Line) {
    const core::Entity line = core::Line{{0.0, 0.0}, {10.0, 5.0}};
    EXPECT_EQ(written(line), "LINE start=0,0 end=10,5");
    EXPECT_EQ(dxfType(line), "LINE");
}

TEST(WriteEntity, PolylineLinetypeGenerationAppearsOnlyWhenEnabled) {
    core::Polyline polyline;
    polyline.addVertex({0, 0});
    polyline.addVertex({10, 0});
    EXPECT_EQ(written(polyline), "LWPOLYLINE closed=no vertex=0,0 vertex=10,0");
    polyline.setContinuousLinetype(true);
    const std::string expected =
        "LWPOLYLINE closed=no linetype-generation=yes vertex=0,0 vertex=10,0";
    EXPECT_EQ(written(polyline), expected);
    EXPECT_EQ(read(expected), core::Entity{polyline});
}

TEST(WriteEntity, Circle) {
    const core::Entity circle = core::Circle{{5.0, 5.0}, 7.0710678118654755};
    EXPECT_EQ(written(circle), "CIRCLE center=5,5 radius=7.0710678118654755");
    EXPECT_EQ(dxfType(circle), "CIRCLE");
}

TEST(WriteEntity, ArcAnglesInDegrees) {
    const core::Entity arc = core::Arc{{0.0, 0.0}, 10.0, 0.0, core::kPi / 2.0};
    EXPECT_EQ(written(arc), "ARC center=0,0 radius=10 start-angle=0 end-angle=90");
    EXPECT_EQ(dxfType(arc), "ARC");
    // Across 0°: the end angle is smaller than the start angle.
    const core::Entity across =
        core::Arc{{0.0, 0.0}, 1.0, core::degreesToRadians(270.0), core::degreesToRadians(90.0)};
    EXPECT_EQ(written(across), "ARC center=0,0 radius=1 start-angle=270 end-angle=90");
}

TEST(ReadEntity, LineCircleAndArc) {
    EXPECT_EQ(read("LINE start=0,0 end=10,5"), (core::Entity{core::Line{{0.0, 0.0}, {10.0, 5.0}}}));
    EXPECT_EQ(read("LINE end=10,5 start=0,0"), (core::Entity{core::Line{{0.0, 0.0}, {10.0, 5.0}}}));
    EXPECT_EQ(read("CIRCLE center=25,35 radius=5"),
              (core::Entity{core::Circle{{25.0, 35.0}, 5.0}}));
    const core::Entity arc = read("ARC center=80,0 radius=10 start-angle=0 end-angle=180");
    const auto* value = std::get_if<core::Arc>(&arc);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->center, (core::Vec2{80.0, 0.0}));
    EXPECT_EQ(value->radius, 10.0);
    EXPECT_EQ(value->startAngle, 0.0);
    EXPECT_EQ(value->endAngle, core::kPi);
}

TEST(ReadEntity, DerivedPropertiesAreIgnored) {
    EXPECT_EQ(read("CIRCLE center=0,0 radius=1 ~length=6.28"),
              (core::Entity{core::Circle{{0.0, 0.0}, 1.0}}));
}

TEST(ReadEntity, WhatIsWrittenReadsBackWithoutLoss) {
    for (const std::string_view text : {
             std::string_view{"LINE start=-1.5,2 end=1e-05,3.0000000000000004"},
             std::string_view{"LINE start=50,50 end=50,50"},
             std::string_view{"CIRCLE center=100,100 radius=1000000"},
             std::string_view{"CIRCLE center=0,0 radius=1e-06"},
             std::string_view{"ARC center=0,0 radius=10 start-angle=0 end-angle=90"},
             std::string_view{"ARC center=5,-625000000 radius=625000000 start-angle=270 "
                              "end-angle=180"},
             std::string_view{"ARC center=1,2 radius=0.0001 start-angle=180 end-angle=0"},
         }) {
        EXPECT_EQ(written(read(text)), text);
    }
    // Any arc: writing and reading again gives the same text, and an arc
    // within a rounding of the original.
    const core::Arc original{{1.25, -3.5}, 2.5, 1.0, 5.5};
    const std::string text = written(original);
    const core::Entity again = read(text);
    EXPECT_EQ(written(again), text);
    const auto& arc = std::get<core::Arc>(again);
    EXPECT_EQ(arc.center, original.center);
    EXPECT_EQ(arc.radius, original.radius);
    EXPECT_NEAR(arc.startAngle, original.startAngle, 1e-15);
    EXPECT_NEAR(arc.endAngle, original.endAngle, 1e-15);
}

TEST(ReadEntity, ErrorsOfLinesCirclesAndArcs) {
    for (const std::string_view text : {
             std::string_view{"LINE start=0,0"},                    // missing end
             std::string_view{"LINE start=0,0 end=1,1 start=2,2"},  // given twice
             std::string_view{"LINE start=0 end=1,1"},              // not a point
             std::string_view{"LINE start=0,0 end=1,1 bulge=1"},    // unknown property
             std::string_view{"CIRCLE center=0,0 radius=0"},        // radius not above 0
             std::string_view{"CIRCLE center=0,0 radius=-1"},
             std::string_view{"CIRCLE center=0,0"},
             std::string_view{"ARC center=0,0 radius=1 start-angle=0"},
             std::string_view{"ARC center=0,0 radius=1 start-angle=0 end-angle=360"},
             std::string_view{"ARC center=0,0 radius=1 start-angle=-10 end-angle=90"},
             std::string_view{"ARC center=0,0 radius=1 start-angle=45 end-angle=45"},
             std::string_view{"ARC center=0,0 radius=1 start-angle=x end-angle=45"},
         }) {
        const EntityError error = failure(text);
        EXPECT_FALSE(error.notImplemented) << text;
        EXPECT_FALSE(error.message.empty()) << text;
    }
}

/// The corners written after " ~box=" at the end of `line`, compared with
/// `expected` within `tolerance`.
void expectBox(const std::string& line, const std::array<core::Vec2, 4>& expected,
               double tolerance) {
    constexpr std::string_view kBox = " ~box=";
    const std::size_t start = line.find(kBox);
    ASSERT_NE(start, std::string::npos) << line;
    std::string_view corners = std::string_view{line}.substr(start + kBox.size());
    for (const core::Vec2& corner : expected) {
        const std::size_t end = corners.find(';');
        const std::optional<core::Vec2> point = parsePoint(corners.substr(0, end));
        if (!point) {
            ADD_FAILURE() << "no point in " << line;
            return;
        }
        EXPECT_NEAR(point->x, corner.x, tolerance) << line;
        EXPECT_NEAR(point->y, corner.y, tolerance) << line;
        corners = end == std::string_view::npos ? std::string_view{} : corners.substr(end + 1);
    }
    EXPECT_TRUE(corners.empty()) << line;
}

TEST(WriteEntity, TextWithEveryPropertyAndItsBox) {
    core::Text text;
    text.value = "Hello";
    const std::string line = written(text);
    EXPECT_TRUE(line.starts_with(
        "TEXT position=0,0 alignment-point=0,0 height=2.5 rotation=0 width-factor=1 oblique=0 "
        "style=\"Standard\" mirror-x=no mirror-y=no horizontal=left vertical=baseline "
        "value=\"Hello\" ~box="))
        << line;
    // Liberation Sans 2.1.5, read independently with fontTools: "Hello" is
    // 3.312278211497516 capital heights wide, ascent 1.3158268275372604,
    // descent -0.3080198722498226.
    expectBox(
        line,
        {core::Vec2{0, -0.7700496806245565}, core::Vec2{8.28069552874379, -0.7700496806245565},
         core::Vec2{8.28069552874379, 3.289567068843151}, core::Vec2{0, 3.289567068843151}},
        1e-12);
    EXPECT_EQ(dxfType(text), "TEXT");

    core::Text full;
    full.position = {10.0, 20.0};
    full.alignmentPoint = {15.0, 25.0};
    full.height = 5.0;
    full.rotation = core::kPi;
    full.widthFactor = 0.8;
    full.obliqueAngle = core::kPi / 2.0;
    full.style = "My \"style\"";
    full.mirroredX = true;
    full.mirroredY = true;
    full.horizontal = core::TextHorizontal::Fit;
    full.vertical = core::TextVertical::Top;
    full.value = "%%c20\t\u00E8";
    const std::string fullLine = written(full);
    EXPECT_TRUE(fullLine.starts_with(
        "TEXT position=10,20 alignment-point=15,25 height=5 rotation=180 width-factor=0.8 "
        "oblique=90 style=\"My \\\"style\\\"\" mirror-x=yes mirror-y=yes horizontal=fit "
        "vertical=top value=\"%%c20\\u{09}\u00E8\" ~box="))
        << fullLine;
    expectBox(fullLine, core::textBox(full), 0.0);
}

TEST(ReadEntity, TextWithDefaults) {
    core::Text expected;
    expected.position = {1.0, 2.0};
    expected.value = "A b";
    EXPECT_EQ(read("TEXT position=1,2 value=\"A b\""), core::Entity{expected});
    // In any order, with a derived box that is ignored.
    EXPECT_EQ(read("TEXT value=\"A b\" ~box=0,0;1,0;1,1;0,1 position=1,2"), core::Entity{expected});
}

TEST(ReadEntity, EveryAlignmentName) {
    const std::array horizontal{
        std::pair{"left", core::TextHorizontal::Left},
        std::pair{"center", core::TextHorizontal::Center},
        std::pair{"right", core::TextHorizontal::Right},
        std::pair{"aligned", core::TextHorizontal::Aligned},
        std::pair{"middle", core::TextHorizontal::Middle},
        std::pair{"fit", core::TextHorizontal::Fit},
    };
    for (const auto& [name, value] : horizontal) {
        const core::Entity entity =
            read("TEXT position=0,0 value=\"\" horizontal=" + std::string{name});
        EXPECT_EQ(std::get<core::Text>(entity).horizontal, value) << name;
    }
    const std::array vertical{
        std::pair{"baseline", core::TextVertical::Baseline},
        std::pair{"bottom", core::TextVertical::Bottom},
        std::pair{"middle", core::TextVertical::Middle},
        std::pair{"top", core::TextVertical::Top},
    };
    for (const auto& [name, value] : vertical) {
        const core::Entity entity =
            read("TEXT position=0,0 value=\"\" vertical=" + std::string{name});
        EXPECT_EQ(std::get<core::Text>(entity).vertical, value) << name;
    }
}

TEST(ReadEntity, TextWithEveryPropertyReadsBackWithoutLoss) {
    for (const std::string_view text : {
             std::string_view{"TEXT position=10,20 alignment-point=15,25 height=5 rotation=180 "
                              "width-factor=0.8 oblique=-90 style=\"ISO \\\"A\\\"\" "
                              "mirror-x=yes mirror-y=no horizontal=center vertical=middle "
                              "value=\"%%c50 %%d90 %%p0.05 100%%% è\\u{0A}\""},
             std::string_view{"TEXT position=-1,0.5 alignment-point=0,0 height=1e-05 rotation=0 "
                              "width-factor=1 oblique=0 style=\"Standard\" mirror-x=no "
                              "mirror-y=yes horizontal=aligned vertical=bottom value=\"\""},
         }) {
        const core::Entity entity = read(text);
        const std::string again = written(entity);
        // The written line adds the derived box at the end.
        EXPECT_TRUE(again.starts_with(std::string{text} + " ~box=")) << again;
        EXPECT_EQ(read(again), entity);
    }
    // Any angle: writing and reading again gives a text within a rounding.
    core::Text original;
    original.position = {1.25, -3.5};
    original.rotation = 1.0;
    original.obliqueAngle = -0.25;
    original.value = "x";
    const core::Entity again = read(written(original));
    const auto& text = std::get<core::Text>(again);
    EXPECT_NEAR(text.rotation, original.rotation, 1e-15);
    EXPECT_NEAR(text.obliqueAngle, original.obliqueAngle, 1e-15);
    EXPECT_EQ(written(again), written(original));
}

TEST(ReadEntity, ErrorsOfTexts) {
    for (const std::string_view text : {
             std::string_view{"TEXT value=\"A\""},                          // missing position
             std::string_view{"TEXT position=0,0"},                         // missing value
             std::string_view{"TEXT position=0,0 value=A"},                 // not quoted
             std::string_view{R"(TEXT position=0,0 value="A" value="B")"},  // given twice
             std::string_view{"TEXT position=0,0 value=\"A\" height=0"},    // not above 0
             std::string_view{"TEXT position=0,0 value=\"A\" height=-1"},
             std::string_view{"TEXT position=0,0 value=\"A\" width-factor=0"},
             std::string_view{"TEXT position=0,0 value=\"A\" rotation=x"},
             std::string_view{"TEXT position=0,0 value=\"A\" oblique=1,2"},
             std::string_view{"TEXT position=0,0 value=\"A\" style=Standard"},
             std::string_view{"TEXT position=0,0 value=\"A\" mirror-x=1"},
             std::string_view{"TEXT position=0,0 value=\"A\" horizontal=justify"},
             std::string_view{"TEXT position=0,0 value=\"A\" vertical=Top"},
             std::string_view{"TEXT position=0 value=\"A\""},
             std::string_view{"TEXT position=0,0 value=\"A\" colour=red"},
             std::string_view{R"(TEXT position=0,0 value="A\q")"},  // unknown escape
         }) {
        const EntityError error = failure(text);
        EXPECT_FALSE(error.notImplemented) << text;
        EXPECT_FALSE(error.message.empty()) << text;
    }
}

// ---------------------------------------------------------------------------
// Layers and properties (stage 0.6, package P5)

void expectContains(std::string_view text, std::string_view part) {
    EXPECT_NE(text.find(part), std::string_view::npos) << "missing: " << part << "\nin: " << text;
}

/// One section of the report of `document`, at rest.
std::string stateOf(core::Document& document) {
    edit::Editor editor(document);
    cmd::CommandRegistry registry;
    cmd::registerBuiltinCommands(registry);
    const cmd::CommandRunner runner(registry, editor);
    std::string out;
    writeState({"final", editor, runner, runner.documentFile(), {}, std::nullopt, {}}, out);
    return out;
}

/// The lines of `report` from `from` (included) to `to` (excluded).
std::string between(const std::string& report, std::string_view from, std::string_view to) {
    const std::size_t start = report.find(from);
    const std::size_t end = report.find(to, start);
    if (start == std::string::npos || end == std::string::npos) {
        ADD_FAILURE() << "missing " << from << " or " << to << " in:\n" << report;
        return {};
    }
    return report.substr(start, end - start);
}

struct Tables {
    core::Document document{core::Document::newDrawing()};
    core::LinetypeId dashed;
    core::LayerId walls;
};

/// A new drawing with a dashed linetype and a layer "Muri esterni" that uses
/// it, with every state on.
Tables drawingWithLayers() {
    Tables tables;
    core::Linetype dashed;
    dashed.name = "ACAD_ISO02W100";
    dashed.description = "ISO dash __ __ __";
    dashed.pattern.elements = {12.0, -3.0};
    tables.dashed = tables.document.addLinetype(dashed);
    core::Layer walls;
    walls.name = "Muri esterni";
    walls.color = core::Color::fromIndex(1);
    walls.linetype = tables.dashed;
    walls.lineweight = 50;
    walls.on = false;
    walls.frozen = true;
    walls.locked = true;
    walls.plot = false;
    tables.walls = tables.document.addLayer(walls);
    return tables;
}

TEST(WriteState, ANewSessionDoesNotChangeByACharacter) {
    core::Document document = core::Document::newDrawing();
    EXPECT_EQ(stateOf(document),
              "\n[final]\n"
              "entities:\n"
              "messages:\n"
              "prompt: Command:\n"
              "preview:\n"
              "selection: none\n"
              "rectangle: none\n"
              "undo:\n"
              "redo:\n"
              "snap: on\n"
              "ortho: off\n"
              "pointer: none\n"
              "active snap: none\n"
              "statistics:\n");
    // An entity drawn in a new session has the current values, which are the
    // defaults: no property is written.
    edit::Editor editor(document);
    editor.beginTransaction("Line");
    editor.add(core::Line{{0.0, 0.0}, {10.0, 5.0}});
    editor.commit();
    expectContains(stateOf(document), "entities:\n  257 LINE start=0,0 end=10,5\nmessages:\n");
}

TEST(WriteState, ADocumentWithoutTablesWritesNoTables) {
    // A file read before package 0.6-P10 brings no table records.
    core::Document document;
    const std::string report = stateOf(document);
    for (const std::string_view field :
         {"layers:", "linetypes:", "current:", "ltscale:", "lwdisplay:"}) {
        EXPECT_EQ(report.find(field), std::string::npos) << field;
    }
}

TEST(WriteState, TablesAndCurrentValuesInTheirOrder) {
    Tables tables = drawingWithLayers();
    core::Document& document = tables.document;
    core::CurrentProperties current = document.current();
    current.layer = tables.walls;
    current.color = core::Color::fromIndex(2);
    document.setCurrent(current);
    document.setLtscale(2.0);
    document.setLwdisplay(true);
    document.setUnits(6);
    EXPECT_EQ(between(stateOf(document), "ortho:", "pointer:"),
              "ortho: off\n"
              "units: 6\n"
              "layers:\n"
              "  \"0\" color=7 linetype=\"Continuous\" lineweight=default\n"
              "  \"Muri esterni\" color=1 linetype=\"ACAD_ISO02W100\" lineweight=0.50 off frozen "
              "locked noplot\n"
              "linetypes:\n"
              "  \"ACAD_ISO02W100\" pattern=12,-3 description=\"ISO dash __ __ __\"\n"
              "current: layer=\"Muri esterni\" color=2 linetype=\"ByLayer\" ltscale=1 "
              "lineweight=bylayer\n"
              "ltscale: 2\n"
              "lwdisplay: on\n");
}

TEST(WriteState, EachFieldOnlyWhenItDiffersFromANewSession) {
    // A linetype alone: the layers are those of a new session. A name may
    // have blanks and any letter, not quotes (docs/adr/0023, point 1); a
    // description may have quotes.
    core::Document document = core::Document::newDrawing();
    core::Linetype dotted;
    dotted.name = "Punti e spazi è";
    dotted.description = "a \"dotted\" line";
    dotted.pattern.elements = {0.0, -2.0};
    document.addLinetype(dotted);
    EXPECT_EQ(between(stateOf(document), "ortho:", "pointer:"),
              "ortho: off\n"
              "linetypes:\n"
              "  \"Punti e spazi è\" pattern=0,-2 description=\"a \\\"dotted\\\" line\"\n");

    // A linetype without elements is continuous.
    core::Document plain = core::Document::newDrawing();
    core::Linetype solid;
    solid.name = "Solid";
    plain.addLinetype(solid);
    expectContains(stateOf(plain), "linetypes:\n  \"Solid\" pattern=continuous description=\"\"\n");

    // One current value: the five are written.
    core::Document scaled = core::Document::newDrawing();
    core::CurrentProperties current = scaled.current();
    current.linetypeScale = 0.5;
    scaled.setCurrent(current);
    EXPECT_EQ(between(stateOf(scaled), "ortho:", "pointer:"),
              "ortho: off\n"
              "current: layer=\"0\" color=bylayer linetype=\"ByLayer\" ltscale=0.5 "
              "lineweight=bylayer\n");

    // The layer 0 changed: the table differs from a new session.
    core::Document changed = core::Document::newDrawing();
    core::Layer zero = *changed.layer(changed.layerZero());
    zero.color = core::Color::fromRgb({255, 128, 0});
    changed.replaceLayer(zero);
    EXPECT_EQ(between(stateOf(changed), "ortho:", "pointer:"),
              "ortho: off\n"
              "layers:\n"
              "  \"0\" color=255,128,0 linetype=\"Continuous\" lineweight=default\n");
}

std::string written(const core::EntityRecord& record, const core::Document& document) {
    std::string text;
    writeEntity(record, document, text);
    return text;
}

/// A LINE of `document` with the default properties.
core::EntityRecord lineOf(const core::Document& document) {
    core::EntityRecord record{{257}, core::Line{{0.0, 0.0}, {10.0, 5.0}}};
    record.properties.layer = document.layerZero();
    record.properties.linetype = document.byLayerLinetype();
    return record;
}

TEST(WriteEntity, EachPropertyOnlyWhenNotItsDefault) {
    const Tables tables = drawingWithLayers();
    const core::Document& document = tables.document;
    EXPECT_EQ(written(lineOf(document), document), "LINE start=0,0 end=10,5");

    struct Case {
        void (*change)(core::EntityProperties&, const Tables&);
        std::string_view expected;
    };

    for (const Case& test : {
             Case{[](core::EntityProperties& p, const Tables& t) { p.layer = t.walls; },
                  " layer=\"Muri esterni\""},
             Case{[](core::EntityProperties& p, const Tables&) {
                      p.color = core::Color::fromIndex(7);
                  },
                  " color=7"},
             Case{
                 [](core::EntityProperties& p, const Tables&) { p.color = core::Color::byBlock(); },
                 " color=byblock"},
             Case{[](core::EntityProperties& p, const Tables& t) { p.linetype = t.dashed; },
                  " linetype=\"ACAD_ISO02W100\""},
             Case{[](core::EntityProperties& p, const Tables& t) {
                      p.linetype = t.document.byBlockLinetype();
                  },
                  " linetype=\"ByBlock\""},
             Case{[](core::EntityProperties& p, const Tables&) { p.linetypeScale = 2.5; },
                  " ltscale=2.5"},
             Case{[](core::EntityProperties& p, const Tables&) {
                      p.lineweight = core::lineweight::kDefault;
                  },
                  " lineweight=default"},
             Case{[](core::EntityProperties& p, const Tables&) { p.lineweight = 0; },
                  " lineweight=0"},
         }) {
        core::EntityRecord record = lineOf(document);
        test.change(record.properties, tables);
        EXPECT_EQ(written(record, document),
                  "LINE start=0,0 end=10,5" + std::string{test.expected});
    }
}

TEST(WriteEntity, PropertiesInOrderAfterThePlacementAndBeforeTheDerived) {
    const Tables tables = drawingWithLayers();
    const core::Document& document = tables.document;
    core::EntityRecord record = lineOf(document);
    record.properties = {tables.walls, core::Color::fromRgb({255, 128, 0}), tables.dashed, 35, 0.5};
    record.extras.placement = {{39, "2"}};
    EXPECT_EQ(written(record, document),
              "LINE start=0,0 end=10,5 thickness=2 layer=\"Muri esterni\" color=255,128,0 "
              "linetype=\"ACAD_ISO02W100\" ltscale=0.5 lineweight=0.35");

    core::Text text;
    text.value = "A";
    core::EntityRecord label{{258}, text};
    label.properties = lineOf(document).properties;
    label.properties.layer = tables.walls;
    const std::string line = written(label, document);
    expectContains(line, R"( value="A" layer="Muri esterni" ~box=)");
}

TEST(WriteEntity, ARecordOfADocumentWithoutTablesHasTheDefaults) {
    // Identifiers that the document does not have, as in a file read before
    // package 0.6-P10: written as the defaults, that is not at all.
    const core::Document document;
    const core::EntityRecord record{{47}, core::Line{{0.0, 0.0}, {1.0, 0.0}}};
    EXPECT_EQ(written(record, document), "LINE start=0,0 end=1,0");
}

TEST(ReadEntity, ALineOfTheReportWithPropertiesReadsBackWithoutLoss) {
    Tables tables = drawingWithLayers();
    core::Linetype spaced;
    spaced.name = "Tratto lungo è corto";
    spaced.pattern.elements = {5.0, -2.0, 0.0, -2.0};
    const core::LinetypeId spacedId = tables.document.addLinetype(spaced);
    const core::Document& document = tables.document;
    std::vector<core::EntityProperties> cases;
    cases.push_back(lineOf(document).properties);
    cases.push_back({tables.walls, core::Color::fromRgb({1, 2, 3}), spacedId, 35, 1e-05});
    cases.push_back({document.layerZero(), core::Color::fromIndex(255), document.byBlockLinetype(),
                     core::lineweight::kByBlock, 0.3333333333333333});
    cases.push_back(
        {tables.walls, core::Color::byBlock(), tables.dashed, core::lineweight::kDefault, 1.0});
    for (const core::EntityProperties& properties : cases) {
        core::EntityRecord record = lineOf(document);
        record.properties = properties;
        const std::string text = written(record, document);
        std::variant<EntityDefinition, EntityError> read = readEntityDefinition(text);
        ASSERT_TRUE(std::holds_alternative<EntityDefinition>(read)) << text;
        const EntityDefinition& definition = std::get<EntityDefinition>(read);
        EXPECT_EQ(definition.entity, record.entity) << text;
        std::variant<core::EntityProperties, EntityError> resolved =
            resolveProperties(definition.properties, document);
        ASSERT_TRUE(std::holds_alternative<core::EntityProperties>(resolved)) << text;
        EXPECT_EQ(std::get<core::EntityProperties>(resolved), properties) << text;
    }
}

TEST(SnapKindName, EveryKind) {
    EXPECT_EQ(snapKindName(core::SnapKind::Endpoint), "Endpoint");
    EXPECT_EQ(snapKindName(core::SnapKind::Midpoint), "Midpoint");
    EXPECT_EQ(snapKindName(core::SnapKind::Center), "Center");
    EXPECT_EQ(snapKindName(core::SnapKind::Insertion), "Insertion");
}

// ---------------------------------------------------------------------------
// Several documents open (stage 0.7.5, package P28b)

/// One section of the report of an empty document with file `file` among
/// `documents`.
std::string stateWith(const edit::DocumentFile& file, std::span<const ReportDocument> documents) {
    core::Document document = core::Document::newDrawing();
    edit::Editor editor(document);
    cmd::CommandRegistry registry;
    cmd::registerBuiltinCommands(registry);
    const cmd::CommandRunner runner(registry, editor);
    std::string out;
    writeState({"final", editor, runner, file, {}, std::nullopt, documents}, out);
    return out;
}

TEST(ReportDocuments, TwoDocumentsAreListedInOrderWithTheActiveMarked) {
    edit::DocumentFile plan;
    plan.setSaved("plan.dxf");
    const std::array<ReportDocument, 2> documents{{{"Drawing1.cadx", false}, {"plan.dxf", true}}};
    const std::string out = stateWith(plan, documents);
    // After "drawing:", the field of the active document, and before "pointer:".
    expectContains(out, "drawing: plan.dxf\ndocuments: Drawing1.cadx, plan.dxf (active)\npointer:");
}

TEST(ReportDocuments, TheActiveOneCanBeAnyOfThem) {
    const edit::DocumentFile untitled;
    const std::array<ReportDocument, 3> documents{
        {{"Drawing1.cadx", true}, {"plan.dxf", false}, {"Drawing2.cadx", false}}};
    expectContains(stateWith(untitled, documents),
                   "ortho: off\ndocuments: Drawing1.cadx (active), plan.dxf, Drawing2.cadx\n");
}

TEST(ReportDocuments, OneDocumentWritesNoField) {
    edit::DocumentFile plan;
    plan.setSaved("plan.dxf");
    const std::array<ReportDocument, 1> one{{{"plan.dxf", true}}};
    const std::string out = stateWith(plan, one);
    expectContains(out, "drawing: plan.dxf\npointer:");
    EXPECT_EQ(out.find("documents:"), std::string::npos);
    EXPECT_EQ(stateWith(plan, {}).find("documents:"), std::string::npos);
}

// ---------------------------------------------------------------------------
// The entity types of stage 0.8 (docs/tappe/0.8/PT9-rapporto-entita-nuove.md)

TEST(WriteNewEntity, SolidHasItsFourVerticesInDxfOrder) {
    const core::Solid solid{{{{0.0, 0.0}, {10.0, 0.0}, {0.0, 5.0}, {10.0, 5.0}}}};
    EXPECT_EQ(written(solid), "SOLID vertex=0,0 vertex=10,0 vertex=0,5 vertex=10,5");
}

TEST(WriteNewEntity, EllipseWritesStartAndEndOnlyForAnArc) {
    core::Ellipse ellipse{{0.0, 0.0}, {10.0, 0.0}, 0.5, 0.0, core::kTwoPi};
    EXPECT_EQ(written(ellipse), "ELLIPSE center=0,0 major-axis=10,0 ratio=0.5");
    ellipse.endParameter = core::kPi / 2.0;
    EXPECT_EQ(written(ellipse), "ELLIPSE center=0,0 major-axis=10,0 ratio=0.5 start=0 end=90");
    // Starting elsewhere, even up to a whole turn, is an arc.
    ellipse.startParameter = core::kPi;
    ellipse.endParameter = core::kTwoPi;
    EXPECT_EQ(written(ellipse), "ELLIPSE center=0,0 major-axis=10,0 ratio=0.5 start=180 end=360");
}

TEST(WriteNewEntity, MTextWithAndWithoutItsOptionalProperties) {
    core::MText mtext;
    mtext.width = 60.0;
    mtext.value = "Prima\\Pseconda";
    // ~lines needs the generated geometry, which comes with the package of
    // MTEXT: until then it is not written.
    EXPECT_EQ(written(mtext),
              R"(MTEXT insertion=0,0 height=2.5 width=60 attachment=top-left rotation=0 )"
              R"(style="Standard" value="Prima\\Pseconda")");
    mtext.insertion = {1.0, 2.0};
    mtext.height = 3.5;
    mtext.width = 0.0;
    mtext.attachment = core::MTextAttachment::BottomRight;
    mtext.rotation = core::kPi / 2.0;
    mtext.style = "Note";
    mtext.lineSpacing = 1.5;
    mtext.exactSpacing = true;
    mtext.directionByStyle = true;
    EXPECT_EQ(written(mtext),
              R"(MTEXT insertion=1,2 height=3.5 width=0 attachment=bottom-right rotation=90 )"
              R"(style="Note" line-spacing=1.5 line-spacing-style=exactly direction=by-style )"
              R"(value="Prima\\Pseconda")");
}

/// The TEXT properties of a text "N" at the origin, as every line writes them.
constexpr std::string_view kTextProperties =
    R"(position=0,0 alignment-point=0,0 height=1.8 rotation=0 width-factor=1 oblique=0 )"
    R"(style="Standard" mirror-x=no mirror-y=no horizontal=left vertical=baseline value="N")";

core::Text textN() {
    core::Text text;
    text.height = 1.8;
    text.value = "N";
    return text;
}

TEST(WriteNewEntity, AttributeDefinitionWritesItsFlagsOnlyWhenYes) {
    core::AttributeDefinition definition{textN(), "TG_PIN1", "", {}};
    std::string line = written(definition);
    EXPECT_EQ(line.substr(0, line.find(" ~box=")),
              "ATTDEF " + std::string{kTextProperties} + R"( tag="TG_PIN1" prompt="")");
    // The derived box is that of its TEXT.
    std::string box;
    detail::writeTextBox(definition.text, box);
    EXPECT_EQ(line.substr(line.find(" ~box=")), box);

    definition.prompt = "Pin";
    definition.flags = {true, true, true, true};
    line = written(definition);
    expectContains(
        line,
        R"( tag="TG_PIN1" prompt="Pin" invisible=yes constant=yes verify=yes preset=yes ~box=)");
    definition.flags = {false, true, false, false};
    expectContains(written(definition), R"( prompt="Pin" constant=yes ~box=)");
}

TEST(WriteNewEntity, InsertWritesBlockInsertionScaleAndRotation) {
    core::Insert insert;
    insert.block = "TG_EL_CONTACT_NO";
    insert.insertion = {10.0, 20.0};
    EXPECT_EQ(written(insert),
              R"(INSERT block="TG_EL_CONTACT_NO" insertion=10,20 scale=1,1 rotation=0)");
    insert.scaleX = -2.0;
    insert.scaleY = 0.5;
    insert.rotation = core::kPi;
    EXPECT_EQ(written(insert),
              R"(INSERT block="TG_EL_CONTACT_NO" insertion=10,20 scale=-2,0.5 rotation=180)");
}

core::Dimension dimensionOf(core::DimensionKind kind) {
    core::Dimension dimension;
    dimension.definition.kind = kind;
    dimension.definition.linePoint = {50.0, 10.0};
    dimension.definition.point13 = {0.0, 0.0};
    dimension.definition.point14 = {50.0, 0.0};
    dimension.definition.point15 = {7.0, 8.0};
    dimension.definition.point16 = {3.0, 4.0};
    dimension.presentation.textPoint = {25.0, 10.625};
    dimension.value.measurement = 50.0;
    return dimension;
}

TEST(WriteNewEntity, DimensionWritesThePointsOfItsKind) {
    constexpr std::string_view kStyle = R"(style="ISO-25")";
    constexpr std::string_view kTail = " text-point=25,10.625 measurement=50";

    struct Case {
        core::DimensionKind kind;
        std::string_view points;
    };

    for (const Case& test : {
             Case{core::DimensionKind::Linear,
                  "kind=linear style origin1=0,0 origin2=50,0 line-point=50,10 rotation=0"},
             Case{core::DimensionKind::Aligned,
                  "kind=aligned style origin1=0,0 origin2=50,0 line-point=50,10"},
             Case{core::DimensionKind::Angular,
                  "kind=angular style line-point=50,10 first=0,0;50,0 second=50,10;7,8 "
                  "arc-point=3,4"},
             Case{core::DimensionKind::Radius, "kind=radius style center=50,10 point=7,8"},
             Case{core::DimensionKind::Diameter, "kind=diameter style point1=7,8 point2=50,10"},
         }) {
        std::string expected{test.points};
        expected.replace(expected.find("style"), 5, kStyle);
        EXPECT_EQ(written(dimensionOf(test.kind)), "DIMENSION " + expected + std::string{kTail});
    }
    core::Dimension other = dimensionOf(core::DimensionKind::Other);
    other.definition.dxfType = 6;
    EXPECT_EQ(written(other),
              "DIMENSION kind=other " + std::string{kStyle} + " type=6" + std::string{kTail});
}

TEST(WriteNewEntity, DimensionWritesItsOptionalPropertiesOnlyWhenSet) {
    core::Dimension dimension = dimensionOf(core::DimensionKind::Aligned);
    dimension.presentation.style = "Elettrico";
    dimension.presentation.textRotation = 0.0;  // explicit, but 0: not written
    const std::string plain = written(dimension);
    EXPECT_EQ(plain.find("text-rotation"), std::string::npos);
    dimension.presentation.textOverride = "<> mm";
    dimension.presentation.textRotation = core::kPi / 2.0;
    dimension.presentation.userTextPosition = true;
    dimension.presentation.overrides = {{"DIMTXT", "3.5"}, {"DIMASZ", "2"}};
    EXPECT_EQ(written(dimension),
              R"(DIMENSION kind=aligned style="Elettrico" origin1=0,0 origin2=50,0 )"
              R"(line-point=50,10 text-point=25,10.625 measurement=50 text="<> mm" )"
              R"(text-rotation=90 user-text-position=yes overrides=DIMASZ:2,DIMTXT:3.5)");
}

core::Hatch hatchOfTheCard() {
    core::Hatch hatch;
    hatch.pattern = "ANSI31";
    core::HatchLoop outer;
    outer.flags = 1U | 2U;  // external, polyline
    outer.polyline = core::Polyline{
        {{{0.0, 0.0}, 0.0}, {{10.0, 0.0}, 0.0}, {{10.0, 10.0}, 0.0}, {{0.0, 10.0}, 0.0}}, true};
    core::HatchLoop hole;
    hole.edges.push_back({core::Arc{{5.0, 5.0}, 2.0, 0.0, core::kTwoPi}});
    hatch.loops = {outer, hole};
    hatch.seeds = {{1.0, 1.0}};
    return hatch;
}

TEST(WriteNewEntity, HatchWritesItsLoopsAndSeeds) {
    core::Hatch hatch = hatchOfTheCard();
    EXPECT_EQ(written(hatch),
              R"(HATCH pattern="ANSI31" solid=no angle=0 scale=1 islands=normal )"
              "loop=external+polyline vertex=0,0 vertex=10,0 vertex=10,10 vertex=0,10 "
              "loop=edges circle=5,5;2 seed=1,1");
    hatch.pattern = "SOLID";
    hatch.solid = true;
    hatch.angle = core::kPi / 4.0;
    hatch.scale = 2.0;
    hatch.doubled = true;
    hatch.islands = core::IslandStyle::Outer;
    hatch.loops[0].flags = 16U | 4U | 2U;  // outermost, derived, polyline
    hatch.loops[0].polyline = core::Polyline{{{{0.0, 0.0}, 1.0}, {{10.0, 0.0}, 0.0}}, true};
    hatch.loops[1].edges = {{core::Line{{0.0, 0.0}, {1.0, 0.0}}},
                            {core::Arc{{0.0, 0.0}, 1.0, 0.0, core::kPi}}};
    hatch.seeds.clear();
    EXPECT_EQ(written(hatch),
              R"(HATCH pattern="SOLID" solid=yes angle=45 scale=2 double=yes islands=outer )"
              "loop=outermost+derived+polyline vertex=0,0 bulge=1 vertex=10,0 "
              "loop=edges line=0,0;1,0 arc=0,0;1;0;180");
}

TEST(WriteNewEntity, PlacementAndPropertiesComeBeforeTheDerived) {
    const Tables tables = drawingWithLayers();
    const core::Document& document = tables.document;
    core::EntityRecord record{{300}, core::AttributeDefinition{textN(), "T", "", {}}};
    record.properties = lineOf(document).properties;
    record.properties.layer = tables.walls;
    record.extras.placement = {{39, "2"}};
    expectContains(written(record, document),
                   R"( tag="T" prompt="" thickness=2 layer="Muri esterni" ~box=)");
}

/// An INSERT 300 on layer 0 with two ATTRIB, 301 and 302, and a LINE 257.
struct InsertWithAttributes {
    Tables tables{drawingWithLayers()};
    core::EntityRecord line;
    core::EntityRecord insert;
};

InsertWithAttributes insertWithAttributes() {
    InsertWithAttributes drawing;
    const core::Document& document = drawing.tables.document;
    drawing.line = lineOf(document);
    core::Insert insert;
    insert.block = "TG_EL";
    insert.insertion = {10.0, 20.0};
    core::Attribute reference{{301}, textN(), "TG_REF", false, {}, lineOf(document).properties};
    reference.text.value = "-K1";
    core::Attribute pin{{302}, textN(), "TG_PIN", true, {}, lineOf(document).properties};
    pin.properties.layer = drawing.tables.walls;
    pin.extras.placement = {{30, "1"}};
    insert.attributes = {reference, pin};
    drawing.insert = core::EntityRecord{{300}, insert};
    drawing.insert.properties = lineOf(document).properties;
    return drawing;
}

TEST(WriteNewEntity, TheAttributesOfAnInsertAreEntitiesWithTheirOwner) {
    const InsertWithAttributes drawing = insertWithAttributes();
    const core::Document& document = drawing.tables.document;
    // In the order of the identifiers, whatever the order of the records.
    const std::array<const core::EntityRecord*, 2> records{&drawing.insert, &drawing.line};
    const std::vector<detail::EntityLine> lines = detail::entityLines(records, document);
    ASSERT_EQ(lines.size(), 4U);
    EXPECT_EQ(lines[0].text, "257 LINE start=0,0 end=10,5");
    EXPECT_EQ(lines[1].text, R"(300 INSERT block="TG_EL" insertion=10,20 scale=1,1 rotation=0)");
    std::string reference = R"(301 ATTRIB owner=300 )" + std::string{kTextProperties};
    reference.replace(reference.find(R"(value="N")"), 9, R"(value="-K1")");
    EXPECT_EQ(lines[2].text.substr(0, lines[2].text.find(" ~box=")),
              reference + R"( tag="TG_REF")");
    // The quotas of a TEXT and the properties of the attribute itself.
    EXPECT_EQ(lines[3].text.substr(0, lines[3].text.find(" ~box=")),
              "302 ATTRIB owner=300 " + std::string{kTextProperties} +
                  R"( tag="TG_PIN" invisible=yes z=1 layer="Muri esterni")");
    for (const detail::EntityLine& line : lines) {
        EXPECT_EQ(line.text.substr(0, line.text.find(' ')), std::to_string(line.id));
    }

    EXPECT_EQ(detail::statisticsLines(records),
              (std::vector<std::string>{"ATTRIB 2", "INSERT 1", "LINE 1"}));
    // An INSERT without attributes writes no ATTRIB.
    core::EntityRecord bare = drawing.insert;
    std::get<core::Insert>(bare.entity).attributes.clear();
    const std::array<const core::EntityRecord*, 1> alone{&bare};
    EXPECT_EQ(detail::entityLines(alone, document).size(), 1U);
    EXPECT_EQ(detail::statisticsLines(alone), (std::vector<std::string>{"INSERT 1"}));
}

TEST(WriteState, ANewSessionHasNoFieldOfTheStylesAndBlocks) {
    // They are written by PT11, PT16 and PT22, when core has their fields.
    core::Document document = core::Document::newDrawing();
    const std::string report = stateOf(document);
    for (const std::string_view field :
         {"text styles:", "text style:", "dimension styles:", "dimension style:",
          "dimension overrides:", "blocks:", "ATTRIB"}) {
        EXPECT_EQ(report.find(field), std::string::npos) << field;
    }
}

TEST(WriteState, TextStylesAndCurrentStyleAreWrittenWhenDifferentFromNew) {
    core::Document document = core::Document::newDrawing();
    core::TextStyleTable styles = document.textStyles();
    core::TextStyle note;
    note.name = "Note";
    note.font = "romans.shx";
    note.widthFactor = 0.8;
    styles.put(std::move(note), document);
    styles.setCurrent("Note");
    document.replaceTextStyles(std::move(styles));

    const std::string report = stateOf(document);
    expectContains(report,
                   "text styles:\n  \"Note\" font=\"romans.shx\" height=0 width-factor=0.8 "
                   "oblique=0 backwards=no upside-down=no\ntext style: \"Note\"");
}

TEST(WriteState, TextStylesAreWrittenWithoutANonstandardCurrentStyle) {
    core::Document document = core::Document::newDrawing();
    core::TextStyleTable styles = document.textStyles();
    core::TextStyle note;
    note.name = "Note";
    note.font = "romans.shx";
    note.widthFactor = 0.8;
    styles.put(std::move(note), document);
    document.replaceTextStyles(std::move(styles));

    const std::string report = stateOf(document);
    expectContains(report, "text styles:\n  \"Note\"");
    EXPECT_EQ(report.find("text style:"), std::string::npos);
}

TEST(WriteState, AChangedStandardTextStyleIsWritten) {
    core::Document document = core::Document::newDrawing();
    core::TextStyleTable styles = document.textStyles();
    core::TextStyle standard = *styles.find("Standard");
    standard.font = "txt.shx";
    styles.put(std::move(standard), document);
    document.replaceTextStyles(std::move(styles));

    expectContains(stateOf(document), "text styles:\n  \"Standard\" font=\"txt.shx\"");
}

// Package 0.8-PT22, point 6: the three fields of the dimension styles.
TEST(WriteState, DimensionStylesOnlyWhenTheTableDiffersFromANewSession) {
    core::Document document = core::Document::newDrawing();
    core::DimStyleTable styles = document.dimStyles();
    core::DimStyle iso25 = *styles.find("ISO-25");
    iso25.vars.textHeight = 3.5;
    styles.put(iso25, document);
    styles.setCurrentVariables(iso25.vars);  // as -DIMSTYLE Save does
    document.replaceDimStyles(std::move(styles));

    const std::string report = stateOf(document);
    expectContains(report,
                   "dimension styles:\n  \"Standard\" DIMASZ:0.18 DIMCEN:0.09 DIMDEC:4 "
                   "DIMDSEP:. DIMEXE:0.18 DIMEXO:0.0625 DIMGAP:0.09 DIMTAD:0 DIMTIH:1 "
                   "DIMTOH:1 DIMTXT:0.18 DIMZIN:0\n  \"ISO-25\" DIMTXT:3.5\npointer:");
    EXPECT_EQ(report.find("dimension style:"), std::string::npos);
    EXPECT_EQ(report.find("dimension overrides:"), std::string::npos);
}

TEST(WriteState, DimensionOverridesAreTheCurrentVariablesThatDifferFromTheStyle) {
    core::Document document = core::Document::newDrawing();
    core::DimStyleTable styles = document.dimStyles();
    core::DimVariables current = styles.currentVariables();
    current.textHeight = 5.0;
    current.decimalSeparator = '.';
    styles.setCurrentVariables(current);
    document.replaceDimStyles(std::move(styles));

    const std::string report = stateOf(document);
    expectContains(report, "ortho: off\ndimension overrides: DIMDSEP:. DIMTXT:5\npointer:");
    EXPECT_EQ(report.find("dimension styles:"), std::string::npos);
    EXPECT_EQ(report.find("dimension style:"), std::string::npos);
}

TEST(WriteState, ACurrentDimensionStyleOtherThanIso25IsWritten) {
    core::Document document = core::Document::newDrawing();
    core::DimStyleTable styles = document.dimStyles();
    styles.setCurrent("Standard");
    styles.setCurrentVariables(styles.find("Standard")->vars);
    document.replaceDimStyles(std::move(styles));

    const std::string report = stateOf(document);
    expectContains(report, "ortho: off\ndimension style: \"Standard\"\npointer:");
    EXPECT_EQ(report.find("dimension styles:"), std::string::npos);
    EXPECT_EQ(report.find("dimension overrides:"), std::string::npos);
}

TEST(WriteState, ADocumentWithoutDimensionStylesWritesNoDimensionField) {
    core::Document document;
    const std::string report = stateOf(document);
    for (const std::string_view field :
         {"dimension styles:", "dimension style:", "dimension overrides:"}) {
        EXPECT_EQ(report.find(field), std::string::npos) << field;
    }
}

TEST(WriteGeneratedItem, AShapeWithTheEffectivePropertiesThatAreNotTheDefault) {
    const Tables tables = drawingWithLayers();
    const core::Document& document = tables.document;
    core::EntityRecord record = lineOf(document);
    const core::GeneratedItem plain{core::Line{{0.0, 0.0}, {10.0, 0.0}},
                                    core::effectiveProperties(document, record)};
    std::string out;
    detail::writeGeneratedItem(plain, out);
    EXPECT_EQ(out, "LINE start=0,0 end=10,0");

    record.properties.color = core::Color::fromIndex(1);
    record.properties.linetypeScale = 2.0;
    record.properties.linetype = tables.dashed;
    record.properties.lineweight = 50;
    core::GeneratedItem coloured{core::Text{}, core::effectiveProperties(document, record)};
    std::get<core::Text>(coloured.shape).value = "A";
    out.clear();
    detail::writeGeneratedItem(coloured, out);
    // The derived box of a TEXT comes last.
    expectContains(out, R"( value="A" color=255,0,0 linetype="ACAD_ISO02W100" ltscale=2 )"
                        R"(lineweight=0.50 ~box=)");
}

TEST(WriteGeneratedItem, ARegionWritesItsLoopsAndAMapOtherThanTheIdentity) {
    const core::Document document = core::Document::newDrawing();
    const core::EntityRecord record = lineOf(document);
    core::FilledRegion region{
        {core::Polyline{{{{0.0, 0.0}, 0.0}, {{10.0, 0.0}, 0.5}, {{10.0, 10.0}, 0.0}}, true}},
        core::Transform2D::identity()};
    std::string out;
    detail::writeGeneratedItem({region, core::effectiveProperties(document, record)}, out);
    EXPECT_EQ(out, "REGION loop=polyline vertex=0,0 vertex=10,0 bulge=0.5 vertex=10,10");
    region.transform = core::Transform2D{2.0, 0.0, 0.0, 1.0, 5.0, -1.0};
    out.clear();
    detail::writeGeneratedItem({region, core::effectiveProperties(document, record)}, out);
    EXPECT_EQ(out,
              "REGION loop=polyline vertex=0,0 vertex=10,0 bulge=0.5 vertex=10,10 "
              "~affine=2,0,0,1,5,-1");
}

}  // namespace
}  // namespace tg::script
