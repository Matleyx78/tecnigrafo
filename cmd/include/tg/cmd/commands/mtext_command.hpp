// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/mtext.hpp"

namespace tg::cmd {

/// MTEXT creates one multiline text entity from a reference rectangle.
class MTextCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t {
        FirstCorner,
        OppositeCorner,
        Height,
        Justify,
        LineSpacingType,
        LineSpacingFactor,
        Rotation,
        Style,
        Width,
        Text
    };

    void finish(CommandContext& context);
    void beginText(double width);
    void useStyle(const core::Document& document, std::string style);

    Stage m_stage{Stage::FirstCorner};
    core::MText m_mtext;
    core::Vec2 m_first;
    bool m_hasLines{false};
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
