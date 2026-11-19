// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/dimension_angular_commands.hpp"

#include "dimension_input.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/proximity.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <variant>

namespace tg::cmd {
namespace {

std::string dimValueText(const core::DimValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        return *text;
    }
    std::ostringstream stream;
    stream << std::setprecision(17) << std::get<double>(value);
    return stream.str();
}

std::vector<std::pair<std::string, std::string>> currentOverrides(
    const core::DimStyleTable& table) {
    std::vector<std::pair<std::string, std::string>> result;
    const core::DimStyle* style = table.find(table.current());
    if (style == nullptr) {
        return result;
    }
    for (const core::DimVariableInfo& info : core::dimVariableInfos()) {
        const core::DimValue current = core::dimValue(table.currentVariables(), info.name);
        if (current != core::dimValue(style->vars, info.name)) {
            result.emplace_back(info.name, dimValueText(current));
        }
    }
    return result;
}

std::optional<core::Line> pickedLine(const core::Entity& entity, const core::Vec2& pick) {
    if (const auto* line = std::get_if<core::Line>(&entity)) {
        return *line;
    }
    const auto* polyline = std::get_if<core::Polyline>(&entity);
    if (polyline == nullptr) {
        return std::nullopt;
    }
    double best = std::numeric_limits<double>::infinity();
    std::optional<core::PolylineSegment> chosen;
    for (std::size_t index = 0; index < polyline->segmentCount(); ++index) {
        const core::PolylineSegment segment = polyline->segment(index);
        const double distance = core::distance(segment, pick);
        if (distance < best) {
            best = distance;
            chosen = segment;
        }
    }
    if (!chosen || chosen->isArc()) {
        return std::nullopt;
    }
    return core::Line{chosen->start, chosen->end};
}

std::optional<core::Vec2> lineIntersection(const core::Line& first, const core::Line& second) {
    const core::Vec2 a = first.end - first.start;
    const core::Vec2 b = second.end - second.start;
    const double cross = (a.x * b.y) - (a.y * b.x);
    if (std::abs(cross) <= 1e-12) {
        return std::nullopt;
    }
    const core::Vec2 delta = second.start - first.start;
    const double t = ((delta.x * b.y) - (delta.y * b.x)) / cross;
    return first.start + (a * t);
}

void addDimension(CommandContext& context, core::Dimension dimension) {
    std::vector<core::Entity> graphics;
    graphics.reserve(dimension.graphics.size());
    for (const core::EntityRecord& graphic : dimension.graphics) {
        graphics.push_back(graphic.entity);
    }
    const core::EntityId id = context.editor.add(std::move(dimension));
    for (core::Entity& graphic : graphics) {
        const core::EntityId graphicId = context.editor.add(std::move(graphic));
        (void)context.editor.remove(graphicId);
    }
    const auto& added = std::get<core::Dimension>(context.editor.document().find(id)->entity);
    const core::DimVariables variables =
        core::dimensionVariables(context.editor.document().dimStyles(), added.presentation.style,
                                 added.presentation.overrides);
    const core::DimensionKind messageKind = added.definition.kind == core::DimensionKind::Angular
                                                ? core::DimensionKind::Angular
                                                : core::DimensionKind::Linear;
    const double messageMeasurement = added.definition.kind == core::DimensionKind::Angular
                                          ? core::radiansToDegrees(added.value.measurement)
                                          : added.value.measurement;
    std::string message = core::dimensionText(variables, messageKind, messageMeasurement, {});
    if (message.ends_with("%%d")) {
        message.resize(message.size() - 3);
    }
    context.message("Dimension text = " + message);
}

}  // namespace

CommandDescription DimangularCommand::description() {
    return {"DIMANGULAR",
            {"DIMANG"},
            "Creates an angular dimension.",
            false,
            {{"first-line",
              "Select arc, circle, line, or <specify vertex>:",
              ValueType::Object,
              {},
              true},
             {"second-line", "Select second line:", ValueType::Object, {}, false},
             {"location",
              "Specify dimension arc line location or [Text/Angle/Quadrant]:",
              ValueType::Point,
              {{"Text", "T"}, {"Angle", "A"}, {"Quadrant", "Q"}},
              false},
             {"text", "Enter dimension text <{default}>:", ValueType::Text, {}, true},
             {"text-angle", "Specify angle of dimension text:", ValueType::Angle, {}, true},
             {"quadrant", "Specify angle quadrant:", ValueType::Point, {}, false}}};
}

