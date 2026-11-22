// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/polyline.hpp"

#include <optional>
#include <vector>

namespace tg::cmd {

/// Draws a closed rectangular LWPOLYLINE, with the session defaults used by
/// AutoCAD's RECTANG command.
class RectangCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage {
        FirstCorner,
        OtherCorner,
        ChamferFirst,
        ChamferSecond,
        Elevation,
        Fillet,
        Thickness,
        Width,
        Area,
        AreaBasis,
        AreaLength,
        AreaWidth,
        DimensionLength,
        DimensionWidth,
        Rotation,
        RotationFirst,
        RotationSecond
    };

    Status finish(CommandContext& context, double dx, double dy);
    void updatePreview(const core::Vec2& other);
    [[nodiscard]] core::Polyline rectangle(double dx, double dy) const;
    [[nodiscard]] std::pair<double, double> projected(const core::Vec2& point) const;

    Stage m_stage{Stage::FirstCorner};
    CommandDefaults* m_defaults{nullptr};
    core::Vec2 m_first{};
    core::Vec2 m_rotationFirst{};
    double m_area{100.0};
    bool m_dimensions{false};
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
