// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {

/// The commands known by name ("PLINE") and alias ("PL"), each with its
/// description (docs/adr/0026, point 2). Lookups ignore the case of ASCII
/// letters.
class CommandRegistry {
public:
    using Factory = std::function<std::unique_ptr<Command>()>;

    /// Registers a command. Preconditions: the description is valid (name not
    /// empty, summary not empty, unique prompt ids, every keyword in square
    /// brackets in its prompt's text; no prompts at all is valid, for commands
    /// that ask nothing), `factory` is not
    /// empty, and neither the name nor any alias is already registered.
    void add(CommandDescription description, Factory factory);

    /// A new instance of the command, or nullptr if the name is unknown.
    [[nodiscard]] std::unique_ptr<Command> create(std::string_view nameOrAlias) const;

    /// The name the command was registered with, or std::nullopt.
    [[nodiscard]] std::optional<std::string> canonicalName(std::string_view nameOrAlias) const;

    /// Every description, in alphabetical order of name.
    [[nodiscard]] std::vector<const CommandDescription*> descriptions() const;

    /// The description of a command, or nullptr if the name is unknown.
    [[nodiscard]] const CommandDescription* describe(std::string_view nameOrAlias) const;

private:
    struct Entry {
        CommandDescription description;
        Factory factory;
    };

    [[nodiscard]] const Entry* find(std::string_view nameOrAlias) const;

    // A deque keeps the addresses handed out by describe() and
    // descriptions() valid when commands are added.
    std::deque<Entry> m_entries;
};

/// True if `description` satisfies the preconditions of
/// CommandRegistry::add(): name and summary not empty, prompt ids not empty
/// and unique, every keyword of a prompt listed between the square brackets
/// of its text.
[[nodiscard]] bool isValid(const CommandDescription& description);

}  // namespace tg::cmd
