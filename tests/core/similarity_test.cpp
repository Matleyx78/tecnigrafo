// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/similarity.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/script/questions.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-9;

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

double normalized(double angle) {
    const double result = std::fmod(angle, kTwoPi);
    return result < 0.0 ? result + kTwoPi : result;
}

std::array<Similarity, 5> similarities() {
    return {Similarity::translation({7.0, -3.0}), Similarity::rotation(kPi / 3.0, {2.0, 1.0}),
            *Similarity::scaling(2.5, {3.0, -4.0}), *Similarity::reflection({1.0, 2.0}, {2.0, 3.0}),
            Similarity::translation({4.0, 8.0}) * Similarity::rotation(0.3, {}) *
                *Similarity::scaling(1.7, {2.0, 4.0}) * *Similarity::reflection({}, {1.0, 0.0})};
}

TEST(Similarity, IdentityCentersAndAxisAreFixed) {
    const Vec2 point{3.0, 4.0};
    expectPoint(Similarity{}.apply(point), point);
    expectPoint(Similarity::translation({2.0, -1.0}).apply(point), {5.0, 3.0});
    expectPoint(Similarity::rotation(kPi / 2.0, {1.0, 2.0}).apply(point), {-1.0, 4.0});
    expectPoint(Similarity::scaling(2.0, {1.0, 2.0})->apply(point), {5.0, 6.0});
    const auto reflection = Similarity::reflection({1.0, 2.0}, {2.0, 3.0});
    ASSERT_TRUE(reflection);
    expectPoint(reflection->apply({4.0, 5.0}), {4.0, 5.0});
    expectPoint(reflection->apply({2.0, 2.0}), {1.0, 3.0});
}

TEST(Similarity, CompositionAndMatrixAgreeForBothParities) {
    for (const Similarity& a : similarities()) {
        for (const Similarity& b : similarities()) {
            const Similarity composed = a * b;
            EXPECT_EQ(composed.reflects(), a.reflects() != b.reflects());
            EXPECT_NEAR(composed.scale(), a.scale() * b.scale(), kTolerance);
            for (const Vec2 point : {Vec2{}, Vec2{1.0, -2.0}, Vec2{-8.0, 5.0}}) {
                expectPoint(composed.apply(point), a.apply(b.apply(point)));
                expectPoint(composed.matrix().apply(point), (a.matrix() * b.matrix()).apply(point));
            }
        }
    }
}

TEST(Similarity, InvalidFactoriesAndVeryShortAxes) {
    const std::array factors{
        0.0,
        -1.0,
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN(),
    };
    for (const double factor : factors) {
        EXPECT_FALSE(Similarity::scaling(factor, {}));
    }
    EXPECT_FALSE(Similarity::reflection({3.0, 4.0}, {3.0, 4.0}));
    const auto tiny = Similarity::reflection({}, {0.0, 1e-100});
    ASSERT_TRUE(tiny);
    expectPoint(tiny->apply({3.0, 4.0}), {-3.0, 4.0});
}

TEST(Similarity, TransformsEveryCurveAndPreservesPolylineAttributes) {
    const Line line{{1.0, 2.0}, {5.0, -3.0}};
    const Circle circle{{3.0, -4.0}, 2.0};
    const Arc arc{{2.0, 1.0}, 3.0, 0.3, 5.9};
    Polyline polyline{{{{1.0, 2.0}, 0.4, 2.0, 3.0}, {{5.0, -3.0}, -0.8, 4.0, 5.0}}, true};
    polyline.setConstantWidth(6.0);
    polyline.setContinuousLinetype(true);
    for (const Similarity& similarity : similarities()) {
        const Line movedLine = std::get<Line>(transformed(line, similarity));
        expectPoint(movedLine.start, similarity.apply(line.start));
        expectPoint(movedLine.end, similarity.apply(line.end));
        const Circle movedCircle = std::get<Circle>(transformed(circle, similarity));
        expectPoint(movedCircle.center, similarity.apply(circle.center));
        EXPECT_NEAR(movedCircle.radius, circle.radius * similarity.scale(), kTolerance);
        const Arc movedArc = std::get<Arc>(transformed(arc, similarity));
        expectPoint(movedArc.center, similarity.apply(arc.center));
        EXPECT_NEAR(movedArc.radius, arc.radius * similarity.scale(), kTolerance);
        const Vec2 first = startPoint(arc.geometry());
        const Vec2 last = endPoint(arc.geometry());
        const Vec2 expectedStart = similarity.apply(similarity.reflects() ? last : first);
        const Vec2 expectedEnd = similarity.apply(similarity.reflects() ? first : last);
        expectPoint(startPoint(movedArc.geometry()), expectedStart);
        expectPoint(endPoint(movedArc.geometry()), expectedEnd);
        EXPECT_NEAR(movedArc.geometry().sweep, arc.geometry().sweep, kTolerance);
        EXPECT_GE(movedArc.startAngle, 0.0);
        EXPECT_LT(movedArc.startAngle, kTwoPi);
        EXPECT_GE(movedArc.endAngle, 0.0);
        EXPECT_LT(movedArc.endAngle, kTwoPi);
        EXPECT_NE(movedArc.startAngle, movedArc.endAngle);
        const Polyline movedPolyline = std::get<Polyline>(transformed(polyline, similarity));
        EXPECT_TRUE(movedPolyline.isClosed());
        EXPECT_TRUE(movedPolyline.continuousLinetype());
        EXPECT_NEAR(movedPolyline.constantWidth(), 6.0 * similarity.scale(), kTolerance);
        ASSERT_EQ(movedPolyline.vertexCount(), polyline.vertexCount());
        for (std::size_t i = 0; i < polyline.vertexCount(); ++i) {
            const auto& before = polyline.vertices()[i];
            const auto& after = movedPolyline.vertices()[i];
            expectPoint(after.position, similarity.apply(before.position));
            const double expectedBulge = before.bulge * (similarity.reflects() ? -1.0 : 1.0);
            EXPECT_NEAR(after.bulge, expectedBulge, kTolerance);
            EXPECT_NEAR(after.startWidth, before.startWidth * similarity.scale(), kTolerance);
            EXPECT_NEAR(after.endWidth, before.endWidth * similarity.scale(), kTolerance);
        }
    }
}

