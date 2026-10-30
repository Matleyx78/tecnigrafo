// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The kernel of Tecnigrafo is a submodule (docs/adr/0030). This test checks
// that its public headers are found and that the contract types they define
// can be used. It calls no function that the headers only declare: those are
// not used before the package of solid-space that implements them is merged.

#include "solid_space/geom2d/curves.hpp"
#include "solid_space/geom2d/diagnosis.hpp"
#include "solid_space/robust/point.hpp"
#include "solid_space/robust/result.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>

namespace tg::core {
namespace {

namespace geom2d = solid_space::geom2d;

TEST(SolidSpace, ResultHoldsAValue) {
    const geom2d::Result<int> result{42};
    ASSERT_TRUE(result.hasValue());
    EXPECT_EQ(result.value(), 42);
}

TEST(SolidSpace, ResultHoldsADiagnosis) {
    const geom2d::Result<int> result{
        geom2d::Diagnosis{geom2d::DiagnosisCode::degenerateCurve, 1, "a single point"}};
    ASSERT_FALSE(result.hasValue());
    EXPECT_EQ(result.diagnosis().code, geom2d::DiagnosisCode::degenerateCurve);
    EXPECT_EQ(result.diagnosis().input, std::size_t{1});
    EXPECT_EQ(result.diagnosis().message, "a single point");
}

TEST(SolidSpace, ResultOfTheRobustComponentTakesItsOwnDiagnosis) {
    // The Result template lives in robust; each component names its own
    // diagnosis type (here, geom2d's).
    const solid_space::robust::Result<double, geom2d::Diagnosis> result{2.5};
    ASSERT_TRUE(result.hasValue());
    EXPECT_EQ(result.value(), 2.5);
}

TEST(SolidSpace, SegmentKeepsItsEnds) {
    const geom2d::Segment segment{{1.0, 2.0}, {4.0, 6.0}};
    EXPECT_EQ(segment.start.x, 1.0);
    EXPECT_EQ(segment.start.y, 2.0);
    EXPECT_EQ(segment.end.x, 4.0);
    EXPECT_EQ(segment.end.y, 6.0);
}

TEST(SolidSpace, ArcKeepsCentreRadiusAndAngles) {
    const geom2d::Arc arc{{-1.0, 0.5}, 3.0, 0.25, -1.5};
    EXPECT_EQ(arc.center.x, -1.0);
    EXPECT_EQ(arc.center.y, 0.5);
    EXPECT_EQ(arc.radius, 3.0);
    EXPECT_EQ(arc.startAngle, 0.25);
    EXPECT_EQ(arc.sweep, -1.5);
}

TEST(SolidSpace, PolylineVertexKeepsPositionAndBulge) {
    const geom2d::PolylineVertex vertex{{7.0, -8.0}, 0.5};
    EXPECT_EQ(vertex.position.x, 7.0);
    EXPECT_EQ(vertex.position.y, -8.0);
    EXPECT_EQ(vertex.bulge, 0.5);

    const geom2d::PolylineVertex straight;
    EXPECT_EQ(straight.bulge, 0.0);
}

TEST(SolidSpace, PointsOfTheTwoComponentsAreTheSameType) {
    const solid_space::robust::Point2 point{3.0, 4.0};
    const geom2d::Point2 same = point;
    EXPECT_EQ(same.x, 3.0);
    EXPECT_EQ(same.y, 4.0);
}

}  // namespace
}  // namespace tg::core
