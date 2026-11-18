// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Linear and aligned dimensions: value, text and graphics
// (docs/tappe/0.8/PT23-quote-lineari.md).

#include "tg/core/dimension.hpp"

#include "tg/core/dim_style_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-9;

void expectNear(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

/// The width of `text` in the bundled font, with height `height`.
double widthOf(std::u32string_view text, double height) {
    return textWidth(text) * height;
}

Dimension linear(const Vec2& p1, const Vec2& p2, const Vec2& linePoint, double rotation = 0.0) {
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Linear;
    dimension.definition.point13 = p1;
    dimension.definition.point14 = p2;
    dimension.definition.linePoint = linePoint;
    dimension.definition.rotation = rotation;
    return dimension;
}

Dimension aligned(const Vec2& p1, const Vec2& p2, const Vec2& linePoint) {
    Dimension dimension = linear(p1, p2, linePoint);
    dimension.definition.kind = DimensionKind::Aligned;
    dimension.definition.dxfType = 1;
    return dimension;
}

Dimension made(const Document& document, Dimension dimension) {
    std::string reason;
    const std::optional<Dimension> result = regenerated(document, std::move(dimension), &reason);
    EXPECT_TRUE(result.has_value()) << reason;
    return result.value_or(Dimension{});
}

/// A new drawing whose ISO-25 is changed by `change`.
Document drawingWith(const std::function<void(DimStyle&)>& change) {
    Document document = Document::newDrawing();
    DimStyleTable table = document.dimStyles();
    DimStyle style = *table.find("ISO-25");
    change(style);
    table.put(std::move(style), document);
    document.replaceDimStyles(std::move(table));
    return document;
}

std::vector<Line> linesOf(const Dimension& dimension) {
    std::vector<Line> lines;
    for (const EntityRecord& record : dimension.graphics) {
        if (const auto* line = std::get_if<Line>(&record.entity)) {
            lines.push_back(*line);
        }
    }
    return lines;
}

std::vector<Solid> arrowsOf(const Dimension& dimension) {
    std::vector<Solid> arrows;
    for (const EntityRecord& record : dimension.graphics) {
        if (const auto* solid = std::get_if<Solid>(&record.entity)) {
            arrows.push_back(*solid);
        }
    }
    return arrows;
}

MText textOf(const Dimension& dimension) {
    for (const EntityRecord& record : dimension.graphics) {
        if (const auto* text = std::get_if<MText>(&record.entity)) {
            return *text;
        }
    }
    ADD_FAILURE() << "no text";
    return {};
}

/// The dimension of a transformation that is not refused.
Dimension dimensionOf(const TransformOutcome& outcome) {
    if (!outcome.entity || !std::holds_alternative<Dimension>(*outcome.entity)) {
        ADD_FAILURE() << "refused: " << outcome.refusal;
        return {};
    }
    return std::get<Dimension>(*outcome.entity);
}

void expectLine(const Line& line, const Vec2& start, const Vec2& end) {
    expectNear(line.start, start);
    expectNear(line.end, end);
}

/// An arrow with the tip in `tip` and the base in `base`, its first vertex
/// on the side of `up`.
void expectArrow(const Solid& arrow, const Vec2& tip, const Vec2& base, const Vec2& up) {
    const Vec2 side = up * (distance(tip, base) / 6.0);
    expectNear(arrow.vertices[0], base + side);
    expectNear(arrow.vertices[1], base - side);
    expectNear(arrow.vertices[2], tip);
    expectNear(arrow.vertices[3], tip);
}

// --- Text ---------------------------------------------------------------

TEST(DimensionText, DecimalsZerosAndSeparator) {
    DimVariables variables;
    variables.zeroSuppression = 0;
    variables.decimals = 2;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.5, ""), "12,50");
    variables.decimals = 3;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.5, ""), "12,500");
    variables.zeroSuppression = 8;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.5, ""), "12,5");
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.0, ""), "12");
    variables.decimalSeparator = '.';
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.25, ""), "12.25");
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, -0.0001, ""), "0");
}

