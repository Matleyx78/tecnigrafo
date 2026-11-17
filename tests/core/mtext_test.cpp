// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext.hpp"

#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/text_style_table.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-12;

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kTolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

/// The width of `text` in the bundled font at height `height`.
double widthOf(std::u32string_view text, double height = 1.0) {
    return textWidth(text) * height;
}

MText mtext(std::string value, double height = 1.0, double width = 0.0) {
    MText result;
    result.insertion = {0, 0};
    result.height = height;
    result.width = width;
    result.value = std::move(value);
    return result;
}

/// The Texts that `entity` generates, alone in a new drawing (or `document`).
std::vector<Text> rows(const MText& entity, Document document = Document::newDrawing()) {
    const EntityRecord record = *document.find(document.add(entity));
    std::vector<Text> texts;
    for (const GeneratedItem& item : generate(document, record)) {
        const Text* text = std::get_if<Text>(&item.shape);
        EXPECT_NE(text, nullptr);
        if (text != nullptr) {
            texts.push_back(*text);
        }
    }
    return texts;
}

std::vector<std::string> values(const MText& entity) {
    std::vector<std::string> result;
    for (const Text& text : rows(entity)) {
        result.push_back(text.value);
    }
    return result;
}

using Values = std::vector<std::string>;

TEST(MTextSupport, IsSupported) {
    EXPECT_TRUE(supported(MText{}));
    EXPECT_TRUE(isSupported(Entity{MText{}}));
}

// --- Codes (PT13, point 2) ----------------------------------------------------

TEST(MTextCodes, ParagraphsStartAtEveryP) {
    EXPECT_EQ(values(mtext(R"(A\PB\PC)")), (Values{"A", "B", "C"}));
}

TEST(MTextCodes, NoBreakSpaceIsShownAsASpace) {
    EXPECT_EQ(values(mtext(R"(A\~B)")), (Values{"A B"}));
}

TEST(MTextCodes, EscapedCharacters) {
    EXPECT_EQ(values(mtext(R"(\\\{\})")), (Values{R"(\{})"}));
    EXPECT_EQ(values(mtext(R"(\\P)")), (Values{R"(\P)"}));
}

TEST(MTextCodes, GroupBracesAreRemoved) {
    EXPECT_EQ(values(mtext("{A{B}C}")), (Values{"ABC"}));
}

TEST(MTextCodes, UnicodeCharacters) {
    EXPECT_EQ(values(mtext(R"(\U+00E9\U+03a9\U+2300)")), (Values{"éΩ⌀"}));
    // Without its four hexadecimal digits the code stays as written.
    EXPECT_EQ(values(mtext(R"(\U+00G9)")), (Values{R"(\U+00G9)"}));
    EXPECT_EQ(values(mtext(R"(\U+00)")), (Values{R"(\U+00)"}));
}

TEST(MTextCodes, PercentCodesAsDisplayText) {
    EXPECT_EQ(values(mtext("%%c %%d %%p %%%")), (Values{"⌀ ° ± %"}));
    EXPECT_EQ(values(mtext("%%C %%D %%P")), (Values{"⌀ ° ±"}));
    EXPECT_EQ(values(mtext("%%x")), (Values{"%%x"}));
}

TEST(MTextCodes, CodesAreDecodedOnce) {
    EXPECT_EQ(values(mtext("%%%c")), (Values{"%c"}));
    EXPECT_EQ(values(mtext("%%%%c")), (Values{"%%c"}));
}

TEST(MTextCodes, CodesWithAnArgumentAreRemoved) {
    EXPECT_EQ(values(mtext(R"(A\fArial|b1;B\FArial;C\H2.5x;D\W2;E\Q30;F\T2;G\A1;H\C1;I)"
                           R"(\c255;J\px1,qc;K)")),
              (Values{"ABCDEFGHIJK"}));
}

