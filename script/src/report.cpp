// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/report.hpp"

#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/script/number_format.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

// Lines of a block (entities, messages...) start with this indentation.
constexpr std::string_view kIndent = "  ";

constexpr std::array kSnapModeNames{SnapModeName{"endpoint", &core::SnapModes::endpoint},
                                    SnapModeName{"midpoint", &core::SnapModes::midpoint},
                                    SnapModeName{"center", &core::SnapModes::center}};

// A new member of core::SnapModes needs its name above, or reports and
// questions would silently ignore it.
static_assert(sizeof(core::SnapModes) == kSnapModeNames.size() * sizeof(bool),
              "every snap mode needs a name in kSnapModeNames");

/// Builds one visitor out of several lambdas, one per entity type.
template <typename... Lambdas>
struct Overloaded : Lambdas... {
    using Lambdas::operator()...;
};

// Explicit deduction guide: aggregate CTAD is missing from Apple Clang 15.
template <typename... Lambdas>
Overloaded(Lambdas...) -> Overloaded<Lambdas...>;

void appendLine(std::string_view text, std::string& out) {
    out += text;
    out += '\n';
}

void appendItem(std::string_view text, std::string& out) {
    out += kIndent;
    appendLine(text, out);
}

void appendField(std::string_view name, std::string_view value, std::string& out) {
    out += name;
    out += ": ";
    appendLine(value, out);
}

std::string_view yesNo(bool value) {
    return value ? "yes" : "no";
}

std::string_view onOff(bool value) {
    return value ? "on" : "off";
}

void writePolyline(const core::Polyline& polyline, std::string& out) {
    out += "LWPOLYLINE closed=";
    out += yesNo(polyline.isClosed());
    for (const core::PolylineVertex& vertex : polyline.vertices()) {
        out += " vertex=";
        out += formatPoint(vertex.position);
        // A straight segment is the default of DXF group 42: only arcs
        // write their bulge.
        if (vertex.bulge != 0.0) {
            out += " bulge=";
            out += formatNumber(vertex.bulge);
        }
    }
}

void writeLine(const core::Line& line, std::string& out) {
    out += "LINE start=";
    out += formatPoint(line.start);
    out += " end=";
    out += formatPoint(line.end);
}

void writeCircle(const core::Circle& circle, std::string& out) {
    out += "CIRCLE center=";
    out += formatPoint(circle.center);
    out += " radius=";
    out += formatNumber(circle.radius);
}

void writeArc(const core::Arc& arc, std::string& out) {
    // Degrees, as in DXF groups 50 and 51 (docs/verifica/rapporto.md).
    out += "ARC center=";
    out += formatPoint(arc.center);
    out += " radius=";
    out += formatNumber(arc.radius);
    out += " start-angle=";
    out += formatNumber(core::radiansToDegrees(arc.startAngle));
    out += " end-angle=";
    out += formatNumber(core::radiansToDegrees(arc.endAngle));
}

std::string selectionText(const cmd::SelectionSet& selection) {
    if (selection.isEmpty()) {
        return "none";
    }
    std::vector<core::EntityId> ids(selection.ids().begin(), selection.ids().end());
    std::ranges::sort(ids, {}, &core::EntityId::value);
    std::string text;
    for (const core::EntityId id : ids) {
        if (!text.empty()) {
            text += ' ';
        }
        text += std::to_string(id.value);
    }
    return text;
}

std::string rectangleText(const std::optional<cmd::SelectionRectangle>& rectangle) {
    if (!rectangle) {
        return "none";
    }
    return std::string{rectangle->crossing ? "crossing " : "window "} +
           formatPoint(rectangle->first) + " " + formatPoint(rectangle->second);
}

/// "on" or "off", followed by the snap modes that are off, if any: the
/// modes of a new session are all on, so a report shows them only when a
/// script has changed them.
std::string snapText(const cmd::DrawingAids& aids) {
    std::string text{onOff(aids.objectSnap)};
    const core::SnapModes& modes = aids.snapModes;
    if (modes == core::SnapModes{}) {
        return text;
    }
    text += ", modes off:";
    for (const SnapModeName& mode : kSnapModeNames) {
        if (!(modes.*mode.member)) {
            text += ' ';
            text += mode.name;
        }
    }
    return text;
}

