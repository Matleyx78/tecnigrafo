// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"
#include "tg/script/compare.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

/// Scripts of the control mode (docs/adr/0011, docs/verifica/script.md).
namespace tg::script {

/// An error in a file of the control mode, with its line.
struct ParseError {
    int line;  ///< 1-based
    std::string message;
    /// True if the file uses something the catalogue describes but the
    /// program does not have yet (an entity type, a question): the case
    /// fails like any case not passed. False for a malformed file, which
    /// is never a valid case (docs/verifica/casi.md).
    bool notImplemented{false};

    friend bool operator==(const ParseError&, const ParseError&) = default;
};

/// A normal line: the text written on the command line, exactly as in the
/// file, then Enter. An empty line is Enter alone.
struct TypedLine {
    std::string text;
    friend bool operator==(const TypedLine&, const TypedLine&) = default;
};

/// !view cx,cy scale WxH: a fixed view, `scale` drawing units per pixel.
struct ViewDirective {
    core::Vec2 center;
    double scale{1.0};
    int width{0};
    int height{0};
    friend bool operator==(const ViewDirective&, const ViewDirective&) = default;
};

/// !move x,y: the pointer moves to a point of the drawing.
struct MoveDirective {
    core::Vec2 point;
    friend bool operator==(const MoveDirective&, const MoveDirective&) = default;
};

/// !click x,y [remove]: the pointer moves to a point of the drawing and
/// clicks there with the left button, with Shift if `remove`.
struct ClickDirective {
    core::Vec2 point;
    bool remove{false};
    friend bool operator==(const ClickDirective&, const ClickDirective&) = default;
};

/// !rightclick: right click, which confirms like Enter.
struct RightClickDirective {
    friend bool operator==(const RightClickDirective&, const RightClickDirective&) = default;
};

/// !esc: the Esc key.
struct EscDirective {
    friend bool operator==(const EscDirective&, const EscDirective&) = default;
};

/// !snap on|off and !ortho on|off.
struct AidDirective {
    enum class Aid : std::uint8_t { Snap, Ortho };
    Aid aid{Aid::Snap};
    bool on{false};
    friend bool operator==(const AidDirective&, const AidDirective&) = default;
};

/// !scr on|off: AutoCAD script sequences, without history messages.
struct ScriptSequencesDirective {
    bool on{false};
    friend bool operator==(const ScriptSequencesDirective&,
                           const ScriptSequencesDirective&) = default;
};

/// !key <key>: a key or a shortcut ("F3", "Ctrl+Z"). Level 2 only.
struct KeyDirective {
    std::string key;
    friend bool operator==(const KeyDirective&, const KeyDirective&) = default;
};

/// !report: a section of the report with the state at this point.
struct ReportDirective {
    friend bool operator==(const ReportDirective&, const ReportDirective&) = default;
};

/// !screenshot <name>: the view saved as <name>.png. Level 2 only.
struct ScreenshotDirective {
    std::string name;
    friend bool operator==(const ScreenshotDirective&, const ScreenshotDirective&) = default;
};

/// !level 1|2: the level the script needs.
struct LevelDirective {
    int level{1};
    friend bool operator==(const LevelDirective&, const LevelDirective&) = default;
};

/// !tolerance <kind> <value>: a tolerance of the comparison, for the whole
/// script (also kept in Script::tolerances).
struct ToleranceDirective {
    enum class Kind : std::uint8_t { Absolute, Relative, Channel, Pixels };
    Kind kind{Kind::Absolute};
    double value{0.0};
    friend bool operator==(const ToleranceDirective&, const ToleranceDirective&) = default;
};

using Action =
    std::variant<TypedLine, ViewDirective, MoveDirective, ClickDirective, RightClickDirective,
                 EscDirective, AidDirective, ScriptSequencesDirective, KeyDirective,
                 ReportDirective, ScreenshotDirective, LevelDirective, ToleranceDirective>;

/// A parsed .tgs script (docs/verifica/script.md).
struct Script {
    std::vector<Action> actions;  ///< one per line, comments dropped
    int level{1};                 ///< 2 if the script contains "!level 2"
    Tolerances tolerances;        ///< after every "!tolerance" of the script
};

/// Reads a script. UTF-8, with or without BOM, LF or CRLF. A normal line is
/// kept exactly as written, spaces included: only the line ending goes.
/// A directive that needs level 2 without "!level 2" in the script is an
/// error, so a script that parses at level 1 always runs there.
/// A .scr source path (case insensitive) adds an initial !scr on action;
/// an empty path or another extension keeps the interactive sequence.
[[nodiscard]] std::variant<Script, ParseError> parseScript(std::string_view text,
                                                           std::string_view sourceName = {});

/// Reads "!tolerance <kind> <value>" without the leading "!tolerance": the
/// words after it, already split. Shared with the question files.
[[nodiscard]] std::variant<ToleranceDirective, std::string> parseTolerance(
    const std::vector<std::string_view>& arguments);

/// `tolerances` with `directive` applied.
void applyTolerance(const ToleranceDirective& directive, Tolerances& tolerances);

}  // namespace tg::script
