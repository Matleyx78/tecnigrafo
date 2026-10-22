// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text.hpp"

#include "tg/core/document.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <utility>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-12;

// Liberation Sans 2.1.5, read independently from the font file with
// fontTools, in units of capital-letter height (docs/adr/0013, point 3).
constexpr double kAscent = 1.3158268275372604;
constexpr double kDescent = -0.3080198722498226;
constexpr double kHelloWidth = 3.312278211497516;

void expectNear(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

Text text(std::string value, Vec2 position = {}, double height = 1.0, double rotation = 0.0,
          double widthFactor = 1.0) {
    Text result;
    result.position = position;
    result.height = height;
    result.rotation = rotation;
    result.widthFactor = widthFactor;
    result.value = std::move(value);
    return result;
}

TEST(Text, DefaultsAreThoseOfTheDxfFormat) {
    const Text defaults;
    EXPECT_EQ(defaults.position, (Vec2{}));
    EXPECT_EQ(defaults.alignmentPoint, (Vec2{}));
    EXPECT_EQ(defaults.height, 2.5);
    EXPECT_EQ(defaults.rotation, 0.0);
    EXPECT_EQ(defaults.widthFactor, 1.0);
    EXPECT_EQ(defaults.obliqueAngle, 0.0);
    EXPECT_EQ(defaults.style, "Standard");
    EXPECT_FALSE(defaults.mirroredX);
    EXPECT_FALSE(defaults.mirroredY);
    EXPECT_EQ(defaults.horizontal, TextHorizontal::Left);
    EXPECT_EQ(defaults.vertical, TextVertical::Baseline);
    EXPECT_TRUE(defaults.value.empty());
}

TEST(Text, BoxOfAnUnrotatedText) {
    const std::array<Vec2, 4> box = textBox(text("Hello"));
    expectNear(box[0], {0.0, kDescent});
    expectNear(box[1], {kHelloWidth, kDescent});
    expectNear(box[2], {kHelloWidth, kAscent});
    expectNear(box[3], {0.0, kAscent});
}

TEST(Text, BoxScalesWithHeightAndWidthFactor) {
    const std::array<Vec2, 4> box = textBox(text("Hello", {10.0, 20.0}, 5.0, 0.0, 0.8));
    expectNear(box[0], {10.0, 20.0 + (5.0 * kDescent)});
    expectNear(box[1], {10.0 + (5.0 * 0.8 * kHelloWidth), 20.0 + (5.0 * kDescent)});
    expectNear(box[2], {10.0 + (5.0 * 0.8 * kHelloWidth), 20.0 + (5.0 * kAscent)});
    expectNear(box[3], {10.0, 20.0 + (5.0 * kAscent)});
}

TEST(Text, BoxTurnsAboutThePosition) {
    // A quarter turn: the baseline goes up, the top of the letters left.
    const std::array<Vec2, 4> box = textBox(text("Hello", {1.0, 2.0}, 2.0, kPi / 2.0));
    expectNear(box[0], {1.0 - (2.0 * kDescent), 2.0});
    expectNear(box[1], {1.0 - (2.0 * kDescent), 2.0 + (2.0 * kHelloWidth)});
    expectNear(box[2], {1.0 - (2.0 * kAscent), 2.0 + (2.0 * kHelloWidth)});
    expectNear(box[3], {1.0 - (2.0 * kAscent), 2.0});

    // Any rotation: the sides keep their lengths and stay perpendicular.
    const double angle = degreesToRadians(30.0);
    const std::array<Vec2, 4> turned = textBox(text("Hello", {-3.0, 4.0}, 2.5, angle, 1.25));
    EXPECT_NEAR(distance(turned[0], turned[1]), 2.5 * 1.25 * kHelloWidth, kTolerance);
    EXPECT_NEAR(distance(turned[1], turned[2]), 2.5 * (kAscent - kDescent), kTolerance);
    EXPECT_NEAR(dot(turned[1] - turned[0], turned[3] - turned[0]), 0.0, kTolerance);
    EXPECT_NEAR(std::atan2(turned[1].y - turned[0].y, turned[1].x - turned[0].x), angle,
                kTolerance);
}

TEST(Text, BoundingBoxIsTheBoxOfTheTurnedRectangle) {
    const Text turned = text("Hello", {0.0, 0.0}, 1.0, kPi / 4.0);
    BBox expected;
    for (const Vec2& corner : textBox(turned)) {
        expected.expand(corner);
    }
    EXPECT_EQ(turned.boundingBox().min(), expected.min());
    EXPECT_EQ(turned.boundingBox().max(), expected.max());
    EXPECT_EQ(boundingBox(Entity{turned}).min(), expected.min());
    EXPECT_EQ(boundingBox(Entity{turned}).max(), expected.max());
    // The top left corner, not the bottom left one, is the leftmost point.
    EXPECT_LT(expected.min().x, textBox(turned)[0].x);
    EXPECT_EQ(expected.min().x, textBox(turned)[3].x);
}

TEST(Text, EmptyTextHasABoxWithoutWidth) {
    const std::array<Vec2, 4> box = textBox(text("", {5.0, 5.0}, 2.0));
    expectNear(box[0], {5.0, 5.0 + (2.0 * kDescent)});
    expectNear(box[1], box[0]);
    expectNear(box[2], {5.0, 5.0 + (2.0 * kAscent)});
    expectNear(box[3], box[2]);
    EXPECT_FALSE(text("").boundingBox().isEmpty());
}

TEST(Text, ControlCodesAreConverted) {
    EXPECT_EQ(displayText("%%c20"), U"⌀20");
    EXPECT_EQ(displayText("45%%d"), U"45°");
    EXPECT_EQ(displayText("%%p0.1"), U"±0.1");
    EXPECT_EQ(displayText("100%%%"), U"100%");
    EXPECT_EQ(displayText("%%C %%D %%P"), U"⌀ ° ±");  // as AutoCAD
    EXPECT_EQ(displayText("%%c%%d"), U"⌀°");
    EXPECT_EQ(displayText("è ⌀"), U"è ⌀");  // UTF-8 decoded
}

TEST(Text, OtherAndIncompleteCodesStayAsWritten) {
    EXPECT_EQ(displayText(""), U"");
    EXPECT_EQ(displayText("10%%"), U"10%%");
    EXPECT_EQ(displayText("%%"), U"%%");
    EXPECT_EQ(displayText("%"), U"%");
    EXPECT_EQ(displayText("%%x"), U"%%x");
    EXPECT_EQ(displayText("%%o%%u%%123"), U"%%o%%u%%123");  // until tappa 0.8
    // "%%%" is converted first; the "c" that follows is an ordinary letter.
    EXPECT_EQ(displayText("%%%c"), U"%c");
}

TEST(Text, ControlCodesChangeTheBoxAsTheConvertedCharacters) {
    for (const auto& [code, converted] :
         std::array{std::pair{"%%c20", "⌀20"}, std::pair{"45%%d", "45°"},
                    std::pair{"%%p0.1", "±0.1"}, std::pair{"100%%%", "100%"}}) {
        EXPECT_EQ(textBox(text(code)), textBox(text(converted))) << code;
    }
    // The diameter sign is measured as Ø, its substitute in the bundled font.
    EXPECT_NEAR(textBox(text("%%c"))[1].x, advanceWidth(U'Ø'), kTolerance);
    // Incomplete codes are measured as written.
    EXPECT_NEAR(textBox(text("10%%"))[1].x, textWidth(U"10%%"), kTolerance);
    EXPECT_NEAR(textBox(text("%%x"))[1].x, textWidth(U"%%x"), kTolerance);
}

TEST(Text, AllFifteenAlignmentsPlaceTheBaselineAndCorners) {
    // ADR 0022: capital top/middle use h and h/2, not the font ascent.
    // P1=(1,2), P2=(10,20), h=2, f=0.8 for ordinary alignments.
    constexpr double kWidth = kHelloWidth * 2.0 * 0.8;

    struct AlignmentCase {
        TextHorizontal horizontal{TextHorizontal::Left};
        TextVertical vertical{TextVertical::Baseline};
        Vec2 origin;
    };

    const std::array cases{
        AlignmentCase{TextHorizontal::Left, TextVertical::Baseline, {1, 2}},
        AlignmentCase{TextHorizontal::Left, TextVertical::Bottom, {10, 20 - 2 * kDescent}},
        AlignmentCase{TextHorizontal::Left, TextVertical::Middle, {10, 19}},
        AlignmentCase{TextHorizontal::Left, TextVertical::Top, {10, 18}},
        AlignmentCase{TextHorizontal::Center, TextVertical::Baseline, {10 - kWidth / 2, 20}},
        AlignmentCase{
            TextHorizontal::Center, TextVertical::Bottom, {10 - kWidth / 2, 20 - 2 * kDescent}},
        AlignmentCase{TextHorizontal::Center, TextVertical::Middle, {10 - kWidth / 2, 19}},
        AlignmentCase{TextHorizontal::Center, TextVertical::Top, {10 - kWidth / 2, 18}},
        AlignmentCase{TextHorizontal::Right, TextVertical::Baseline, {10 - kWidth, 20}},
        AlignmentCase{
            TextHorizontal::Right, TextVertical::Bottom, {10 - kWidth, 20 - 2 * kDescent}},
        AlignmentCase{TextHorizontal::Right, TextVertical::Middle, {10 - kWidth, 19}},
        AlignmentCase{TextHorizontal::Right, TextVertical::Top, {10 - kWidth, 18}},
        AlignmentCase{
            TextHorizontal::Middle, TextVertical::Top, {10 - kWidth / 2, 20 - (1 + kDescent)}}};
    for (const auto& entry : cases) {
        SCOPED_TRACE(static_cast<int>(entry.horizontal));
        SCOPED_TRACE(static_cast<int>(entry.vertical));
        Text value = text("Hello", {1, 2}, 2, 0, 0.8);
        value.alignmentPoint = {10, 20};
        value.horizontal = entry.horizontal;
        value.vertical = entry.vertical;
        const TextFrame frame = textFrame(value);
        expectNear(frame.origin, entry.origin);
        expectNear(frame.xAxis, {1, 0});
        expectNear(frame.yAxis, {0, 1});
        const std::array<Vec2, 4> expected{
            entry.origin + Vec2{0, 2 * kDescent}, entry.origin + Vec2{kWidth, 2 * kDescent},
            entry.origin + Vec2{kWidth, 2 * kAscent}, entry.origin + Vec2{0, 2 * kAscent}};
        const auto box = textBox(value);
        for (std::size_t i = 0; i < box.size(); ++i) {
            expectNear(box.at(i), expected.at(i));
        }
    }
    // A 3-4-5 baseline: used rotation ignores group 50, vertical ignores 73.
    for (const auto horizontal : {TextHorizontal::Aligned, TextHorizontal::Fit}) {
        Text value = text("Hello", {1, 2}, 2, 0.5, 0.8);
        value.alignmentPoint = {4, 6};
        value.horizontal = horizontal;
        value.vertical = TextVertical::Top;
        const Text stored = value;
        const double height = horizontal == TextHorizontal::Aligned ? 5 / (kHelloWidth * 0.8) : 2;
        const TextFrame frame = textFrame(value);
        expectNear(frame.origin, {1, 2});
        expectNear(frame.insertion, {1, 2});
        expectNear(frame.xAxis, {0.6, 0.8});
        expectNear(frame.yAxis, {-0.8, 0.6});
        EXPECT_NEAR(frame.height, height, kTolerance);
        EXPECT_NEAR(frame.widthFactor,
                    horizontal == TextHorizontal::Fit ? 5 / (kHelloWidth * 2) : 0.8, kTolerance);
        EXPECT_NEAR(frame.rotation, std::atan2(4.0, 3.0), kTolerance);
        const std::array<Vec2, 4> expected{
            Vec2{1 - 0.8 * kDescent * height, 2 + 0.6 * kDescent * height},
            Vec2{4 - 0.8 * kDescent * height, 6 + 0.6 * kDescent * height},
            Vec2{4 - 0.8 * kAscent * height, 6 + 0.6 * kAscent * height},
            Vec2{1 - 0.8 * kAscent * height, 2 + 0.6 * kAscent * height}};
        const auto box = textBox(value);
        for (std::size_t i = 0; i < box.size(); ++i) {
            expectNear(box.at(i), expected.at(i));
        }
        EXPECT_EQ(value, stored);
    }
}

TEST(Text, ObliqueRotationAndMirroringTransformEveryCornerAboutTheAnchor) {
    // tan(15 degrees)=2-sqrt(3); rotation is 30 degrees. MC anchors at (w/2,h/2).
    constexpr double kCosine = std::numbers::sqrt3 / 2.0;
    for (const double shear : {2 - std::numbers::sqrt3, std::numbers::sqrt3 - 2}) {
        for (const bool mirrorX : {false, true}) {
            for (const bool mirrorY : {false, true}) {
                Text value = text("Hello", {1, 2}, 2, kPi / 6, 0.8);
                value.alignmentPoint = {10, 20};
                value.horizontal = TextHorizontal::Center;
                value.vertical = TextVertical::Middle;
                value.obliqueAngle = std::atan(shear);
                value.mirroredX = mirrorX;
                value.mirroredY = mirrorY;
                const double width = kHelloWidth * 2 * 0.8;
                const auto place = [&](double x, double y) {
                    const double shearedX = (mirrorX ? -1 : 1) * (x - width / 2 + shear * (y - 1));
                    const double mirroredY = (mirrorY ? -1 : 1) * (y - 1);
                    return Vec2{10 + kCosine * shearedX - 0.5 * mirroredY,
                                20 + 0.5 * shearedX + kCosine * mirroredY};
                };
                expectNear(textFrame(value).origin, place(0, 0));
                const auto box = textBox(value);
                const std::array expected{place(0, 2 * kDescent), place(width, 2 * kDescent),
                                          place(width, 2 * kAscent), place(0, 2 * kAscent)};
                for (std::size_t i = 0; i < box.size(); ++i) {
                    expectNear(box.at(i), expected.at(i));
                }
                EXPECT_EQ(cross(box[1] - box[0], box[3] - box[0]) > 0, mirrorX == mirrorY);
            }
        }
    }
}

TEST(Text, DegenerateAlignedAndFitUseStoredLeftBaselineGeometry) {
    for (const auto horizontal : {TextHorizontal::Aligned, TextHorizontal::Fit}) {
        for (const bool empty : {false, true}) {
            Text value = text(empty ? "" : "Hello", {1, 2}, 2, 0.5, 0.8);
            value.alignmentPoint = empty ? Vec2{4, 6} : value.position;
            value.horizontal = horizontal;
            value.vertical = TextVertical::Top;
            Text left = value;
            left.horizontal = TextHorizontal::Left;
            left.vertical = TextVertical::Baseline;
            const TextFrame frame = textFrame(value);
            expectNear(frame.origin, value.position);
            EXPECT_DOUBLE_EQ(frame.height, 2);
            EXPECT_DOUBLE_EQ(frame.widthFactor, 0.8);
            EXPECT_DOUBLE_EQ(frame.rotation, 0.5);
            EXPECT_EQ(textBox(value), textBox(left));
        }
    }
}

TEST(Text, VeryShortAlignedBaselineIsNotCollapsed) {
    Text value = text("Hello", {}, 2, 1, 0.8);
    value.horizontal = TextHorizontal::Aligned;
    value.alignmentPoint = {1e-18, 0};
    const TextFrame frame = textFrame(value);
    EXPECT_NEAR(frame.height, 1e-18 / (kHelloWidth * 0.8), 1e-32);
    EXPECT_DOUBLE_EQ(frame.rotation, 0);
}

TEST(Text, UnusedDxfPropertiesDoNotChangeLeftBaselineGeometry) {
    const Text plain = text("Hello", {1.0, 2.0}, 2.0, 0.5);
    Text full = plain;
    full.alignmentPoint = {7.0, 8.0};
    full.style = "Other";
    EXPECT_NE(plain, full);
    EXPECT_EQ(textBox(plain), textBox(full));
}

}  // namespace
}  // namespace tg::core
