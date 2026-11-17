// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext.hpp"

#include "mtext_layout.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_style_table.hpp"

#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

namespace tg::core {
namespace {

/// What the style gives to every row (PT13, points 3 and 4): defaults when
/// the style is not in the table.
struct StyleValues {
    double widthFactor{1.0};
    double oblique{0.0};
    bool backwards{false};
    bool upsideDown{false};
};

StyleValues styleValues(const Document& document, const MText& mtext) {
    StyleValues values;
    if (const TextStyle* style = document.textStyles().find(mtext.style)) {
        values.widthFactor = style->widthFactor;
        values.oblique = style->oblique;
        values.backwards = style->backwards;
        values.upsideDown = style->upsideDown;
    }
    return values;
}

bool onFrozenLayer(const Document& document, const EntityRecord& record) {
    const Layer* layer = document.layer(record.properties.layer);
    return layer != nullptr && layer->frozen;
}

/// The Text of every row with a character other than a space, in drawing
/// coordinates, its value written so that displayText() gives the row back
/// (PT13, point 4).
std::vector<Text> rowTexts(const Document& document, const MText& mtext) {
    const StyleValues style = styleValues(document, mtext);
    const SinCos direction = sinCos(mtext.rotation);
    const Vec2 xAxis{direction.cosine, direction.sine};
    const Vec2 yAxis{-direction.sine, direction.cosine};
    std::vector<Text> texts;
    for (const mtext_layout::Row& row : mtext_layout::layout(mtext, style.widthFactor)) {
        if (row.blank) {
            continue;
        }
        Text text;
        text.position = mtext.insertion + (xAxis * row.start.x) + (yAxis * row.start.y);
        text.height = mtext.height;
        text.rotation = mtext.rotation;
        text.widthFactor = style.widthFactor;
        text.obliqueAngle = style.oblique;
        text.style = mtext.style;
        text.mirroredX = mtext.directionByStyle && style.backwards;
        text.mirroredY = mtext.directionByStyle && style.upsideDown;
        text.horizontal = TextHorizontal::Left;
        text.vertical = TextVertical::Baseline;
        // Decoded once: displayText() of the value gives the row back.
        text.value = mtext_layout::toUtf8(mtext_layout::literalValue(row.text));
        texts.push_back(std::move(text));
    }
    return texts;
}

/// True when the MTEXT shows nothing and stands for its insertion point
/// (PT5, point B04): no row to show, on a layer that is on and thawed.
bool standsForInsertion(const Document& document, const EntityRecord& record, const MText& mtext) {
    return isVisible(document, record) && rowTexts(document, mtext).empty();
}

}  // namespace

bool supported(const MText& /*mtext*/) {
    return true;
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const MText& mtext) {
    if (onFrozenLayer(document, record)) {
        return {};
    }
    const EffectiveProperties properties = effectiveProperties(document, record);
    std::vector<GeneratedItem> items;
    for (Text& text : rowTexts(document, mtext)) {
        items.push_back(GeneratedItem{std::move(text), properties});
    }
    return items;
}

BBox extentOf(const Document& document, const EntityRecord& record, const MText& mtext) {
    if (onFrozenLayer(document, record)) {
        return {};
    }
    const std::vector<GeneratedItem> items = generateOf(document, record, mtext);
    if (items.empty()) {
        return {mtext.insertion, mtext.insertion};
    }
    return extentOfItems(items);
}

double distanceOf(const Document& document, const EntityRecord& record, const MText& mtext,
                  const Vec2& point) {
    if (standsForInsertion(document, record, mtext)) {
        return distance(mtext.insertion, point);
    }
    return distanceToDrawnItems(generateOf(document, record, mtext), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const MText& mtext,
                const BBox& rectangle) {
    if (standsForInsertion(document, record, mtext)) {
        return rectangle.contains(mtext.insertion);
    }
    return drawnItemsInside(generateOf(document, record, mtext), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const MText& mtext,
                  const BBox& rectangle) {
    if (standsForInsertion(document, record, mtext)) {
        return rectangle.contains(mtext.insertion);
    }
    return drawnItemsIntersect(generateOf(document, record, mtext), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           const MText& mtext, const SnapModes& modes, std::vector<SnapPoint>& out,
                           const std::optional<Vec2>& /*from*/) {
    if (modes.insertion) {
        out.push_back({mtext.insertion, SnapKind::Insertion});
    }
}

TransformOutcome transformedOf(const Document& /*document*/, const MText& mtext,
                               const Similarity& similarity, MirrorText /*mirrorText*/) {
    MText result = mtext;
    result.insertion = similarity.apply(mtext.insertion);
    result.height = mtext.height * similarity.scale();
    result.width = mtext.width * similarity.scale();
    if (!similarity.reflects()) {
        result.rotation = normalizeAngle(mtext.rotation + similarity.rotationAngle());
    } else {
        // Readable whatever MIRRTEXT, as AutoCAD (PT5, point B03): the row of
        // MIRRTEXT 0 in the table of docs/adr/0031, point 3, applied to the
        // direction of the column.
        const double alpha = normalizeAngle(similarity.rotationAngle() - mtext.rotation);
        const bool reverse = alpha > kPi / 2.0 && alpha <= 3.0 * kPi / 2.0;
        result.rotation = normalizeAngle(reverse ? alpha - kPi : alpha);
    }
    return {Entity{result}, {}};
}

std::vector<Vec2> gripPointsOf(const MText& mtext) {
    return {mtext.insertion};
}

GripResult moveGripOf(const Document& /*document*/, const MText& mtext,
                      [[maybe_unused]] std::size_t index, const Vec2& target) {
    assert(index == 0 && "an MTEXT has one grip");
    MText result = mtext;
    result.insertion = target;
    return {GripOutcome::Moved, Entity{result}, {}};
}

}  // namespace tg::core
