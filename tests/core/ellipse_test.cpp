// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/ellipse.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/transform2d.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <optional>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-12;

double degrees(double value) {
    return degreesToRadians(value);
}

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kTolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

/// True when `point` lies on the curve of `ellipse` (the whole ellipse): in
/// the frame of its axes, (u/|a|)² + (v/|b|)² = 1.
void expectOnEllipse(const Ellipse& ellipse, const Vec2& point) {
    const Vec2 relative = point - ellipse.center;
    const double major = ellipse.majorAxis.length();
    const double minor = major * ellipse.ratio;
    const double u = dot(relative, ellipse.majorAxis) / major;
    const double v = cross(ellipse.majorAxis, relative) / major;
    EXPECT_NEAR(((u / major) * (u / major)) + ((v / minor) * (v / minor)), 1.0, 1e-12);
}

/// The image of the curve of `e` under `m` is the curve of `result`, end
/// points included: start and end of the image are those of the result,
/// swapped by a reflection.
void expectImage(const Ellipse& e, const Transform2D& m, const Ellipse& result) {
    ASSERT_TRUE(supported(result));
    EXPECT_GT(result.ratio, 0.0);
    EXPECT_LE(result.ratio, 1.0);
    for (int step = 0; step < 16; ++step) {
        const double t = e.startParameter + (parameterSpan(e) * step / 15.0);
        expectOnEllipse(result, m.apply(pointAt(e, t)));
    }
    if (parameterSpan(e) == kTwoPi) {
        EXPECT_EQ(result.startParameter, 0.0);
        EXPECT_EQ(result.endParameter, kTwoPi);
        return;
    }
    const bool reflects = m.determinant() < 0.0;
    expectNear(pointAt(result, result.startParameter),
               m.apply(pointAt(e, reflects ? e.endParameter : e.startParameter)), 1e-9);
    expectNear(pointAt(result, result.endParameter),
               m.apply(pointAt(e, reflects ? e.startParameter : e.endParameter)), 1e-9);
    EXPECT_NEAR(parameterSpan(result), parameterSpan(e), 1e-12);
}

Ellipse sample() {
    return Ellipse{{0, 0}, {4, 0}, 0.5, 0.0, kTwoPi};
}

TEST(Ellipse, IsSupportedWithAnAxisARatioInZeroOneAndFiniteParameters) {
    EXPECT_TRUE(supported(sample()));
    EXPECT_TRUE(supported(Ellipse{{0, 0}, {4, 0}, 1.0, 0.0, kTwoPi}));
    EXPECT_TRUE(supported(Ellipse{{0, 0}, {4, 0}, 1e-6, 0.0, kTwoPi}));
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {0, 0}, 0.5, 0.0, kTwoPi}));
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {4, 0}, 0.0, 0.0, kTwoPi}));
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {4, 0}, -0.5, 0.0, kTwoPi}));
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {4, 0}, 1.5, 0.0, kTwoPi}));
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {4, 0}, nan, 0.0, kTwoPi}));
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {4, 0}, 0.5, nan, kTwoPi}));
    EXPECT_FALSE(supported(Ellipse{{0, 0}, {4, 0}, 0.5, 0.0, inf}));
}

TEST(Ellipse, PointAtTheFourQuadrants) {
    // a = (4, 3), b = (-3, 4) * 0.5: the minor axis is a turned by +90°.
    const Ellipse ellipse{{10, 20}, {4, 3}, 0.5, 0.0, kTwoPi};
    expectNear(pointAt(ellipse, 0.0), {14, 23});
    expectNear(pointAt(ellipse, kPi / 2.0), {8.5, 22});
    expectNear(pointAt(ellipse, kPi), {6, 17});
    expectNear(pointAt(ellipse, 3.0 * kPi / 2.0), {11.5, 18});
}

TEST(Ellipse, TheSpanOfTheParametersIsInZeroTwoPi) {
    EXPECT_EQ(parameterSpan(sample()), kTwoPi);
    EXPECT_EQ(parameterSpan(Ellipse{{0, 0}, {4, 0}, 0.5, 1.0, 1.0}), kTwoPi);
    EXPECT_NEAR(parameterSpan(Ellipse{{0, 0}, {4, 0}, 0.5, degrees(300), degrees(30)}), degrees(90),
                kTolerance);
    EXPECT_NEAR(parameterSpan(Ellipse{{0, 0}, {4, 0}, 0.5, degrees(30), degrees(120)}), degrees(90),
                kTolerance);
}

