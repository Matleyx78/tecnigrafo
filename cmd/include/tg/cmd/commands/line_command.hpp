// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

#include <optional>
#include <vector>

namespace tg::cmd {

/// LINE adds each completed segment immediately, within one undo step.
/// Esc keeps those segments and discards only the pointer preview.
class LineCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    Status finish(CommandContext& context);
    void updatePreview();

    std::vector<core::Vec2> m_points;
    std::vector<core::EntityId> m_segments;
    std::optional<core::Vec2> m_previousPoint;
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