TEST(MTextCodes, StacksKeepTheirTextWithASlash) {
    EXPECT_EQ(values(mtext(R"(\S1^2; \S3#4; \S5/6;)")), (Values{"1/2 3/4 5/6"}));
}

TEST(MTextCodes, TogglesAreRemoved) {
    EXPECT_EQ(values(mtext(R"(\LA\l\OB\o\KC\k)")), (Values{"ABC"}));
}

TEST(MTextCodes, AnyOtherCodeStaysAsWritten) {
    EXPECT_EQ(values(mtext(R"(A\XB)")), (Values{R"(A\XB)"}));
    EXPECT_EQ(values(mtext(R"(A\)")), (Values{R"(A\)"}));
}

TEST(MTextCodes, ACodeWithoutItsSemicolonStaysButBreaksAtP) {
    EXPECT_EQ(values(mtext(R"(A\H2\PB)")), (Values{R"(A\H2)", "B"}));
    EXPECT_EQ(values(mtext(R"(A\S1^2)")), (Values{R"(A\S1^2)"}));
    // The ';' after a \P does not end the code before it.
    EXPECT_EQ(values(mtext(R"(A\H2\PB;C)")), (Values{R"(A\H2)", "B;C"}));
}

// --- Layout (PT13, point 3) -----------------------------------------------------

TEST(MTextLayout, WrapsBeforeTheWordBeyondTheWidth) {
    EXPECT_EQ(values(mtext("A B C", 1.0, 3.0)), (Values{"A B", "C"}));
    EXPECT_EQ(values(mtext("A B C")), (Values{"A B C"}));
}

TEST(MTextLayout, AWordWiderThanTheColumnStandsAlone) {
    const std::vector<Text> texts = rows(mtext("AAAA B CC", 1.0, 2.5));
    ASSERT_EQ(texts.size(), 3U);
    EXPECT_EQ(texts[0].value, "AAAA");
    EXPECT_EQ(texts[1].value, "B");
    EXPECT_EQ(texts[2].value, "CC");
    const double step = 5.0 / 3.0;
    expectNear(texts[0].position, {0, -1});
    expectNear(texts[1].position, {0, -1 - step});
    expectNear(texts[2].position, {0, -1 - (2 * step)});
}

TEST(MTextLayout, ARowAsWideAsTheColumnFits) {
    // At height 1409 the widths are the advances of the font file: 3301.
    const double width = widthOf(U"A B", 1409.0);
    EXPECT_EQ(values(mtext("A B", 1409.0, width)), (Values{"A B"}));
}

TEST(MTextLayout, NoBreakSpaceKeepsTheWordsTogether) {
    const double width = widthOf(U"A B", 1409.0);
    EXPECT_EQ(values(mtext(R"(A\~B C)", 1409.0, width)), (Values{"A B", "C"}));
    // Even in a column narrower than both: one word.
    EXPECT_EQ(values(mtext(R"(A\~B C)", 1.0, 1.5)), (Values{"A B", "C"}));
}

TEST(MTextLayout, SpacesWhereTheRowBreaksAreDropped) {
    EXPECT_EQ(values(mtext("A   B", 1.0, 1.5)), (Values{"A", "B"}));
    // Spaces elsewhere stay.
    EXPECT_EQ(values(mtext(" A  B ")), (Values{" A  B "}));
}

TEST(MTextLayout, StyleWidthFactorWidensTheRows) {
    Document document = Document::newDrawing();
    TextStyleTable styles = document.textStyles();
    TextStyle wide;
    wide.name = "Wide";
    wide.widthFactor = 2.0;
    wide.oblique = degreesToRadians(15.0);
    styles.put(wide, document);
    document.replaceTextStyles(styles);
    MText entity = mtext("A B C", 1.0, 3.0);
    entity.style = "Wide";
    const std::vector<Text> texts = rows(entity, document);
    ASSERT_EQ(texts.size(), 3U);  // "A B" is 4.68 wide with the factor 2
    EXPECT_EQ(texts[0].widthFactor, 2.0);
    EXPECT_EQ(texts[0].obliqueAngle, degreesToRadians(15.0));
    EXPECT_EQ(texts[0].style, "Wide");
}

