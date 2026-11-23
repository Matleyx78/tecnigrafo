// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <cstddef>
#include <cstdint>

namespace tg::cmd {

/// Edits annotation text, one committed undo step per object (ADR 0054, point 6).
class TexteditCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<TextEditRequest> textEditing() const override;

private:
    enum class Stage : std::uint8_t {
        Object,
        Mode,
        Text,
        AttributeTag,
        AttributePrompt,
        Attributes
    };

    void select(CommandContext& context, core::EntityId id);
    void completeObject(CommandContext& context);
    void resetObject();

    Stage m_stage{Stage::Object};
    core::EntityId m_target;
    core::Entity m_original{core::Line{}};
    std::size_t m_attribute{0};
    bool m_multiple{true};
    bool m_objectChanged{false};
    std::size_t m_edits{0};
    const core::Document* m_document{nullptr};
};

}  // namespace tg::cmd
