// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// The protocol of commands, whatever they work on (docs/adr/0041, point 5):
// prompts, keywords, status and the context every command receives. Nothing
// here belongs to the 2D drawing: this header includes nothing of core, edit
// or io. Inputs, selection, snap, grips, previews and coordinates are 2D and
// stay in command.hpp and in the runner of the drawing.

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {

/// An option of a prompt: {"Close", "C"}. The user can type either string,
/// in any case.
struct Keyword {
    std::string name;
    std::string shortcut;

    friend bool operator==(const Keyword&, const Keyword&) = default;
};

/// File dialogs may replace these text questions in the application (ADR 0021).
enum class FilePrompt : std::uint8_t { None, Open, Save };

/// What a prompt asks for, which decides how CommandRunner reads the answer
/// (docs/adr/0014 and docs/adr/0032):
/// - Point, Distance, Angle: a point, or a bare number for Distance and Angle;
/// - Text: a line, as typed;
/// - Object: one entity, picked with a click or a typed point (ObjectInput);
/// - Number: a pure number, with no unit or angle (ValueInput), or a point;
/// - Selection: a selection built with the runner's gestures and written
///   options, handed over with Enter (SelectionInput).
enum class PromptKind : std::uint8_t { Point, Distance, Angle, Text, Object, Number, Selection };

/// What a command asks the user. `text` is the complete sentence shown on
/// the command line, keywords included ("Specify next point or
/// [Close/Undo]:"): it is never assembled from pieces, so that it can be
/// translated as a whole (docs/adr/0005). `keywords` lists the options the
/// runner recognises in typed text.
struct Prompt {
    std::string text;
    std::vector<Keyword> keywords;
    PromptKind kind{PromptKind::Point};
    FilePrompt file{FilePrompt::None};

    friend bool operator==(const Prompt&, const Prompt&) = default;
};

enum class Status : std::uint8_t { Continue, Finished };

/// What every command needs, whatever it works on (docs/adr/0041, point 5).
struct ProtocolContext {
    std::function<void(std::string)> message;  ///< writes to the history
    bool scriptSequences{false};               ///< AutoCAD script sequences (ADR 0032, points 6-8).
};

/// True if `text` names `keyword`: its name or, if not empty, its shortcut,
/// in any case (ASCII only, so that the result does not depend on the
/// locale). Nothing is trimmed: " C" names no keyword.
[[nodiscard]] bool matchesKeyword(const Keyword& keyword, std::string_view text);

/// The first keyword of `prompt` that `text` names (matchesKeyword()), or
/// nullptr if none. The pointer refers into `prompt.keywords`.
[[nodiscard]] const Keyword* findKeyword(const Prompt& prompt, std::string_view text);

/// The answer that a line typed at a Text prompt hands the command: the line
/// unchanged, spaces included; at a file prompt (`prompt.file` not None)
/// without the double quotes around it, if it begins and ends with one, as
/// a path with spaces is written in AutoCAD scripts.
[[nodiscard]] std::string textAnswer(const Prompt& prompt, std::string_view text);

}  // namespace tg::cmd
