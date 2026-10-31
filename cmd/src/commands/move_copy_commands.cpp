// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/move_copy_commands.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/similarity.hpp"

#include <span>
#include <string>
#include <variant>

namespace tg::cmd {
namespace {

// The displacement that Enter takes at a "Specify displacement" prompt.
constexpr core::Vec2 kDefaultDisplacement{0.0, 0.0};

/// "0.0000, 0.0000": the default of the displacement prompt, as AutoCAD
/// shows it.
std::string formatDisplacement(const core::Vec2& displacement) {
    return formatLength(displacement.x) + ", " + formatLength(displacement.y);
}

/// The entities of the selection that still exist, in selection order.
std::vector<DisplacedEntity> snapshot(const CommandContext& context,
                                      std::span<const core::EntityId> ids) {
    std::vector<DisplacedEntity> result;
    result.reserve(ids.size());
    for (const core::EntityId id : ids) {
        if (const core::EntityRecord* record = context.editor.document().find(id)) {
            result.push_back({id, record->entity});
        }
    }
    return result;
}

/// `entity` moved by `displacement`. A translation never reflects, so the
/// text mirroring mode does not matter.
core::Entity displaced(const core::Entity& entity, const core::Vec2& displacement) {
    return core::transformed(entity, core::Similarity::translation(displacement));
}

void appendDisplaced(const std::vector<DisplacedEntity>& selected, const core::Vec2& displacement,
                     std::vector<core::Entity>& out) {
    for (const DisplacedEntity& item : selected) {
        out.push_back(displaced(item.entity, displacement));
    }
}

Prompt basePrompt() {
    return {"Specify base point or [Displacement] <Displacement>:", {{"Displacement", "D"}}};
}

Prompt secondPrompt() {
    return {"Specify second point or <use first point as displacement>:", {}};
}

Prompt displacementPrompt() {
    return {"Specify displacement <" + formatDisplacement(kDefaultDisplacement) + ">:", {}};
}

bool isDisplacementKeyword(const Input& input) {
    const auto* keyword = std::get_if<KeywordInput>(&input);
    return keyword != nullptr && keyword->keyword == "Displacement";
}

}  // namespace

// MoveCommand

void MoveCommand::start(CommandContext& context) {
    m_stage = Stage::Selection;
    m_selected.clear();
    m_base = {};
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Move");
}

Status MoveCommand::handle(CommandContext& context, const Input& input) {
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
            if (const auto* point = std::get_if<PointInput>(&input)) {
                m_base = point->point;
                // The "@" of the next point is measured from the base point.
                context.lastPoint = point->point;
                m_stage = Stage::Second;
            } else if (isDisplacementKeyword(input) ||
                       std::holds_alternative<ConfirmInput>(input)) {
                m_stage = Stage::Displacement;
            }
            break;
        case Stage::Second:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                context.lastPoint = point->point;
                return apply(context, point->point - m_base);
            }
            if (std::holds_alternative<ConfirmInput>(input)) {
                // Enter: the first point is the displacement.
                return apply(context, m_base);
            }
            break;
        case Stage::Displacement:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                return apply(context, point->point);
            }
            if (std::holds_alternative<ConfirmInput>(input)) {
                return apply(context, kDefaultDisplacement);
            }
            break;
    }
    updatePreview();
    return Status::Continue;
}

void MoveCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt MoveCommand::prompt() const {
    switch (m_stage) {
        case Stage::Selection:
            return {"Select objects:", {}};
        case Stage::Base:
            return basePrompt();
        case Stage::Second:
            return secondPrompt();
        case Stage::Displacement:
            return displacementPrompt();
    }
    return {};
}

std::span<const core::Entity> MoveCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> MoveCommand::basePoint() const {
    return m_stage == Stage::Second ? std::optional<core::Vec2>{m_base} : std::nullopt;
}

bool MoveCommand::needsSelection() const {
    return true;
}

