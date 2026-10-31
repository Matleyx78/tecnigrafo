// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/rotate_scale_commands.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/similarity.hpp"

#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

// What AutoCAD writes for a scale factor or a length that is not positive.
constexpr std::string_view kNotPositive = "Value must be positive and nonzero.";

// The length that Enter takes at the reference and new length prompts of SCALE.
constexpr double kDefaultLength = 1.0;

/// The entities of the selection that still exist, in selection order.
std::vector<SelectedEntity> snapshot(const CommandContext& context,
                                     std::span<const core::EntityId> ids) {
    std::vector<SelectedEntity> result;
    result.reserve(ids.size());
    for (const core::EntityId id : ids) {
        if (const core::EntityRecord* record = context.editor.document().find(id)) {
            result.push_back({id, record->entity});
        }
    }
    return result;
}

/// The direction of `vector` from +X, in radians; 0 for the null vector.
double directionOf(const core::Vec2& vector) {
    return std::atan2(vector.y, vector.x);
}

/// Writes the transformed selection: in place, with the same identity and
/// data, or as copies that take the properties of their source
/// (docs/adr/0031, points 5 and 6).
void writeTransformed(CommandContext& context, const std::vector<SelectedEntity>& selected,
                      const core::Similarity& similarity, bool copy) {
    for (const SelectedEntity& item : selected) {
        core::Entity result = core::transformed(item.entity, similarity);
        if (copy) {
            context.editor.addCopy(item.id, std::move(result));
        } else {
            context.editor.replace(item.id, std::move(result));
        }
    }
}

void appendTransformed(const std::vector<SelectedEntity>& selected,
                       const core::Similarity& similarity, std::vector<core::Entity>& out) {
    for (const SelectedEntity& item : selected) {
        out.push_back(core::transformed(item.entity, similarity));
    }
}

}  // namespace

// RotateCommand

void RotateCommand::start(CommandContext& context) {
    m_stage = Stage::Selection;
    m_selected.clear();
    m_base = {};
    m_firstPoint = {};
    m_reference = 0.0;
    m_defaultRotation = context.defaults.rotation;
    m_copy = false;
    m_pointer.reset();
    m_preview.clear();
    // Two spaces after the colon and between the two settings, as in AutoCAD.
    context.message("Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0");
    context.editor.beginTransaction("Rotate");
}

Status RotateCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const auto* value = std::get_if<ValueInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Selection:
            if (const auto* selection = std::get_if<SelectionInput>(&input)) {
                m_selected = snapshot(context, selection->ids);
                if (m_selected.empty()) {
                    context.editor.rollback();
                    return Status::Finished;
                }
                m_stage = Stage::Base;
            }
            break;
        case Stage::Base:
            if (point != nullptr) {
                m_base = point->point;
                // The "@" of the next point is measured from the base point.
                context.lastPoint = point->point;
                m_stage = Stage::Angle;
            } else if (confirmed) {
                // No default: Enter ends the command without effects (docs/adr/0014).
                context.editor.rollback();
                return Status::Finished;
            }
            break;
        case Stage::Angle:
            if (value != nullptr) {
                return apply(context, value->value);
            }
            if (point != nullptr) {
                context.lastPoint = point->point;
                // A click gives the angle of the line from the base point.
                return apply(context, directionOf(point->point - m_base));
            }
            if (confirmed) {
                return apply(context, m_defaultRotation);
            }
            if (keyword != nullptr) {
                if (keyword->keyword == "Copy") {
                    m_copy = true;
                    context.message("Rotating a copy of the selected objects.");
                } else if (keyword->keyword == "Reference") {
                    m_stage = Stage::ReferenceAngle;
                }
            }
            break;
        case Stage::ReferenceAngle:
            if (value != nullptr) {
                m_reference = value->value;
                m_stage = Stage::NewAngle;
            } else if (point != nullptr) {
                context.lastPoint = point->point;
                m_firstPoint = point->point;
                m_stage = Stage::ReferenceSecond;
            } else if (confirmed) {
                m_reference = 0.0;
                m_stage = Stage::NewAngle;
            }
            break;
        case Stage::ReferenceSecond:
            if (point != nullptr) {
                context.lastPoint = point->point;
                m_reference = directionOf(point->point - m_firstPoint);
                m_stage = Stage::NewAngle;
            }
            break;
        case Stage::NewAngle:
            if (value != nullptr) {
                return apply(context, value->value - m_reference);
            }
            if (point != nullptr) {
                context.lastPoint = point->point;
                return apply(context, directionOf(point->point - m_base) - m_reference);
            }
            if (confirmed) {
                return apply(context, -m_reference);  // the new angle is the default, 0
            }
            if (keyword != nullptr && keyword->keyword == "Points") {
                m_stage = Stage::NewFirst;
            }
            break;
        case Stage::NewFirst:
            if (point != nullptr) {
                context.lastPoint = point->point;
                m_firstPoint = point->point;
                m_stage = Stage::NewSecond;
            }
            break;
        case Stage::NewSecond:
            if (point != nullptr) {
                context.lastPoint = point->point;
                return apply(context, directionOf(point->point - m_firstPoint) - m_reference);
            }
            break;
    }
    updatePreview();
    return Status::Continue;
}

void RotateCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt RotateCommand::prompt() const {
    switch (m_stage) {
        case Stage::Selection:
            return {"Select objects:", {}};
        case Stage::Base:
            return {"Specify base point:", {}};
        case Stage::Angle:
            return {"Specify rotation angle or [Copy/Reference] <" +
                        formatAngle(m_defaultRotation) + ">:",
                    {{"Copy", "C"}, {"Reference", "R"}},
                    PromptKind::Angle};
        case Stage::ReferenceAngle:
            return {"Specify the reference angle <0>:", {}, PromptKind::Angle};
        case Stage::ReferenceSecond:
            return {"Specify second point:", {}};
        case Stage::NewAngle:
            return {"Specify the new angle or [Points] <0>:", {{"Points", "P"}}, PromptKind::Angle};
        case Stage::NewFirst:
            return {"Specify first point:", {}};
        case Stage::NewSecond:
            return {"Specify second point:", {}};
    }
    return {};
}

std::span<const core::Entity> RotateCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> RotateCommand::basePoint() const {
    switch (m_stage) {
        case Stage::Selection:
        case Stage::Base:
        case Stage::NewFirst:
            return std::nullopt;
        case Stage::Angle:
        case Stage::ReferenceAngle:
        case Stage::NewAngle:
            return m_base;
        case Stage::ReferenceSecond:
        case Stage::NewSecond:
            return m_firstPoint;
    }
    return std::nullopt;
}

bool RotateCommand::needsSelection() const {
    return true;
}

Status RotateCommand::apply(CommandContext& context, double rotation) {
    writeTransformed(context, m_selected, core::Similarity::rotation(rotation, m_base), m_copy);
    // The next ROTATE proposes this angle (docs/adr/0014, point 4).
    context.defaults.rotation = rotation;
    // An empty step (a zero rotation of originals) leaves no trace in the history.
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

std::optional<double> RotateCommand::rotationAt(const core::Vec2& pointer) const {
    switch (m_stage) {
        case Stage::Angle:
            return directionOf(pointer - m_base);
        case Stage::NewAngle:
            return directionOf(pointer - m_base) - m_reference;
        case Stage::NewSecond:
            return directionOf(pointer - m_firstPoint) - m_reference;
        case Stage::Selection:
        case Stage::Base:
        case Stage::ReferenceAngle:
        case Stage::ReferenceSecond:
        case Stage::NewFirst:
            return std::nullopt;
    }
    return std::nullopt;
}

void RotateCommand::updatePreview() {
    m_preview.clear();
    if (!m_pointer) {
        return;
    }
    if (const std::optional<double> rotation = rotationAt(*m_pointer)) {
        appendTransformed(m_selected, core::Similarity::rotation(*rotation, m_base), m_preview);
    }
}

// ScaleObjectsCommand

void ScaleObjectsCommand::start(CommandContext& context) {
    m_stage = Stage::Selection;
    m_selected.clear();
    m_base = {};
    m_firstPoint = {};
    m_reference = kDefaultLength;
    m_copy = false;
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Scale");
}

Status ScaleObjectsCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const auto* value = std::get_if<ValueInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Selection:
            if (const auto* selection = std::get_if<SelectionInput>(&input)) {
                m_selected = snapshot(context, selection->ids);
                if (m_selected.empty()) {
                    context.editor.rollback();
                    return Status::Finished;
                }
                m_stage = Stage::Base;
            }
            break;
        case Stage::Base:
            if (point != nullptr) {
                m_base = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::Factor;
            } else if (confirmed) {
                // No default: Enter ends the command without effects (docs/adr/0014).
                context.editor.rollback();
                return Status::Finished;
            }
            break;
        case Stage::Factor:
            if (value != nullptr) {
                return apply(context, value->value);
            }
            if (point != nullptr) {
                context.lastPoint = point->point;
                // A click gives the distance from the base point.
                return apply(context, (point->point - m_base).length());
            }
            if (confirmed) {
                context.editor.rollback();
                return Status::Finished;
            }
            if (keyword != nullptr) {
                if (keyword->keyword == "Copy") {
                    m_copy = true;
                    context.message("Scaling a copy of the selected objects.");
                } else if (keyword->keyword == "Reference") {
                    m_stage = Stage::ReferenceLength;
                }
            }
            break;
        case Stage::ReferenceLength:
            if (value != nullptr) {
                if (validLength(context, value->value)) {
                    m_reference = value->value;
                    m_stage = Stage::NewLength;
                }
            } else if (point != nullptr) {
                context.lastPoint = point->point;
                m_firstPoint = point->point;
                m_stage = Stage::ReferenceSecond;
            } else if (confirmed) {
                m_reference = kDefaultLength;
                m_stage = Stage::NewLength;
            }
            break;
        case Stage::ReferenceSecond:
            if (point != nullptr) {
                context.lastPoint = point->point;
                const double length = (point->point - m_firstPoint).length();
                if (validLength(context, length)) {
                    m_reference = length;
                    m_stage = Stage::NewLength;
                }
            }
            break;
        case Stage::NewLength:
            if (value != nullptr) {
                if (validLength(context, value->value)) {
                    return apply(context, value->value / m_reference);
                }
            } else if (point != nullptr) {
                context.lastPoint = point->point;
                const double length = (point->point - m_base).length();
                if (validLength(context, length)) {
                    return apply(context, length / m_reference);
                }
            } else if (confirmed) {
                return apply(context, kDefaultLength / m_reference);
            } else if (keyword != nullptr && keyword->keyword == "Points") {
                m_stage = Stage::NewFirst;
            }
            break;
        case Stage::NewFirst:
            if (point != nullptr) {
                context.lastPoint = point->point;
                m_firstPoint = point->point;
                m_stage = Stage::NewSecond;
            }
            break;
        case Stage::NewSecond:
            if (point != nullptr) {
                context.lastPoint = point->point;
                const double length = (point->point - m_firstPoint).length();
                if (validLength(context, length)) {
                    return apply(context, length / m_reference);
                }
            }
            break;
    }
    updatePreview();
    return Status::Continue;
}

void ScaleObjectsCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt ScaleObjectsCommand::prompt() const {
    switch (m_stage) {
        case Stage::Selection:
            return {"Select objects:", {}};
        case Stage::Base:
            return {"Specify base point:", {}};
        case Stage::Factor:
            return {"Specify scale factor or [Copy/Reference]:",
                    {{"Copy", "C"}, {"Reference", "R"}},
                    PromptKind::Number};
        case Stage::ReferenceLength:
            return {"Specify reference length <" + formatLength(kDefaultLength) + ">:",
                    {},
                    PromptKind::Distance};
        case Stage::ReferenceSecond:
            return {"Specify second point:", {}};
        case Stage::NewLength:
            return {"Specify new length or [Points] <" + formatLength(kDefaultLength) + ">:",
                    {{"Points", "P"}},
                    PromptKind::Distance};
        case Stage::NewFirst:
            return {"Specify first point:", {}};
        case Stage::NewSecond:
            return {"Specify second point:", {}};
    }
    return {};
}

std::span<const core::Entity> ScaleObjectsCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> ScaleObjectsCommand::basePoint() const {
    switch (m_stage) {
        case Stage::Selection:
        case Stage::Base:
        case Stage::NewFirst:
            return std::nullopt;
        case Stage::Factor:
        case Stage::ReferenceLength:
        case Stage::NewLength:
            return m_base;
        case Stage::ReferenceSecond:
        case Stage::NewSecond:
            return m_firstPoint;
    }
    return std::nullopt;
}

bool ScaleObjectsCommand::needsSelection() const {
    return true;
}

Status ScaleObjectsCommand::apply(CommandContext& context, double factor) {
    // Only a finite factor greater than zero is a similarity (docs/adr/0031,
    // point 1): the question stays otherwise.
    const std::optional<core::Similarity> scaling = core::Similarity::scaling(factor, m_base);
    if (!scaling) {
        context.message(std::string{kNotPositive});
        updatePreview();
        return Status::Continue;
    }
    writeTransformed(context, m_selected, *scaling, m_copy);
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

bool ScaleObjectsCommand::validLength(CommandContext& context, double length) {
    if (std::isfinite(length) && length > 0.0) {
        return true;
    }
    context.message(std::string{kNotPositive});
    return false;
}

std::optional<double> ScaleObjectsCommand::factorAt(const core::Vec2& pointer) const {
    switch (m_stage) {
        case Stage::Factor:
            return (pointer - m_base).length();
        case Stage::NewLength:
            return (pointer - m_base).length() / m_reference;
        case Stage::NewSecond:
            return (pointer - m_firstPoint).length() / m_reference;
        case Stage::Selection:
        case Stage::Base:
        case Stage::ReferenceLength:
        case Stage::ReferenceSecond:
        case Stage::NewFirst:
            return std::nullopt;
    }
    return std::nullopt;
}

void ScaleObjectsCommand::updatePreview() {
    m_preview.clear();
    if (!m_pointer) {
        return;
    }
    const std::optional<double> factor = factorAt(*m_pointer);
    if (!factor) {
        return;
    }
    // A pointer on the base point gives a zero factor: nothing to draw.
    if (const std::optional<core::Similarity> scaling =
            core::Similarity::scaling(*factor, m_base)) {
        appendTransformed(m_selected, *scaling, m_preview);
    }
}

}  // namespace tg::cmd