TEST(MTextLayout, MissingStyleUsesTheDefaults) {
    MText entity = mtext("A");
    entity.style = "Missing";
    const std::vector<Text> texts = rows(entity);
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts[0].widthFactor, 1.0);
    EXPECT_EQ(texts[0].obliqueAngle, 0.0);
    EXPECT_EQ(texts[0].style, "Missing");
}

struct AlignmentCase {
    MTextAttachment attachment;
    double firstX;  ///< times the width of the row
    double firstY;  ///< baseline of the first row
};

TEST(MTextLayout, NineAttachmentsWithTwoRows) {
    // h = 3, two rows: L = 5. Top: -3; middle: (3 + 5) / 2 - 3 = 1; bottom: 5.
    const std::array cases{
        AlignmentCase{MTextAttachment::TopLeft, 0.0, -3.0},
        AlignmentCase{MTextAttachment::TopCenter, -0.5, -3.0},
        AlignmentCase{MTextAttachment::TopRight, -1.0, -3.0},
        AlignmentCase{MTextAttachment::MiddleLeft, 0.0, 1.0},
        AlignmentCase{MTextAttachment::MiddleCenter, -0.5, 1.0},
        AlignmentCase{MTextAttachment::MiddleRight, -1.0, 1.0},
        AlignmentCase{MTextAttachment::BottomLeft, 0.0, 5.0},
        AlignmentCase{MTextAttachment::BottomCenter, -0.5, 5.0},
        AlignmentCase{MTextAttachment::BottomRight, -1.0, 5.0},
    };
    for (const double width : {0.0, 12.0}) {
        for (const AlignmentCase& alignment : cases) {
            MText entity = mtext(R"(A\PBB)", 3.0, width);
            entity.insertion = {20, 30};
            entity.attachment = alignment.attachment;
            const std::vector<Text> texts = rows(entity);
            ASSERT_EQ(texts.size(), 2U);
            const double a = widthOf(U"A", 3.0);
            const double bb = widthOf(U"BB", 3.0);
            expectNear(texts[0].position, {20 + (alignment.firstX * a), 30 + alignment.firstY});
            expectNear(texts[1].position,
                       {20 + (alignment.firstX * bb), 30 + alignment.firstY - 5.0});
            for (const Text& text : texts) {
                EXPECT_EQ(text.horizontal, TextHorizontal::Left);
                EXPECT_EQ(text.vertical, TextVertical::Baseline);
                EXPECT_EQ(text.height, 3.0);
            }
        }
    }
}

TEST(MTextLayout, LineSpacing) {
    for (const bool exact : {false, true}) {
        MText entity = mtext(R"(A\PB)", 3.0);
        entity.insertion = {10, 20};
        entity.lineSpacing = 1.5;
        entity.exactSpacing = exact;
        const std::vector<Text> texts = rows(entity);
        ASSERT_EQ(texts.size(), 2U);
        expectNear(texts[0].position, {10, 17});
        expectNear(texts[1].position, {10, 9.5});
    }
}

TEST(MTextLayout, RotationTurnsTheColumn) {
    MText entity = mtext(R"(A\PB)", 3.0);
    entity.insertion = {10, 20};
    entity.rotation = degreesToRadians(30.0);
    const std::vector<Text> texts = rows(entity);
    ASSERT_EQ(texts.size(), 2U);
    // x' = 10 + sqrt(3)/2 x - y/2, y' = 20 + x/2 + sqrt(3)/2 y.
    const double root3 = std::sqrt(3.0);
    expectNear(texts[0].position, {11.5, 20 - (1.5 * root3)});
    expectNear(texts[1].position, {14, 20 - (4 * root3)});
    EXPECT_EQ(texts[0].rotation, entity.rotation);
}