void expectTextImage(const Text& before, const Text& after, const Similarity& similarity,
                     MirrorText mode) {
    const auto source = textBox(before);
    const auto target = textBox(after);
    for (std::size_t i = 0; i < source.size(); ++i) {
        const Vec2 expected = similarity.apply(source[i]);
        if (mode == MirrorText::Mirrored || !similarity.reflects()) {
            expectPoint(target[i], expected);
        } else {
            EXPECT_TRUE(std::ranges::any_of(target, [&](const Vec2& point) {
                return nearlyEqual(point, expected, kTolerance);
            }));
        }
    }
    EXPECT_EQ(after.horizontal, before.horizontal);
    EXPECT_EQ(after.vertical, before.vertical);
    EXPECT_EQ(after.value, before.value);
    EXPECT_EQ(after.style, before.style);
    EXPECT_EQ(after.mirroredY, before.mirroredY);
    const bool flipX = similarity.reflects() && mode == MirrorText::Mirrored;
    EXPECT_EQ(after.mirroredX, before.mirroredX != flipX);
    EXPECT_NEAR(after.height, before.height * similarity.scale(), kTolerance);
    EXPECT_DOUBLE_EQ(after.widthFactor, before.widthFactor);
    const bool flipOblique = similarity.reflects() && mode == MirrorText::Readable;
    const double expectedOblique = flipOblique ? -before.obliqueAngle : before.obliqueAngle;
    EXPECT_DOUBLE_EQ(after.obliqueAngle, expectedOblique);

    const bool baseline =
        before.horizontal == TextHorizontal::Aligned || before.horizontal == TextHorizontal::Fit;
    const double length = distance(before.position, before.alignmentPoint);
    const bool fitted = baseline && length > 0.0 && textWidth(displayText(before.value)) > 0.0;
    double expectedAngle = normalized(textFrame(before).rotation + similarity.rotationAngle());
    if (similarity.reflects()) {
        const double alpha = normalized(similarity.rotationAngle() - textFrame(before).rotation);
        if (mode == MirrorText::Mirrored) {
            expectedAngle = normalized(alpha + kPi);
        } else {
            const bool reverse = alpha > kPi / 2.0 && alpha <= 3.0 * kPi / 2.0;
            expectedAngle = normalized(reverse ? alpha - kPi : alpha);
        }
        EXPECT_NEAR(after.rotation, expectedAngle, kTolerance);
    }
    EXPECT_NEAR(std::cos(textFrame(after).rotation), std::cos(expectedAngle), kTolerance);
    EXPECT_NEAR(std::sin(textFrame(after).rotation), std::sin(expectedAngle), kTolerance);
    if (fitted) {
        EXPECT_NEAR(distance(after.position, after.alignmentPoint), length * similarity.scale(),
                    kTolerance);
    } else if (baseline && !(length > 0.0)) {
        expectPoint(after.position, after.alignmentPoint);
    } else if (baseline || (before.horizontal == TextHorizontal::Left &&
                            before.vertical == TextVertical::Baseline)) {
        expectPoint(after.alignmentPoint, similarity.apply(before.alignmentPoint));
    } else {
        expectPoint(after.position, similarity.apply(before.position));
    }
}

