// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Text helpers shared by the sources of tg_script. Not installed, not public.
namespace tg::script::detail {

inline constexpr std::string_view kBlanks = " \t";

[[nodiscard]] inline std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(kBlanks);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kBlanks);
    return text.substr(first, last - first + 1);
}

/// The words of `text`, separated by runs of spaces and tabs.
[[nodiscard]] inline std::vector<std::string_view> splitWords(std::string_view text) {
    std::vector<std::string_view> words;
    std::size_t position = 0;
    while (true) {
        const std::size_t start = text.find_first_not_of(kBlanks, position);
        if (start == std::string_view::npos) {
            break;
        }
        const std::size_t end = text.find_first_of(kBlanks, start);
        words.push_back(text.substr(start, end == std::string_view::npos ? end : end - start));
        if (end == std::string_view::npos) {
            break;
        }
        position = end;
    }
    return words;
}

/// The tokens of `text`, separated by runs of spaces and tabs outside
/// double quotes: `value="a b" x=1` is two tokens. Inside quotes a
/// backslash escapes the next character. std::nullopt if a quote is not
/// closed.
[[nodiscard]] inline std::optional<std::vector<std::string_view>> splitTokens(
    std::string_view text) {
    std::vector<std::string_view> tokens;
    std::size_t position = 0;
    while (position < text.size()) {
        if (kBlanks.find(text[position]) != std::string_view::npos) {
            ++position;
            continue;
        }
        const std::size_t start = position;
        bool quoted = false;
        while (position < text.size() &&
               (quoted || kBlanks.find(text[position]) == std::string_view::npos)) {
            if (quoted && text[position] == '\\') {
                ++position;  // the escaped character, whatever it is
            } else if (text[position] == '"') {
                quoted = !quoted;
            }
            ++position;
        }
        if (quoted || position > text.size()) {
            return std::nullopt;
        }
        tokens.push_back(text.substr(start, position - start));
    }
    return tokens;
}

}  // namespace tg::script::detail
