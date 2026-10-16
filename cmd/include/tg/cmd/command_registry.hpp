// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {

/// The commands known by name ("PLINE") and alias ("PL"). Lookups ignore
/// the case of ASCII letters.
class CommandRegistry {
public:
    using Factory = std::function<std::unique_ptr<Command>()>;

    /// Registers a command. Preconditions: `name` is not empty, `factory` is
    /// not empty, and neither `name` nor any alias is already registered.
    void add(std::string name, std::vector<std::string> aliases, Factory factory);

    /// A new instance of the command, or nullptr if the name is unknown.
    [[nodiscard]] std::unique_ptr<Command> create(std::string_view nameOrAlias) const;

    /// The name the command was registered with, or std::nullopt.
    [[nodiscard]] std::optional<std::string> canonicalName(std::string_view nameOrAlias) const;

private:
    struct Entry {
        std::string name;
        std::vector<std::string> aliases;
        Factory factory;
    };

    [[nodiscard]] const Entry* find(std::string_view nameOrAlias) const;

    std::vector<Entry> m_entries;
};

}  // namespace tg::cmd
