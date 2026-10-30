// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_registry.hpp"

#include "text.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <set>
#include <string_view>
#include <utility>

namespace tg::cmd {
namespace {

constexpr std::string_view kDefault = "{default}";

/// The text between the n-th "{default}" and the next one, for n from 0.
std::vector<std::string_view> literalPieces(std::string_view text) {
    std::vector<std::string_view> pieces;
    std::size_t start = 0;
    for (std::size_t found = text.find(kDefault); found != std::string_view::npos;
         found = text.find(kDefault, start)) {
        pieces.push_back(text.substr(start, found - start));
        start = found + kDefault.size();
    }
    pieces.push_back(text.substr(start));
    return pieces;
}

/// A value put in place of "{default}": not empty, without '<' or '>'.
bool isValue(std::string_view value) {
    return !value.empty() && value.find_first_of("<>") == std::string_view::npos;
}

/// True if `text` is pieces[0], a value, pieces[1], a value, and so on.
/// A value may contain the text that follows it, so every occurrence of the
/// next piece is tried: `starts` holds every position where the current
/// piece may begin.
bool matchesPieces(const std::vector<std::string_view>& pieces, std::string_view text) {
    std::set<std::size_t> starts{0};
    for (std::size_t index = 0; index + 1 < pieces.size(); ++index) {
        std::set<std::size_t> next;
        for (const std::size_t position : starts) {
            if (!text.substr(position).starts_with(pieces[index])) {
                continue;
            }
            const std::size_t valueStart = position + pieces[index].size();
            // One past the value's first character keeps the value non-empty.
            for (std::size_t found = text.find(pieces[index + 1], valueStart + 1);
                 found != std::string_view::npos; found = text.find(pieces[index + 1], found + 1)) {
                if (!isValue(text.substr(valueStart, found - valueStart))) {
                    break;  // a longer value would contain the same '<' or '>'
                }
                next.insert(found);
            }
        }
        starts = std::move(next);
    }
    return std::ranges::any_of(
        starts, [&](std::size_t position) { return text.substr(position) == pieces.back(); });
}

bool matchesText(std::string_view pattern, std::string_view text) {
    return matchesPieces(literalPieces(pattern), text);
}

bool compatible(PromptKind kind, ValueType value) {
    switch (kind) {
        case PromptKind::Point:
            return value == ValueType::Point || value == ValueType::Keyword;
        case PromptKind::Distance:
            return value == ValueType::Distance || value == ValueType::Keyword;
        case PromptKind::Angle:
            return value == ValueType::Angle || value == ValueType::Keyword;
        case PromptKind::Text:
            return value != ValueType::Point && value != ValueType::Distance &&
                   value != ValueType::Angle && value != ValueType::Object &&
                   value != ValueType::Selection;
        case PromptKind::Object:
            return value == ValueType::Object || value == ValueType::Keyword;
        case PromptKind::Number:
            return value == ValueType::Real || value == ValueType::Keyword;
        case PromptKind::Selection:
            return value == ValueType::Selection || value == ValueType::Keyword;
    }
    return false;
}

/// The options between square brackets in `text`: "[Close/Undo]" gives
/// "Close" and "Undo".
std::vector<std::string_view> bracketOptions(std::string_view text) {
    std::vector<std::string_view> options;
    for (std::size_t open = text.find('['); open != std::string_view::npos;
         open = text.find('[', open + 1)) {
        const std::size_t close = text.find(']', open);
        if (close == std::string_view::npos) {
            break;
        }
        std::string_view list = text.substr(open + 1, close - open - 1);
        for (std::size_t slash = list.find('/'); slash != std::string_view::npos;
             slash = list.find('/')) {
            options.push_back(list.substr(0, slash));
            list.remove_prefix(slash + 1);
        }
        options.push_back(list);
    }
    return options;
}

bool isValid(const PromptDescription& prompt) {
    if (prompt.id.empty() || prompt.text.empty()) {
        return false;
    }
    const std::vector<std::string_view> options = bracketOptions(prompt.text);
    return std::ranges::all_of(prompt.keywords, [&options](const Keyword& keyword) {
        return !keyword.name.empty() && std::ranges::find(options, keyword.name) != options.end();
    });
}

}  // namespace

bool matches(const PromptDescription& description, const Prompt& prompt) {
    return description.keywords == prompt.keywords && compatible(prompt.kind, description.value) &&
           matchesText(description.text, prompt.text);
}

bool isValid(const CommandDescription& description) {
    if (description.name.empty() || description.summary.empty()) {
        return false;
    }
    std::set<std::string_view> ids;
    return std::ranges::all_of(description.prompts, [&ids](const PromptDescription& prompt) {
        return isValid(prompt) && ids.insert(prompt.id).second;
    });
}

void CommandRegistry::add(CommandDescription description, Factory factory) {
    assert(isValid(description) && "invalid command description");
    assert(factory && "a command needs a factory");
    assert(find(description.name) == nullptr && "command name already registered");
    assert(std::ranges::none_of(description.aliases,
                                [this](const std::string& alias) {
                                    return alias.empty() || find(alias) != nullptr;
                                }) &&
           "command alias empty or already registered");
    m_entries.push_back(Entry{std::move(description), std::move(factory)});
}

std::unique_ptr<Command> CommandRegistry::create(std::string_view nameOrAlias) const {
    const Entry* entry = find(nameOrAlias);
    return entry != nullptr ? entry->factory() : nullptr;
}

std::optional<std::string> CommandRegistry::canonicalName(std::string_view nameOrAlias) const {
    const Entry* entry = find(nameOrAlias);
    return entry != nullptr ? std::optional<std::string>{entry->description.name} : std::nullopt;
}

std::vector<const CommandDescription*> CommandRegistry::descriptions() const {
    std::vector<const CommandDescription*> result;
    result.reserve(m_entries.size());
    for (const Entry& entry : m_entries) {
        result.push_back(&entry.description);
    }
    std::ranges::sort(result, {}, &CommandDescription::name);
    return result;
}

const CommandDescription* CommandRegistry::describe(std::string_view nameOrAlias) const {
    const Entry* entry = find(nameOrAlias);
    return entry != nullptr ? &entry->description : nullptr;
}

const CommandRegistry::Entry* CommandRegistry::find(std::string_view nameOrAlias) const {
    const auto matches = [nameOrAlias](std::string_view candidate) {
        return detail::equalsIgnoringCase(candidate, nameOrAlias);
    };
    const auto it = std::ranges::find_if(m_entries, [&](const Entry& entry) {
        return matches(entry.description.name) ||
               std::ranges::any_of(entry.description.aliases, matches);
    });
    return it != m_entries.end() ? &*it : nullptr;
}

}  // namespace tg::cmd
