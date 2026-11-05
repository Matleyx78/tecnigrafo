// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace tg::cmd {

class OffsetCommand final : public Command {
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
    enum class Stage : std::uint8_t { Distance, SecondPoint, Erase, Layer, Object, Side };

    struct Step {
        edit::Editor::Savepoint before;
        core::EntityId source;
        core::EntityId copy;
    };

    void handleDistance(CommandContext& context, const Input& input);
    void useDistance(CommandContext& context, double distance);
    void pickObject(CommandContext& context, core::EntityId id);
    void offset(CommandContext& context, const core::Vec2& side);
    void undo(CommandContext& context);

    Stage m_stage{Stage::Distance};
    std::optional<double> m_distance;
    bool m_erase{false};
    bool m_currentLayer{false};
    bool m_multiple{false};
    core::Vec2 m_firstPoint{};
    core::EntityId m_source{};
    std::vector<Step> m_steps;
};

}  // namespace tg::cmd
