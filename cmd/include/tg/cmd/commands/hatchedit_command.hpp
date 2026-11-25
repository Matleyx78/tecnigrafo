// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/hatch.hpp"

#include <cstdint>

namespace tg::cmd {

/// -HATCHEDIT: changes the properties, island style or origin of one hatch.
class HatcheditCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Select,
        Option,
        Pattern,
        Scale,
        Angle,
        Style,
        OriginOption,
        OriginPoint,
        StoreOrigin
    };

    Status apply(CommandContext& context);
    bool select(CommandContext& context, core::EntityId id);

    Stage m_stage{Stage::Select};
    core::EntityId m_target{};
    core::Hatch m_hatch;
    core::EntityExtras m_extras;
    core::Vec2 m_origin{};
    core::Vec2 m_newOrigin{};
    std::string m_pattern;
    double m_scale{1.0};
    double m_angle{0.0};
};

}  // namespace tg::cmd