TEST(Ellipse, CirclesAndArcsAsEllipses) {
    EXPECT_EQ(asEllipse(Circle{{1, 2}, 3.0}), (Ellipse{{1, 2}, {3, 0}, 1.0, 0.0, kTwoPi}));
    EXPECT_EQ(asEllipse(Arc{{1, 2}, 3.0, 0.5, 2.0}), (Ellipse{{1, 2}, {3, 0}, 1.0, 0.5, 2.0}));
}

TEST(EllipseTransform, ARotationTurnsTheAxis) {
    const Ellipse arc{{10, 20}, {4, 0}, 0.5, degrees(30), degrees(120)};
    const Transform2D quarter = Similarity::rotation(kPi / 2.0, {0, 0}).matrix();
    const Ellipse result = transformed(arc, quarter);
    EXPECT_EQ(result.center, (Vec2{-20, 10}));
    EXPECT_EQ(result.majorAxis, (Vec2{0, 4}));
    EXPECT_EQ(result.ratio, 0.5);
    EXPECT_NEAR(result.startParameter, degrees(30), kTolerance);
    EXPECT_NEAR(result.endParameter, degrees(120), kTolerance);
    expectImage(arc, quarter, result);

    const Transform2D slanted = Transform2D::rotation(0.3) * Transform2D::translation({1, -2});
    expectImage(arc, slanted, transformed(arc, slanted));
}

TEST(EllipseTransform, ScalingACircleTwoByOneGivesRatioOneHalf) {
    const Transform2D stretch = Transform2D::scaling(2.0, 1.0);
    const Ellipse result = transformed(asEllipse(Circle{{1, 1}, 3.0}), stretch);
    EXPECT_EQ(result, (Ellipse{{2, 1}, {6, 0}, 0.5, 0.0, kTwoPi}));
    expectImage(asEllipse(Circle{{1, 1}, 3.0}), stretch, result);
}

TEST(EllipseTransform, ScalingOneByTwoSwapsTheAxes) {
    const Transform2D stretch = Transform2D::scaling(1.0, 2.0);
    const Ellipse arc = asEllipse(Arc{{0, 0}, 3.0, degrees(10), degrees(100)});
    const Ellipse result = transformed(arc, stretch);
    EXPECT_EQ(result.majorAxis, (Vec2{0, 6}));
    EXPECT_EQ(result.ratio, 0.5);
    // The old parameter t is t - 90° on the new axes.
    EXPECT_NEAR(result.startParameter, degrees(280), kTolerance);
    EXPECT_NEAR(result.endParameter, degrees(10), kTolerance);
    expectImage(arc, stretch, result);
}

TEST(EllipseTransform, AReflectionReversesTheParameters) {
    const Ellipse arc{{10, 20}, {4, 0}, 0.5, degrees(30), degrees(120)};
    const Transform2D acrossX = Transform2D::scaling(1.0, -1.0);
    const Ellipse mirrored = transformed(arc, acrossX);
    EXPECT_EQ(mirrored.center, (Vec2{10, -20}));
    EXPECT_EQ(mirrored.majorAxis, (Vec2{4, 0}));
    EXPECT_NEAR(mirrored.startParameter, degrees(240), kTolerance);
    EXPECT_NEAR(mirrored.endParameter, degrees(330), kTolerance);
    expectImage(arc, acrossX, mirrored);

    // Across Y the major axis would point at 180°: its other end is taken.
    const Transform2D acrossY = Transform2D::scaling(-1.0, 1.0);
    const Ellipse other = transformed(arc, acrossY);
    EXPECT_EQ(other.majorAxis, (Vec2{4, 0}));
    EXPECT_NEAR(other.startParameter, degrees(60), kTolerance);
    EXPECT_NEAR(other.endParameter, degrees(150), kTolerance);
    expectImage(arc, acrossY, other);

    // A reflection that is not a similarity.
    const Transform2D skewMirror{-2.0, 0.5, 0.3, 1.0, 4.0, 1.0};
    expectImage(arc, skewMirror, transformed(arc, skewMirror));
}

