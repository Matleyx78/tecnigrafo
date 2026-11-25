// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/drawing_settings.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace tg::cmd {

// The commands of the grid, of the snap to the grid and of the UCS icon
// (docs/adr/0055, point 3; docs/tappe/0.8.1/PT28-griglia-cmd.md, point 1).
// They change core::GridSettings of the current drawing only with
// edit::DocumentTables::setGrid(): no undo step, the drawing becomes
// modified. A command changes the settings once, when it finishes; Esc
// leaves them as they were. Text that is neither a number nor an option
// writes "Invalid option keyword." at a question with options, and
// "Requires numeric distance." at a spacing alone; the question stays.

/// GRID: "Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect]
/// <10.0000>:". A spacing, or Enter, switches the grid on; "2x" is twice the
/// snap spacing of each axis; 0 and Snap set GRIDUNIT to 0,0, Snap
/// leaving GRIDMODE as it is. A negative spacing writes "Value must be
/// positive." and the question stays. The proposal is "<A>" when the two
/// spacings differ. Aspect asks the two spacings, each of them possibly 0
/// or a multiple of the snap spacing of its axis, and switches the grid on,
/// as a spacing does; Major asks
/// GRIDMAJOR, from 1 to 100 ("Requires an integer between 1 and 100."
/// otherwise); aDaptive asks bit 2 of GRIDDISPLAY and, with Yes, bit 4; No
/// clears both, as subdivision works only with the adaptive grid
/// (docs/adr/0055, point 5).
class GridCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Spacing,
        HorizontalSpacing,
        VerticalSpacing,
        Major,
        Adaptive,
        Subdivision
    };

    Status spacing(CommandContext& context, std::string_view text);
    Status aspect(CommandContext& context, std::string_view text);
    Status major(CommandContext& context, std::string_view text);
    Status adaptive(CommandContext& context, std::string_view text);
    /// Applies m_grid and ends the command.
    Status finish(CommandContext& context);

    core::GridSettings m_grid;
    Stage m_stage{Stage::Spacing};
};

/// SNAP: "Specify snap spacing or [ON/OFF/Aspect] <10.0000>:". A spacing,
/// or Enter, switches the snap on; 0 or a negative spacing writes "Value
/// must be positive and nonzero." and the question stays. Aspect asks the
/// two spacings and switches the snap on; the proposal is then "<A>" when
/// they differ.
class SnapCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t { Spacing, HorizontalSpacing, VerticalSpacing };

    Status finish(CommandContext& context);

    core::GridSettings m_grid;
    Stage m_stage{Stage::Spacing};
};

/// UCSICON: "Enter an option [ON/OFF/Noorigin/ORigin] <ON>:", the proposal
/// being the current state; Enter keeps it. OFF gives 2, ON 3, Noorigin 1
/// and ORigin 3, as AutoCAD 2021 gave them in the session of 2026-11-22.
class UcsiconCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    int m_value{3};
};

/// The variables of the grid as commands, with "Enter new value for <NAME>
/// <value>:"; Enter keeps the value. Points are written x,y with four
/// decimals ("<10.0000,10.0000>"):
/// - GRIDMODE, SNAPMODE: 0 or 1, else "Requires an integer between 0 and 1."
///   and the question stays, as MIRRTEXT;
/// - GRIDMAJOR: from 1 to 100, else "Requires an integer between 1 and 100."
///   and the question stays, as the Major option of GRID;
/// - GRIDDISPLAY: an integer from 0 to 15, the bits AutoCAD defines, else
///   "Requires an integer between 0 and 15." and the question stays;
/// - GRIDUNIT: two spacings >= 0, 0 on one axis included; SNAPUNIT: two
///   spacings > 0. Any other answer writes "Cannot set <NAME> to that
///   value." and "*Invalid*", and the command ends (docs/DECISIONI.md,
///   2026-11-22, "Esiti della seconda seduta in AutoCAD della 0.8.1",
///   point 7);
/// - SNAPBASE: any point; any other answer as GRIDUNIT.
class GridVariableCommand final : public Command {
public:
    enum class Variable : std::uint8_t {
        GridMode,
        GridUnit,
        SnapMode,
        SnapUnit,
        SnapBase,
        GridDisplay,
        GridMajor
    };

    explicit GridVariableCommand(Variable variable) : m_variable(variable) {}

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    /// The name of `variable`, "GRIDMODE".
    [[nodiscard]] static std::string_view name(Variable variable);

    [[nodiscard]] static CommandDescription description(Variable variable);

private:
    Variable m_variable;
    core::GridSettings m_grid;
};

/// The grid node nearest to `point`, axis by axis (docs/REGOLE-NUMERICHE.md,
/// section 3, point 6): SNAPBASE + std::round((p - SNAPBASE) / SNAPUNIT) *
/// SNAPUNIT, with the SNAPUNIT of X for x and of Y for y. A tie half way
/// between two nodes goes to the node farther from SNAPBASE. Whether the
/// snap is on is not looked at.
[[nodiscard]] core::Vec2 gridNode(const core::GridSettings& grid, const core::Vec2& point);

}  // namespace tg::cmd
