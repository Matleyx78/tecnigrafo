// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/dimension.hpp"

#include "dimension_graphics.hpp"
#include "mtext_layout.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/text_style_table.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {

namespace dimension_graphics {

DimVariables scaledVariables(DimVariables variables) {
    const double scale = variables.scale == 0.0 ? 1.0 : variables.scale;
    variables.arrowSize *= scale;
    variables.extOffset *= scale;
    variables.extExtension *= scale;
    variables.gap *= scale;
    variables.textHeight *= scale;
    variables.centerMark *= scale;
    return variables;
}

Vec2 readingDirection(const Vec2& direction) {
    const bool reads = direction.x > 0.0 || (direction.x == 0.0 && direction.y > 0.0);
    return reads ? direction : -direction;
}

double angleOf(const Vec2& direction) {
    return normalizeAngle(atan2(direction.y, direction.x));
}

TextSize textSize(const Document& document, const MText& mtext) {
    const TextStyle* style = document.textStyles().find(mtext.style);
    const double widthFactor = style != nullptr ? style->widthFactor : 1.0;
    const std::vector<mtext_layout::Row> rows = mtext_layout::layout(mtext, widthFactor);
    TextSize size;
    for (const mtext_layout::Row& row : rows) {
        if (!row.blank) {
            size.width = std::max(size.width, textWidth(row.text) * mtext.height * widthFactor);
        }
    }
    // The rows are at least one (mtext_layout::layout()).
    size.height = rows.front().start.y - rows.back().start.y + mtext.height;
    return size;
}

Builder::Builder(const Document& document, const DimVariables& variables)
    : m_document(&document), m_variables(&variables) {}

EntityProperties Builder::properties(const Color& color, int lineweight) const {
    EntityProperties properties;
    properties.layer = m_document->layerZero();
    properties.color = color;
    properties.linetype = m_document->byBlockLinetype();
    properties.lineweight = lineweight;
    return properties;
}

void Builder::line(const Vec2& start, const Vec2& end, Part part) {
    if (start == end) {
        return;
    }
    const bool extension = part == Part::ExtensionLine;
    m_records.push_back(EntityRecord{
        EntityId{0},
        Line{start, end},
        {},
        m_records.size() + 1,
        properties(extension ? m_variables->extColor : m_variables->dimColor,
                   extension ? m_variables->extLineweight : m_variables->dimLineweight)});
}

void Builder::arrow(const Vec2& tip, const Vec2& base, const Vec2& up) {
    if (tip == base) {
        return;
    }
    const Vec2 side = up * (distance(tip, base) / 6.0);
    Solid solid;
    solid.vertices = {base + side, base - side, tip, tip};
    m_records.push_back(
        EntityRecord{EntityId{0},
                     solid,
                     {},
                     m_records.size() + 1,
                     properties(m_variables->dimColor, m_variables->dimLineweight)});
}

void Builder::text(MText mtext) {
    m_records.push_back(EntityRecord{EntityId{0},
                                     std::move(mtext),
                                     {},
                                     m_records.size() + 1,
                                     properties(m_variables->textColor, lineweight::kByBlock)});
}

std::vector<EntityRecord> Builder::take() {
    return std::move(m_records);
}

}  // namespace dimension_graphics

namespace {

/// The variables of docs/adr/0047, point 2, that block the regeneration and
/// are not essential, by name, with their group in a DIMSTYLE record: an
/// override of one of them in DimensionPresentation::overrides is checked as
/// the extras of a style (DimStyleTable::regenerable()). DIMLDRBLK, which a
/// DIMSTYLE record holds only as a handle, is checked as a name.
struct BlockingVariable {
    std::string_view name;
    std::int16_t code;
};

constexpr std::array<BlockingVariable, 19> kBlockingVariables{{
    {"DIMALT", 170},   {"DIMATFIT", 289}, {"DIMAUNIT", 275}, {"DIMBLK", 5},    {"DIMBLK1", 6},
    {"DIMBLK2", 7},    {"DIMFRAC", 276},  {"DIMJUST", 280},  {"DIMLDRBLK", 5}, {"DIMLIM", 72},
    {"DIMLUNIT", 277}, {"DIMSAH", 173},   {"DIMSOXD", 175},  {"DIMTIX", 174},  {"DIMTMOVE", 279},
    {"DIMTOFL", 172},  {"DIMTOL", 71},    {"DIMTSZ", 142},   {"DIMTVP", 145},
}};

/// The overrides of blocking variables as groups of a DIMSTYLE record.
RawGroups blockingOverrides(std::span<const std::pair<std::string, std::string>> overrides) {
    RawGroups groups;
    for (const auto& [name, value] : overrides) {
        for (const BlockingVariable& variable : kBlockingVariables) {
            if (variable.name == name) {
                groups.push_back(RawGroup{variable.code, value});
                break;
            }
        }
    }
    return groups;
}

bool isTranslation(const Similarity& similarity) {
    return similarity.scale() == 1.0 && similarity.rotationAngle() == 0.0 && !similarity.reflects();
}

/// An explicit text angle after `similarity` (PT5, point B10): turned with
/// it; a reflection keeps the text readable, as for an MTEXT (PT5, B03).
double transformedTextAngle(double angle, const Similarity& similarity) {
    if (!similarity.reflects()) {
        return normalizeAngle(angle + similarity.rotationAngle());
    }
    const double alpha = normalizeAngle(similarity.rotationAngle() - angle);
    const bool reverse = alpha > kPi / 2.0 && alpha <= 3.0 * kPi / 2.0;
    return normalizeAngle(reverse ? alpha - kPi : alpha);
}

/// The shapes of an entity of the graphics, `local` carrying the properties
/// it is shown with: as an entity of a block (insert.cpp).
std::vector<GeneratedItem> localItems(const Document& document, const EntityRecord& local) {
    return std::visit(
        [&](const auto& value) -> std::vector<GeneratedItem> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Polyline> || std::is_same_v<T, Line> ||
                          std::is_same_v<T, Circle> || std::is_same_v<T, Arc> ||
                          std::is_same_v<T, Text>) {
                return {GeneratedItem{value, effectiveProperties(document, local)}};
            } else if constexpr (std::is_same_v<T, AttributeDefinition>) {
                return {};  // not drawn inside a block
            } else {
                if (!supported(value)) {
                    return {};
                }
                return generateOf(document, local, value);
            }
        },
        local.entity);
}

}  // namespace