TEST(EllipseTransform, AShearGivesPerpendicularAxes) {
    // x' = x + y: the unit circle becomes the ellipse of half axes φ and 1/φ.
    const Transform2D shear{1.0, 0.0, 1.0, 1.0, 0.0, 0.0};
    const Ellipse circle = asEllipse(Circle{{0, 0}, 1.0});
    const Ellipse result = transformed(circle, shear);
    const double golden = std::numbers::phi;
    EXPECT_NEAR(result.majorAxis.length(), golden, kTolerance);
    EXPECT_NEAR(result.ratio, 1.0 / (golden * golden), kTolerance);
    EXPECT_GE(atan2(result.majorAxis.y, result.majorAxis.x), 0.0);
    EXPECT_LT(atan2(result.majorAxis.y, result.majorAxis.x), kPi);
    expectImage(circle, shear, result);

    const Ellipse arc{{3, -1}, {2, 1}, 0.4, degrees(200), degrees(20)};
    const Transform2D general{0.7, -1.3, 2.1, 0.4, -5.0, 2.0};
    expectImage(arc, general, transformed(arc, general));
}

TEST(EllipseTransform, ARatioOneIsACircleAlongPositiveX) {
    // Axis (0, 2) and ratio 0.5 under a scale (2, 1): a circle of radius 2.
    const Ellipse ellipse{{0, 0}, {0, 2}, 0.5, 0.0, kTwoPi};
    const Transform2D stretch = Transform2D::scaling(2.0, 1.0);
    const Ellipse circle = transformed(ellipse, stretch);
    EXPECT_EQ(circle, (Ellipse{{0, 0}, {2, 0}, 1.0, 0.0, kTwoPi}));

    // An arc of a circle: the parameters follow the axis to +X.
    const Ellipse arc{{0, 0}, {0, 2}, 1.0, 0.0, degrees(90)};
    const Ellipse turned = transformed(arc, Transform2D{});
    EXPECT_EQ(turned.majorAxis, (Vec2{2, 0}));
    EXPECT_NEAR(turned.startParameter, degrees(90), kTolerance);
    EXPECT_NEAR(turned.endParameter, degrees(180), kTolerance);
    const Ellipse halfArc{{0, 0}, {0, 2}, 0.5, 0.0, degrees(90)};
    expectImage(halfArc, stretch, transformed(halfArc, stretch));
}

TEST(EllipseTransform, AnAxisAtTwoHundredDegreesIsWrittenAtTwenty) {
    const Ellipse ellipse{{0, 0}, {4, 0}, 0.5, degrees(30), degrees(120)};
    const Transform2D rotation = Similarity::rotation(degrees(200), {0, 0}).matrix();
    const Ellipse result = transformed(ellipse, rotation);
    expectNear(result.majorAxis, {4.0 * cos(degrees(20)), 4.0 * sin(degrees(20))});
    EXPECT_NEAR(result.startParameter, degrees(210), 1e-12);
    EXPECT_NEAR(result.endParameter, degrees(300), 1e-12);
    expectImage(ellipse, rotation, result);
}

TEST(EllipseTransform, ASingularMapGivesAnUnsupportedEllipse) {
    EXPECT_FALSE(supported(transformed(sample(), Transform2D::scaling(1.0, 0.0))));
    EXPECT_FALSE(supported(transformed(sample(), Transform2D::scaling(0.0, 0.0))));
}

