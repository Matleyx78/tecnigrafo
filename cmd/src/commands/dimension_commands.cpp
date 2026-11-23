// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/dimension_commands.hpp"

#include "dimension_input.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/proximity.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <variant>

namespace tg::cmd {
namespace {

constexpr double kPointTolerance = 1e-9;

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

}  // namespace

CommandDescription DimensionCommand::description(Kind kind) {
    const bool linear = kind == Kind::Linear;
    // Built before the list: conditional temporaries inside it make GCC
    // report -Wmaybe-uninitialized.
    std::string locationPrompt = "Specify dimension line location or [Mtext/Text/Angle]:";
    std::vector<Keyword> locationKeywords{{"Mtext", "M"}, {"Text", "T"}, {"Angle", "A"}};
    if (linear) {
        locationPrompt =
            "Specify dimension line location or [Mtext/Text/Angle/Horizontal/Vertical/Rotated]:";
        locationKeywords.push_back({"Horizontal", "H"});
        locationKeywords.push_back({"Vertical", "V"});
        locationKeywords.push_back({"Rotated", "R"});
    }
    std::vector<PromptDescription> prompts{
        {"first-origin",
         "Specify first extension line origin or <select object>:",
         ValueType::Point,
         {},
         true},
        {"select-object", "Select object to dimension:", ValueType::Object, {}, false},
        {"second-origin", "Specify second extension line origin:", ValueType::Point, {}, false},
        {"location", std::move(locationPrompt), ValueType::Point, std::move(locationKeywords),
         false},
        {"location-axis",
         "Specify dimension line location or [Mtext/Text/Angle]:",
         ValueType::Point,
         {{"Mtext", "M"}, {"Text", "T"}, {"Angle", "A"}},
         false},
        {"text", "Enter dimension text <{default}>:", ValueType::Text, {}, true},
        {"text-angle", "Specify angle of dimension text:", ValueType::Angle, {}, true},
    };
    if (linear) {
        prompts.push_back(
            {"rotation", "Specify angle of dimension line <0>:", ValueType::Angle, {}, true});
    }
    if (linear) {
        return {"DIMLINEAR",
                {"DIMLIN", "DLI"},
                "Creates a linear dimension.",
                false,
                std::move(prompts)};
    }
    return {"DIMALIGNED",
            {"DIMALI", "DAL"},
            "Creates an aligned dimension.",
            false,
            std::move(prompts)};
}

void DimensionCommand::start(CommandContext& context) {
    m_stage = Stage::First;
    m_pointer.reset();
    m_location.reset();
    m_fixedRotation.reset();
    m_axisFixed = false;
    m_textOverride.clear();
    m_textRotation.reset();
    m_preview.clear();
    m_document = &context.editor.document();
    context.editor.beginTransaction(m_kind == Kind::Linear ? "DIMLINEAR" : "DIMALIGNED");
}

Status DimensionCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    if (m_stage == Stage::First) {
        if (point != nullptr) {
            m_first = point->point;
            context.lastPoint = m_first;
            m_stage = Stage::Second;
        } else if (confirm) {
            m_stage = Stage::Object;
        }
    } else if (m_stage == Stage::Object) {
        if (const auto* object = std::get_if<ObjectInput>(&input)) {
            selectObject(context, *object);
        }
    } else if (m_stage == Stage::Second && point != nullptr) {
        if (core::nearlyEqual(m_first, point->point, kPointTolerance)) {
            context.message("Points coincide.");
            return Status::Continue;
        }
        m_second = point->point;
        context.lastPoint = m_second;
        m_stage = Stage::Location;
    } else if (m_stage == Stage::Location) {
        if (point != nullptr) {
            return finish(context, point->point);
        }
        if (keyword != nullptr) {
            if (keyword->keyword == "Mtext" || keyword->keyword == "Text") {
                m_stage = Stage::Text;
            } else if (keyword->keyword == "Angle") {
                m_stage = Stage::Angle;
            } else if (keyword->keyword == "Horizontal") {
                m_fixedRotation = 0.0;
                m_axisFixed = true;
            } else if (keyword->keyword == "Vertical") {
                m_fixedRotation = core::kPi / 2.0;
                m_axisFixed = true;
            } else if (keyword->keyword == "Rotated") {
                m_stage = Stage::Rotation;
            }
        }
    } else if ((m_stage == Stage::Text && detail::acceptDimensionText(input, m_textOverride)) ||
               (m_stage == Stage::Angle &&
                detail::acceptDimensionAngle(input, m_first, m_textRotation))) {
        m_stage = Stage::Location;
    } else if (m_stage == Stage::Rotation &&
               (std::get_if<ValueInput>(&input) != nullptr || point != nullptr || confirm)) {
        if (const auto* value = std::get_if<ValueInput>(&input)) {
            m_fixedRotation = value->value;
        } else if (point != nullptr) {
            const core::Vec2 delta = point->point - m_first;
            m_fixedRotation = core::atan2(delta.y, delta.x);
        } else {
            m_fixedRotation = 0.0;
        }
        m_stage = Stage::Location;
    }
    updatePreview();
    return Status::Continue;
}

void DimensionCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt DimensionCommand::prompt() const {
    switch (m_stage) {
        case Stage::First:
            return {"Specify first extension line origin or <select object>:", {}};
        case Stage::Object:
            return {"Select object to dimension:", {}, PromptKind::Object};
        case Stage::Second:
            return {"Specify second extension line origin:", {}};
        case Stage::Location:
            if (m_kind == Kind::Aligned || m_axisFixed) {
                return {"Specify dimension line location or [Mtext/Text/Angle]:",
                        {{"Mtext", "M"}, {"Text", "T"}, {"Angle", "A"}}};
            }
            return {
                "Specify dimension line location or "
                "[Mtext/Text/Angle/Horizontal/Vertical/Rotated]:",
                {{"Mtext", "M"},
                 {"Text", "T"},
                 {"Angle", "A"},
                 {"Horizontal", "H"},
                 {"Vertical", "V"},
                 {"Rotated", "R"}}};
        case Stage::Text: {
            const core::DimStyleTable& styles = m_document->dimStyles();
            const auto overrides = currentOverrides(styles);
            const core::DimVariables variables =
                core::dimensionVariables(styles, styles.current(), overrides);
            const core::Vec2 delta = m_second - m_first;
            double measurement = core::distance(m_first, m_second);
            if (m_kind == Kind::Linear) {
                if (m_fixedRotation) {
                    const auto direction = core::sinCos(*m_fixedRotation);
                    measurement =
                        std::abs((delta.x * direction.cosine) + (delta.y * direction.sine));
                } else {
                    measurement = std::max(std::abs(delta.x), std::abs(delta.y));
                }
            }
            const std::string value = core::dimensionText(
                variables,
                m_kind == Kind::Linear ? core::DimensionKind::Linear : core::DimensionKind::Aligned,
                measurement, {});
            return {"Enter dimension text <" + value + ">:", {}, PromptKind::Text};
        }
        case Stage::Angle:
            return {"Specify angle of dimension text:", {}, PromptKind::Angle};
        case Stage::Rotation:
            return {"Specify angle of dimension line <0>:", {}, PromptKind::Angle};
    }
    return {};
}

std::span<const core::Entity> DimensionCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> DimensionCommand::basePoint() const {
    if (m_stage == Stage::Second || m_stage == Stage::Angle || m_stage == Stage::Rotation) {
        return m_first;
    }
    return std::nullopt;
}

std::optional<TextEditRequest> DimensionCommand::textEditing() const {
    if (m_stage != Stage::Text || !m_pointer || m_document == nullptr) {
        return std::nullopt;
    }
    const auto dimension = regeneratedAt(*m_pointer);
    if (!dimension) {
        return std::nullopt;
    }
    const auto graphic = std::ranges::find_if(dimension->graphics, [](const auto& record) {
        return std::holds_alternative<core::MText>(record.entity);
    });
    if (graphic == dimension->graphics.end()) {
        return std::nullopt;
    }
    const auto variables = core::dimensionVariables(
        m_document->dimStyles(), dimension->presentation.style, dimension->presentation.overrides);
    const std::string measurement = core::dimensionText(variables, dimension->definition.kind,
                                                        dimension->value.measurement, "<>");
    return TextEditRequest{std::nullopt, std::get<core::MText>(graphic->entity),
                           m_textOverride.empty() ? "<>" : m_textOverride, measurement};
}