std::optional<Dimension> regenerated(const Document& document, Dimension dimension,
                                     std::string* reason) {
    const auto refuse = [&](std::string_view why) -> std::optional<Dimension> {
        if (reason != nullptr) {
            *reason = why;
        }
        return std::nullopt;
    };
    switch (dimension.definition.kind) {
        case DimensionKind::Linear:
        case DimensionKind::Aligned:
            break;
        case DimensionKind::Angular:
        case DimensionKind::Radius:
        case DimensionKind::Diameter:
            // PT24 realizes them.
            return refuse(kDimensionKindNotRealized);
        case DimensionKind::Other:
            return refuse(kDimensionNotRegenerable);
    }
    const DimStyleTable& table = document.dimStyles();
    const DimensionPresentation& presentation = dimension.presentation;
    const DimStyle* style = table.find(presentation.style);
    if (style == nullptr) {
        style = table.find("Standard");  // as dimensionVariables() (PT5, B06)
    }
    if ((style != nullptr && !DimStyleTable::regenerable(style->extras)) ||
        !DimStyleTable::regenerable(blockingOverrides(presentation.overrides))) {
        return refuse(kDimensionNotRegenerable);
    }
    const DimVariables variables =
        dimensionVariables(table, presentation.style, presentation.overrides);
    if (!DimStyleTable::inDomain(variables, document.textStyles())) {
        return refuse(kDimensionNotRegenerable);
    }
    return dimension_graphics::regeneratedLinear(
        document, std::move(dimension), dimension_graphics::scaledVariables(variables), reason);
}

bool supported(const Dimension& /*dimension*/) {
    return true;
}

namespace {

/// Calls `visit(local, items)` for every entity of the graphics of
/// `dimension` that is generated, in order: `local` carries the properties
/// it is shown with, `items` are its shapes, those on a layer that is off
/// not visible. The graphics are in drawing coordinates: an anonymous block
/// inserted at the origin (docs/adr/0047, point 4).
template <typename Visit>
void visitGraphics(const Document& document, const EntityRecord& record, const Dimension& dimension,
                   Visit visit) {
    const LayerState state = layerState(document, record.properties);
    if (state.frozen) {
        return;
    }
    for (const EntityRecord& graphic : dimension.graphics) {
        const EntityRecord local{
            graphic.id,
            graphic.entity,
            {},
            graphic.order,
            propertiesInBlock(document, record.properties, graphic.properties)};
        const LayerState inner = layerState(document, local.properties, state);
        if (inner.frozen) {
            continue;
        }
        std::vector<GeneratedItem> items = localItems(document, local);
        for (GeneratedItem& item : items) {
            item.properties.visible = item.properties.visible && inner.on;
        }
        visit(local, std::move(items));
    }
}

}  // namespace

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Dimension& dimension) {
    std::vector<GeneratedItem> items;
    visitGraphics(document, record, dimension,
                  [&](const EntityRecord& /*local*/, std::vector<GeneratedItem> shapes) {
                      std::ranges::move(shapes, std::back_inserter(items));
                  });
    return items;
}

