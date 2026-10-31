// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tg::cmd {

/// The selection a transformation command works on, as it was when the
/// command started: the preview and the changes are built from these
/// snapshots (docs/adr/0031, point 5).
struct SelectedEntity {
    core::EntityId id;
    core::Entity entity;
};

/// ROTATE: rotates the selected entities about a base point, in place or on
/// copies (docs/adr/0031, points 5 and 6). One undo step, "Rotate". The angle
/// can be given as a value, with a click (measured from the base point), or
/// through the Reference option as the difference between a new angle and a
/// reference angle. The last rotation is remembered in
/// CommandDefaults::rotation (docs/adr/0014, point 4).
class RotateCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] bool needsSelection() const override;

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Selection,        ///< before the runner hands over the selection
        Base,             ///< the center of the rotation
        Angle,            ///< the rotation angle, with Copy and Reference
        ReferenceAngle,   ///< the reference angle, as a value or as a first point
        ReferenceSecond,  ///< the second point of the reference angle
        NewAngle,         ///< the new angle, as a value, a point or Points
        NewFirst,         ///< Points: the first point of the new angle
        NewSecond         ///< Points: the second point of the new angle
    };

    Status apply(CommandContext& context, double rotation);
    /// The rotation the pointer would give at the current stage, if any.
    [[nodiscard]] std::optional<double> rotationAt(const core::Vec2& pointer) const;
    void updatePreview();

    Stage m_stage{Stage::Selection};
    std::vector<SelectedEntity> m_selected;
    core::Vec2 m_base;
    core::Vec2 m_firstPoint;  ///< the first point of a two-point angle
    double m_reference{0.0};  ///< the reference angle, radians
    double m_defaultRotation{0.0};
    bool m_copy{false};
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

/// SCALE: scales the selected entities about a base point, in place or on
/// copies (docs/adr/0031, points 1, 5 and 6). One undo step, "Scale". The
/// factor can be a number, a click (its distance from the base point), or,
/// through the Reference option, the ratio of a new length to a reference
/// length. A factor that is not positive is refused and the prompt stays.
/// Named ScaleObjectsCommand because ScaleCommand is the one of LTSCALE.
class ScaleObjectsCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] bool needsSelection() const override;

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Selection,        ///< before the runner hands over the selection
        Base,             ///< the fixed point of the scaling
        Factor,           ///< the scale factor, with Copy and Reference
        ReferenceLength,  ///< the reference length, as a value or as a first point
        ReferenceSecond,  ///< the second point of the reference length
        NewLength,        ///< the new length, as a value, a point or Points
        NewFirst,         ///< Points: the first point of the new length
        NewSecond         ///< Points: the second point of the new length
    };

    Status apply(CommandContext& context, double factor);
    /// Takes `length` as a reference or new length; false, with a message,
    /// if it is not a positive finite number.
    [[nodiscard]] static bool validLength(CommandContext& context, double length);
    /// The factor the pointer would give at the current stage, if any.
    [[nodiscard]] std::optional<double> factorAt(const core::Vec2& pointer) const;
    void updatePreview();

    Stage m_stage{Stage::Selection};
    std::vector<SelectedEntity> m_selected;
    core::Vec2 m_base;
    core::Vec2 m_firstPoint;  ///< the first point of a two-point length
    double m_reference{1.0};  ///< the reference length
    bool m_copy{false};
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
