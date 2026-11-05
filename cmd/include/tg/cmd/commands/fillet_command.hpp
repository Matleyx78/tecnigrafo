// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

namespace tg::cmd {

class FilletCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& /*point*/) override {}
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t { First, Second, Radius, RadiusPoint, Trim, Polyline };

    void choose(const KeywordInput& keyword, CommandContext& context);
    Status setRadius(CommandContext& context, double radius);
    Status resumeAfterSetting(CommandContext& context);
    Status filletObjects(CommandContext& context, const ObjectInput& object);
    Status filletPolyline(CommandContext& context, core::EntityId id);
    Status finishAttempt(CommandContext& context);

    Stage m_stage{Stage::First};
    ObjectInput m_first{};
    core::Vec2 m_radiusBase{};
    double m_radius{0.0};
    bool m_trim{true};
    bool m_multiple{false};
    std::vector<edit::Editor::Savepoint> m_steps;
};

}  // namespace tg::cmd