TEST(MTextLayout, BlankRowsTakeABaselineButNoText) {
    const std::vector<Text> texts = rows(mtext(R"(  \PZ)", 3.0));
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts[0].value, "Z");
    expectNear(texts[0].position, {0, -8});

    // A trailing \P makes an empty last row: Z on the middle baseline.
    MText bottom = mtext(R"(\PZ\P)", 3.0);
    bottom.attachment = MTextAttachment::BottomLeft;
    const std::vector<Text> lifted = rows(bottom);
    ASSERT_EQ(lifted.size(), 1U);
    expectNear(lifted[0].position, {0, 5});
}

TEST(MTextLayout, BlankRowsDoNotWidenTheColumn) {
    MText entity = mtext(R"(          \PA)", 1.0);
    entity.attachment = MTextAttachment::TopRight;
    const std::vector<Text> texts = rows(entity);
    ASSERT_EQ(texts.size(), 1U);
    expectNear(texts[0].position, {-widthOf(U"A"), -1 - (5.0 / 3.0)});
}

TEST(MTextLayout, DirectionByStyleFollowsTheStyle) {
    Document document = Document::newDrawing();
    TextStyleTable styles = document.textStyles();
    TextStyle mirrored;
    mirrored.name = "Mirrored";
    mirrored.backwards = true;
    mirrored.upsideDown = true;
    styles.put(mirrored, document);
    document.replaceTextStyles(styles);
    MText entity = mtext("A");
    entity.style = "Mirrored";
    const std::vector<Text> plain = rows(entity, document);
    ASSERT_EQ(plain.size(), 1U);
    EXPECT_FALSE(plain[0].mirroredX);
    EXPECT_FALSE(plain[0].mirroredY);
    entity.directionByStyle = true;
    const std::vector<Text> byStyle = rows(entity, document);
    ASSERT_EQ(byStyle.size(), 1U);
    EXPECT_TRUE(byStyle[0].mirroredX);
    EXPECT_TRUE(byStyle[0].mirroredY);
}

// --- Selection, snap and grips (PT13, points 4 and 6) ---------------------------

TEST(MTextDocument, EmptyMTextIsItsInsertionPoint) {
    for (const std::string_view value : {"", "   ", R"(\P\P)"}) {
        Document document = Document::newDrawing();
        MText entity = mtext(std::string{value});
        entity.insertion = {10, 20};
        const EntityId id = document.add(entity);
        const EntityRecord& record = *document.find(id);
        EXPECT_TRUE(generate(document, record).empty());
        const BBox box = extent(document, record);
        EXPECT_EQ(box.min(), (Vec2{10, 20}));
        EXPECT_EQ(box.max(), (Vec2{10, 20}));
        EXPECT_EQ(distance(document, record, {13, 24}), 5.0);
        EXPECT_EQ(pickEntity(document, {10, 20}, 0.0), std::optional<EntityId>{id});
        EXPECT_EQ(pickEntity(document, {11, 20}, 0.1), std::nullopt);
        EXPECT_EQ(selectInRectangle(document, BBox{{9, 19}, {11, 21}}, RectangleMode::Window),
                  std::vector<EntityId>{id});
        EXPECT_TRUE(intersects(document, record, BBox{{10, 20}, {10, 20}}));
        EXPECT_FALSE(intersects(document, record, BBox{{11, 21}, {12, 22}}));
    }
}

TEST(MTextDocument, EmptyMTextOnALayerOffIsNotPicked) {
    Document document = Document::newDrawing();
    Layer off;
    off.name = "Off";
    off.linetype = document.continuousLinetype();
    off.on = false;
    const LayerId layer = document.addLayer(off);
    MText entity = mtext("");
    entity.insertion = {10, 20};
    const EntityId id = document.add(entity);
    EntityRecord record = *document.find(id);
    record.properties.layer = layer;
    EXPECT_EQ(distance(document, record, {10, 20}), std::numeric_limits<double>::infinity());
    EXPECT_FALSE(isInside(document, record, BBox{{9, 19}, {11, 21}}));
    EXPECT_EQ(extent(document, record).min(), (Vec2{10, 20}));
}

