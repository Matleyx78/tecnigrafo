// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/offset_command.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/offset.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/text.hpp"

#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr auto kCannotOffset = "Cannot offset that object.";

}  // namespace

void OffsetCommand::start(CommandContext& context) {
    m_stage = Stage::Distance;
    m_distance = context.defaults.offsetDistance;
    m_erase = context.defaults.offsetEraseSource;
    m_currentLayer = context.defaults.offsetCurrentLayer;
    m_multiple = false;
    m_steps.clear();
    context.editor.beginTransaction("Offset");
    context.message(std::string{"Current settings: Erase source="} + (m_erase ? "Yes" : "No") +
                    "  Layer=" + (m_currentLayer ? "Current" : "Source") + "  OFFSETGAPTYPE=0");
}

Status OffsetCommand::handle(CommandContext& context, const Input& input) {
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const auto* point = std::get_if<PointInput>(&input);
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Distance:
            handleDistance(context, input);
            break;
        case Stage::SecondPoint:
            if (point != nullptr) {
                context.lastPoint = point->point;
                useDistance(context, core::distance(m_firstPoint, point->point));
            } else if (confirmed) {
                m_stage = Stage::Distance;
            }
            break;
        case Stage::Erase:
            if (keyword != nullptr) {
                m_erase = keyword->keyword == "Yes";
                context.defaults.offsetEraseSource = m_erase;
            }
            if (keyword != nullptr || confirmed) {
                m_stage = Stage::Distance;
            }
            break;
        case Stage::Layer:
            if (keyword != nullptr) {
                m_currentLayer = keyword->keyword == "Current";
                context.defaults.offsetCurrentLayer = m_currentLayer;
            }
            if (keyword != nullptr || confirmed) {
                m_stage = Stage::Distance;
            }
            break;
        case Stage::Object:
        case Stage::Side:
            if (confirmed && m_multiple) {
                m_multiple = false;
                m_stage = Stage::Object;
            } else if (confirmed || (keyword != nullptr && keyword->keyword == "Exit")) {
                context.editor.commit();
                return Status::Finished;
            } else if (keyword != nullptr && keyword->keyword == "Undo") {
                undo(context);
            } else if (m_stage == Stage::Object) {
                if (const auto* object = std::get_if<ObjectInput>(&input)) {
                    pickObject(context, object->id);
                }
            } else if (keyword != nullptr && keyword->keyword == "Multiple") {
                m_multiple = true;
            } else if (point != nullptr) {
                context.lastPoint = point->point;
                offset(context, point->point);
            }
            break;
    }
    return Status::Continue;
}

void OffsetCommand::cancel(CommandContext& context) {
    // Esc keeps the offsets already made (docs/adr/0035, point 21).
    context.editor.commit();
}

Prompt OffsetCommand::prompt() const {
    switch (m_stage) {
        case Stage::Distance:
            return {"Specify offset distance or [Through/Erase/Layer] <" +
                        (m_distance ? formatLength(*m_distance) : "Through") + ">:",
                    {{"Through", "T"}, {"Erase", "E"}, {"Layer", "L"}},
                    PromptKind::Distance};
        case Stage::SecondPoint:
            return {"Specify second point:", {}};
        case Stage::Erase:
            return {std::string{"Erase source object after offsetting? [Yes/No] <"} +
                        (m_erase ? "Yes" : "No") + ">:",
                    {{"Yes", "Y"}, {"No", "N"}}};
        case Stage::Layer:
            return {std::string{"Enter layer option for offset objects [Current/Source] <"} +
                        (m_currentLayer ? "Current" : "Source") + ">:",
                    {{"Current", "C"}, {"Source", "S"}}};
        case Stage::Object:
            return {"Select object to offset or [Exit/Undo] <Exit>:",
                    {{"Exit", "E"}, {"Undo", "U"}},
                    PromptKind::Object};
        case Stage::Side:
            if (m_multiple) {
                return {m_distance ? "Specify point on side to offset or [Exit/Undo] <Exit>:"
                                   : "Specify through point or [Exit/Undo] <Exit>:",
                        {{"Exit", "E"}, {"Undo", "U"}}};
            }
            return {m_distance ? "Specify point on side to offset or [Exit/Multiple/Undo] <Exit>:"
                               : "Specify through point or [Exit/Multiple/Undo] <Exit>:",
                    {{"Exit", "E"}, {"Multiple", "M"}, {"Undo", "U"}}};
    }
    return {};
}

