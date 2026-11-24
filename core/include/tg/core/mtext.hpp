// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tg::core {

class Document;
class Similarity;
struct EntityRecord;
struct GeneratedItem;
struct GripResult;
struct SnapModes;
struct SnapPoint;
struct TransformOutcome;
enum class MirrorText : std::uint8_t;

/// DXF group 71, in the order of its values (1 to 9).
enum class MTextAttachment : std::uint8_t {
    TopLeft = 1,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/// DXF MTEXT (docs/adr/0046, points 1 and 2). Angles in radians.
struct MText {
    Vec2 insertion;                                        ///< 10
    double height{2.5};                                    ///< 40
    double width{0.0};                                     ///< 41, reference rectangle width
    MTextAttachment attachment{MTextAttachment::TopLeft};  ///< 71
    double rotation{0.0};                                  ///< 50 (or 11 as a direction)
    std::string style{"Standard"};                         ///< 7
    double lineSpacing{1.0};                               ///< 44
    bool exactSpacing{false};                              ///< 73 = 2
    bool directionByStyle{false};                          ///< 72 = 5
    std::string value;  ///< groups 3 and 1 joined, UTF-8, formatting codes kept

    friend bool operator==(const MText&, const MText&) = default;
};

// The functions of MText that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// The layout is in docs/adr/0046, point 2, and
// docs/tappe/0.8/PT13-mtext-core.md, points 2 and 3.

/// True: MText is supported (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const MText& mtext);

/// The pieces of every row with a character other than a space, in reading
/// order (docs/adr/0054, points 3 and 4;
/// docs/tappe/0.8.1/PT8-impaginazione-formati.md, point 3): a Text for a
/// Regular piece with tracking 1, a FormattedText for the others, the two
/// texts of a stacked fraction and its TextLine, and a TextLine for each
/// underline, overline and strike-through. A text is left aligned on its
/// baseline, with the rotation of the entity and the height, width factor
/// and oblique angle of its piece (those of the entity and its style
/// without codes: 1 and 0 when the style is not in the table), the mirror
/// signals of the style with directionByStyle, and as value its characters
/// written so that displayText() gives them back: every '%' that forms
/// %%c, %%d, %%p or %%% with the two characters after it is written %%%
/// (the row "%%c" has the value "%%%%c"). The shapes of a piece have the
/// effective properties of the entity, with the colour of its `\C` or `\c`.
/// Without formats, one Text per row, as in stage 0.8. Nothing on a frozen
/// layer; properties.visible false on a layer that is off (docs/adr/0044,
/// point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record, const MText& mtext);

/// The text each layout row shows, pieces joined in reading order (a stack as
/// "top/bottom"), for an MTEXT or for the text of a dimension
/// (docs/tappe/0.8.1/PT8-impaginazione-formati.md, point 10). Only the rows
/// with a character other than a space: `A\P\PB\P` gives "A" and "B". For a
/// dimension, its graphics in drawing order: the rows of an MTEXT that is
/// drawn, the texts of the drawn shapes of another entity (a TEXT read from
/// a DXF). Nothing for the other types. The layer of an MTEXT does not
/// count, nor does that of the dimension.
[[nodiscard]] std::vector<std::u32string> shownLines(const Document& document,
                                                     const EntityRecord& record);

/// The bounding box of every shape of generateOf(), drawn or not; the
/// insertion point when there is none, except on a frozen layer (PT5, B04).
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const MText& mtext);

/// The least distance of the drawn shapes, infinity without any. An MTEXT
/// without a row to show, on a visible layer, is its insertion point.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const MText& mtext, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one;
/// without a row to show, the insertion point inside.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const MText& mtext, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`; without a row
/// to show, the insertion point inside.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const MText& mtext, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives: the insertion
/// point, with modes.insertion.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record, const MText& mtext,
                           const SnapModes& modes, std::vector<SnapPoint>& out,
                           const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, never refused: the insertion
/// point transformed, height and width times the scale, the rotation plus the
/// angle of the similarity. A reflection keeps the text readable whatever
/// `mirrorText`, as AutoCAD (PT5, point B03): the rotation is that of the
/// MIRRTEXT 0 row of the table of docs/adr/0031, point 3.
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const MText& mtext,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order: the insertion point.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const MText& mtext);

/// Moves grip `index` to `target`: the insertion point, which moves the text.
/// Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const MText& mtext, std::size_t index,
                                    const Vec2& target);

}  // namespace tg::core
