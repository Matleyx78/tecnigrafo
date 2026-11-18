// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

namespace tg::cmd {

/// ELLIPSE constructs a full ellipse or an elliptical arc.
class EllipseCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t {
        FirstAxis,
        OtherAxis,
        Center,
        AxisEndpoint,
        Distance,
        Rotation,
        StartAngle,
        EndAngle,
        IncludedAngle
    };

    bool setAxis(CommandContext& context, const core::Vec2& point);
    bool setSize(CommandContext& context, double distance);
    bool setRotation(CommandContext& context, double angle);
    Status afterShape(CommandContext& context);
    Status setAngle(CommandContext& context, double angle);
    Status finish(CommandContext& context);
    [[nodiscard]] core::Ellipse ellipseFor(double otherRadius) const;
    void updatePreview();

    Stage m_stage{Stage::FirstAxis};
    bool m_arc{false};
    bool m_centerMethod{false};
    core::Vec2 m_first;
    core::Vec2 m_center;
    core::Vec2 m_axis;
    double m_otherRadius{0.0};
    double m_startDirection{0.0};
    core::Ellipse m_ellipse;
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