std::vector<GeneratedItem> snapItemsOf(const Document& document, const EntityRecord& record,
                                       const Dimension& dimension) {
    std::vector<GeneratedItem> items;
    visitGraphics(document, record, dimension,
                  [&](const EntityRecord& local, std::vector<GeneratedItem> shapes) {
                      if (std::holds_alternative<MText>(local.entity)) {
                          return;  // its rows give no snap points
                      }
                      for (GeneratedItem& item : shapes) {
                          if (item.properties.visible) {  // only the drawn ones, as generate()
                              items.push_back(std::move(item));
                          }
                      }
                  });
    return items;
}

BBox extentOf(const Document& document, const EntityRecord& record, const Dimension& dimension) {
    return extentOfItems(generateOf(document, record, dimension));
}

double distanceOf(const Document& document, const EntityRecord& record, const Dimension& dimension,
                  const Vec2& point) {
    return distanceToDrawnItems(generateOf(document, record, dimension), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Dimension& dimension,
                const BBox& rectangle) {
    return drawnItemsInside(generateOf(document, record, dimension), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Dimension& dimension,
                  const BBox& rectangle) {
    return drawnItemsIntersect(generateOf(document, record, dimension), rectangle);
}

void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const Dimension& dimension, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from) {
    // The snap points of a dimension are those of its graphics (PT23, point
    // 7): an MTEXT gives its own, when it is drawn, and its rows none.
    visitGraphics(
        document, record, dimension,
        [&](const EntityRecord& local, const std::vector<GeneratedItem>& shapes) {
            const auto* mtext = std::get_if<MText>(&local.entity);
            if (mtext != nullptr && std::ranges::any_of(shapes, [](const GeneratedItem& item) {
                    return item.properties.visible;
                })) {
                appendOwnSnapPointsOf(document, local, *mtext, modes, out, from);
            }
        });
}

TransformOutcome transformedOf(const Document& document, const Dimension& dimension,
                               const Similarity& similarity, MirrorText mirrorText) {
    Dimension result = dimension;
    DimensionDefinition& definition = result.definition;
    DimensionPresentation& presentation = result.presentation;
    for (Vec2* point : {&definition.linePoint, &definition.point13, &definition.point14,
                        &definition.point15, &definition.point16, &presentation.textPoint}) {
        *point = similarity.apply(*point);
    }
    if (isTranslation(similarity)) {
        // MOVE and COPY move points and graphics without regenerating
        // (docs/adr/0047, point 4), whatever the kind.
        for (EntityRecord& graphic : result.graphics) {
            graphic.entity = transformed(graphic.entity, similarity, mirrorText);
        }
        return {Entity{std::move(result)}, {}};
    }
    if (definition.kind == DimensionKind::Linear) {
        // A direction at angle r goes to r + θ, or to θ - r with a reflection
        // (docs/adr/0031, point 1).
        const double angle = similarity.rotationAngle();
        definition.rotation = normalizeAngle(similarity.reflects() ? angle - definition.rotation
                                                                   : definition.rotation + angle);
    }
    if (presentation.textRotation) {
        presentation.textRotation = transformedTextAngle(*presentation.textRotation, similarity);
    }
    std::optional<Dimension> regeneration = regenerated(document, std::move(result));
    if (!regeneration) {
        return {std::nullopt, std::string{kDimensionNotRegenerable}};
    }
    return {Entity{std::move(*regeneration)}, {}};
}

std::vector<Vec2> gripPointsOf(const Dimension& dimension) {
    const DimensionDefinition& definition = dimension.definition;
    if (definition.kind != DimensionKind::Linear && definition.kind != DimensionKind::Aligned) {
        return {};
    }
    return {definition.point13, definition.point14, definition.linePoint,
            dimension.presentation.textPoint};
}

GripResult moveGripOf(const Document& document, const Dimension& dimension, std::size_t index,
                      const Vec2& target) {
    if (!std::isfinite(target.x) || !std::isfinite(target.y)) {
        return {GripOutcome::Invalid, Entity{dimension}, {}};
    }
    constexpr std::size_t kTextGrip = 3;
    Dimension moved = dimension;
    if (index == kTextGrip) {
        // Only the text moves (docs/adr/0047, point 4): the middle of the
        // text in the point (PT5, point B10).
        moved.presentation.textPoint = target;
        moved.presentation.userTextPosition = true;
        for (EntityRecord& graphic : moved.graphics) {
            if (auto* text = std::get_if<MText>(&graphic.entity)) {
                text->insertion = target;
                text->attachment = MTextAttachment::MiddleCenter;
            }
        }
        return {GripOutcome::Moved, Entity{std::move(moved)}, {}};
    }
    DimensionDefinition& definition = moved.definition;
    const std::array<Vec2*, 3> points{&definition.point13, &definition.point14,
                                      &definition.linePoint};
    *points.at(index) = target;
    std::optional<Dimension> regeneration = regenerated(document, std::move(moved));
    if (!regeneration) {
        return {GripOutcome::Invalid, Entity{dimension}, std::string{kDimensionNotRegenerable}};
    }
    return {GripOutcome::Moved, Entity{std::move(*regeneration)}, {}};
}

}  // namespace tg::core
