// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/report.hpp"

#include "new_entities_format.hpp"
#include "text.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/color.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/drawing_settings.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
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
#include <span>
#include <string>
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

// The three modes of stage 0.8.1 (nearest, extension, parallel) have no
// name above, so the reports written before them do not change: a report
// lists the modes above that are off, and the question snap takes nearest as
// "nea" (question_table.cpp; docs/tappe/0.8.1/PT30-snap-vicino.md).
constexpr std::size_t kStageEightOneModes = 3;

// A new member of core::SnapModes needs its name above, or reports and
// questions would silently ignore it.
static_assert(sizeof(core::SnapModes) ==
                  (kSnapModeNames.size() + kStageEightOneModes) * sizeof(bool),
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
    if (polyline.continuousLinetype()) {
        out += " linetype-generation=yes";
    }
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

void writeText(const core::Text& text, std::string& out) {
    out += "TEXT ";
    detail::writeTextProperties(text, out);
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
            // The types of the 0.8 have no quota in docs/verifica/rapporto.md:
            // only thickness and extrusion.
            [](const core::Solid& /*solid*/) {},
            [](const core::Ellipse& /*ellipse*/) {},
            [](const core::MText& /*mtext*/) {},
            [](const core::AttributeDefinition& /*definition*/) {},
            [](const core::Insert& /*insert*/) {},
            [](const core::Dimension& /*dimension*/) {},
            [](const core::Hatch& /*hatch*/) {},
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

/// The essential dimension variables of `variables` that differ from
/// `reference`, `NAME:value`, separated by a space, in alphabetical order
/// (docs/verifica/rapporto.md, "Stili e blocchi").
std::string dimVariableDifferences(const core::DimVariables& variables,
                                   const core::DimVariables& reference) {
    std::string text;
    for (const auto& info : core::dimVariableInfos()) {
        const core::DimValue value = core::dimValue(variables, info.name);
        if (value == core::dimValue(reference, info.name)) {
            continue;
        }
        if (!text.empty()) {
            text += ' ';
        }
        text += std::string{info.name} + ":";
        if (const auto* number = std::get_if<double>(&value)) {
            text += formatNumber(*number);
        } else {
            text += std::get<std::string>(value);
        }
    }
    return text;
}

/// `dimension styles:`, `dimension style:` and `dimension overrides:`, when
/// they differ from a new session (package 0.8-PT22, point 6).
void writeDimensionStyles(const core::Document& document, std::string& out) {
    const core::DimStyleTable& table = document.dimStyles();
    static const core::Document kFreshDocument = core::Document::newDrawing();
    const core::DimStyleTable& fresh = kFreshDocument.dimStyles();
    const auto sameStyles = std::ranges::equal(
        table.styles(), fresh.styles(), [](const core::DimStyle& a, const core::DimStyle& b) {
            return a.name == b.name && a.vars == b.vars;
        });
    const core::DimVariables iso25;
    if (!table.styles().empty() && !sameStyles) {
        appendLine("dimension styles:", out);
        for (const core::DimStyle& style : table.styles()) {
            const std::string differences = dimVariableDifferences(style.vars, iso25);
            appendItem(quoteText(style.name) + (differences.empty() ? "" : " " + differences), out);
        }
    }
    if (!table.current().empty() && !core::sameTableName(table.current(), "ISO-25")) {
        appendField("dimension style", quoteText(table.current()), out);
    }
    const core::DimStyle* current = table.find(table.current());
    const std::string overrides = dimVariableDifferences(
        table.currentVariables(), current != nullptr ? current->vars : iso25);
    if (!overrides.empty()) {
        appendField("dimension overrides", overrides, out);
    }
}

/// The block definitions, only when there are some (docs/verifica/rapporto.md,
/// "Stili e blocchi"): for each one its line, then one line for each of its
/// entities, in the order of the definition, with the ATTRIB of an INSERT.
void writeBlocks(const core::Document& document, std::string& out) {
    const std::span<const core::BlockDefinition> blocks = document.blocks().definitions();
    if (blocks.empty()) {
        return;
    }
    appendLine("blocks:", out);
    for (const core::BlockDefinition& block : blocks) {
        const std::string name = quoteText(block.name);
        std::string line = name + " base=" + formatPoint(block.base);
        // Each one only when different from its default, in this order.
        if (!block.explodable) {
            line += " explodable=no";
        }
        if (block.uniformScale) {
            line += " uniform=yes";
        }
        if (block.units != 0) {
            line += " units=" + std::to_string(block.units);
        }
        if (!block.description.empty()) {
            line += " description=" + quoteText(block.description);
        }
        appendItem(line, out);
        for (const core::EntityRecord& record : block.entities) {
            const std::array<const core::EntityRecord*, 1> alone{&record};
            for (const detail::EntityLine& entity : detail::entityLines(alone, document)) {
                appendItem(name + " " + entity.text, out);
            }
        }
    }
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
    if (document.attributeDisplay() != core::AttributeDisplay::Normal) {
        appendField("attdisp", onOff(document.attributeDisplay() == core::AttributeDisplay::On),
                    out);
    }
    std::vector<const core::TextStyle*> changedStyles;
    const core::TextStyle* defaultStandard = kNewSession.textStyles().find("Standard");
    for (const core::TextStyle& style : document.textStyles().styles()) {
        const bool standardLike = core::sameTableName(style.name, "Standard") &&
                                  defaultStandard != nullptr &&
                                  style.font == defaultStandard->font &&
                                  style.fixedHeight == defaultStandard->fixedHeight &&
                                  style.widthFactor == defaultStandard->widthFactor &&
                                  style.oblique == defaultStandard->oblique &&
                                  style.backwards == defaultStandard->backwards &&
                                  style.upsideDown == defaultStandard->upsideDown;
        if (!standardLike) {
            changedStyles.push_back(&style);
        }
    }
    if (!changedStyles.empty()) {
        appendLine("text styles:", out);
        for (const core::TextStyle* style : changedStyles) {
            const std::string line =
                quoteText(style->name) + " font=" + quoteText(style->font) +
                " height=" + formatNumber(style->fixedHeight) +
                " width-factor=" + formatNumber(style->widthFactor) +
                " oblique=" + formatNumber(core::radiansToDegrees(style->oblique)) +
                " backwards=" + std::string{yesNo(style->backwards)} +
                " upside-down=" + std::string{yesNo(style->upsideDown)};
            appendItem(line, out);
        }
    }
    if (!document.textStyles().current().empty() &&
        !core::sameTableName(document.textStyles().current(), "Standard")) {
        appendField("text style", quoteText(document.textStyles().current()), out);
    }
    writeDimensionStyles(document, out);
    writeBlocks(document, out);
}

/// The derived properties (names starting with '~'), last on the line,
/// but those that need the generated geometry (writeGeneratedDerived()).
void writeDerived(const core::Entity& entity, std::string& out) {
    if (const auto* text = std::get_if<core::Text>(&entity)) {
        detail::writeTextBox(*text, out);
    } else {
        detail::writeNewDerived(entity, out);
    }
}

/// The type and the properties of the model, without the derived ones.
void writeModel(const core::Entity& entity, std::string& out) {
    std::visit(
        Overloaded{
            [&out](const core::Polyline& polyline) { writePolyline(polyline, out); },
            [&out](const core::Line& line) { writeLine(line, out); },
            [&out](const core::Circle& circle) { writeCircle(circle, out); },
            [&out](const core::Arc& arc) { writeArc(arc, out); },
            [&out](const core::Text& text) { writeText(text, out); },
            // The types of the 0.8 (docs/verifica/rapporto.md, "Tappa 0.8").
            [&out, &entity](const auto& /*composite*/) { detail::writeNewEntity(entity, out); },
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

/// "on" or "off", followed by the snap modes that are off, if any: a report
/// shows the modes only when a script has changed them from those of a new
/// session (docs/adr/0033, point 6).
std::string snapText(const cmd::DrawingAids& aids) {
    std::string text{onOff(aids.objectSnap)};
    const core::SnapModes& modes = aids.snapModes;
    if (modes == core::SnapModes{}) {
        return text;
    }
    text += ", modes off:";
    bool anyOff = false;
    for (const SnapModeName& mode : kSnapModeNames) {
        if (!(modes.*mode.member)) {
            text += ' ';
            text += mode.name;
            anyOff = true;
        }
    }
    // Every mode on is not the state of a new session since stage 0.7, where
    // perpendicular, tangent and quadrant start off: the report still says so.
    if (!anyOff) {
        text += " none";
    }
    return text;
}

std::string snapPointText(const std::optional<core::SnapPoint>& snap) {
    if (!snap) {
        return "none";
    }
    return std::string{snapKindName(snap->kind)} + " " + formatPoint(snap->point);
}

/// Every record of the document, for entityLines() and statisticsLines().
std::vector<const core::EntityRecord*> documentRecords(const core::Document& document) {
    std::vector<const core::EntityRecord*> records;
    records.reserve(document.entityCount());
    for (const core::EntityRecord& record : document.entities()) {
        records.push_back(&record);
    }
    return records;
}

void writeEntities(const core::Document& document, std::string& out) {
    appendLine("entities:", out);
    for (const detail::EntityLine& line :
         detail::entityLines(documentRecords(document), document)) {
        appendItem(line.text, out);
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
void writeDrawing(const ReportInput& input, std::string& out) {
    const edit::Editor& editor = input.editor;
    const edit::DocumentFile& file = input.file;
    const core::Document& document = editor.document();
    if (file.hasFile()) {
        appendField("drawing", file.name() + (editor.isModified() ? ", modified" : ""), out);
    }
    if (document.units() != core::Document::kDefaultUnits) {
        appendField("units", std::to_string(document.units()), out);
    }
    writeTables(document, out);
    if (!document.rawEntities().empty()) {
        appendField("unhandled", unhandledText(document), out);
    }
    // Only with several documents open (docs/verifica/rapporto.md), so that
    // the reports of a single document stay as they were.
    if (input.documents.size() > 1) {
        std::string names;
        for (const ReportDocument& open : input.documents) {
            if (!names.empty()) {
                names += ", ";
            }
            names += open.name;
            if (open.active) {
                names += " (active)";
            }
        }
        appendField("documents", names, out);
    }
}

void writeStatistics(const core::Document& document, std::string& out) {
    appendLine("statistics:", out);
    for (const std::string& item : detail::statisticsLines(documentRecords(document))) {
        appendItem(item, out);
    }
}

/// The colour, linetype, linetype scale and lineweight of `properties` as
/// the question `effective` writes them (docs/verifica/domande.md), and
/// locked=yes for an entity on a locked layer.
std::vector<std::string> effectiveTexts(const core::EffectiveProperties& properties) {
    std::vector<std::string> texts{
        "color=" + core::formatColor(core::Color::fromRgb(properties.color)),
        "linetype=" + quoteText(properties.linetype->name),
        "ltscale=" + formatNumber(properties.linetypeScale),
        "lineweight=" + core::formatLineweight(properties.lineweight)};
    if (properties.locked) {
        texts.emplace_back("locked=yes");
    }
    return texts;
}

/// The effective properties of an entity with the default properties in a
/// new session: those that the question `generated` leaves out.
const std::vector<std::string>& defaultEffectiveTexts() {
    static const std::vector<std::string> kDefaults = [] {
        const core::Document document = core::Document::newDrawing();
        core::EntityRecord record{core::EntityId{}, core::Line{}};
        record.properties.layer = document.layerZero();
        record.properties.linetype = document.byLayerLinetype();
        return effectiveTexts(core::effectiveProperties(document, record));
    }();
    return kDefaults;
}

/// The six coefficients of `transform` (transform2d.hpp), "a,b,c,d,e,f".
std::string affineText(const core::Transform2D& transform) {
    const core::Vec2 x = transform.applyToVector({1.0, 0.0});
    const core::Vec2 y = transform.applyToVector({0.0, 1.0});
    const core::Vec2 offset = transform.apply({0.0, 0.0});
    return formatNumber(x.x) + ',' + formatNumber(x.y) + ',' + formatNumber(y.x) + ',' +
           formatNumber(y.y) + ',' + formatNumber(offset.x) + ',' + formatNumber(offset.y);
}

/// A filled region of the generated geometry: "REGION loop=polyline
/// vertex=… bulge=…", one loop= per loop, as the boundary paths of HATCH.
void writeRegion(const core::FilledRegion& region, std::string& out) {
    out += "REGION";
    for (const core::Polyline& loop : region.loops) {
        out += " loop=polyline";
        for (const core::PolylineVertex& vertex : loop.vertices()) {
            out += " vertex=";
            out += formatPoint(vertex.position);
            if (vertex.bulge != 0.0) {
                out += " bulge=";
                out += formatNumber(vertex.bulge);
            }
        }
    }
}

}  // namespace

void detail::writeGeneratedItem(const core::GeneratedItem& item, std::string& out) {
    // A formatted piece with its variant and its box, not those of the Text
    // of shapeEntity() (docs/tappe/0.8.1/PT8-impaginazione-formati.md,
    // point 9).
    const auto* piece = std::get_if<core::FormattedText>(&item.shape);
    const std::optional<core::Entity> entity =
        piece != nullptr ? std::nullopt : core::shapeEntity(item.shape);
    const auto* region = std::get_if<core::FilledRegion>(&item.shape);
    if (piece != nullptr) {
        writeFormattedText(*piece, out);
    } else if (entity) {
        writeModel(*entity, out);
    } else if (region != nullptr) {
        writeRegion(*region, out);
    }
    const std::vector<std::string>& defaults = defaultEffectiveTexts();
    for (const std::string& text : effectiveTexts(item.properties)) {
        if (std::ranges::find(defaults, text) == defaults.end()) {
            out += ' ';
            out += text;
        }
    }
    if (piece != nullptr) {
        writeFormattedTextBox(*piece, out);
    } else if (entity) {
        writeDerived(*entity, out);
    } else if (region != nullptr) {
        // The identity is the default of Transform2D: exact on purpose, a
        // map computed to be the identity writes its coefficients.
        const std::string affine = affineText(region->transform);
        if (affine != affineText(core::Transform2D::identity())) {
            out += " ~affine=";
            out += affine;
        }
    }
}

std::vector<detail::EntityLine> detail::entityLines(
    std::span<const core::EntityRecord* const> records, const core::Document& document) {
    std::vector<EntityLine> lines;
    lines.reserve(records.size());
    for (const core::EntityRecord* record : records) {
        std::string line = std::to_string(record->id.value) + " ";
        writeEntity(*record, document, line);
        lines.push_back(EntityLine{record->id.value, std::move(line)});
        const auto* insert = std::get_if<core::Insert>(&record->entity);
        if (insert == nullptr) {
            continue;
        }
        // The ATTRIB are entities of the DXF, with their own identifier and
        // properties (docs/verifica/rapporto.md, "Tappa 0.8"); their quotas
        // are those of a TEXT.
        for (const core::Attribute& attribute : insert->attributes) {
            std::string text = std::to_string(attribute.id.value) + " ";
            writeAttribute(attribute, record->id, text);
            writePlacementProperties(core::Entity{attribute.text}, attribute.extras, text);
            writeProperties(attribute.properties, document, text);
            writeTextBox(attribute.text, text);
            lines.push_back(EntityLine{attribute.id.value, std::move(text)});
        }
    }
    // The document keeps the drawing order, which undo and redo change:
    // the report lists entities by identifier (docs/adr/0011, point 5).
    std::ranges::stable_sort(lines, {}, &EntityLine::id);
    return lines;
}

std::vector<std::string> detail::statisticsLines(
    std::span<const core::EntityRecord* const> records) {
    std::map<std::string_view, std::size_t> counts;  // sorted by type name
    for (const core::EntityRecord* record : records) {
        ++counts[dxfType(record->entity)];
        if (const auto* insert = std::get_if<core::Insert>(&record->entity);
            insert != nullptr && !insert->attributes.empty()) {
            counts["ATTRIB"] += insert->attributes.size();
        }
    }
    std::vector<std::string> lines;
    lines.reserve(counts.size());
    for (const auto& [type, count] : counts) {
        lines.push_back(std::string{type} + " " + std::to_string(count));
    }
    return lines;
}

namespace {

// Object snap tracking (docs/adr/0049, point 8): each line only when it
// differs from a new session ("Come cresce il formato").
void writeTracking(const cmd::CommandRunner& runner, std::string& out) {
    using Guide = cmd::CommandRunner::TrackingState::Guide;
    if (runner.drawingAids().tracking) {
        appendField("tracking", "on", out);
    }
    const cmd::CommandRunner::TrackingState& tracking = runner.tracking();
    if (!tracking.acquired.empty()) {
        appendLine("acquired:", out);
        for (const core::Vec2& point : tracking.acquired) {
            appendItem(formatPoint(point), out);
        }
    }
    if (!tracking.guidePoint) {
        return;
    }
    const std::string point = formatPoint(*tracking.guidePoint);
    const std::string from = tracking.from ? " from " + formatPoint(*tracking.from) : "";
    switch (tracking.guide) {
        case Guide::None:
            break;
        case Guide::Horizontal:
            appendField("active tracking", "Horizontal " + point + from, out);
            break;
        case Guide::Vertical:
            appendField("active tracking", "Vertical " + point + from, out);
            break;
        case Guide::Crossing:
            appendField("active tracking", "Crossing " + point, out);
            break;
        case Guide::WithPolar:
            appendField("active tracking", "Polar crossing " + point, out);
            break;
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
        // The document gives the derived properties of the generated
        // geometry (~lines of an MTEXT, ~text of a DIMENSION).
        std::string line;
        writeEntity(entity, line);
        detail::writeGeneratedDerived(document, core::EntityRecord{core::EntityId{}, entity}, line);
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
    // Only with the snap to the grid on (docs/adr/0055, point 7), so that
    // the reports of the drawings with the snap off stay as they were.
    if (const core::GridSettings& grid = document.grid(); grid.snapOn) {
        appendField("grid snap",
                    "unit=" + formatPoint(grid.snapUnit) + " base=" + formatPoint(grid.snapBase),
                    out);
    }
    writeDrawing(input, out);
    appendField("pointer", input.pointer ? formatPoint(*input.pointer) : "none", out);
    appendField("active snap", snapPointText(runner.activeSnap()), out);
    writeTracking(runner, out);
    writeStatistics(document, out);
}

std::string_view dxfType(const core::Entity& entity) {
    return std::visit(Overloaded{
                          [](const core::Polyline& /*polyline*/) { return "LWPOLYLINE"; },
                          [](const core::Line& /*line*/) { return "LINE"; },
                          [](const core::Circle& /*circle*/) { return "CIRCLE"; },
                          [](const core::Arc& /*arc*/) { return "ARC"; },
                          [](const core::Text& /*text*/) { return "TEXT"; },
                          [](const core::Solid& /*solid*/) { return "SOLID"; },
                          [](const core::Ellipse& /*ellipse*/) { return "ELLIPSE"; },
                          [](const core::MText& /*mtext*/) { return "MTEXT"; },
                          [](const core::AttributeDefinition& /*definition*/) { return "ATTDEF"; },
                          [](const core::Insert& /*insert*/) { return "INSERT"; },
                          [](const core::Dimension& /*dimension*/) { return "DIMENSION"; },
                          [](const core::Hatch& /*hatch*/) { return "HATCH"; },
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
    detail::writeGeneratedDerived(document, record, out);
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
        case core::SnapKind::Nearest:
            return "Nearest";
        case core::SnapKind::Extension:
            return "Extension";
        case core::SnapKind::Parallel:
            return "Parallel";
    }
    return "Unknown";
}

std::span<const SnapModeName> snapModeNames() {
    return kSnapModeNames;
}

}  // namespace tg::script
