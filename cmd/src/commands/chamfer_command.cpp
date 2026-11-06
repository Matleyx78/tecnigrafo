// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/chamfer_command.hpp"

#include "tg/cmd/coordinate_input.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"

#include <cmath>
#include <utility>

namespace tg::cmd {
namespace {

const char* failureMessage(core::CornerOutcome outcome) {
    switch (outcome) {
        case core::CornerOutcome::AmbiguousPick:
            return "Pick a point that is not on the other object.";
        case core::CornerOutcome::DistanceTooLarge:
            return "Chamfer distance too large.";
        case core::CornerOutcome::OutOfDomain:
            return "Result is beyond the drawing limits.";
        case core::CornerOutcome::None:
        case core::CornerOutcome::InfinitelyMany:
            return "Lines are parallel.";
        default:
            return "Cannot chamfer those objects.";
    }
}

void applyKeptPart(edit::Editor& editor, core::EntityId id, std::optional<core::Entity>& part) {
    if (part) {
        editor.replace(id, std::move(*part));
    } else {
        editor.remove(id);
    }
}

constexpr double kRadiansPerDegree = core::kPi / 180.0;

}  // namespace

void ChamferCommand::start(CommandContext& context) {
    m_stage = Stage::First;
    m_returnStage = Stage::First;
    m_distances = {context.defaults.chamferFirstDistance, context.defaults.chamferSecondDistance};
    m_angle = {context.defaults.chamferLength, context.defaults.chamferAngle};
    m_useAngle = context.defaults.chamferUseAngle;
    m_trim = context.defaults.trimMode;
    m_multiple = false;
    m_steps.clear();
    context.editor.beginTransaction("Chamfer");
    const std::string mode = m_trim ? "(TRIM mode)" : "(NOTRIM mode)";
    const std::string first = m_useAngle ? "Length = " + formatLength(m_angle.length)
                                         : "Dist1 = " + formatLength(m_distances.first);
    const std::string second = m_useAngle ? "Angle = " + formatAngle(m_angle.angle)
                                          : "Dist2 = " + formatLength(m_distances.second);
    context.message(mode + " Current chamfer " + first + ", " + second);
}

Status ChamferCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::Trim) {
            m_trim = keyword->keyword == "Trim";
            context.defaults.trimMode = m_trim;
            return resumeAfterSetting(context);
        }
        if (m_stage == Stage::Method) {
            m_useAngle = keyword->keyword == "Angle";
            context.defaults.chamferUseAngle = m_useAngle;
            return resumeAfterSetting(context);
        }
        choose(context, *keyword);
    } else if (const auto* object = std::get_if<ObjectInput>(&input)) {
        switch (m_stage) {
            case Stage::First:
                m_first = *object;
                m_stage = Stage::Second;
                break;
            case Stage::Second:
                return chamferObjects(context, *object);
            case Stage::Polyline:
                return chamferPolyline(context, object->id);
            default:
                break;
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        if (expectsNumber()) {
            return setNumber(context, defaultNumber());
        }
        if (m_stage == Stage::Trim || m_stage == Stage::Method) {
            return resumeAfterSetting(context);
        }
        context.editor.commit();
        return Status::Finished;
    } else if (expectsNumber()) {
        if (const auto* number = std::get_if<ValueInput>(&input)) {
            return setNumber(context, number->value);
        }
        if (const auto* text = std::get_if<TextInput>(&input)) {
            const auto number = parseCoordinate(text->text);
            if (number && number->kind == PointSpec::Kind::Distance) {
                return setNumber(context, number->a);
            }
        }
        context.message("Requires numeric distance.");
    }
    return Status::Continue;
}

void ChamferCommand::cancel(CommandContext& context) {
    context.editor.commit();
}

Prompt ChamferCommand::prompt() const {
    switch (m_stage) {
        case Stage::First:
            return {"Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]:",
                    {{"Undo", "U"},
                     {"Polyline", "P"},
                     {"Distance", "D"},
                     {"Angle", "A"},
                     {"Trim", "T"},
                     {"mEthod", "E"},
                     {"Multiple", "M"}},
                    PromptKind::Object};
        case Stage::Second:
            return {
                "Select second line or shift-select to apply corner or [Distance/Angle/Method]:",
                {{"Distance", "D"}, {"Angle", "A"}, {"Method", "M"}},
                PromptKind::Object};
        case Stage::Polyline:
            return {"Select 2D polyline or [Distance/Angle/Method]:",
                    {{"Distance", "D"}, {"Angle", "A"}, {"Method", "M"}},
                    PromptKind::Object};
        case Stage::FirstDistance:
            return {"Specify first chamfer distance <" + formatLength(m_distances.first) + ">:",
                    {},
                    PromptKind::Text};
        case Stage::SecondDistance:
            return {"Specify second chamfer distance <" + formatLength(m_pending) + ">:",
                    {},
                    PromptKind::Text};
        case Stage::Length: {
            const auto text =
                "Specify chamfer length on the first line <" + formatLength(m_angle.length) + ">:";
            return {text, {}, PromptKind::Text};
        }
        case Stage::Angle: {
            // CHAMFER uses the distance diagnostic for all four numeric prompts.
            const auto text =
                "Specify chamfer angle from the first line <" + formatAngle(m_angle.angle) + ">:";
            return {text, {}, PromptKind::Text};
        }
        case Stage::Trim:
            return {m_trim ? "Enter Trim mode option [Trim/No trim] <Trim>:"
                           : "Enter Trim mode option [Trim/No trim] <No trim>:",
                    {{"Trim", "T"}, {"No trim", "N"}}};
        case Stage::Method:
            return {m_useAngle ? "Enter trim method [Distance/Angle] <Angle>:"
                               : "Enter trim method [Distance/Angle] <Distance>:",
                    {{"Distance", "D"}, {"Angle", "A"}}};
    }
    return {};
}

