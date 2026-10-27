// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/entity_id.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {

/// Changes the color, layer, linetype, linetype scale or lineweight of the
/// selected entities (docs/adr/0023, point 2), in one undoable transaction
/// named "CHPROP". Selection belongs to CommandRunner (docs/adr/0008): the
/// command starts once it has one.
///
/// It asks for the property until Enter, and each value applies at once to
/// every selected entity. The value between < > is the one the selected
/// entities share, or "varies". Enter at a value question leaves the entities
/// as they are; a value that is not valid writes its message and the question
/// stays. Esc keeps the changes already made (docs/adr/0015).
///
/// An entity can be given a frozen layer, as in AutoCAD: it then disappears
/// from the view.
class ChpropCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] bool needsSelection() const override { return true; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Selection,  ///< before the runner hands over the selection
        Property,
        Color,
        TrueColor,
        Layer,
        Linetype,
        LinetypeScale,
        Lineweight
    };

    /// Moves to the question for `stage`, with `shared` between < >.
    void ask(Stage stage, std::string shared);

    Status chooseProperty(CommandContext& context, std::string_view text);
    void setColor(CommandContext& context, std::string_view text);
    void setLayer(CommandContext& context, std::string_view name);
    void setLinetype(CommandContext& context, std::string_view name);
    void setLinetypeScale(CommandContext& context, std::string_view text);
    void setLineweight(CommandContext& context, std::string_view text);

    Stage m_stage{Stage::Selection};
    std::string m_shared;  ///< what the question shows between < >
    std::vector<core::EntityId> m_ids;
};

}  // namespace tg::cmd
