// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

namespace tg::cmd {

/// CIRCLE constructs one circle from a centre and size, or two or three points.
class CircleCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t {
        Center,
        Radius,
        Diameter,
        TwoFirst,
        TwoSecond,
        ThreeFirst,
        ThreeSecond,
        ThreeThird
    };
    [[nodiscard]] std::optional<core::Circle> circleAt(const core::Vec2& point) const;
    Status finish(CommandContext& context, const core::Circle& circle);
    void updatePreview();

    Stage m_stage{Stage::Center};
    core::Vec2 m_first;
    core::Vec2 m_second;
    std::optional<double> m_defaultRadius;
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