TEST(MTextDocument, NothingOnAFrozenLayer) {
    Document document = Document::newDrawing();
    Layer frozen;
    frozen.name = "Frozen";
    frozen.linetype = document.continuousLinetype();
    frozen.frozen = true;
    const LayerId layer = document.addLayer(frozen);
    for (const std::string_view value : {"", "A"}) {
        EntityRecord record = *document.find(document.add(mtext(std::string{value})));
        record.properties.layer = layer;
        EXPECT_TRUE(generate(document, record).empty());
        EXPECT_TRUE(extent(document, record).isEmpty());
    }
}

TEST(MTextDocument, RowsAreSelectedAsTexts) {
    Document document = Document::newDrawing();
    MText entity = mtext(R"(A\PB)", 3.0);
    entity.insertion = {30, 20};
    const EntityId id = document.add(entity);
    const EntityRecord& record = *document.find(id);
    EXPECT_EQ(distance(document, record, {31, 17}), 0.0);
    EXPECT_NEAR(distance(document, record, {29, 17}), 1.0, kTolerance);
    EXPECT_EQ(pickEntity(document, {31, 17}, 0.1), std::optional<EntityId>{id});
    EXPECT_TRUE(isInside(document, record, BBox{{29, 10}, {34, 22}}));
    EXPECT_FALSE(isInside(document, record, BBox{{30, 16}, {32, 18}}));
    EXPECT_TRUE(intersects(document, record, BBox{{30, 16}, {32, 18}}));
}

TEST(MTextDocument, InsertionSnapAndGrip) {
    Document document = Document::newDrawing();
    MText entity = mtext(R"(A\PB)", 3.0, 12.0);
    entity.insertion = {10, 20};
    entity.attachment = MTextAttachment::MiddleCenter;
    const EntityRecord& record = *document.find(document.add(entity));
    SnapModes insertion{};
    insertion.endpoint = false;
    insertion.midpoint = false;
    insertion.center = false;
    insertion.intersection = false;
    std::vector<SnapPoint> own;
    appendOwnSnapPointsOf(document, record, entity, insertion, own, std::nullopt);
    ASSERT_EQ(own.size(), 1U);
    EXPECT_EQ(own[0].point, (Vec2{10, 20}));
    EXPECT_EQ(own[0].kind, SnapKind::Insertion);
    SnapModes none = insertion;
    none.insertion = false;
    own.clear();
    appendOwnSnapPointsOf(document, record, entity, none, own, std::nullopt);
    EXPECT_TRUE(own.empty());

    EXPECT_EQ(gripPoints(Entity{entity}), std::vector<Vec2>{(Vec2{10, 20})});
    const GripResult moved = moveGrip(Entity{entity}, 0, {25, 35});
    EXPECT_EQ(moved.outcome, GripOutcome::Moved);
    MText expected = entity;
    expected.insertion = {25, 35};
    EXPECT_EQ(std::get<MText>(moved.entity), expected);
}

// --- Transformation (PT13, point 5) ---------------------------------------------

MText transformedMText(const MText& entity, const Similarity& similarity, MirrorText mirrorText) {
    const TransformOutcome outcome =
        transformedIn(Document::newDrawing(), Entity{entity}, similarity, mirrorText);
    EXPECT_TRUE(outcome.refusal.empty());
    EXPECT_TRUE(outcome.entity.has_value());
    return std::get<MText>(outcome.entity.value_or(Entity{entity}));
}

MText sampleColumn() {
    MText entity = mtext(R"(A\PB)", 3.0, 12.0);
    entity.insertion = {10, 20};
    entity.attachment = MTextAttachment::MiddleCenter;
    entity.rotation = degreesToRadians(30.0);
    entity.lineSpacing = 1.5;
    return entity;
}

