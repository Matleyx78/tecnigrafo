// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/report.hpp"

#include "text.hpp"
#include "tg/core/color.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/text_format.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

// Lines of a block (entities, messages...) start with this indentation.
constexpr std::string_view kIndent = "  ";

constexpr std::array kSnapModeNames{SnapModeName{"endpoint", &core::SnapModes::endpoint},
                                    SnapModeName{"midpoint", &core::SnapModes::midpoint},
                                    SnapModeName{"center", &core::SnapModes::center},
                                    SnapModeName{"insertion", &core::SnapModes::insertion},
                                    SnapModeName{"intersection", &core::SnapModes::intersection},
                                    SnapModeName{"perpendicular", &core::SnapModes::perpendicular},
                                    SnapModeName{"tangent", &core::SnapModes::tangent},
                                    SnapModeName{"quadrant", &core::SnapModes::quadrant}};

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
    if (polyline.constantWidth() != 0.0) {
        out += " width=";
        out += formatNumber(polyline.constantWidth());
    }
    for (const core::PolylineVertex& vertex : polyline.vertices()) {
        out += " vertex=";
        out += formatPoint(vertex.position);
        // A straight segment is the default of DXF group 42: only arcs
        // write their bulge.
        if (vertex.bulge != 0.0) {
            out += " bulge=";
            out += formatNumber(vertex.bulge);
        }
        if (vertex.startWidth != 0.0) {
            out += " start-width=";
            out += formatNumber(vertex.startWidth);
        }
        if (vertex.endWidth != 0.0) {
            out += " end-width=";
            out += formatNumber(vertex.endWidth);
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

std::string_view textHorizontalName(core::TextHorizontal horizontal) {
    switch (horizontal) {
        case core::TextHorizontal::Left:
            return "left";
        case core::TextHorizontal::Center:
            return "center";
        case core::TextHorizontal::Right:
            return "right";
        case core::TextHorizontal::Aligned:
            return "aligned";
        case core::TextHorizontal::Middle:
            return "middle";
        case core::TextHorizontal::Fit:
            return "fit";
    }
    return "unknown";
}

std::string_view textVerticalName(core::TextVertical vertical) {
    switch (vertical) {
        case core::TextVertical::Baseline:
            return "baseline";
        case core::TextVertical::Bottom:
            return "bottom";
        case core::TextVertical::Middle:
            return "middle";
        case core::TextVertical::Top:
            return "top";
    }
    return "unknown";
}

/// The corners of the text box, as points separated by ';'.
std::string textBoxText(const core::Text& text) {
    std::string out;
    for (const core::Vec2& corner : core::textBox(text)) {
        if (!out.empty()) {
            out += ';';
        }
        out += formatPoint(corner);
    }
    return out;
}

void writeText(const core::Text& text, std::string& out) {
    // Every property, in the order of docs/verifica/rapporto.md; angles in
    // degrees, as in DXF groups 50 and 51.
    out += "TEXT position=";
    out += formatPoint(text.position);
    out += " alignment-point=";
    out += formatPoint(text.alignmentPoint);
    out += " height=";
    out += formatNumber(text.height);
    out += " rotation=";
    out += formatNumber(core::radiansToDegrees(text.rotation));
    out += " width-factor=";
    out += formatNumber(text.widthFactor);
    out += " oblique=";
    out += formatNumber(core::radiansToDegrees(text.obliqueAngle));
    out += " style=";
    out += quoteText(text.style);
    out += " mirror-x=";
    out += yesNo(text.mirroredX);
    out += " mirror-y=";
    out += yesNo(text.mirroredY);
    out += " horizontal=";
    out += textHorizontalName(text.horizontal);
    out += " vertical=";
    out += textVerticalName(text.vertical);
    out += " value=";
    out += quoteText(text.value);
}

/// The value of the first placement group with `code`, if it is a number.
std::optional<double> placementValue(const core::EntityExtras& extras, int code) {
    for (const core::RawGroup& group : extras.placement) {
        if (group.code == code) {
            return io::parseNumber(detail::trim(group.value));
        }
    }
    return std::nullopt;
}

/// Writes ` name=value` for the placement group `code`, unless it is
/// missing or 0, its default.
void writePlacementNumber(const core::EntityExtras& extras, int code, std::string_view name,
                          std::string& out) {
    const std::optional<double> value = placementValue(extras, code);
    if (value && *value != 0.0) {
        out += ' ';
        out += name;
        out += '=';
        out += formatNumber(*value);
    }
}

/// Quota, thickness and extrusion as read (docs/verifica/rapporto.md, stage
/// 0.5): only those different from their default, quotas first.
void writePlacementProperties(const core::Entity& entity, const core::EntityExtras& extras,
                              std::string& out) {
    std::visit(
        Overloaded{
            [&](const core::Polyline& /*polyline*/) {
                writePlacementNumber(extras, 38, "elevation", out);
            },
            [&](const core::Line& /*line*/) {
                writePlacementNumber(extras, 30, "start-z", out);
                writePlacementNumber(extras, 31, "end-z", out);
            },
            [&](const core::Circle& /*circle*/) { writePlacementNumber(extras, 30, "z", out); },
            [&](const core::Arc& /*arc*/) { writePlacementNumber(extras, 30, "z", out); },
            [&](const core::Text& /*text*/) {
                writePlacementNumber(extras, 30, "z", out);
                writePlacementNumber(extras, 31, "alignment-z", out);
            },
        },
        entity);
    writePlacementNumber(extras, 39, "thickness", out);
    constexpr std::array kExtrusionCodes{210, 220, 230};
    // A missing component is the one of the default direction (0,0,1).
    constexpr std::array kDefaultExtrusion{0.0, 0.0, 1.0};
    std::array<double, 3> extrusion = kDefaultExtrusion;
    for (std::size_t i = 0; i < extrusion.size(); ++i) {
        extrusion.at(i) = placementValue(extras, kExtrusionCodes.at(i)).value_or(extrusion.at(i));
    }
    if (extrusion != kDefaultExtrusion) {
        out += " extrusion=";
        out += formatNumber(extrusion[0]) + ',' + formatNumber(extrusion[1]) + ',' +
               formatNumber(extrusion[2]);
    }
}

/// The name of a layer for the report: a layer the document does not have
/// (a file read without its tables) is the default, "0".
std::string_view layerName(const core::Document& document, core::LayerId id) {
    const core::Layer* layer = document.layer(id);
    return layer != nullptr ? std::string_view{layer->name} : std::string_view{"0"};
}

/// The name of a linetype for the report, "ByLayer" if the document does not
/// have it.
std::string_view linetypeName(const core::Document& document, core::LinetypeId id) {
    const core::Linetype* linetype = document.linetype(id);
    return linetype != nullptr ? std::string_view{linetype->name} : std::string_view{"ByLayer"};
}

/// The properties of stage 0.6, each only if different from its default
/// (docs/verifica/rapporto.md), so that the reports already written stay
/// valid.
void writeProperties(const core::EntityProperties& properties, const core::Document& document,
                     std::string& out) {
    const std::string_view layer = layerName(document, properties.layer);
    if (!core::sameTableName(layer, "0")) {
        out += " layer=";
        out += quoteText(layer);
    }
    if (properties.color != core::Color::byLayer()) {
        out += " color=";
        out += core::formatColor(properties.color);
    }
    const std::string_view linetype = linetypeName(document, properties.linetype);
    if (!core::sameTableName(linetype, "ByLayer")) {
        out += " linetype=";
        out += quoteText(linetype);
    }
    if (properties.linetypeScale != 1.0) {
        out += " ltscale=";
        out += formatNumber(properties.linetypeScale);
    }
    if (properties.lineweight != core::lineweight::kByLayer) {
        out += " lineweight=";
        out += core::formatLineweight(properties.lineweight);
    }
}

/// The five current values, as "current:" writes them.
std::string currentText(const core::Document& document) {
    const core::CurrentProperties& current = document.current();
    return "layer=" + quoteText(layerName(document, current.layer)) +
           " color=" + core::formatColor(current.color) +
           " linetype=" + quoteText(linetypeName(document, current.linetype)) +
           " ltscale=" + formatNumber(current.linetypeScale) +
           " lineweight=" + core::formatLineweight(current.lineweight);
}

/// The tables and the current values (docs/adr/0023, points 1 and 3), each
/// written only when it differs from a new session. A document read from
/// a file before stage 0.6-P10 has no tables at all: it writes no layers.
void writeTables(const core::Document& document, std::string& out) {
    static const core::Document kNewSession = core::Document::newDrawing();
    const std::vector<std::string> layers = layerLines(document);
    if (!layers.empty() && layers != layerLines(kNewSession)) {
        appendLine("layers:", out);
        for (const std::string& line : layers) {
            appendItem(line, out);
        }
    }
    const std::vector<std::string> linetypes = linetypeLines(document);
    if (!linetypes.empty()) {
        appendLine("linetypes:", out);
        for (const std::string& line : linetypes) {
            appendItem(line, out);
        }
    }
    const std::string current = currentText(document);
    if (current != currentText(kNewSession)) {
        appendField("current", current, out);
    }
    if (document.ltscale() != kNewSession.ltscale()) {
        appendField("ltscale", formatNumber(document.ltscale()), out);
    }
    if (document.lwdisplay() != kNewSession.lwdisplay()) {
        appendField("lwdisplay", onOff(document.lwdisplay()), out);
    }
}

/// The derived properties (names starting with '~'), last on the line.
void writeDerived(const core::Entity& entity, std::string& out) {
    if (const auto* text = std::get_if<core::Text>(&entity)) {
        out += " ~box=";
        out += textBoxText(*text);
    }
}

/// The type and the properties of the model, without the derived ones.
void writeModel(const core::Entity& entity, std::string& out) {
    std::visit(Overloaded{
                   [&out](const core::Polyline& polyline) { writePolyline(polyline, out); },
                   [&out](const core::Line& line) { writeLine(line, out); },
                   [&out](const core::Circle& circle) { writeCircle(circle, out); },
                   [&out](const core::Arc& arc) { writeArc(arc, out); },
                   [&out](const core::Text& text) { writeText(text, out); },
               },
               entity);
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
/// modes of a new session are the default ones of core::SnapModes, so a report
/// shows them only when a script has changed them.
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
        writeEntity(*record, document, line);
        appendItem(line, out);
    }
}

/// The unhandled entities by type, in alphabetical order:
/// "HATCH 2, INSERT 1" (docs/adr/0018, point 3).
std::string unhandledText(const core::Document& document) {
    std::map<std::string_view, std::size_t> counts;  // sorted by type name
    for (const core::RawEntity& raw : document.rawEntities()) {
        ++counts[raw.type];
    }
    std::string text;
    for (const auto& [type, count] : counts) {
        if (!text.empty()) {
            text += ", ";
        }
        text += std::string{type} + " " + std::to_string(count);
    }
    return text;
}

/// The fields of the drawing (docs/adr/0021, point 6), each written only
/// when it differs from a new session, so that older reports stay valid.
/// A drawing never opened nor saved has no file to name: its changes alone
/// do not write "drawing:", or every report of milestone 0.4 would change.
void writeDrawing(const edit::Editor& editor, std::string& out) {
    const core::Document& document = editor.document();
    if (editor.hasFile()) {
        appendField("drawing", editor.fileName() + (editor.isModified() ? ", modified" : ""), out);
    }
    if (document.units() != core::Document::kDefaultUnits) {
        appendField("units", std::to_string(document.units()), out);
    }
    writeTables(document, out);
    if (!document.rawEntities().empty()) {
        appendField("unhandled", unhandledText(document), out);
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
    writeDrawing(input.editor, out);
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
                          [](const core::Text& /*text*/) { return "TEXT"; },
                      },
                      entity);
}