TEST(EllipseTransform, TransformedShapeMakesEllipsesOfCirclesArcsAndEllipses) {
    const Transform2D stretch = Transform2D::scaling(2.0, 1.0);
    const std::vector<GeneratedShape> circle = transformedShape(Circle{{1, 1}, 3.0}, stretch);
    ASSERT_EQ(circle.size(), 1U);
    EXPECT_EQ(std::get<Ellipse>(circle.front()), (Ellipse{{2, 1}, {6, 0}, 0.5, 0.0, kTwoPi}));

    const std::vector<GeneratedShape> arc =
        transformedShape(Arc{{0, 0}, 1.0, 0.0, kPi / 2.0}, stretch);
    ASSERT_EQ(arc.size(), 1U);
    const auto& elliptical = std::get<Ellipse>(arc.front());
    EXPECT_EQ(elliptical.majorAxis, (Vec2{2, 0}));
    EXPECT_NEAR(elliptical.endParameter, kPi / 2.0, kTolerance);

    // An ellipse takes the same path under a similarity.
    const Transform2D move = Transform2D::translation({5, -7});
    const std::vector<GeneratedShape> moved = transformedShape(sample(), move);
    ASSERT_EQ(moved.size(), 1U);
    EXPECT_EQ(std::get<Ellipse>(moved.front()), (Ellipse{{5, -7}, {4, 0}, 0.5, 0.0, kTwoPi}));

    // Under a similarity a circle stays a circle (PT6).
    EXPECT_TRUE(std::holds_alternative<Circle>(
        transformedShape(Circle{{1, 1}, 3.0}, Transform2D::translation({1, 0})).front()));
    // A singular map: nothing.
    EXPECT_TRUE(transformedShape(Circle{{1, 1}, 3.0}, Transform2D::scaling(0.0, 1.0)).empty());
    EXPECT_TRUE(transformedShape(sample(), Transform2D::scaling(0.0, 0.0)).empty());
}

TEST(EllipseBox, AFullEllipseHasItsExtremes) {
    const BBox box = boundingBox(Ellipse{{0, 0}, {4, 3}, 0.5, 0.0, kTwoPi});
    expectNear(box.min(), {-std::sqrt(18.25), -std::sqrt(13.0)});
    expectNear(box.max(), {std::sqrt(18.25), std::sqrt(13.0)});
}

TEST(EllipseBox, AnArcThroughAnExtremeAndOneWithout) {
    // Through the top of the ellipse (t = 90°).
    const BBox through = boundingBox(Ellipse{{0, 0}, {4, 0}, 0.5, degrees(30), degrees(150)});
    expectNear(through.min(), {-2.0 * std::numbers::sqrt3, 1.0});
    expectNear(through.max(), {2.0 * std::numbers::sqrt3, 2.0});
    // Between 30° and 60° X and Y are monotonic: only the end points count.
    const BBox without = boundingBox(Ellipse{{0, 0}, {4, 0}, 0.5, degrees(30), degrees(60)});
    expectNear(without.min(), {2.0, 1.0});
    expectNear(without.max(), {2.0 * std::numbers::sqrt3, std::numbers::sqrt3});
    // An oblique arc whose extremes lie inside it.
    const BBox oblique = boundingBox(Ellipse{{0, 0}, {4, 3}, 0.5, 0.0, kPi});
    expectNear(oblique.min(), {-std::sqrt(18.25), -3.0});
    expectNear(oblique.max(), {4.0, std::sqrt(13.0)});
}

TEST(EllipseSelection, OnlyAWindowAroundTheBoxSelectsUntilPT50) {
    const Ellipse ellipse{{10, 20}, {4, 0}, 0.5, 0.0, kTwoPi};
    EXPECT_TRUE(isInsideOf(ellipse, BBox{{6, 18}, {14, 22}}));
    EXPECT_FALSE(isInsideOf(ellipse, BBox{{6.001, 18}, {14, 22}}));
    EXPECT_FALSE(isInsideOf(ellipse, BBox{}));
    EXPECT_EQ(distanceOf(ellipse, {14, 20}), std::numeric_limits<double>::infinity());
    EXPECT_FALSE(intersectsOf(ellipse, BBox{{13, 19}, {15, 21}}));
}

std::vector<SnapPoint> snaps(const Ellipse& ellipse, const SnapModes& modes) {
    std::vector<SnapPoint> out;
    appendSnapPointsOf(ellipse, modes, out, std::nullopt);
    return out;
}