std::optional<core::Vec2> OffsetCommand::basePoint() const {
    return m_stage == Stage::SecondPoint ? std::optional{m_firstPoint} : std::nullopt;
}

void OffsetCommand::handleDistance(CommandContext& context, const Input& input) {
    if (const auto* value = std::get_if<ValueInput>(&input)) {
        useDistance(context, value->value);
    } else if (const auto* point = std::get_if<PointInput>(&input)) {
        // OFFSET measures a picked distance between two points (ADR 0035, point 9).
        m_firstPoint = point->point;
        context.lastPoint = point->point;
        m_stage = Stage::SecondPoint;
    } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "Erase") {
            m_stage = Stage::Erase;
        } else if (keyword->keyword == "Layer") {
            m_stage = Stage::Layer;
        } else if (keyword->keyword == "Through") {
            m_distance.reset();
            context.defaults.offsetDistance.reset();
            m_stage = Stage::Object;
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        m_stage = Stage::Object;
    }
}

void OffsetCommand::useDistance(CommandContext& context, double distance) {
    if (!(distance > 0.0) || !std::isfinite(distance)) {
        context.message("Value must be positive and nonzero.");
        return;
    }
    m_distance = distance;
    context.defaults.offsetDistance = distance;
    m_stage = Stage::Object;
}

void OffsetCommand::pickObject(CommandContext& context, core::EntityId id) {
    const core::EntityRecord* record = context.editor.document().find(id);
    if (record == nullptr || std::holds_alternative<core::Text>(record->entity)) {
        context.message(kCannotOffset);
        return;
    }
    m_source = id;
    m_stage = Stage::Side;
}

void OffsetCommand::offset(CommandContext& context, const core::Vec2& side) {
    edit::Editor& editor = context.editor;
    const core::Document& document = editor.document();
    const core::EntityRecord* source = document.find(m_source);
    if (!m_multiple) {
        m_stage = Stage::Object;
    }
    if (source == nullptr) {
        context.message(kCannotOffset);
        m_multiple = false;
        m_stage = Stage::Object;
        return;
    }

    const double distance = m_distance ? *m_distance : core::distance(source->entity, side);
    // A zero Through distance is NoSide. Never call the facade with a value
    // outside its precondition (an empty curve can have infinite distance).
    if (!(distance > 0.0) || !std::isfinite(distance)) {
        context.message(kCannotOffset);
        return;
    }
    core::OffsetResult result =
        core::offsetEntity(source->entity, distance, side, core::ToleranceRule::of(document));
    if (result.outcome != core::OffsetOutcome::Done || !result.entity) {
        context.message(kCannotOffset);
        return;
    }

    core::EntityProperties properties = source->properties;
    properties.layer = document.current().layer;
    const edit::Editor::Savepoint before = editor.savepoint();
    const auto copy = editor.addCopy(m_source, std::move(*result.entity));
    if (!copy) {
        context.message(kCannotOffset);
        return;
    }
    if (m_currentLayer) {
        editor.setProperties(*copy, properties);
    }
    if (m_erase) {
        editor.remove(m_source);
    }
    m_steps.push_back({before, m_source, *copy});
    if (m_multiple) {
        m_source = *copy;
    }
}

void OffsetCommand::undo(CommandContext& context) {
    if (m_steps.empty()) {
        return;
    }
    const Step step = m_steps.back();
    context.editor.rollbackTo(step.before);
    m_steps.pop_back();
    if (m_stage == Stage::Side && m_source == step.copy) {
        if (m_multiple) {
            m_source = step.source;
        } else {
            m_stage = Stage::Object;
        }
    }
}

}  // namespace tg::cmd