void DimangularCommand::start(CommandContext& context) {
    m_stage = Stage::FirstLine;
    m_pointer.reset();
    m_quadrantPoint.reset();
    m_textOverride.clear();
    m_textRotation.reset();
    m_preview.clear();
    m_document = &context.editor.document();
    context.editor.beginTransaction("Dimangular");
}

Status DimangularCommand::handle(CommandContext& context, const Input& input) {
    const auto* object = std::get_if<ObjectInput>(&input);
    const auto* point = std::get_if<PointInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    if (m_stage == Stage::FirstLine) {
        if (object == nullptr || !selectLine(context, *object, true)) {
            context.message("Object selected is not a line.");
        }
    } else if (m_stage == Stage::SecondLine && object != nullptr) {
        selectLine(context, *object, false);
    } else if (m_stage == Stage::Location) {
        if (point != nullptr) {
            return finish(context, point->point);
        }
        if (keyword != nullptr) {
            if (keyword->keyword == "Text") {
                m_stage = Stage::Text;
            } else if (keyword->keyword == "Angle") {
                m_stage = Stage::Angle;
            } else if (keyword->keyword == "Quadrant") {
                m_stage = Stage::Quadrant;
            }
        }
    } else if ((m_stage == Stage::Text && detail::acceptDimensionText(input, m_textOverride)) ||
               (m_stage == Stage::Angle &&
                detail::acceptDimensionAngle(input, m_first.start, m_textRotation))) {
        m_stage = Stage::Location;
    } else if (m_stage == Stage::Quadrant && point != nullptr) {
        m_quadrantPoint = point->point;
        m_stage = Stage::Location;
    }
    updatePreview();
    return Status::Continue;
}

void DimangularCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt DimangularCommand::prompt() const {
    switch (m_stage) {
        case Stage::FirstLine:
            return {"Select arc, circle, line, or <specify vertex>:", {}, PromptKind::Object};
        case Stage::SecondLine:
            return {"Select second line:", {}, PromptKind::Object};
        case Stage::Location:
            return {"Specify dimension arc line location or [Text/Angle/Quadrant]:",
                    {{"Text", "T"}, {"Angle", "A"}, {"Quadrant", "Q"}}};
        case Stage::Text:
            return {"Enter dimension text <>:", {}, PromptKind::Text};
        case Stage::Angle:
            return {"Specify angle of dimension text:", {}, PromptKind::Angle};
        case Stage::Quadrant:
            return {"Specify angle quadrant:", {}};
    }
    return {};
}

std::span<const core::Entity> DimangularCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> DimangularCommand::basePoint() const {
    if (m_stage == Stage::Angle) {
        return m_first.start;
    }
    return std::nullopt;
}

bool DimangularCommand::selectLine(CommandContext& context, const ObjectInput& object, bool first) {
    const core::EntityRecord* record = context.editor.document().find(object.id);
    const std::optional<core::Line> line =
        record == nullptr ? std::nullopt : pickedLine(record->entity, object.point);
    if (!line) {
        if (!first) {
            context.message("Object selected is not a line.");
        }
        return false;
    }
    if (first) {
        m_first = *line;
        m_stage = Stage::SecondLine;
        return true;
    }
    if (!lineIntersection(m_first, *line)) {
        context.message("Lines are parallel.");
        return false;
    }
    m_second = *line;
    m_stage = Stage::Location;
    return true;
}