TEST(EllipseSnap, CenterQuadrantsAndEndpoints) {
    SnapModes all;
    all.quadrant = true;
    const std::vector<SnapPoint> whole = snaps(Ellipse{{10, 20}, {4, 0}, 0.5, 0.0, kTwoPi}, all);
    ASSERT_EQ(whole.size(), 5U);
    EXPECT_EQ(whole[0], (SnapPoint{{10, 20}, SnapKind::Center}));
    EXPECT_EQ(whole[1], (SnapPoint{{14, 20}, SnapKind::Quadrant}));
    EXPECT_EQ(whole[2], (SnapPoint{{10, 22}, SnapKind::Quadrant}));
    EXPECT_EQ(whole[3], (SnapPoint{{6, 20}, SnapKind::Quadrant}));
    EXPECT_EQ(whole[4], (SnapPoint{{10, 18}, SnapKind::Quadrant}));

    const std::vector<SnapPoint> arc =
        snaps(Ellipse{{0, 0}, {4, 0}, 0.5, degrees(300), degrees(30)}, all);
    ASSERT_EQ(arc.size(), 4U);
    EXPECT_EQ(arc[0].kind, SnapKind::Endpoint);
    expectNear(arc[0].point, {2, -std::numbers::sqrt3});
    EXPECT_EQ(arc[1].kind, SnapKind::Endpoint);
    expectNear(arc[1].point, {2.0 * std::numbers::sqrt3, 1.0});
    EXPECT_EQ(arc[2], (SnapPoint{{0, 0}, SnapKind::Center}));
    EXPECT_EQ(arc[3], (SnapPoint{{4, 0}, SnapKind::Quadrant}));

    const SnapModes none{false, false, false, false, false, false, false, false};
    EXPECT_TRUE(snaps(sample(), none).empty());
}

TEST(EllipseGrips, CentreAndTheEndsOfTheAxes) {
    const std::vector<Vec2> grips = gripPointsOf(Ellipse{{10, 20}, {4, 0}, 0.5, 0.0, kTwoPi});
    const std::vector<Vec2> expected{{10, 20}, {14, 20}, {10, 22}, {6, 20}, {10, 18}};
    EXPECT_EQ(grips, expected);
    const GripResult moved = moveGrip(Entity{sample()}, 0, {10, 20});
    EXPECT_EQ(moved.outcome, GripOutcome::Moved);
    EXPECT_EQ(std::get<Ellipse>(moved.entity), (Ellipse{{10, 20}, {4, 0}, 0.5, 0.0, kTwoPi}));
}

Ellipse grip(const Ellipse& ellipse, std::size_t index, const Vec2& target) {
    const GripResult result = moveGrip(Entity{ellipse}, index, target);
    EXPECT_EQ(result.outcome, GripOutcome::Moved);
    EXPECT_TRUE(result.refusal.empty());
    return std::get<Ellipse>(result.entity);
}

TEST(EllipseGrips, AnAxisGripSetsTheLengthOfItsHalfAxis) {
    // The minor grip at (1, 5): a minor half axis of √26 along Y, longer
    // than the major one, so the axes swap.
    const Ellipse swapped = grip(sample(), 2, {1, 5});
    EXPECT_EQ(swapped.majorAxis, (Vec2{0, std::sqrt(26.0)}));
    EXPECT_NEAR(swapped.ratio, 4.0 / std::sqrt(26.0), kTolerance);

    // The major grip from (4, 0) to (1, 0): shorter than the minor one.
    EXPECT_EQ(grip(sample(), 1, {1, 0}), (Ellipse{{0, 0}, {0, 2}, 0.5, 0.0, kTwoPi}));
    // To (2, 0): a circle with the axis (2, 0).
    EXPECT_EQ(grip(sample(), 1, {2, 0}), (Ellipse{{0, 0}, {2, 0}, 1.0, 0.0, kTwoPi}));
    // Longer, from the other end: the direction stays.
    EXPECT_EQ(grip(sample(), 3, {-3, 4}), (Ellipse{{0, 0}, {5, 0}, 0.4, 0.0, kTwoPi}));
    EXPECT_EQ(grip(sample(), 4, {0, -1}), (Ellipse{{0, 0}, {4, 0}, 0.25, 0.0, kTwoPi}));
}