void ChamferCommand::choose(CommandContext& context, const KeywordInput& keyword) {
    const auto& choice = keyword.keyword;
    m_returnStage = m_stage;
    if (choice == "Distance") {
        m_stage = Stage::FirstDistance;
    } else if (choice == "Angle") {
        m_stage = Stage::Length;
    } else if (choice == "Trim") {
        m_stage = Stage::Trim;
    } else if (choice == "Method" || choice == "mEthod") {
        m_stage = Stage::Method;
    } else if (choice == "Polyline") {
        m_stage = Stage::Polyline;
    } else if (choice == "Multiple") {
        m_multiple = true;
    } else if (choice == "Undo" && !m_steps.empty()) {
        context.editor.rollbackTo(m_steps.back());
        m_steps.pop_back();
    }
}

bool ChamferCommand::expectsNumber() const {
    return m_stage == Stage::FirstDistance || m_stage == Stage::SecondDistance ||
           m_stage == Stage::Length || m_stage == Stage::Angle;
}

double ChamferCommand::defaultNumber() const {
    switch (m_stage) {
        case Stage::FirstDistance:
            return m_distances.first;
        case Stage::Length:
            return m_angle.length;
        case Stage::Angle:
            return m_angle.angle / kRadiansPerDegree;
        default:
            return m_pending;
    }
}

Status ChamferCommand::setNumber(CommandContext& context, double number) {
    if (number < 0.0 || !std::isfinite(number)) {
        context.message("Value must be positive.");
        return Status::Continue;
    }
    if (m_stage == Stage::FirstDistance || m_stage == Stage::Length) {
        m_pending = number;
        m_stage = m_stage == Stage::FirstDistance ? Stage::SecondDistance : Stage::Angle;
        return Status::Continue;
    }
    m_useAngle = m_stage == Stage::Angle;
    if (m_useAngle) {
        m_angle = {m_pending, number * kRadiansPerDegree};
        context.defaults.chamferLength = m_angle.length;
        context.defaults.chamferAngle = m_angle.angle;
    } else {
        m_distances = {m_pending, number};
        context.defaults.chamferFirstDistance = m_distances.first;
        context.defaults.chamferSecondDistance = m_distances.second;
    }
    context.defaults.chamferUseAngle = m_useAngle;
    return resumeAfterSetting(context);
}

Status ChamferCommand::resumeAfterSetting(CommandContext& context) {
    if (context.scriptSequences) {
        context.editor.commit();
        return Status::Finished;
    }
    m_stage = m_returnStage;
    return Status::Continue;
}

core::ChamferSpec ChamferCommand::spec(bool shift) const {
    if (shift) {
        return core::ChamferDistances{};
    }
    return m_useAngle ? core::ChamferSpec{m_angle} : core::ChamferSpec{m_distances};
}

Status ChamferCommand::chamferObjects(CommandContext& context, const ObjectInput& object) {
    edit::Editor& editor = context.editor;
    const core::Document& document = editor.document();
    const core::EntityRecord* first = document.find(m_first.id);
    const core::EntityRecord* second = document.find(object.id);
    const bool same = m_first.id == object.id;
    const auto rule = core::ToleranceRule::of(document);
    const auto settings = spec(object.shift);
    core::CornerResult result;
    result.outcome = core::CornerOutcome::NotAllowed;
    if (first != nullptr && second != nullptr) {
        if (!same) {
            result = core::chamfer(first->entity, m_first.point, second->entity, object.point,
                                   settings, m_trim, rule);
        } else if (const auto* polyline = std::get_if<core::Polyline>(&first->entity)) {
            result = core::chamferSegments(*polyline, m_first.point, object.point, settings, m_trim,
                                           rule);
        }
    }
    if (result.outcome != core::CornerOutcome::Done) {
        context.message(failureMessage(result.outcome));
        return finishAttempt(context);
    }

    const auto before = editor.savepoint();
    if (result.added && first != nullptr && second != nullptr) {
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

Status ChamferCommand::chamferPolyline(CommandContext& context, core::EntityId id) {
    edit::Editor& editor = context.editor;
    const core::Document& document = editor.document();
    const core::EntityRecord* source = document.find(id);
    const auto* polyline =
        source == nullptr ? nullptr : std::get_if<core::Polyline>(&source->entity);
    if (polyline == nullptr) {
        context.message(failureMessage(core::CornerOutcome::NotAllowed));
        return finishAttempt(context);
    }

    const auto rule = core::ToleranceRule::of(document);
    auto result = core::chamferPolyline(*polyline, spec(), m_trim, rule);
    const auto before = editor.savepoint();
    if (m_trim && result.done != 0) {
        editor.replace(id, std::move(result.polyline));
    }
    for (auto& added : result.added) {
        editor.addCopy(id, std::move(added));
    }
    if (editor.savepoint().changes != before.changes) {
        m_steps.push_back(before);
    }
    context.message(std::to_string(result.done) + " lines were chamfered");
    if (result.tooShort != 0) {
        context.message(std::to_string(result.tooShort) + " were too short");
    }
    return finishAttempt(context);
}

Status ChamferCommand::finishAttempt(CommandContext& context) {
    if (m_multiple) {
        m_stage = Stage::First;
        return Status::Continue;
    }
    context.editor.commit();
    return Status::Finished;
}

}  // namespace tg::cmd