core::Dimension DimangularCommand::makeDimension(const core::Vec2& location) const {
    core::Dimension dimension;
    dimension.definition.kind = core::DimensionKind::Angular;
    dimension.definition.dxfType = 2;
    dimension.definition.point13 = m_second.start;
    dimension.definition.point14 = m_second.end;
    dimension.definition.linePoint = m_first.end;
    dimension.definition.point15 = m_first.start;
    const core::Vec2 vertex = lineIntersection(m_first, m_second).value_or(core::Vec2{});
    dimension.definition.point16 = location;
    if (m_quadrantPoint) {
        core::Vec2 direction = *m_quadrantPoint - vertex;
        const double length = core::distance(vertex, *m_quadrantPoint);
        if (length > 0.0) {
            direction = direction * (core::distance(vertex, location) / length);
        }
        dimension.definition.point16 = vertex + direction;
    }
    const core::DimStyleTable& styles = m_document->dimStyles();
    dimension.presentation.style = styles.current();
    dimension.presentation.overrides = currentOverrides(styles);
    dimension.presentation.textOverride = m_textOverride;
    dimension.presentation.textRotation = m_textRotation;
    return dimension;
}

void DimangularCommand::updatePreview() {
    m_preview.clear();
    if (m_stage != Stage::Location || !m_pointer || m_document == nullptr) {
        return;
    }
    if (auto dimension = core::regenerated(*m_document, makeDimension(*m_pointer))) {
        m_preview.emplace_back(std::move(*dimension));
    }
}

Status DimangularCommand::finish(CommandContext& context, const core::Vec2& location) {
    const core::Vec2 vertex = lineIntersection(m_first, m_second).value_or(core::Vec2{});
    if (core::distance(location, vertex) == 0.0) {
        context.message("Points coincide.");
        return Status::Continue;
    }
    std::string reason;
    auto dimension = core::regenerated(*m_document, makeDimension(location), &reason);
    if (!dimension) {
        context.message(reason);
        return Status::Continue;
    }
    addDimension(context, std::move(*dimension));
    context.lastPoint = location;
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

CommandDescription RadialDimensionCommand::description(Kind kind) {
    const bool radius = kind == Kind::Radius;
    return {radius ? "DIMRADIUS" : "DIMDIAMETER",
            {radius ? "DIMRAD" : "DIMDIA"},
            radius ? "Creates a radius dimension." : "Creates a diameter dimension.",
            false,
            {{"curve", "Select arc or circle:", ValueType::Object, {}, false},
             {"location",
              "Specify dimension line location or [Text/Angle]:",
              ValueType::Point,
              {{"Text", "T"}, {"Angle", "A"}},
              false},
             {"text", "Enter dimension text <{default}>:", ValueType::Text, {}, true},
             {"text-angle", "Specify angle of dimension text:", ValueType::Angle, {}, true}}};
}

void RadialDimensionCommand::start(CommandContext& context) {
    m_stage = Stage::Curve;
    m_pointer.reset();
    m_textOverride.clear();
    m_textRotation.reset();
    m_preview.clear();
    m_document = &context.editor.document();
    context.editor.beginTransaction(m_kind == Kind::Radius ? "Dimradius" : "Dimdiameter");
}

Status RadialDimensionCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    if (m_stage == Stage::Curve) {
        if (const auto* object = std::get_if<ObjectInput>(&input)) {
            selectCurve(context, *object);
        }
    } else if (m_stage == Stage::Location) {
        if (point != nullptr) {
            return finish(context, point->point);
        }
        if (keyword != nullptr) {
            m_stage = keyword->keyword == "Text" ? Stage::Text : Stage::Angle;
        }
    } else if ((m_stage == Stage::Text && detail::acceptDimensionText(input, m_textOverride)) ||
               (m_stage == Stage::Angle &&
                detail::acceptDimensionAngle(input, m_center, m_textRotation))) {
        m_stage = Stage::Location;
    }
    updatePreview();
    return Status::Continue;
}

void RadialDimensionCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt RadialDimensionCommand::prompt() const {
    switch (m_stage) {
        case Stage::Curve:
            return {"Select arc or circle:", {}, PromptKind::Object};
        case Stage::Location:
            return {"Specify dimension line location or [Text/Angle]:",
                    {{"Text", "T"}, {"Angle", "A"}}};
        case Stage::Text: {
            const core::DimStyleTable& styles = m_document->dimStyles();
            const core::DimVariables variables =
                core::dimensionVariables(styles, styles.current(), currentOverrides(styles));
            const auto kind = m_kind == Kind::Radius ? core::DimensionKind::Radius
                                                     : core::DimensionKind::Diameter;
            const double measurement = m_kind == Kind::Radius ? m_radius : 2.0 * m_radius;
            return {"Enter dimension text <" +
                        core::dimensionText(variables, kind, measurement, {}) + ">:",
                    {},
                    PromptKind::Text};
        }
        case Stage::Angle:
            return {"Specify angle of dimension text:", {}, PromptKind::Angle};
    }
    return {};
}

std::span<const core::Entity> RadialDimensionCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> RadialDimensionCommand::basePoint() const {
    return m_stage == Stage::Location || m_stage == Stage::Angle ? std::optional{m_center}
                                                                 : std::nullopt;
}

bool RadialDimensionCommand::selectCurve(CommandContext& context, const ObjectInput& object) {
    const core::EntityRecord* record = context.editor.document().find(object.id);
    if (record == nullptr) {
        return false;
    }
    if (const auto* circle = std::get_if<core::Circle>(&record->entity)) {
        m_center = circle->center;
        m_radius = circle->radius;
    } else if (const auto* arc = std::get_if<core::Arc>(&record->entity)) {
        m_center = arc->center;
        m_radius = arc->radius;
    } else if (const auto* polyline = std::get_if<core::Polyline>(&record->entity)) {
        double best = std::numeric_limits<double>::infinity();
        std::optional<core::PolylineSegment> chosen;
        for (std::size_t index = 0; index < polyline->segmentCount(); ++index) {
            const core::PolylineSegment segment = polyline->segment(index);
            const double distance = core::distance(segment, object.point);
            if (distance < best) {
                best = distance;
                chosen = segment;
            }
        }
        if (!chosen || !chosen->isArc()) {
            return false;
        }
        const core::ArcGeometry geometry =
            core::arcFromBulge(chosen->start, chosen->end, chosen->bulge);
        m_center = geometry.center;
        m_radius = geometry.radius;
    } else {
        return false;
    }
    m_stage = Stage::Location;
    return true;
}

core::Dimension RadialDimensionCommand::makeDimension(const core::Vec2& location) const {
    core::Dimension dimension;
    const bool radius = m_kind == Kind::Radius;
    dimension.definition.kind =
        radius ? core::DimensionKind::Radius : core::DimensionKind::Diameter;
    dimension.definition.dxfType = radius ? 4 : 3;
    const core::Vec2 delta = location - m_center;
    const double length = core::distance(location, m_center);
    const core::Vec2 onCurve = m_center + (delta * (m_radius / length));
    dimension.definition.point15 = onCurve;
    dimension.definition.linePoint = radius ? m_center : m_center - (onCurve - m_center);
    const core::DimStyleTable& styles = m_document->dimStyles();
    dimension.presentation.style = styles.current();
    dimension.presentation.overrides = currentOverrides(styles);
    dimension.presentation.textPoint = location;
    dimension.presentation.userTextPosition = true;
    dimension.presentation.textOverride = m_textOverride;
    dimension.presentation.textRotation = m_textRotation;
    return dimension;
}

void RadialDimensionCommand::updatePreview() {
    m_preview.clear();
    if (m_stage != Stage::Location || !m_pointer || m_document == nullptr ||
        core::distance(*m_pointer, m_center) == 0.0) {
        return;
    }
    if (auto dimension = core::regenerated(*m_document, makeDimension(*m_pointer))) {
        m_preview.emplace_back(std::move(*dimension));
    }
}

Status RadialDimensionCommand::finish(CommandContext& context, const core::Vec2& location) {
    if (core::distance(location, m_center) == 0.0) {
        context.message("Points coincide.");
        return Status::Continue;
    }
    std::string reason;
    auto dimension = core::regenerated(*m_document, makeDimension(location), &reason);
    if (!dimension) {
        context.message(reason);
        return Status::Continue;
    }
    addDimension(context, std::move(*dimension));
    context.lastPoint = location;
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

}  // namespace tg::cmd
