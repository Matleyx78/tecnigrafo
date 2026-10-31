// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/math.hpp"
#include "tg/core/vec2.hpp"

#include <span>

namespace tg::cmd {

/// POLARANG: the increment of polar tracking, in degrees, greater than 0 and
/// at most 180 (docs/adr/0033, point 8). Any other answer is refused and
/// the question stays; Enter keeps the current value. A value of the
/// session, kept in CommandDefaults::polarAngle, so it opens no undo step.
class PolarangCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    double m_value{core::kPi / 2.0};  ///< radians
};

}  // namespace tg::cmd
