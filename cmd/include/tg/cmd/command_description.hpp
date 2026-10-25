// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tg::cmd {

/// What a typed answer to a prompt is (docs/adr/0026, point 1). Keyword: the
/// prompt takes only its options.
enum class ValueType : std::uint8_t {
    Point,
    Distance,
    Angle,
    Integer,
    Real,
    Text,
    Name,
    Color,
    Keyword,
    FileOpen,
    FileSave
};

/// One prompt that a command can show.
struct PromptDescription {
    std::string id;    ///< stable, English, kebab-case: "next-point"
    std::string text;  ///< as shown, "{default}" for the value in < >
    ValueType value{ValueType::Point};
    std::vector<Keyword> keywords;
    bool acceptsEmpty{false};  ///< Enter alone is an answer

    friend bool operator==(const PromptDescription&, const PromptDescription&) = default;
};

/// A command as a machine reads it (docs/adr/0026): every prompt it can
/// show, once each, in the order of a normal run; the variants an option
/// opens follow the prompt that opens them. The prompts of the runner
/// ("Select objects:") are not listed: `selectsFirst` stands for them.
struct CommandDescription {
    std::string name;
    std::vector<std::string> aliases;
    std::string summary;  ///< one English sentence, ending with a full stop
    bool selectsFirst{false};
    std::vector<PromptDescription> prompts;

    friend bool operator==(const CommandDescription&, const CommandDescription&) = default;
};

/// True if `prompt`, as a command shows it, matches `description`: the text
/// equal once each "{default}" is replaced by a non-empty text without '<'
/// or '>', the keywords equal and in the same order, and the kind
/// compatible with the value type: a Point, Distance or Angle prompt with
/// the same value type or Keyword; a Text prompt with any value type but
/// Point, Distance and Angle.
[[nodiscard]] bool matches(const PromptDescription& description, const Prompt& prompt);

}  // namespace tg::cmd
