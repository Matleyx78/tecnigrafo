// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/report.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text.hpp"
#include "tg/script/entity_reader.hpp"
#include "tg/script/number_format.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

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
             std::string_view{"TEXT position=0,0 value=\"A\" color=red"},
             std::string_view{R"(TEXT position=0,0 value="A\q")"},  // unknown escape
         }) {
        const EntityError error = failure(text);
        EXPECT_FALSE(error.notImplemented) << text;
        EXPECT_FALSE(error.message.empty()) << text;
    }
}

TEST(SnapKindName, EveryKind) {
    EXPECT_EQ(snapKindName(core::SnapKind::Endpoint), "Endpoint");
    EXPECT_EQ(snapKindName(core::SnapKind::Midpoint), "Midpoint");
    EXPECT_EQ(snapKindName(core::SnapKind::Center), "Center");
    EXPECT_EQ(snapKindName(core::SnapKind::Insertion), "Insertion");
}

}  // namespace
}  // namespace tg::script
