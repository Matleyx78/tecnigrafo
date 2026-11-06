// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/fillet_command.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/fillet.hpp"

#include <cmath>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

const char* failureMessage(core::CornerOutcome outcome) {
    switch (outcome) {
        case core::CornerOutcome::AmbiguousPick:
            return "Pick a point that is not on the other object.";
        case core::CornerOutcome::RadiusTooLarge:
            return "Radius is too large.";
        case core::CornerOutcome::OutOfDomain:
            return "Result is beyond the drawing limits.";
        default:
            return "Cannot fillet those objects.";
    }
}

void applyKeptPart(edit::Editor& editor, core::EntityId id, std::optional<core::Entity>& part) {
    if (part) {
        editor.replace(id, std::move(*part));
    } else {
        editor.remove(id);
    }
}

}  // namespace

void FilletCommand::start(CommandContext& context) {
    m_stage = Stage::First;
    m_radius = context.defaults.filletRadius;
    m_trim = context.defaults.trimMode;
    m_multiple = false;
    m_steps.clear();
    context.editor.beginTransaction("Fillet");
    context.message(std::string{"Current settings: Mode = "} + (m_trim ? "TRIM" : "NO TRIM") +
                    ", Radius = " + formatLength(m_radius));
}

Status FilletCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::Trim) {
            m_trim = keyword->keyword == "Trim";
            context.defaults.trimMode = m_trim;
            return resumeAfterSetting(context);
        }
        choose(*keyword, context);
    } else if (const auto* object = std::get_if<ObjectInput>(&input)) {
        if (m_stage == Stage::First) {
            m_first = *object;
            m_stage = Stage::Second;
        } else if (m_stage == Stage::Second) {
            return filletObjects(context, *object);
        } else if (m_stage == Stage::Polyline) {
            return filletPolyline(context, object->id);
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        if (m_stage == Stage::Radius) {
            return setRadius(context, m_radius);
        }
        if (m_stage == Stage::Trim) {
            return resumeAfterSetting(context);
        }
        context.editor.commit();
        return Status::Finished;
    } else if (m_stage == Stage::Radius || m_stage == Stage::RadiusPoint) {
        if (const auto* value = std::get_if<ValueInput>(&input)) {
            return setRadius(context, value->value);
        }
        if (const auto* point = std::get_if<PointInput>(&input)) {
            context.lastPoint = point->point;
            if (m_stage == Stage::RadiusPoint) {
                return setRadius(context, core::distance(m_radiusBase, point->point));
            }
            m_radiusBase = point->point;
            m_stage = Stage::RadiusPoint;
        }
    }
    return Status::Continue;
}

void FilletCommand::cancel(CommandContext& context) {
    // Esc keeps the fillets already completed (docs/adr/0035, point 21).
    context.editor.commit();
}

Prompt FilletCommand::prompt() const {
    switch (m_stage) {
        case Stage::First:
            return {"Select first object or [Undo/Polyline/Radius/Trim/Multiple]:",
                    {{"Undo", "U"},
                     {"Polyline", "P"},
                     {"Radius", "R"},
                     {"Trim", "T"},
                     {"Multiple", "M"}},
                    PromptKind::Object};
        case Stage::Second:
            return {"Select second object or shift-select to apply corner or [Radius]:",
                    {{"Radius", "R"}},
                    PromptKind::Object};
        case Stage::Radius:
            return {"Specify fillet radius <" + formatLength(m_radius) + ">:",
                    {},
                    PromptKind::Distance};
        case Stage::RadiusPoint:
            return {"Specify second point:", {}};
        case Stage::Trim:
            return {m_trim ? "Enter Trim mode option [Trim/No trim] <Trim>:"
                           : "Enter Trim mode option [Trim/No trim] <No trim>:",
                    {{"Trim", "T"}, {"No trim", "N"}}};
        case Stage::Polyline:
            return {"Select 2D polyline or [Radius]:", {{"Radius", "R"}}, PromptKind::Object};
    }
    return {};
}

std::optional<core::Vec2> FilletCommand::basePoint() const {
    return m_stage == Stage::RadiusPoint ? std::optional{m_radiusBase} : std::nullopt;
}

