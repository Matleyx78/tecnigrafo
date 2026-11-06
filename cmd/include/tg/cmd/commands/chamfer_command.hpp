// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/chamfer.hpp"

namespace tg::cmd {

class ChamferCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    enum class Stage : std::uint8_t {
        First,
        Second,
        Polyline,
        FirstDistance,
        SecondDistance,
        Length,
        Angle,
        Trim,
        Method,
    };

    void choose(CommandContext& context, const KeywordInput& keyword);
    Status setNumber(CommandContext& context, double number);
    Status resumeAfterSetting(CommandContext& context);
    Status chamferObjects(CommandContext& context, const ObjectInput& object);
    Status chamferPolyline(CommandContext& context, core::EntityId id);
    Status finishAttempt(CommandContext& context);
    [[nodiscard]] double defaultNumber() const;
    [[nodiscard]] bool expectsNumber() const;
    [[nodiscard]] core::ChamferSpec spec(bool shift = false) const;

    Stage m_stage{Stage::First};
    Stage m_returnStage{Stage::First};
    ObjectInput m_first{};
    core::ChamferDistances m_distances;
    core::ChamferAngle m_angle;
    double m_pending{0.0};
    bool m_useAngle{false};
    bool m_trim{true};
    bool m_multiple{false};
    std::vector<edit::Editor::Savepoint> m_steps;
};

}  // namespace tg::cmd
