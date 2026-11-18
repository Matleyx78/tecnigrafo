// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/hatch.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tg::cmd {

/// -HATCH: creates a non-associative hatch from internal points and selected
/// closed objects (docs/adr/0048, point 6). One undo step, "Hatch".
class HatchCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t { Boundary, Pattern, Scale, Angle, Selection };

    void currentPattern(CommandContext& context) const;
    Status finish(CommandContext& context);
    void usePoint(CommandContext& context, const core::Vec2& point);
    void useSelection(CommandContext& context, const SelectionInput& selection);

    Stage m_stage{Stage::Boundary};
    std::string m_pattern{"ANSI31"};
    double m_scale{1.0};
    double m_angle{0.0};
    core::IslandStyle m_islands{core::IslandStyle::Outer};
    std::vector<core::HatchLoop> m_pointLoops;
    std::vector<core::HatchLoop> m_selectedLoops;
    std::vector<core::Vec2> m_seeds;
    bool m_analyzed{false};
};

/// HPISLANDDETECTION: the island style remembered for the session.
class HpislanddetectionCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    core::IslandStyle m_value{core::IslandStyle::Outer};
};

}  // namespace tg::cmd