TEST(DimensionText, RoundAndFactor) {
    DimVariables variables;
    variables.round = 0.5;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.25, ""), "12,5");
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.125, ""), "12");
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, -12.25, ""), "-12,5");
    variables.round = 0.0;
    variables.linearFactor = 2.0;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 5.0, ""), "10");
    variables.round = 0.5;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 3.125, ""), "6,5");
}

TEST(DimensionText, PostAndOverride) {
    DimVariables variables;
    variables.post = "[<>]";
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.0, ""), "[12]");
    variables.post = "mm";
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.0, ""), "12mm");
    variables.post.clear();
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.0, "<> mm"), "12 mm");
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.0, "REF"), "REF");
    EXPECT_EQ(dimensionText(variables, DimensionKind::Linear, 12.0, " "), "");
}

TEST(DimensionText, AnglesIgnoreFactorAndRound) {
    DimVariables variables;
    variables.linearFactor = 2.0;
    variables.round = 4.0;
    EXPECT_EQ(dimensionText(variables, DimensionKind::Angular, 90.0, ""), "90%%d");
    EXPECT_EQ(dimensionText(DimVariables{}, DimensionKind::Radius, 5.0, ""), "R5");
    EXPECT_EQ(dimensionText(DimVariables{}, DimensionKind::Diameter, 5.0, ""), "%%c5");
}

// --- Graphics -----------------------------------------------------------

TEST(DimensionGraphics, WideHorizontalLinear) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, linear({0, 0}, {40, 0}, {40, 10}));
    EXPECT_TRUE(supported(dimension));
    EXPECT_EQ(dimension.value.measurement, 40.0);
    const std::vector<Line> lines = linesOf(dimension);
    ASSERT_EQ(lines.size(), 3U);
    expectLine(lines[0], {0, 0.625}, {0, 11.25});
    expectLine(lines[1], {40, 0.625}, {40, 11.25});
    expectLine(lines[2], {2.5, 10}, {37.5, 10});
    const std::vector<Solid> arrows = arrowsOf(dimension);
    ASSERT_EQ(arrows.size(), 2U);
    expectArrow(arrows[0], {0, 10}, {2.5, 10}, {0, 1});
    expectArrow(arrows[1], {40, 10}, {37.5, 10}, {0, 1});
    const MText text = textOf(dimension);
    EXPECT_EQ(text.value, "40");
    EXPECT_EQ(text.attachment, MTextAttachment::BottomCenter);
    expectNear(text.insertion, {20, 10.625});
    EXPECT_DOUBLE_EQ(text.height, 2.5);
    // The text point is the middle of the text (PT5, point B10).
    expectNear(dimension.presentation.textPoint, {20, 11.875});
    // The graphics are drawn, on layer 0 and ByBlock, as those of a block.
    for (const EntityRecord& record : dimension.graphics) {
        EXPECT_EQ(record.id, EntityId{0});
        EXPECT_EQ(record.properties.layer, document.layerZero());
        EXPECT_EQ(record.properties.color, Color::byBlock());
    }
    const EntityRecord record{EntityId{9}, dimension};
    EXPECT_EQ(generate(document, record).size(), 6U);
}

TEST(DimensionGraphics, NarrowLinearHasTheArrowsOutside) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, linear({0, 0}, {4, 0}, {4, 10}));
    const std::vector<Line> lines = linesOf(dimension);
    ASSERT_EQ(lines.size(), 5U);
    expectLine(lines[2], {-2.5, 10}, {-5, 10});
    expectLine(lines[3], {6.5, 10}, {9, 10});
    expectLine(lines[4], {0, 10}, {4, 10});
    const std::vector<Solid> arrows = arrowsOf(dimension);
    ASSERT_EQ(arrows.size(), 2U);
    expectArrow(arrows[0], {0, 10}, {-2.5, 10}, {0, 1});
    expectArrow(arrows[1], {4, 10}, {6.5, 10}, {0, 1});
    expectNear(textOf(dimension).insertion, {2, 10.625});
}