void FilletCommand::choose(const KeywordInput& keyword, CommandContext& context) {
    if (keyword.keyword == "Radius") {
        m_stage = Stage::Radius;
    } else if (keyword.keyword == "Polyline") {
        m_stage = Stage::Polyline;
    } else if (keyword.keyword == "Trim") {
        m_stage = Stage::Trim;
    } else if (keyword.keyword == "Multiple") {
        m_multiple = true;
    } else if (keyword.keyword == "Undo" && !m_steps.empty()) {
        context.editor.rollbackTo(m_steps.back());
        m_steps.pop_back();
    }
}

Status FilletCommand::setRadius(CommandContext& context, double radius) {
    if (radius < 0.0 || !std::isfinite(radius)) {
        context.message("Value must be positive.");
        m_stage = Stage::Radius;
        return Status::Continue;
    }
    m_radius = radius;
    context.defaults.filletRadius = radius;
    return resumeAfterSetting(context);
}

Status FilletCommand::resumeAfterSetting(CommandContext& context) {
    if (context.scriptSequences) {
        context.editor.commit();
        return Status::Finished;
    }
    m_stage = Stage::First;
    return Status::Continue;
}

Status FilletCommand::filletObjects(CommandContext& context, const ObjectInput& object) {
    edit::Editor& editor = context.editor;
    const core::Document& document = editor.document();
    const core::EntityRecord* first = document.find(m_first.id);
    const core::EntityRecord* second = document.find(object.id);
    if (first == nullptr || second == nullptr) {
        context.message(failureMessage(core::CornerOutcome::NotAllowed));
        return finishAttempt(context);
    }

    const bool same = m_first.id == object.id;
    const double radius = object.shift ? 0.0 : m_radius;
    const auto rule = core::ToleranceRule::of(document);
    core::CornerResult result;
    result.outcome = core::CornerOutcome::NotAllowed;
    if (!same) {
        result = core::fillet(first->entity, m_first.point, second->entity, object.point, radius,
                              m_trim, rule);
    } else if (const auto* polyline = std::get_if<core::Polyline>(&first->entity)) {
        result = core::filletSegments(*polyline, m_first.point, object.point, radius, m_trim, rule);
    }
    if (result.outcome != core::CornerOutcome::Done) {
        context.message(failureMessage(result.outcome));
        return finishAttempt(context);
    }

    const auto before = editor.savepoint();
    if (result.added) {
        // Take the properties before trimming can erase either source.
        if (first->properties == second->properties) {
            editor.addCopy(m_first.id, std::move(*result.added));
        } else {
            editor.add(std::move(*result.added));
        }
    }
    if (m_trim) {
        applyKeptPart(editor, m_first.id, result.first);
        if (!same) {
            applyKeptPart(editor, object.id, result.second);
        }
    }
    if (editor.savepoint().changes != before.changes) {
        m_steps.push_back(before);
    }
    return finishAttempt(context);
}

Status FilletCommand::filletPolyline(CommandContext& context, core::EntityId id) {
    edit::Editor& editor = context.editor;
    const core::Document& document = editor.document();
    const core::EntityRecord* source = document.find(id);
    const auto* polyline =
        source == nullptr ? nullptr : std::get_if<core::Polyline>(&source->entity);
    if (polyline == nullptr) {
        context.message(failureMessage(core::CornerOutcome::NotAllowed));
        return finishAttempt(context);
    }

    auto result =
        core::filletPolyline(*polyline, m_radius, m_trim, core::ToleranceRule::of(document));
    const auto before = editor.savepoint();
    if (m_trim) {
        editor.replace(id, std::move(result.polyline));
    }
    for (auto& added : result.added) {
        editor.addCopy(id, std::move(added));
    }
    if (editor.savepoint().changes != before.changes) {
        m_steps.push_back(before);
    }
    context.message(std::to_string(result.done) + " lines were filleted");
    if (result.tooShort != 0) {
        context.message(std::to_string(result.tooShort) + " were too short");
    }
    return finishAttempt(context);
}

Status FilletCommand::finishAttempt(CommandContext& context) {
    if (m_multiple) {
        m_stage = Stage::First;
        return Status::Continue;
    }
    context.editor.commit();
    return Status::Finished;
}

}  // namespace tg::cmd
