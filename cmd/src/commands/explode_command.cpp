// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/explode_command.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/explode.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

// DXF groups of a polyline's position that a line or an arc would write with
// other groups: the elevation and the thickness (docs/adr/0031, point 8).
constexpr int kElevationGroup = 38;
constexpr int kThicknessGroup = 39;

/// True if the number written in `text` is not zero. Decided on the digits
/// alone, so that it does not depend on the locale: "0", "-0.0" and "0E5"
/// are zero, "0.5" and "1e-3" are not.
bool isNonzero(const std::string& text) {
    return std::ranges::any_of(text, [](char c) { return c >= '1' && c <= '9'; });
}

/// True if the polyline carries an elevation or a thickness other than zero
/// among the groups of its position.
bool hasElevationOrThickness(const core::EntityExtras& extras) {
    return std::ranges::any_of(extras.placement, [](const core::RawGroup& group) {
        return (group.code == kElevationGroup || group.code == kThicknessGroup) &&
               isNonzero(group.value);
    });
}

/// True if exploding the polyline discards a width: the constant width or the
/// width of any vertex (docs/adr/0031, point 8).
bool hasWidth(const core::Polyline& polyline) {
    return polyline.constantWidth() != 0.0 ||
           std::ranges::any_of(polyline.vertices(), [](const core::PolylineVertex& vertex) {
               return vertex.startWidth != 0.0 || vertex.endWidth != 0.0;
           });
}

/// The arc, always counter-clockwise (docs/adr/0012), of a bulged segment.
core::Arc arcOf(const core::PolylineSegment& segment) {
    const core::ArcGeometry geometry =
        core::arcFromBulge(segment.start, segment.end, segment.bulge);
    // A clockwise segment is the same arc travelled from its end to its start.
    const double first =
        geometry.sweep > 0.0 ? geometry.startAngle : geometry.startAngle + geometry.sweep;
    const double last =
        geometry.sweep > 0.0 ? geometry.startAngle + geometry.sweep : geometry.startAngle;
    return core::Arc{geometry.center, geometry.radius, core::normalizeAngle(first),
                     core::normalizeAngle(last)};
}

/// Replaces the polyline `id` with its lines and arcs.
void explode(edit::Editor& editor, core::EntityId id, const core::Polyline& polyline) {
    for (std::size_t index = 0; index < polyline.segmentCount(); ++index) {
        const core::PolylineSegment segment = polyline.segment(index);
        if (segment.isArc()) {
            editor.addCopy(id, arcOf(segment));
        } else {
            editor.addCopy(id, core::Line{segment.start, segment.end});
        }
    }
    editor.remove(id);
}

/// Replaces the entity `id` with what it exploded into, in the order of the
/// block or of the graphics: the two lists merged by order key, a record
/// first on a tie (docs/tappe/0.8/PT29-explode-blocchi-quote.md, point 2).
void replaceWith(edit::Editor& editor, core::EntityId id, core::Exploded exploded) {
    std::size_t m = 0;
    std::size_t r = 0;
    while (m < exploded.entities.size() || r < exploded.raw.size()) {
        if (r < exploded.raw.size() &&
            (m == exploded.entities.size() || exploded.raw[r].order < exploded.entities[m].order)) {
            editor.addRaw(std::move(exploded.raw[r++]));
        } else {
            editor.addRecord(std::move(exploded.entities[m++]));
        }
    }
    editor.remove(id);
}

/// The graphics of the dimension of `record` with the properties they are
/// shown with: layer 0 and ByBlock become those of the dimension, as AutoCAD
/// does (PT29, point 1).
core::Exploded explodeDimensionOf(const core::Document& document, const core::EntityRecord& record,
                                  const core::Dimension& dimension) {
    core::Exploded exploded = core::explodeDimension(document, dimension);
    for (core::EntityRecord& graphic : exploded.entities) {
        graphic.properties =
            core::propertiesInBlock(document, record.properties, graphic.properties);
    }
    return exploded;
}

}  // namespace

void ExplodeCommand::start(CommandContext& context) {
    context.editor.beginTransaction("Explode");
}

Status ExplodeCommand::handle(CommandContext& context, const Input& input) {
    const auto* selection = std::get_if<SelectionInput>(&input);
    if (selection == nullptr) {
        return Status::Continue;
    }
    std::size_t notExploded = 0;
    bool widthLost = false;
    for (const core::EntityId id : selection->ids) {
        const core::Document& document = context.editor.document();
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr) {
            continue;
        }
        const auto* insert = std::get_if<core::Insert>(&record->entity);
        const auto* dimension = std::get_if<core::Dimension>(&record->entity);
        if (insert != nullptr || dimension != nullptr) {
            core::Exploded exploded = insert != nullptr
                                          ? core::explodeInsert(document, *insert)
                                          : explodeDimensionOf(document, *record, *dimension);
            if (!exploded.refusal.empty()) {
                // The entity stays (PT29, point 2).
                context.message(exploded.refusal);
                continue;
            }
            replaceWith(context.editor, id, std::move(exploded));
            continue;
        }
        const auto* polyline = std::get_if<core::Polyline>(&record->entity);
        if (polyline == nullptr || polyline->segmentCount() == 0 ||
            hasElevationOrThickness(record->extras)) {
            ++notExploded;
            continue;
        }
        widthLost = widthLost || hasWidth(*polyline);
        // explode() adds to the document, so the polyline is copied first.
        const core::Polyline source = *polyline;
        explode(context.editor, id, source);
    }
    if (widthLost) {
        // The messages of AutoCAD, written once for the whole command.
        context.message("Exploding this polyline has lost width information.");
        context.message("The UNDO command will restore it.");
    }
    if (notExploded == 1) {
        context.message("1 was not able to be exploded.");
    } else if (notExploded > 1) {
        context.message(std::to_string(notExploded) + " were not able to be exploded.");
    }
    // Nothing exploded leaves an empty step, which the editor does not keep.
    context.editor.commit();
    return Status::Finished;
}

void ExplodeCommand::pointerMoved(const core::Vec2& /*point*/) {}

Prompt ExplodeCommand::prompt() const {
    return {"Select objects:", {}};
}

std::span<const core::Entity> ExplodeCommand::preview() const {
    return {};
}

bool ExplodeCommand::needsSelection() const {
    return true;
}

}  // namespace tg::cmd
