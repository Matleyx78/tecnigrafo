// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/explode_command.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
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
        const core::EntityRecord* record = context.editor.document().find(id);
        if (record == nullptr) {
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