TEST(DimensionGraphics, TextCentredBreaksTheLineWithDimtadZero) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.overrides = {{"DIMTAD", "0"}};
    dimension = made(document, std::move(dimension));
    const double half = (widthOf(U"40", 2.5) / 2.0) + 0.625;
    const std::vector<Line> lines = linesOf(dimension);
    ASSERT_EQ(lines.size(), 4U);
    expectLine(lines[2], {2.5, 10}, {20 - half, 10});
    expectLine(lines[3], {37.5, 10}, {20 + half, 10});
    const MText text = textOf(dimension);
    EXPECT_EQ(text.attachment, MTextAttachment::MiddleCenter);
    expectNear(text.insertion, {20, 10});
    expectNear(dimension.presentation.textPoint, {20, 10});
}

TEST(DimensionGraphics, AlignedAtThirtyDegrees) {
    const Document document = Document::newDrawing();
    const SinCos thirty = sinCos(kPi / 6.0);
    const Vec2 u{thirty.cosine, thirty.sine};
    const Vec2 n = u.perpendicular();
    const Dimension dimension = made(document, aligned({0, 0}, u * 20.0, n * 10.0));
    EXPECT_NEAR(dimension.value.measurement, 20.0, kTolerance);
    const std::vector<Line> lines = linesOf(dimension);
    ASSERT_EQ(lines.size(), 3U);
    expectLine(lines[0], n * 0.625, n * 11.25);
    expectLine(lines[2], (n * 10.0) + (u * 2.5), (n * 10.0) + (u * 17.5));
    const MText text = textOf(dimension);
    EXPECT_NEAR(text.rotation, kPi / 6.0, kTolerance);
    expectNear(text.insertion, (n * 10.625) + (u * 10.0));
    expectArrow(arrowsOf(dimension)[0], n * 10.0, (n * 10.0) + (u * 2.5), n);
}

TEST(DimensionGraphics, TheFourCasesOfTheFit) {
    const Document document = Document::newDrawing();
    // 1: L = 8, text 8 and two arrows of 2.5 (L >= t + 2a, the AutoCAD sample).
    const Dimension both = made(document, linear({0, 0}, {8, 0}, {8, 10}));
    expectLine(linesOf(both)[2], {2.5, 10}, {5.5, 10});
    expectArrow(arrowsOf(both)[0], {0, 10}, {2.5, 10}, {0, 1});
    // 2: L = 3, the text only (t <= L < t + 2a).
    const Dimension textInside = made(document, linear({0, 0}, {3, 0}, {3, 10}));
    expectArrow(arrowsOf(textInside)[0], {0, 10}, {-2.5, 10}, {0, 1});
    expectNear(textOf(textInside).insertion, {1.5, 10.625});
    // 3: the arrows only (2a <= L < t): the text past f2, DIMASZ + DIMGAP
    // from it, with the dimension line extended to DIMGAP past it.
    Dimension arrowsInside = linear({0, 0}, {12, 0}, {12, 10});
    arrowsInside.presentation.textOverride = "WWWWW";
    arrowsInside = made(document, std::move(arrowsInside));
    const double wide = widthOf(U"WWWWW", 2.5);
    ASSERT_GT(wide, 12.0);
    const std::vector<Line> lines3 = linesOf(arrowsInside);
    ASSERT_EQ(lines3.size(), 4U);
    expectLine(lines3[2], {12, 10}, {12 + 2.5 + 0.625 + wide + 0.625, 10});
    expectLine(lines3[3], {2.5, 10}, {9.5, 10});
    expectArrow(arrowsOf(arrowsInside)[1], {12, 10}, {9.5, 10}, {0, 1});
    expectNear(textOf(arrowsInside).insertion, {12 + 3.125 + (wide / 2.0), 10.625});
    // 4: L = 1.5, all outside: the text 2·DIMASZ + DIMGAP past f2.
    const Dimension outside = made(document, linear({0, 0}, {1.5, 0}, {1.5, 10}));
    const double t = widthOf(U"1,5", 2.5);
    const std::vector<Line> lines4 = linesOf(outside);
    ASSERT_EQ(lines4.size(), 5U);
    expectLine(lines4[2], {-2.5, 10}, {-5, 10});
    expectLine(lines4[3], {4, 10}, {1.5 + 5.625 + t + 0.625, 10});
    expectLine(lines4[4], {0, 10}, {1.5, 10});
    expectNear(textOf(outside).insertion, {1.5 + 5.625 + (t / 2.0), 10.625});
}

