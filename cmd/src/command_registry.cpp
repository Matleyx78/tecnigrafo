// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_registry.hpp"

#include "text.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace tg::cmd {

void CommandRegistry::add(std::string name, std::vector<std::string> aliases, Factory factory) {
    assert(!name.empty() && "a command needs a name");
    assert(factory && "a command needs a factory");
    assert(find(name) == nullptr && "command name already registered");
    assert(std::ranges::none_of(aliases,
                                [this](const std::string& alias) {
                                    return alias.empty() || find(alias) != nullptr;
                                }) &&
           "command alias empty or already registered");
    m_entries.push_back(Entry{std::move(name), std::move(aliases), std::move(factory)});
}

std::unique_ptr<Command> CommandRegistry::create(std::string_view nameOrAlias) const {
    const Entry* entry = find(nameOrAlias);
    return entry != nullptr ? entry->factory() : nullptr;
}

std::optional<std::string> CommandRegistry::canonicalName(std::string_view nameOrAlias) const {
    const Entry* entry = find(nameOrAlias);
    return entry != nullptr ? std::optional<std::string>{entry->name} : std::nullopt;
}

const CommandRegistry::Entry* CommandRegistry::find(std::string_view nameOrAlias) const {
    const auto matches = [nameOrAlias](std::string_view candidate) {
        return detail::equalsIgnoringCase(candidate, nameOrAlias);
    };
    const auto it = std::ranges::find_if(m_entries, [&](const Entry& entry) {
        return matches(entry.name) || std::ranges::any_of(entry.aliases, matches);
    });
    return it != m_entries.end() ? &*it : nullptr;
}

}  // namespace tg::cmd
