// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tg::cmd {

/// Draws a regular, closed polygon as an LWPOLYLINE.
class PolygonCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t { Sides, Center, Option, Radius, EdgeFirst, EdgeSecond };

    Status finishRadius(CommandContext& context, double radius, double firstAngle);
    Status finishEdge(CommandContext& context, const core::Vec2& second);
    void updatePreview(const core::Vec2& point);

    Stage m_stage{Stage::Sides};
    CommandDefaults* m_defaults{nullptr};
    core::Vec2 m_center;
    core::Vec2 m_edgeFirst;
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
