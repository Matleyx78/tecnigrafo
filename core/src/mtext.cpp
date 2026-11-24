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
#include "tg/core/mtext_format.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/text_style_table.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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

// The rules of docs/adr/0054, point 3, in units of the height of a piece,
// from the baseline of its row.
constexpr double kNumeratorBaseline = 4.0 / 3.0;  // rule 5
constexpr double kFractionLine = 7.0 / 6.0;       // rule 5, the line of '/'
constexpr double kDiagonalTop = 7.0 / 3.0;        // rule 5, the end of the line of '#'
constexpr double kUnderline = -0.2;               // rule 6
constexpr double kOverline = 1.2;                 // rule 6
constexpr double kStrike = 0.5;                   // rule 6

/// The layout of `mtext` with the values of its style.
mtext_layout::Layout layoutOf(const Document& document, const MText& mtext) {
    const StyleValues style = styleValues(document, mtext);
    return mtext_layout::layout(mtext, style.widthFactor, style.oblique);
}

/// Places the shapes of a layout in the drawing (PT13, point 4;
/// docs/tappe/0.8.1/PT8-impaginazione-formati.md, point 3).
class PieceShapes {
public:
    PieceShapes(const Document& document, const EntityRecord& record, const MText& mtext,
                const mtext_layout::Layout& layout)
        : m_document(document),
          m_record(record),
          m_mtext(mtext),
          m_layout(layout),
          m_style(styleValues(document, mtext)),
          m_properties(effectiveProperties(document, record)) {
        const SinCos direction = sinCos(mtext.rotation);
        m_xAxis = {direction.cosine, direction.sine};
        m_yAxis = {-direction.sine, direction.cosine};
    }

    /// The shapes of every row with something to show, in reading order:
    /// for each piece its text (the two of a stacked fraction and its
    /// line), then its underline, overline and strike-through.
    std::vector<GeneratedItem> items() {
        for (const mtext_layout::Row& row : m_layout.rows) {
            if (row.blank) {
                continue;
            }
            for (const mtext_layout::Piece& piece : row.pieces) {
                appendPiece(row, piece);
            }
        }
        return std::move(m_items);
    }

private:
    [[nodiscard]] Vec2 place(double x, double y) const {
        return m_mtext.insertion + (m_xAxis * x) + (m_yAxis * y);
    }

    /// The effective properties of a piece: those of the entity, with the
    /// colour of `\C` or `\c` when it has one.
    [[nodiscard]] EffectiveProperties propertiesOf(const MTextFormat& format) const {
        if (!format.color) {
            return m_properties;
        }
        EntityRecord coloured = m_record;
        coloured.properties.color = *format.color;
        return effectiveProperties(m_document, coloured);
    }

    /// A text of a piece at (x, y) of the column: a Text when it is Regular
    /// with tracking 1, as in stage 0.8, otherwise a FormattedText.
    void appendText(const MTextFormat& format, std::u32string_view text, double x, double y,
                    const EffectiveProperties& properties) {
        Text shape;
        shape.position = place(x, y);
        shape.height = format.height;
        shape.rotation = m_mtext.rotation;
        shape.widthFactor = format.widthFactor;
        shape.obliqueAngle = format.oblique;
        shape.style = m_mtext.style;
        shape.mirroredX = m_mtext.directionByStyle && m_style.backwards;
        shape.mirroredY = m_mtext.directionByStyle && m_style.upsideDown;
        shape.horizontal = TextHorizontal::Left;
        shape.vertical = TextVertical::Baseline;
        // Decoded once: displayText() of the value gives the text back.
        shape.value = mtext_layout::toUtf8(mtext_layout::literalValue(text));
        const FontFace face = mtext_layout::faceOf(format);
        if (face == FontFace::Regular && format.tracking == 1.0) {
            m_items.push_back(GeneratedItem{std::move(shape), properties});
        } else {
            m_items.push_back(
                GeneratedItem{FormattedText{std::move(shape), face, format.tracking}, properties});
        }
    }

    void appendLine(double x0, double y0, double x1, double y1,
                    const EffectiveProperties& properties) {
        m_items.push_back(GeneratedItem{TextLine{Line{place(x0, y0), place(x1, y1)}}, properties});
    }

    /// The two texts of a stacked fraction, centred on its width, and its
    /// line (rule 5).
    void appendStack(const MTextFormat& format, const MTextStack& stack,
                     const mtext_layout::Piece& piece, double baseline,
                     const EffectiveProperties& properties) {
        const FontFace face = mtext_layout::faceOf(format);
        const double scale = format.height * format.widthFactor * format.tracking;
        const auto centred = [&](const std::u32string& text) {
            return piece.x + ((piece.width - (textWidth(text, face) * scale)) / 2.0);
        };
        const auto shows = [](const std::u32string& text) {
            return std::ranges::any_of(text, [](char32_t c) { return c != U' '; });
        };
        if (shows(stack.top)) {
            appendText(format, stack.top, centred(stack.top),
                       baseline + (kNumeratorBaseline * format.height), properties);
        }
        if (shows(stack.bottom)) {
            appendText(format, stack.bottom, centred(stack.bottom), baseline, properties);
        }
        switch (stack.kind) {
            case StackKind::Horizontal: {
                const double y = baseline + (kFractionLine * format.height);
                appendLine(piece.x, y, piece.x + piece.width, y, properties);
                break;
            }
            case StackKind::Diagonal:
                appendLine(piece.x, baseline, piece.x + piece.width,
                           baseline + (kDiagonalTop * format.height), properties);
                break;
            case StackKind::Tolerance:
                break;
        }
    }

    void appendPiece(const mtext_layout::Row& row, const mtext_layout::Piece& piece) {
        const MTextRun& run = m_layout.runOf(piece);
        const MTextFormat& format = run.format;
        const EffectiveProperties properties = propertiesOf(format);
        const double baseline = row.start.y;
        if (run.stack) {
            appendStack(format, *run.stack, piece, baseline, properties);
        } else if (!piece.blank) {
            appendText(format, piece.text, piece.x, baseline, properties);
        }
        const auto decoration = [&](bool on, double offset) {
            if (on) {
                const double y = baseline + (offset * format.height);
                appendLine(piece.x, y, piece.x + piece.width, y, properties);
            }
        };
        decoration(format.underline, kUnderline);
        decoration(format.overline, kOverline);
        decoration(format.strike, kStrike);
    }

    const Document& m_document;
    const EntityRecord& m_record;
    const MText& m_mtext;
    const mtext_layout::Layout& m_layout;
    StyleValues m_style;
    EffectiveProperties m_properties;
    Vec2 m_xAxis;
    Vec2 m_yAxis;
    std::vector<GeneratedItem> m_items;
};

/// True when no row has something to show.
bool showsNothing(const mtext_layout::Layout& layout) {
    return std::ranges::all_of(layout.rows, [](const mtext_layout::Row& row) { return row.blank; });
}

/// True when the MTEXT shows nothing and stands for its insertion point
/// (PT5, point B04): no row to show, on a layer that is on and thawed.
bool standsForInsertion(const Document& document, const EntityRecord& record, const MText& mtext) {
    return isVisible(document, record) && showsNothing(layoutOf(document, mtext));
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
    const mtext_layout::Layout layout = layoutOf(document, mtext);
    return PieceShapes(document, record, mtext, layout).items();
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