TEST(DimensionGraphics, ZeroMeasurementHasNoPieceOfLengthZero) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, linear({0, 0}, {0, 10}, {0, 20}));
    EXPECT_EQ(dimension.value.measurement, 0.0);
    EXPECT_EQ(textOf(dimension).value, "0");
    for (const Line& line : linesOf(dimension)) {
        EXPECT_GT(line.length(), 0.0);
    }
    for (const Solid& arrow : arrowsOf(dimension)) {
        EXPECT_NE(arrow.vertices[0], arrow.vertices[2]);
    }
}

TEST(DimensionGraphics, AlignedWithCoincidentPointsIsNotRegenerable) {
    std::string reason;
    EXPECT_FALSE(
        regenerated(Document::newDrawing(), aligned({1, 1}, {1, 1}, {5, 5}), &reason).has_value());
    EXPECT_EQ(reason, "Points coincide.");
}

TEST(DimensionGraphics, DimscaleScalesTheGraphicsNotTheMeasurement) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {10, 0}, {10, 10});
    dimension.presentation.overrides = {{"DIMSCALE", "2"}};
    dimension = made(document, std::move(dimension));
    EXPECT_EQ(dimension.value.measurement, 10.0);
    EXPECT_EQ(textOf(dimension).value, "10");
    EXPECT_DOUBLE_EQ(textOf(dimension).height, 5.0);
    for (const Solid& arrow : arrowsOf(dimension)) {
        EXPECT_NEAR(distance(arrow.vertices[2], (arrow.vertices[0] + arrow.vertices[1]) * 0.5), 5.0,
                    kTolerance);
    }
}

TEST(DimensionGraphics, Dimsd1RemovesTheHalfOnTheSideOfTheFirstFoot) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, -10}, {20, -10}, {20, 0});
    dimension.presentation.overrides = {{"DIMSD1", "1"}, {"DIMSE1", "1"}, {"DIMSE2", "1"}};
    dimension = made(document, std::move(dimension));
    const std::vector<Solid> arrows = arrowsOf(dimension);
    ASSERT_EQ(arrows.size(), 1U);
    expectNear(arrows[0].vertices[2], {20, 0});
    const std::vector<Line> lines = linesOf(dimension);
    ASSERT_EQ(lines.size(), 1U);
    expectLine(lines[0], {10, 0}, {17.5, 0});
}

TEST(DimensionGraphics, SuppressionsComeAfterTheFit) {
    // L = 8, arrows of 2, a text 6 wide with its margins: case 2 with two
    // arrows, which stays case 2 when one arrow is suppressed.
    const Document document = drawingWith([](DimStyle& style) {
        style.vars.arrowSize = 2.0;
        style.vars.textHeight = 4.75 / textWidth(U"MMMM");
    });
    for (const char* const suppressed : {"DIMSD1", "DIMSD2"}) {
        Dimension dimension = linear({0, -10}, {8, -10}, {8, 0});
        dimension.presentation.textOverride = "MMMM";
        dimension.presentation.overrides = {{suppressed, "1"}};
        dimension = made(document, std::move(dimension));
        const std::vector<Solid> arrows = arrowsOf(dimension);
        ASSERT_EQ(arrows.size(), 1U) << suppressed;
        const bool first = std::string{suppressed} == "DIMSD1";
        if (first) {
            expectArrow(arrows[0], {8, 0}, {10, 0}, {0, 1});
        } else {
            expectArrow(arrows[0], {0, 0}, {-2, 0}, {0, 1});
        }
        const std::vector<Line> lines = linesOf(dimension);
        ASSERT_EQ(lines.size(), 4U) << suppressed;
        if (first) {
            expectLine(lines[2], {10, 0}, {12, 0});
            expectLine(lines[3], {4, 0}, {8, 0});
        } else {
            expectLine(lines[2], {-2, 0}, {-4, 0});
            expectLine(lines[3], {0, 0}, {4, 0});
        }
    }
}

