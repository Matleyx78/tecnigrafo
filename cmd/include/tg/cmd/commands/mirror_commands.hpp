// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/cmd/commands/rotate_scale_commands.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tg::cmd {

/// MIRROR: reflects the selected entities about a line given by two points
/// (docs/adr/0031, points 1-3). Without erasing the source it adds reflected
/// copies; with Yes the entities are reflected in place, keeping their
/// identity (docs/adr/0031, points 5 and 6). Text follows MIRRTEXT
/// (CommandDefaults::mirrorText). One undo step, "Mirror".
class MirrorCommand final : public Command {
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
        Selection,  ///< before the runner hands over the selection
        First,      ///< the first point of the mirror line
        Second,     ///< the second point; the question stays while it coincides with the first
        Erase       ///< whether to erase the source objects
    };

    Status apply(CommandContext& context, bool eraseSource);
    void updatePreview();

    Stage m_stage{Stage::Selection};
    std::vector<SelectedEntity> m_selected;
    core::Vec2 m_first;
    std::optional<core::Similarity> m_line;  ///< the reflection, once the line is given
    core::MirrorText m_mirrorText{core::MirrorText::Readable};
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

/// MIRRTEXT: whether MIRROR reflects text (1) or keeps it readable (0). A
/// value of the session, not of the drawing (docs/adr/0031, consequences),
/// so it opens no undo step.
class MirrtextCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    core::MirrorText m_value{core::MirrorText::Readable};
};

}  // namespace tg::cmd
