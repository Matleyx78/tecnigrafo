// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/dimension.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tg::cmd {

class DimensionCommand final : public Command {
public:
    enum class Kind : std::uint8_t { Linear, Aligned };

    explicit DimensionCommand(Kind kind) : m_kind(kind) {}

    [[nodiscard]] static CommandDescription description(Kind kind);
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] std::optional<TextEditRequest> textEditing() const override;

private:
    enum class Stage : std::uint8_t { First, Object, Second, Location, Text, Angle, Rotation };

    [[nodiscard]] core::Dimension makeDimension(const core::Vec2& location) const;
    [[nodiscard]] std::optional<core::Dimension> regeneratedAt(const core::Vec2& location,
                                                               std::string* reason = nullptr) const;
    void updatePreview();
    bool selectObject(CommandContext& context, const ObjectInput& object);
    Status finish(CommandContext& context, const core::Vec2& location);

    Kind m_kind;
    Stage m_stage{Stage::First};
    core::Vec2 m_first;
    core::Vec2 m_second;
    std::optional<core::Vec2> m_pointer;
    std::optional<core::Vec2> m_location;
    std::optional<double> m_fixedRotation;
    bool m_axisFixed{false};
    std::string m_textOverride;
    std::optional<double> m_textRotation;
    const core::Document* m_document{nullptr};
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