TEST(DimensionGraphics, ColoursAndLineweightsOfTheVariables) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.overrides = {
        {"DIMCLRD", "1"}, {"DIMCLRE", "3"}, {"DIMCLRT", "5"}, {"DIMLWD", "50"}, {"DIMLWE", "70"}};
    dimension = made(document, std::move(dimension));
    const std::vector<EntityRecord>& graphics = dimension.graphics;
    ASSERT_EQ(graphics.size(), 6U);
    EXPECT_EQ(graphics[0].properties.color, Color::fromIndex(3));
    EXPECT_EQ(graphics[0].properties.lineweight, 70);
    EXPECT_EQ(graphics[2].properties.color, Color::fromIndex(1));
    EXPECT_EQ(graphics[3].properties.lineweight, 50);
    EXPECT_EQ(graphics[5].properties.color, Color::fromIndex(5));
}

// --- Regeneration -------------------------------------------------------

TEST(DimensionRegeneration, DimtolOneIsRefused) {
    const Document document =
        drawingWith([](DimStyle& style) { style.extras.push_back(RawGroup{71, "1"}); });
    std::string reason;
    EXPECT_FALSE(regenerated(document, linear({0, 0}, {40, 0}, {40, 10}), &reason).has_value());
    EXPECT_EQ(reason, kDimensionNotRegenerable);
    Dimension overridden = linear({0, 0}, {40, 0}, {40, 10});
    overridden.presentation.overrides = {{"DIMTOL", "1"}};
    EXPECT_FALSE(regenerated(Document::newDrawing(), overridden).has_value());
}

TEST(DimensionRegeneration, DimatfitOneIsNotRegenerable) {
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.overrides = {{"DIMATFIT", "1"}};
    EXPECT_FALSE(regenerated(Document::newDrawing(), dimension).has_value());
    dimension.presentation.overrides = {{"DIMATFIT", "3"}};
    EXPECT_TRUE(regenerated(Document::newDrawing(), dimension).has_value());
}

TEST(DimensionRegeneration, KindsOfOtherPackages) {
    std::string reason;
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.definition.kind = DimensionKind::Other;
    EXPECT_FALSE(regenerated(Document::newDrawing(), dimension, &reason).has_value());
    EXPECT_EQ(reason, kDimensionNotRegenerable);
    dimension.definition.kind = DimensionKind::Radius;
    EXPECT_FALSE(regenerated(Document::newDrawing(), dimension, &reason).has_value());
    EXPECT_EQ(reason, kDimensionKindNotRealized);
}

// --- Transformations and grips -------------------------------------------

TEST(DimensionTransform, TranslationMovesWithoutRegenerating) {
    const Document document = Document::newDrawing();
    Dimension dimension = made(document, linear({0, 0}, {40, 0}, {40, 10}));
    dimension.value.measurement = 123.0;  // kept: not regenerated
    const TransformOutcome outcome =
        transformedOf(document, dimension, Similarity::translation({5, 7}), MirrorText::Readable);
    const Dimension moved = dimensionOf(outcome);
    EXPECT_EQ(moved.value.measurement, 123.0);
    expectNear(moved.definition.point13, {5, 7});
    expectLine(linesOf(moved)[2], {7.5, 17}, {42.5, 17});
    expectNear(textOf(moved).insertion, {25, 17.625});
}