TEST(MTextTransform, MoveRotateScale) {
    const MText entity = sampleColumn();
    MText moved = transformedMText(entity, Similarity::translation({5, -7}), MirrorText::Readable);
    expectNear(moved.insertion, {15, 13});
    EXPECT_EQ(moved.rotation, entity.rotation);

    const MText turned = transformedMText(entity, Similarity::rotation(degreesToRadians(90.0), {}),
                                          MirrorText::Readable);
    expectNear(turned.insertion, {-20, 10});
    EXPECT_NEAR(turned.rotation, degreesToRadians(120.0), kTolerance);

    const std::optional<Similarity> scaling = Similarity::scaling(2.0, {5, 10});
    ASSERT_TRUE(scaling.has_value());
    const MText scaled =
        transformedMText(entity, scaling.value_or(Similarity{}), MirrorText::Readable);
    expectNear(scaled.insertion, {15, 30});
    EXPECT_EQ(scaled.height, 6.0);
    EXPECT_EQ(scaled.width, 24.0);
    EXPECT_EQ(scaled.lineSpacing, 1.5);
    EXPECT_EQ(scaled.value, entity.value);
}

struct MirrorCase {
    Vec2 a;
    Vec2 b;
    Vec2 insertion;
    double degrees;
};

TEST(MTextTransform, ReflectionStaysReadableWhateverMirrtext) {
    const std::array cases{
        MirrorCase{{0, 0}, {1, 0}, {10, -20}, 330.0},
        MirrorCase{{0, 0}, {0, 1}, {-10, 20}, 330.0},
        MirrorCase{{0, 0}, {1, 1}, {20, 10}, 60.0},
    };
    for (const bool backwards : {false, true}) {
        Document document = Document::newDrawing();
        TextStyleTable styles = document.textStyles();
        TextStyle style;
        style.name = "Back";
        style.backwards = backwards;
        styles.put(style, document);
        document.replaceTextStyles(styles);
        MText entity = sampleColumn();
        entity.style = "Back";
        entity.directionByStyle = true;
        for (const MirrorCase& mirror : cases) {
            const std::optional<Similarity> reflection = Similarity::reflection(mirror.a, mirror.b);
            ASSERT_TRUE(reflection.has_value());
            const Similarity similarity = reflection.value_or(Similarity{});
            const MText readable = transformedMText(entity, similarity, MirrorText::Readable);
            const MText mirrored = transformedMText(entity, similarity, MirrorText::Mirrored);
            EXPECT_EQ(readable, mirrored);
            expectNear(readable.insertion, mirror.insertion);
            EXPECT_NEAR(readable.rotation, degreesToRadians(mirror.degrees), 1e-12);
            EXPECT_EQ(readable.directionByStyle, true);
            EXPECT_EQ(readable.style, "Back");
            // The rows keep the signals of the style: readable as before.
            const std::vector<Text> texts = rows(readable, document);
            ASSERT_EQ(texts.size(), 2U);
            EXPECT_EQ(texts[0].mirroredX, backwards);
            EXPECT_FALSE(texts[0].mirroredY);
        }
    }
}

TEST(MTextTransform, ReflectionOfAnObliqueColumn) {
    MText entity = mtext("A");
    entity.insertion = {2, 3};
    entity.rotation = degreesToRadians(120.0);
    const std::optional<Similarity> acrossX = Similarity::reflection({0, 0}, {1, 0});
    ASSERT_TRUE(acrossX.has_value());
    for (const MirrorText mirrorText : {MirrorText::Readable, MirrorText::Mirrored}) {
        const MText result = transformedMText(entity, acrossX.value_or(Similarity{}), mirrorText);
        expectNear(result.insertion, {2, -3});
        EXPECT_NEAR(result.rotation, degreesToRadians(60.0), 1e-12);
    }
}

}  // namespace
}  // namespace tg::core