TEST(EllipseGrips, AnInclinedEllipseBecomesACircleAlongPositiveX) {
    // Axis (3, 4), length 5, ratio 0.4: minor half axis 2.
    const Ellipse inclined{{1, 1}, {3, 4}, 0.4, degrees(10), degrees(100)};
    const Ellipse circle = grip(inclined, 1, {1, 3});
    EXPECT_EQ(circle.center, (Vec2{1, 1}));
    expectNear(circle.majorAxis, {2, 0});
    EXPECT_EQ(circle.majorAxis.y, 0.0);
    EXPECT_EQ(circle.ratio, 1.0);
    // The end points stay at the same parametric angles, measured now from +X.
    const double axisAngle = atan2(4.0, 3.0);
    EXPECT_NEAR(circle.startParameter, degrees(10) + axisAngle, kTolerance);
    EXPECT_NEAR(circle.endParameter, degrees(100) + axisAngle, kTolerance);
}

TEST(EllipseGrips, AnArcKeepsItsEndsAtTheSameParametricAngles) {
    const Ellipse arc{{0, 0}, {4, 0}, 0.5, 0.0, degrees(90)};
    const Ellipse swapped = grip(arc, 2, {1, 5});
    EXPECT_NEAR(swapped.startParameter, degrees(270), kTolerance);
    EXPECT_NEAR(swapped.endParameter, 0.0, kTolerance);
    const Ellipse shorter = grip(arc, 1, {1, 0});
    EXPECT_EQ(shorter.majorAxis, (Vec2{0, 2}));
    EXPECT_NEAR(shorter.startParameter, degrees(270), kTolerance);
    EXPECT_NEAR(shorter.endParameter, 0.0, kTolerance);
}

TEST(EllipseGrips, AHalfAxisOfLengthZeroIsRefused) {
    for (const std::size_t index : {1U, 2U, 3U, 4U}) {
        const GripResult result = moveGrip(Entity{sample()}, index, {0, 0});
        EXPECT_EQ(result.outcome, GripOutcome::Invalid) << index;
        EXPECT_EQ(std::get<Ellipse>(result.entity), sample());
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(moveGrip(Entity{sample()}, 1, {nan, 0}).outcome, GripOutcome::Invalid);
}

TEST(EllipseDocument, OnlyAWindowSelectsAnEllipseUntilPT50) {
    Document document = Document::newDrawing();
    const EntityId ellipse = document.add(Ellipse{{10, 20}, {4, 0}, 0.5, 0.0, kTwoPi});
    const EntityId line = document.add(Line{{7, 19}, {8, 19}});
    const BBox around{{5, 17}, {15, 23}};
    EXPECT_EQ(selectInRectangle(document, around, RectangleMode::Window),
              (std::vector<EntityId>{ellipse, line}));
    // A crossing window takes the line inside it, not the ellipse.
    EXPECT_EQ(selectInRectangle(document, around, RectangleMode::Crossing),
              (std::vector<EntityId>{line}));
    EXPECT_FALSE(pickEntity(document, {14, 20}, 0.1).has_value());
}

TEST(EllipseDocument, GeneratesItselfExceptOnAFrozenLayer) {
    Document document = Document::newDrawing();
    const Ellipse ellipse{{10, 20}, {4, 0}, 0.5, 0.0, kTwoPi};
    const EntityRecord record = *document.find(document.add(ellipse));
    const std::vector<GeneratedItem> items = generate(document, record);
    ASSERT_EQ(items.size(), 1U);
    EXPECT_EQ(std::get<Ellipse>(items.front().shape), ellipse);
    const BBox box = extent(document, record);
    EXPECT_EQ(box.min(), (Vec2{6, 18}));
    EXPECT_EQ(box.max(), (Vec2{14, 22}));

    Layer frozen;
    frozen.name = "Frozen";
    frozen.linetype = document.continuousLinetype();
    frozen.frozen = true;
    const LayerId layer = document.addLayer(frozen);
    EntityRecord onFrozen = record;
    onFrozen.properties.layer = layer;
    EXPECT_TRUE(generate(document, onFrozen).empty());
    EXPECT_TRUE(extent(document, onFrozen).isEmpty());

    const TransformOutcome moved =
        transformedIn(document, Entity{ellipse}, Similarity::translation({5, -7}), {});
    ASSERT_TRUE(moved.entity.has_value());
    EXPECT_EQ(std::get<Ellipse>(moved.entity.value_or(Entity{})),
              (Ellipse{{15, 13}, {4, 0}, 0.5, 0.0, kTwoPi}));
}

}  // namespace
}  // namespace tg::core