TEST(DimensionTransform, RotationScaleAndMirrorRegenerate) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, linear({0, 0}, {40, 0}, {40, 10}));

    const TransformOutcome rotated = transformedOf(
        document, dimension, Similarity::rotation(kPi / 2.0, {0, 0}), MirrorText::Readable);
    const Dimension turned = dimensionOf(rotated);
    EXPECT_NEAR(turned.definition.rotation, kPi / 2.0, kTolerance);
    EXPECT_NEAR(turned.value.measurement, 40.0, kTolerance);
    expectLine(linesOf(turned)[2], {-10, 2.5}, {-10, 37.5});
    EXPECT_NEAR(textOf(turned).rotation, kPi / 2.0, kTolerance);

    const TransformOutcome scaled =
        transformedOf(document, dimension, Similarity::scaling(2.0, {0, 0}).value_or(Similarity{}),
                      MirrorText::Readable);
    const Dimension doubled = dimensionOf(scaled);
    EXPECT_NEAR(doubled.value.measurement, 80.0, kTolerance);
    EXPECT_EQ(textOf(doubled).value, "80");
    EXPECT_DOUBLE_EQ(textOf(doubled).height, 2.5);  // DIMTXT, not scaled

    const TransformOutcome mirrored = transformedOf(
        document, dimension, Similarity::reflection({0, 0}, {1, 0}).value_or(Similarity{}),
        MirrorText::Mirrored);
    const Dimension reflected = dimensionOf(mirrored);
    EXPECT_NEAR(sin(reflected.definition.rotation), 0.0, kTolerance);
    expectLine(linesOf(reflected)[2], {2.5, -10}, {37.5, -10});
    // Readable: upright, above the line.
    EXPECT_NEAR(sin(textOf(reflected).rotation), 0.0, kTolerance);
    EXPECT_GT(cos(textOf(reflected).rotation), 0.0);
    expectNear(textOf(reflected).insertion, {20, -9.375});
}

TEST(DimensionTransform, ExplicitAngleTurnsWithTheDimension) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.textRotation = kPi / 6.0;
    dimension = made(document, std::move(dimension));
    const TransformOutcome outcome = transformedOf(
        document, dimension, Similarity::rotation(kPi / 2.0, {0, 0}), MirrorText::Readable);
    const Dimension turned = dimensionOf(outcome);
    EXPECT_NEAR(turned.presentation.textRotation.value_or(0.0), 2.0 * kPi / 3.0, kTolerance);
    EXPECT_NEAR(textOf(turned).rotation, 2.0 * kPi / 3.0, kTolerance);
}

TEST(DimensionTransform, NotRegenerableIsRefused) {
    const Dimension dimension = made(Document::newDrawing(), linear({0, 0}, {40, 0}, {40, 10}));
    const Document document =
        drawingWith([](DimStyle& style) { style.extras.push_back(RawGroup{71, "1"}); });
    const TransformOutcome outcome = transformedOf(
        document, dimension, Similarity::rotation(kPi / 2.0, {0, 0}), MirrorText::Readable);
    EXPECT_FALSE(outcome.entity.has_value());
    EXPECT_EQ(outcome.refusal, kDimensionNotRegenerable);
    // A translation always works (docs/adr/0047, point 4).
    EXPECT_TRUE(
        transformedOf(document, dimension, Similarity::translation({1, 1}), MirrorText::Readable)
            .entity.has_value());
}

TEST(DimensionGrips, TextGripMovesOnlyTheText) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.overrides = {{"DIMTAD", "1"}};
    dimension = made(document, std::move(dimension));
    const std::vector<Vec2> grips = gripPointsOf(dimension);
    ASSERT_EQ(grips.size(), 4U);
    expectNear(grips[3], dimension.presentation.textPoint);
    const GripResult result = moveGripOf(document, dimension, 3, {15, 8});
    ASSERT_EQ(result.outcome, GripOutcome::Moved);
    const auto& moved = std::get<Dimension>(result.entity);
    EXPECT_TRUE(moved.presentation.userTextPosition);
    expectNear(moved.presentation.textPoint, {15, 8});
    const MText text = textOf(moved);
    EXPECT_EQ(text.attachment, MTextAttachment::MiddleCenter);
    expectNear(text.insertion, {15, 8});
    EXPECT_EQ(linesOf(moved), linesOf(dimension));
    // Regenerated, the text stays with its middle in the point.
    const Dimension again = made(document, moved);
    expectNear(textOf(again).insertion, {15, 8});
    EXPECT_EQ(textOf(again).attachment, MTextAttachment::MiddleCenter);
}