std::string snapPointText(const std::optional<core::SnapPoint>& snap) {
    if (!snap) {
        return "none";
    }
    return std::string{snapKindName(snap->kind)} + " " + formatPoint(snap->point);
}

void writeEntities(const core::Document& document, std::string& out) {
    std::vector<const core::EntityRecord*> records;
    records.reserve(document.entityCount());
    for (const core::EntityRecord& record : document.entities()) {
        records.push_back(&record);
    }
    // The document keeps the drawing order, which undo and redo change:
    // the report lists entities by identifier (docs/adr/0011, point 5).
    std::ranges::sort(records, {},
                      [](const core::EntityRecord* record) { return record->id.value; });
    appendLine("entities:", out);
    for (const core::EntityRecord* record : records) {
        std::string line = std::to_string(record->id.value) + " ";
        writeEntity(record->entity, line);
        appendItem(line, out);
    }
}

void writeStatistics(const core::Document& document, std::string& out) {
    std::map<std::string_view, std::size_t> counts;  // sorted by type name
    for (const core::EntityRecord& record : document.entities()) {
        ++counts[dxfType(record.entity)];
    }
    appendLine("statistics:", out);
    for (const auto& [type, count] : counts) {
        appendItem(std::string{type} + " " + std::to_string(count), out);
    }
}

}  // namespace

void writeState(const ReportInput& input, std::string& out) {
    const cmd::CommandRunner& runner = input.runner;
    const core::Document& document = input.editor.document();

    out += '\n';
    appendLine("[" + std::string{input.title} + "]", out);
    writeEntities(document, out);
    appendLine("messages:", out);
    for (const std::string& message : input.messages) {
        appendItem(message, out);
    }
    appendField("prompt", runner.promptText(), out);
    appendLine("preview:", out);
    for (const core::Entity& entity : runner.preview()) {
        std::string line;
        writeEntity(entity, line);
        appendItem(line, out);
    }
    appendField("selection", selectionText(runner.selection()), out);
    appendField("rectangle", rectangleText(runner.selectionRectangle()), out);
    appendLine("undo:", out);
    for (const std::string& name : input.editor.undoNames()) {
        appendItem(name, out);
    }
    appendLine("redo:", out);
    for (const std::string& name : input.editor.redoNames()) {
        appendItem(name, out);
    }
    appendField("snap", snapText(runner.drawingAids()), out);
    appendField("ortho", onOff(runner.drawingAids().ortho), out);
    appendField("pointer", input.pointer ? formatPoint(*input.pointer) : "none", out);
    appendField("active snap", snapPointText(runner.activeSnap()), out);
    writeStatistics(document, out);
}

std::string_view dxfType(const core::Entity& entity) {
    return std::visit(Overloaded{
                          [](const core::Polyline& /*polyline*/) { return "LWPOLYLINE"; },
                          [](const core::Line& /*line*/) { return "LINE"; },
                          [](const core::Circle& /*circle*/) { return "CIRCLE"; },
                          [](const core::Arc& /*arc*/) { return "ARC"; },
                      },
                      entity);
}

void writeEntity(const core::Entity& entity, std::string& out) {
    std::visit(Overloaded{
                   [&out](const core::Polyline& polyline) { writePolyline(polyline, out); },
                   [&out](const core::Line& line) { writeLine(line, out); },
                   [&out](const core::Circle& circle) { writeCircle(circle, out); },
                   [&out](const core::Arc& arc) { writeArc(arc, out); },
               },
               entity);
}

std::string_view snapKindName(core::SnapKind kind) {
    switch (kind) {
        case core::SnapKind::Endpoint:
            return "Endpoint";
        case core::SnapKind::Midpoint:
            return "Midpoint";
        case core::SnapKind::Center:
            return "Center";
    }
    return "Unknown";
}

std::span<const SnapModeName> snapModeNames() {
    return kSnapModeNames;
}

}  // namespace tg::script
