// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

namespace tg::cmd {

/// ARC constructs one circular arc from three points or start, centre and end.
class ArcCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t { Start, Second, Third, Center, End };
    [[nodiscard]] std::optional<core::Arc> arcAt(const core::Vec2& point) const;
    Status finish(CommandContext& context, const core::Arc& arc, const core::Vec2& end);
    void updatePreview();

    Stage m_stage{Stage::Start};
    core::Vec2 m_start{};
    core::Vec2 m_second{};
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
