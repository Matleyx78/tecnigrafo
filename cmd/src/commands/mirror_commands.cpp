// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/mirror_commands.hpp"

#include "../text.hpp"
#include "tg/core/similarity.hpp"

#include <charconv>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

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

// The values MIRRTEXT accepts: 0 keeps text readable, 1 reflects it.
constexpr int kMinMirrorText = 0;
constexpr int kMaxMirrorText = 1;

}  // namespace

// MirrorCommand

void MirrorCommand::start(CommandContext& context) {
    m_stage = Stage::Selection;
    m_selected.clear();
    m_first = {};
    m_line.reset();
    m_mirrorText = context.defaults.mirrorText;
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Mirror");
}

Status MirrorCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Selection:
            if (const auto* selection = std::get_if<SelectionInput>(&input)) {
                m_selected = snapshot(context, selection->ids);
                if (m_selected.empty()) {
                    context.editor.rollback();
                    return Status::Finished;
                }
                m_stage = Stage::First;
            }
            break;
        case Stage::First:
            if (point != nullptr) {
                m_first = point->point;
                // The "@" of the next point is measured from the first point.
                context.lastPoint = point->point;
                m_stage = Stage::Second;
            } else if (confirmed) {
                // No default: Enter ends the command without effects (docs/adr/0014).
                context.editor.rollback();
                return Status::Finished;
            }
            break;
        case Stage::Second:
            if (point != nullptr) {
                context.lastPoint = point->point;
                // Coincident points define no line: the question stays.
                m_line = core::Similarity::reflection(m_first, point->point);
                if (m_line) {
                    m_stage = Stage::Erase;
                }
            } else if (confirmed) {
                context.editor.rollback();
                return Status::Finished;
            }
            break;
        case Stage::Erase:
            if (confirmed) {
                return apply(context, false);  // the default is No
            }
            if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
                if (keyword->keyword == "Yes") {
                    return apply(context, true);
                }
                if (keyword->keyword == "No") {
                    return apply(context, false);
                }
            }
            break;
    }
    updatePreview();
    return Status::Continue;
}

void MirrorCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt MirrorCommand::prompt() const {
    switch (m_stage) {
        case Stage::Selection:
            return {"Select objects:", {}};
        case Stage::First:
            return {"Specify first point of mirror line:", {}};
        case Stage::Second:
            return {"Specify second point of mirror line:", {}};
        case Stage::Erase:
            return {"Erase source objects? [Yes/No] <No>:", {{"Yes", "Y"}, {"No", "N"}}};
    }
    return {};
}

std::span<const core::Entity> MirrorCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> MirrorCommand::basePoint() const {
    return m_stage == Stage::Second ? std::optional<core::Vec2>{m_first} : std::nullopt;
}

bool MirrorCommand::needsSelection() const {
    return true;
}

Status MirrorCommand::apply(CommandContext& context, bool eraseSource) {
    if (!m_line) {
        // Not reachable: the question after the mirror line is asked only once
        // the line is valid. Leave the drawing untouched all the same.
        context.editor.rollback();
        return Status::Finished;
    }
    const core::Similarity line = *m_line;
    for (const SelectedEntity& item : m_selected) {
        core::Entity result = core::transformed(item.entity, line, m_mirrorText);
        if (eraseSource) {
            // The source is reflected where it is: same identity, same data.
            context.editor.replace(item.id, std::move(result));
        } else {
            // The reflected copies take the properties and the unhandled data
            // of their source (docs/adr/0031, point 6).
            context.editor.addCopy(item.id, std::move(result));
        }
    }
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

void MirrorCommand::updatePreview() {
    m_preview.clear();
    std::optional<core::Similarity> line;
    if (m_stage == Stage::Second && m_pointer) {
        line = core::Similarity::reflection(m_first, *m_pointer);
    } else if (m_stage == Stage::Erase) {
        line = m_line;
    }
    if (!line) {
        return;
    }
    for (const SelectedEntity& item : m_selected) {
        m_preview.push_back(core::transformed(item.entity, *line, m_mirrorText));
    }
}

// MirrtextCommand

void MirrtextCommand::start(CommandContext& context) {
    m_value = context.defaults.mirrorText;
}

Status MirrtextCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        std::string_view value = detail::trim(text->text);
        // from_chars accepts a minus sign but not an explicit plus sign.
        if (value.starts_with('+')) {
            value.remove_prefix(1);
        }
        int number = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
            number < kMinMirrorText || number > kMaxMirrorText) {
            context.message("Requires an integer between 0 and 1.");
            return Status::Continue;
        }
        context.defaults.mirrorText =
            number == 1 ? core::MirrorText::Mirrored : core::MirrorText::Readable;
        return Status::Finished;
    }
    // Enter keeps the current value; no other input answers this question.
    return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
}

Prompt MirrtextCommand::prompt() const {
    return {"Enter new value for MIRRTEXT <" +
                std::string{m_value == core::MirrorText::Mirrored ? "1" : "0"} + ">:",
            {},
            PromptKind::Text};
}

}  // namespace tg::cmd