TEST(DimensionGrips, LinePointGripRegenerates) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, linear({0, 0}, {40, 0}, {40, 10}));
    const GripResult result = moveGripOf(document, dimension, 2, {40, 20});
    ASSERT_EQ(result.outcome, GripOutcome::Moved);
    expectLine(linesOf(std::get<Dimension>(result.entity))[2], {2.5, 20}, {37.5, 20});
}

TEST(DimensionGrips, GripOnANotRegenerableDimensionIsRefused) {
    const Dimension dimension = made(Document::newDrawing(), linear({0, 0}, {40, 0}, {40, 10}));
    const Document document =
        drawingWith([](DimStyle& style) { style.extras.push_back(RawGroup{71, "1"}); });
    const GripResult result = moveGripOf(document, dimension, 0, {5, 0});
    EXPECT_EQ(result.outcome, GripOutcome::Invalid);
    EXPECT_EQ(result.refusal, kDimensionNotRegenerable);
    EXPECT_EQ(std::get<Dimension>(result.entity), dimension);
}

// --- Text angle -----------------------------------------------------------

TEST(DimensionTextAngle, ExplicitAngleHoldsWithDimtih) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.textRotation = kPi / 6.0;
    dimension.presentation.overrides = {{"DIMTIH", "1"}};
    dimension = made(document, std::move(dimension));
    EXPECT_NEAR(textOf(dimension).rotation, kPi / 6.0, kTolerance);
}

TEST(DimensionTextAngle, ZeroOnAnAlignedAtFortyFiveDegrees) {
    const Document document = Document::newDrawing();
    Dimension dimension = aligned({0, 0}, {20, 20}, {40, 10});
    dimension.presentation.textRotation = 0.0;
    dimension = made(document, std::move(dimension));
    EXPECT_NEAR(sin(textOf(dimension).rotation), 0.0, kTolerance);
    EXPECT_GT(cos(textOf(dimension).rotation), 0.0);
}

TEST(DimensionTextAngle, UserPositionPutsTheMiddleOfTheText) {
    const Document document = Document::newDrawing();
    Dimension dimension = linear({0, 0}, {40, 0}, {40, 10});
    dimension.presentation.textPoint = {15, 8};
    dimension.presentation.userTextPosition = true;
    dimension.presentation.overrides = {{"DIMTAD", "1"}};
    dimension = made(document, std::move(dimension));
    expectNear(dimension.presentation.textPoint, {15, 8});
    const MText text = textOf(dimension);
    EXPECT_EQ(text.attachment, MTextAttachment::MiddleCenter);
    expectNear(text.insertion, {15, 8});
}

/// The middle of the rows of the horizontal text of `dimension`, as they are
/// drawn: from the last baseline to the top of the first row, from the
/// leftmost start to the rightmost end.
Vec2 middleOfTheRows(const Document& document, const Dimension& dimension) {
    const MText text = textOf(dimension);
    double bottom = 0.0;
    double top = 0.0;
    double left = 0.0;
    double right = 0.0;
    bool first = true;
    for (const GeneratedItem& item : generate(document, EntityRecord{EntityId{9}, text})) {
        const Text& row = std::get<Text>(item.shape);
        const double end = row.position.x + widthOf(displayText(row.value), row.height);
        bottom = first ? row.position.y : std::min(bottom, row.position.y);
        top = first ? row.position.y + row.height : std::max(top, row.position.y + row.height);
        left = first ? row.position.x : std::min(left, row.position.x);
        right = first ? end : std::max(right, end);
        first = false;
    }
    EXPECT_FALSE(first) << "no row";
    return {(left + right) / 2.0, (bottom + top) / 2.0};
}