core::Dimension DimensionCommand::makeDimension(const core::Vec2& location) const {
    core::Dimension dimension;
    dimension.definition.kind =
        m_kind == Kind::Linear ? core::DimensionKind::Linear : core::DimensionKind::Aligned;
    dimension.definition.dxfType = m_kind == Kind::Linear ? 0 : 1;
    dimension.definition.point13 = m_first;
    dimension.definition.point14 = m_second;
    if (m_kind == Kind::Aligned) {
        const core::Vec2 delta = m_second - m_first;
        dimension.definition.rotation = core::atan2(delta.y, delta.x);
    } else if (m_fixedRotation) {
        dimension.definition.rotation = *m_fixedRotation;
    } else {
        const core::Vec2 middle = (m_first + m_second) * 0.5;
        const double minX = std::min(m_first.x, m_second.x);
        const double maxX = std::max(m_first.x, m_second.x);
        const double minY = std::min(m_first.y, m_second.y);
        const double maxY = std::max(m_first.y, m_second.y);
        if (location.y < minY || location.y > maxY) {
            dimension.definition.rotation = 0.0;
        } else if (location.x < minX || location.x > maxX) {
            dimension.definition.rotation = core::kPi / 2.0;
        } else {
            dimension.definition.rotation =
                std::abs(location.y - middle.y) >= std::abs(location.x - middle.x)
                    ? 0.0
                    : core::kPi / 2.0;
        }
    }
    if (dimension.definition.rotation == 0.0) {
        dimension.definition.linePoint = {m_second.x, location.y};
    } else if (dimension.definition.rotation == core::kPi / 2.0) {
        dimension.definition.linePoint = {location.x, m_second.y};
    } else {
        const auto direction = core::sinCos(dimension.definition.rotation);
        const core::Vec2 along{direction.cosine, direction.sine};
        dimension.definition.linePoint = location + (along * core::dot(m_second - location, along));
    }
    const core::DimStyleTable& styles = m_document->dimStyles();
    dimension.presentation.style = styles.current();
    dimension.presentation.overrides = currentOverrides(styles);
    dimension.presentation.textOverride = m_textOverride;
    dimension.presentation.textRotation = m_textRotation;
    return dimension;
}

std::optional<core::Dimension> DimensionCommand::regeneratedAt(const core::Vec2& location,
                                                               std::string* reason) const {
    return core::regenerated(*m_document, makeDimension(location), reason);
}

void DimensionCommand::updatePreview() {
    m_preview.clear();
    if (m_stage != Stage::Location || !m_pointer || m_document == nullptr) {
        return;
    }
    if (auto dimension = regeneratedAt(*m_pointer)) {
        m_preview.emplace_back(std::move(*dimension));
    }
}

bool DimensionCommand::selectObject(CommandContext& context, const ObjectInput& object) {
    const core::EntityRecord* record = context.editor.document().find(object.id);
    if (record == nullptr) {
        return false;
    }
    if (const auto* line = std::get_if<core::Line>(&record->entity)) {
        m_first = line->start;
        m_second = line->end;
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
        if (!chosen || chosen->isArc()) {
            return false;
        }
        m_first = chosen->start;
        m_second = chosen->end;
    } else {
        return false;
    }
    if (core::nearlyEqual(m_first, m_second, kPointTolerance)) {
        return false;
    }
    context.lastPoint = m_second;
    m_stage = Stage::Location;
    return true;
}

Status DimensionCommand::finish(CommandContext& context, const core::Vec2& location) {
    std::string reason;
    auto dimension = regeneratedAt(location, &reason);
    if (!dimension) {
        context.message(reason);
        return Status::Continue;
    }
    std::vector<core::Entity> graphicEntities;
    graphicEntities.reserve(dimension->graphics.size());
    for (const core::EntityRecord& graphic : dimension->graphics) {
        graphicEntities.push_back(graphic.entity);
    }
    const core::EntityId dimensionId = context.editor.add(std::move(*dimension));
    // The anonymous dimension block consumes handles even though its records
    // keep handle 0 until the DXF writer assigns the block (PT23).
    for (core::Entity& graphic : graphicEntities) {
        const core::EntityId id = context.editor.add(std::move(graphic));
        (void)context.editor.remove(id);
    }
    context.lastPoint = location;
    const auto& added =
        std::get<core::Dimension>(context.editor.document().find(dimensionId)->entity);
    const core::DimVariables variables =
        core::dimensionVariables(context.editor.document().dimStyles(), added.presentation.style,
                                 added.presentation.overrides);
    context.message("Dimension text = " + core::dimensionText(variables, added.definition.kind,
                                                              added.value.measurement, {}));
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

}  // namespace tg::cmd