Status MoveCommand::apply(CommandContext& context, const core::Vec2& displacement) {
    for (const DisplacedEntity& item : m_selected) {
        // Same identity, same data; an unchanged entity (a zero displacement)
        // leaves no change in the step.
        context.editor.replace(item.id, displaced(item.entity, displacement));
    }
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

void MoveCommand::updatePreview() {
    m_preview.clear();
    if (!m_pointer) {
        return;
    }
    if (m_stage == Stage::Second) {
        appendDisplaced(m_selected, *m_pointer - m_base, m_preview);
    } else if (m_stage == Stage::Displacement) {
        appendDisplaced(m_selected, *m_pointer, m_preview);
    }
}

// CopyCommand

void CopyCommand::start(CommandContext& context) {
    m_stage = Stage::Selection;
    m_selected.clear();
    m_base = {};
    m_pointer.reset();
    m_savepoints.clear();
    m_preview.clear();
    m_scriptSequence = context.scriptSequences;
    // Written after the selection, as AutoCAD does (the runner starts the
    // command once it has the selection). Two spaces after the colon, as in
    // AutoCAD.
    context.message("Current settings:  Copy mode = Multiple");
    context.editor.beginTransaction("Copy");
}

Status CopyCommand::handle(CommandContext& context, const Input& input) {
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
            if (const auto* point = std::get_if<PointInput>(&input)) {
                m_base = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::First;
            } else if (isDisplacementKeyword(input) ||
                       std::holds_alternative<ConfirmInput>(input)) {
                m_stage = Stage::Displacement;
            }
            break;
        case Stage::First:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                context.lastPoint = point->point;
                return copyBy(context, point->point - m_base);
            }
            if (std::holds_alternative<ConfirmInput>(input)) {
                // Enter: the first point is the displacement.
                return copyBy(context, m_base);
            }
            break;
        case Stage::Displacement:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                return copyBy(context, point->point);
            }
            if (std::holds_alternative<ConfirmInput>(input)) {
                return copyBy(context, kDefaultDisplacement);
            }
            break;
        case Stage::More:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                context.lastPoint = point->point;
                return copyBy(context, point->point - m_base);
            }
            if (std::holds_alternative<ConfirmInput>(input)) {
                return finish(context);
            }
            if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
                if (keyword->keyword == "Exit") {
                    return finish(context);
                }
                if (keyword->keyword == "Undo" && !m_savepoints.empty()) {
                    context.editor.rollbackTo(m_savepoints.back());
                    m_savepoints.pop_back();
                }
            }
            break;
    }
    updatePreview();
    return Status::Continue;
}

void CopyCommand::cancel(CommandContext& context) {
    // Esc keeps the copies already made (docs/adr/0015).
    finish(context);
}

void CopyCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt CopyCommand::prompt() const {
    switch (m_stage) {
        case Stage::Selection:
            return {"Select objects:", {}};
        case Stage::Base:
            return basePrompt();
        case Stage::First:
            return secondPrompt();
        case Stage::Displacement:
            return displacementPrompt();
        case Stage::More:
            return {"Specify second point or [Exit/Undo] <Exit>:", {{"Exit", "E"}, {"Undo", "U"}}};
    }
    return {};
}

std::span<const core::Entity> CopyCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> CopyCommand::basePoint() const {
    return m_stage == Stage::First || m_stage == Stage::More ? std::optional<core::Vec2>{m_base}
                                                             : std::nullopt;
}

bool CopyCommand::needsSelection() const {
    return true;
}

Status CopyCommand::copyBy(CommandContext& context, const core::Vec2& displacement) {
    m_savepoints.push_back(context.editor.savepoint());
    for (const DisplacedEntity& item : m_selected) {
        // The copies take the properties and the unhandled data of their
        // source (docs/adr/0031, point 6).
        context.editor.addCopy(item.id, displaced(item.entity, displacement));
    }
    // A typed displacement makes one copy, and so does the sequence of the
    // AutoCAD scripts (docs/adr/0032, points 6-8).
    if (m_stage == Stage::Displacement || m_scriptSequence) {
        return finish(context);
    }
    m_stage = Stage::More;
    updatePreview();
    return Status::Continue;
}

Status CopyCommand::finish(CommandContext& context) {
    // An empty step (every copy undone) leaves no trace in the history.
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

void CopyCommand::updatePreview() {
    m_preview.clear();
    if (!m_pointer) {
        return;
    }
    if (m_stage == Stage::First || m_stage == Stage::More) {
        appendDisplaced(m_selected, *m_pointer - m_base, m_preview);
    } else if (m_stage == Stage::Displacement) {
        appendDisplaced(m_selected, *m_pointer, m_preview);
    }
}

}  // namespace tg::cmd
