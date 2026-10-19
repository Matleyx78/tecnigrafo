// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/report.hpp"

#include "tg/core/math.hpp"
#include "tg/script/entity_reader.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <variant>

// Lines, circles and arcs in the format of docs/verifica/rapporto.md: written
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

}  // namespace
}  // namespace tg::script