void writeEntity(const core::Entity& entity, std::string& out) {
    writeModel(entity, out);
    writeDerived(entity, out);
}

void writeEntity(const core::EntityRecord& record, std::string& out) {
    writeModel(record.entity, out);
    writePlacementProperties(record.entity, record.extras, out);
    writeDerived(record.entity, out);
}

void writeEntity(const core::EntityRecord& record, const core::Document& document,
                 std::string& out) {
    writeModel(record.entity, out);
    writePlacementProperties(record.entity, record.extras, out);
    writeProperties(record.properties, document, out);
    writeDerived(record.entity, out);
}

std::vector<std::string> layerLines(const core::Document& document) {
    std::vector<std::string> lines;
    for (const core::Layer& layer : document.layers()) {
        std::string line = quoteText(layer.name);
        line += " color=";
        line += core::formatColor(layer.color);
        line += " linetype=";
        line += quoteText(linetypeName(document, layer.linetype));
        line += " lineweight=";
        line += core::formatLineweight(layer.lineweight);
        // The states only when on, in this order (docs/verifica/rapporto.md).
        if (!layer.on) {
            line += " off";
        }
        if (layer.frozen) {
            line += " frozen";
        }
        if (layer.locked) {
            line += " locked";
        }
        if (!layer.plot) {
            line += " noplot";
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

std::vector<std::string> linetypeLines(const core::Document& document) {
    std::vector<std::string> lines;
    for (const core::Linetype& linetype : document.linetypes()) {
        // Every drawing has these three (docs/adr/0025, point 2).
        if (core::sameTableName(linetype.name, "ByBlock") ||
            core::sameTableName(linetype.name, "ByLayer") ||
            core::sameTableName(linetype.name, "Continuous")) {
            continue;
        }
        std::string line = quoteText(linetype.name);
        line += " pattern=";
        if (linetype.pattern.continuous()) {
            line += "continuous";
        }
        for (std::size_t i = 0; i < linetype.pattern.elements.size(); ++i) {
            line += i == 0 ? "" : ",";
            line += formatNumber(linetype.pattern.elements[i]);
        }
        line += " description=";
        line += quoteText(linetype.description);
        lines.push_back(std::move(line));
    }
    return lines;
}

std::string_view snapKindName(core::SnapKind kind) {
    switch (kind) {
        case core::SnapKind::Endpoint:
            return "Endpoint";
        case core::SnapKind::Midpoint:
            return "Midpoint";
        case core::SnapKind::Center:
            return "Center";
        case core::SnapKind::Insertion:
            return "Insertion";
        case core::SnapKind::Intersection:
            return "Intersection";
        case core::SnapKind::Perpendicular:
            return "Perpendicular";
        case core::SnapKind::Tangent:
            return "Tangent";
        case core::SnapKind::Quadrant:
            return "Quadrant";
    }
    return "Unknown";
}

std::span<const SnapModeName> snapModeNames() {
    return kSnapModeNames;
}

}  // namespace tg::script
