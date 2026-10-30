// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/text_command.hpp"

#include "tg/cmd/value_format.hpp"

#include <cmath>
#include <variant>

namespace tg::cmd {

void TextCommand::start(CommandContext& context) {
    m_stage = Stage::Start;
    m_start = {};
    m_next = {};
    m_height = context.defaults.textHeight;
    m_rotation = context.defaults.textRotation;
    m_hasLines = false;
    m_preview.clear();
    context.editor.beginTransaction("Text");
}

Status TextCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const auto* value = std::get_if<ValueInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Start:
            if (point != nullptr) {
                m_start = point->point;
                m_next = m_start;
                context.lastPoint = m_start;
                m_stage = Stage::Height;
            } else if (confirm) {
                finish(context);
                return Status::Finished;
            }
            break;
        case Stage::Height:
            if (point != nullptr || value != nullptr || confirm) {
                double height = m_height;
                if (point != nullptr) {
                    height = core::distance(m_start, point->point);
                } else if (value != nullptr) {
                    height = value->value;
                }
                if (!(height > 0.0) || !std::isfinite(height)) {
                    context.message("Value must be positive and nonzero.");
                    return Status::Continue;
                }
                m_height = height;
                m_stage = Stage::Rotation;
            }
            break;
        case Stage::Rotation:
            if (point != nullptr || value != nullptr || confirm) {
                double rotation = m_rotation;
                if (point != nullptr) {
                    rotation = std::atan2(point->point.y - m_start.y, point->point.x - m_start.x);
                } else if (value != nullptr) {
                    rotation = value->value;
                }
                if (!std::isfinite(rotation)) {
                    return Status::Continue;
                }
                m_rotation = rotation;
                m_stage = Stage::Text;
            }
            break;
        case Stage::Text:
            if (confirm) {
                finish(context);
                return Status::Finished;
            }
            if (const auto* text = std::get_if<TextInput>(&input)) {
                core::Text line;
                line.position = m_next;
                line.height = m_height;
                line.rotation = m_rotation;
                line.value = text->text;
                context.editor.add(line);
                if (!m_hasLines) {
                    // Defaults describe a created text, not an abandoned setup.
                    context.defaults.textHeight = m_height;
                    context.defaults.textRotation = m_rotation;
                }
                context.lastPoint = m_next;
                m_hasLines = true;
                if (context.scriptSequences) {
                    finish(context);
                    return Status::Finished;
                }
                // ADR 0013: advance down the text's local Y axis by 5/3 of its height.
                constexpr double kLineSpacing = 5.0 / 3.0;
                const double spacing = kLineSpacing * m_height;
                m_next = {m_next.x + spacing * std::sin(m_rotation),
                          m_next.y - spacing * std::cos(m_rotation)};
            }
            break;
    }
    updatePreview();
    return Status::Continue;
}

void TextCommand::finish(CommandContext& context) {
    if (m_hasLines) {
        context.editor.commit();
    } else {
        context.editor.rollback();
    }
    m_preview.clear();
}

void TextCommand::cancel(CommandContext& context) {
    finish(context);
}

void TextCommand::pointerMoved(const core::Vec2& /*point*/) {
    // The preview marks the next baseline; moving the pointer does not move it.
}

Prompt TextCommand::prompt() const {
    switch (m_stage) {
        case Stage::Start:
            return {"Specify start point of text:", {}};
        case Stage::Height:
            return {"Specify height <" + formatLength(m_height) + ">:", {}, PromptKind::Distance};
        case Stage::Rotation:
            return {"Specify rotation angle of text <" + formatAngle(m_rotation) + ">:",
                    {},
                    PromptKind::Angle};
        case Stage::Text:
            return {"Enter text:", {}, PromptKind::Text};
    }
    return {};
}

std::span<const core::Entity> TextCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> TextCommand::basePoint() const {
    return m_stage == Stage::Height || m_stage == Stage::Rotation
               ? std::optional<core::Vec2>{m_start}
               : std::nullopt;
}

void TextCommand::updatePreview() {
    m_preview.clear();
    if (m_stage == Stage::Rotation || m_stage == Stage::Text) {
        core::Text character;
        character.position = m_next;
        character.height = m_height;
        character.rotation = m_rotation;
        character.value = "M";
        core::Polyline box;
        for (const auto& corner : core::textBox(character)) {
            box.addVertex(corner);
        }
        box.setClosed(true);
        m_preview.emplace_back(box);
    }
}

}  // namespace tg::cmd
