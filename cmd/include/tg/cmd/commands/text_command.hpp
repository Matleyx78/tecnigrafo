// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

namespace tg::cmd {

/// TEXT creates baseline-aligned lines in one undoable transaction.
class TextCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t { Start, Height, Rotation, Text };
    void finish(CommandContext& context);
    void updatePreview();
    Stage m_stage{Stage::Start};
    core::Vec2 m_start;
    core::Vec2 m_next;
    double m_height{2.5};
    double m_rotation{0.0};
    bool m_hasLines{false};
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