TEST(DimensionTextPoint, MiddleOfATextOfTwoRowsAboveTheLine) {
    const Document document = Document::newDrawing();
    // Inside: above the line, from its middle (DIMTAD 1 of ISO-25).
    Dimension inside = linear({0, 0}, {40, 0}, {40, 10});
    inside.presentation.textOverride = "A\\PB";
    inside = made(document, std::move(inside));
    ASSERT_EQ(textOf(inside).attachment, MTextAttachment::BottomCenter);
    ASSERT_EQ(generate(document, EntityRecord{EntityId{9}, textOf(inside)}).size(), 2U);
    const Vec2 middleInside = middleOfTheRows(document, inside);
    expectNear(inside.presentation.textPoint, middleInside);
    // Higher than the middle of one row (11.875): the second row counts.
    EXPECT_GT(middleInside.y, 11.875 + 1.0);
    // Outside (case 3): past f2, above the dimension line extended.
    Dimension outside = linear({0, 0}, {12, 0}, {12, 10});
    outside.presentation.textOverride = "WWWWW\\PW";
    outside = made(document, std::move(outside));
    const double wide = widthOf(U"WWWWW", 2.5);
    ASSERT_GT(wide, 12.0);
    ASSERT_EQ(textOf(outside).attachment, MTextAttachment::BottomCenter);
    expectNear(textOf(outside).insertion, {12 + 3.125 + (wide / 2.0), 10.625});
    expectNear(outside.presentation.textPoint, middleOfTheRows(document, outside));
    // The grip of the text is that point.
    expectNear(gripPointsOf(outside)[3], middleOfTheRows(document, outside));
}

// --- Snaps ----------------------------------------------------------------

SnapModes only(bool SnapModes::*mode) {
    SnapModes modes{false, false, false, false, false, false, false, false};
    modes.*mode = true;
    return modes;
}

TEST(DimensionSnaps, TheTextGivesItsInsertionPointNotThoseOfItsRows) {
    Document document = Document::newDrawing();
    const Dimension dimension = made(document, linear({0, 0}, {40, 0}, {40, 10}));
    const MText text = textOf(dimension);
    ASSERT_EQ(text.attachment, MTextAttachment::BottomCenter);
    const EntityRecord record{EntityId{9}, dimension};
    // The row starts left of the insertion point of the MTEXT.
    const Text row =
        std::get<Text>(generate(document, EntityRecord{EntityId{9}, text}).at(0).shape);
    ASSERT_LT(row.position.x, text.insertion.x - 1.0);

    std::vector<SnapPoint> insertions;
    appendSnapPoints(document, record, only(&SnapModes::insertion), insertions, std::nullopt);
    EXPECT_EQ(insertions, (std::vector<SnapPoint>{{text.insertion, SnapKind::Insertion}}));

    // The other shapes of the graphics give theirs, as before.
    std::vector<SnapPoint> ends;
    appendSnapPoints(document, record, only(&SnapModes::endpoint), ends, std::nullopt);
    for (const Vec2& end : {Vec2{0, 0.625}, Vec2{0, 11.25}, Vec2{2.5, 10}, Vec2{37.5, 10}}) {
        EXPECT_TRUE(std::ranges::any_of(ends,
                                        [&](const SnapPoint& snap) {
                                            return snap.kind == SnapKind::Endpoint &&
                                                   distance(snap.point, end) < kTolerance;
                                        }))
            << end.x << ", " << end.y;
    }

    // Through the document: the insertion point of the MTEXT, never the
    // start of a row.
    document.add(dimension);
    const std::optional<SnapPoint> near =
        findSnap(document, text.insertion + Vec2{0.1, 0.1}, 0.5, only(&SnapModes::insertion));
    EXPECT_EQ(near, (std::optional<SnapPoint>{SnapPoint{text.insertion, SnapKind::Insertion}}));
    EXPECT_FALSE(findSnap(document, row.position, 0.5, only(&SnapModes::insertion)).has_value());
}

}  // namespace
}  // namespace tg::core