TEST(Similarity, RoundedArcAnglesRemainDistinctAndNormalized) {
    const double adjacent = std::nextafter(1.0, 2.0);
    for (const Arc arc : {Arc{{}, 1.0, 1.0, adjacent}, Arc{{}, 1.0, adjacent, 1.0}}) {
        for (const Similarity& similarity : similarities()) {
            const Arc after = std::get<Arc>(transformed(arc, similarity));
            EXPECT_GE(after.startAngle, 0.0);
            EXPECT_LT(after.startAngle, kTwoPi);
            EXPECT_GE(after.endAngle, 0.0);
            EXPECT_LT(after.endAngle, kTwoPi);
            EXPECT_NE(after.startAngle, after.endAngle);
        }
    }
}

TEST(Similarity, TextJustificationsObliqueFlagsAndDegeneracies) {
    const std::array horizontals{
        TextHorizontal::Left,
        TextHorizontal::Center,
        TextHorizontal::Right,
        TextHorizontal::Middle,
        TextHorizontal::Aligned,
        TextHorizontal::Fit,
    };
    const std::array verticals{
        TextVertical::Baseline,
        TextVertical::Bottom,
        TextVertical::Middle,
        TextVertical::Top,
    };
    std::vector<Similarity> transformations;
    for (const auto& similarity : similarities()) {
        transformations.push_back(similarity);
    }
    transformations.push_back(*Similarity::reflection({}, {1.0, 0.0}));
    transformations.push_back(*Similarity::reflection({}, {0.0, 1.0}));
    for (const auto horizontal : horizontals) {
        for (const auto vertical : verticals) {
            for (const double oblique : {-0.3, 0.0, 0.2}) {
                for (int flags = 0; flags < 4; ++flags) {
                    for (int degeneration = 0; degeneration < 3; ++degeneration) {
                        Text text;
                        text.position = {3.0, 4.0};
                        text.alignmentPoint = degeneration == 1 ? text.position : Vec2{13.0, 7.0};
                        text.rotation = 0.7;
                        text.height = 3.0;
                        text.widthFactor = 0.8;
                        text.obliqueAngle = oblique;
                        text.value = degeneration == 2 ? "" : "Tg1";
                        text.style = "Other";
                        text.horizontal = horizontal;
                        text.vertical = vertical;
                        text.mirroredX = (flags & 1) != 0;
                        text.mirroredY = (flags & 2) != 0;
                        for (const auto& similarity : transformations) {
                            for (const auto mode : {MirrorText::Readable, MirrorText::Mirrored}) {
                                const auto entity = transformed(text, similarity, mode);
                                const auto& after = std::get<Text>(entity);
                                expectTextImage(text, after, similarity, mode);
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST(Similarity, ReadableTextDirectionAtNinetyAndTwoHundredSeventyDegrees) {
    const auto reflection = *Similarity::reflection({}, {1.0, 0.0});
    const std::array angles{
        kPi / 2.0 - 1e-8,
        kPi / 2.0,
        kPi / 2.0 + 1e-8,
        3.0 * kPi / 2.0 - 1e-8,
        3.0 * kPi / 2.0,
        3.0 * kPi / 2.0 + 1e-8,
    };
    for (const double alpha : angles) {
        Text text;
        text.rotation = kTwoPi - alpha;
        text.value = "Tg1";
        const Text after = std::get<Text>(transformed(text, reflection));
        const bool reverse = alpha > kPi / 2.0 && alpha <= 3.0 * kPi / 2.0;
        const double expected = normalized(reverse ? alpha - kPi : alpha);
        EXPECT_NEAR(after.rotation, expected, kTolerance);
        expectTextImage(text, after, reflection, MirrorText::Readable);
    }
}

TEST(Similarity, TransformQuestionsRejectMalformedArgumentsAndDoNotMutateTheSource) {
    const std::string definition = "L = LINE start=1,2 end=3,4\n";
    const std::array questions{
        "transform L",
        "transform Missing move 1,2",
        "transform L move 1",
        "transform L move 1,2 extra",
        "transform L rotate nan 0,0",
        "transform L scale 2 0",
        "transform L mirror 0,0 1,0 mirrtext=2",
        "transform L mirror 0,0 1,0 unexpected=1",
    };
    for (const auto* question : questions) {
        const std::string input = definition + "? " + question + " -> invalid\n";
        const auto answers = tg::script::answerQuestions(input);
        EXPECT_TRUE(std::holds_alternative<tg::script::ParseError>(answers));
    }
    std::string input = definition;
    input += "? transform L move 2,3 -> LINE start=3,5 end=5,7\n";
    input += "? transform L move 0,0 -> LINE start=1,2 end=3,4\n";
    const auto answers = tg::script::answerQuestions(input);
    ASSERT_TRUE(std::holds_alternative<std::vector<tg::script::Answer>>(answers));
    for (const auto& answer : std::get<std::vector<tg::script::Answer>>(answers)) {
        EXPECT_TRUE(answer.matches) << answer.actual;
    }
}

}  // namespace
}  // namespace tg::core
