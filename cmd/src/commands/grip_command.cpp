// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "grip_command.hpp"

#include "tg/core/grips.hpp"

#include <cassert>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

// The messages and the prompt of docs/adr/0034, point 3; AutoCAD's options
// (Base point, Copy, Undo, eXit) are left out.
constexpr std::string_view kStretchMode = "** STRETCH **";
constexpr std::string_view kStretchPoint = "Specify stretch point:";
constexpr std::string_view kInvalidGripEdit = "Invalid grip edit.";
constexpr std::string_view kTransactionName = "Grip edit";

}  // namespace

GripCommand::GripCommand(core::EntityId id, core::Entity entity, std::size_t index,
                         const core::Vec2& grip)
    : m_id(id), m_entity(std::move(entity)), m_index(index), m_grip(grip) {
    assert(m_index < core::gripPoints(m_entity).size() && "the grip must exist");
}

void GripCommand::start(CommandContext& context) {
    // Typed relative coordinates ("@5,0") start from the hot grip, as in
    // AutoCAD, where the grip becomes the last point.
    context.lastPoint = m_grip;
    context.message(std::string{kStretchMode});
}

Status GripCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    if (point == nullptr) {
        // Enter, or anything else the runner forwards, leaves the grip with
        // no change, as Esc does.
        return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
    }
    const core::GripResult result = core::moveGrip(m_entity, m_index, point->point);
    if (result.outcome == core::GripOutcome::Invalid) {
        context.message(std::string{kInvalidGripEdit});
        return Status::Finished;
    }
    context.lastPoint = point->point;
    context.editor.beginTransaction(std::string{kTransactionName});
    // A result equal to the entity changes nothing: the empty transaction
    // then leaves no undo step.
    context.editor.replace(m_id, result.entity);
    context.editor.commit();
    return Status::Finished;
}

void GripCommand::pointerMoved(const core::Vec2& point) {
    m_preview.clear();
    core::GripResult result = core::moveGrip(m_entity, m_index, point);
    if (result.outcome == core::GripOutcome::Moved) {
        m_preview.push_back(std::move(result.entity));
    }
}

Prompt GripCommand::prompt() const {
    return {std::string{kStretchPoint}, {}, PromptKind::Point};
}

std::span<const core::Entity> GripCommand::preview() const {
    return m_preview;
}

}  // namespace tg::cmd
