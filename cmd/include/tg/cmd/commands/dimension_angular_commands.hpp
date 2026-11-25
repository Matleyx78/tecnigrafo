// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/line.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tg::cmd {

class DimangularCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] std::optional<TextEditRequest> textEditing() const override;

private:
    enum class Stage : std::uint8_t { FirstLine, SecondLine, Location, Text, Angle, Quadrant };

    bool selectLine(CommandContext& context, const ObjectInput& object, bool first);
    [[nodiscard]] core::Dimension makeDimension(const core::Vec2& location) const;
    void updatePreview();
    Status finish(CommandContext& context, const core::Vec2& location);

    Stage m_stage{Stage::FirstLine};
    core::Line m_first;
    core::Line m_second;
    std::optional<core::Vec2> m_pointer;
    std::optional<core::Vec2> m_quadrantPoint;
    std::string m_textOverride;
    std::optional<double> m_textRotation;
    const core::Document* m_document{nullptr};
    std::vector<core::Entity> m_preview;
};

class RadialDimensionCommand final : public Command {
public:
    enum class Kind : std::uint8_t { Radius, Diameter };

    explicit RadialDimensionCommand(Kind kind) : m_kind(kind) {}

    [[nodiscard]] static CommandDescription description(Kind kind);
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] std::optional<TextEditRequest> textEditing() const override;

private:
    enum class Stage : std::uint8_t { Curve, Location, Text, Angle };

    bool selectCurve(CommandContext& context, const ObjectInput& object);
    [[nodiscard]] core::Dimension makeDimension(const core::Vec2& location) const;
    void updatePreview();
    Status finish(CommandContext& context, const core::Vec2& location);

    Kind m_kind;
    Stage m_stage{Stage::Curve};
    core::Vec2 m_center;
    double m_radius{0.0};
    std::optional<core::Vec2> m_pointer;
    std::string m_textOverride;
    std::optional<double> m_textRotation;
    const core::Document* m_document{nullptr};
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
