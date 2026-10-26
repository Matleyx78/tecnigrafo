// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "name_list.hpp"

#include "text.hpp"

#include <algorithm>
#include <cstddef>

namespace tg::cmd::detail {

std::vector<std::string> splitNameList(std::string_view text) {
    std::vector<std::string> names;
    for (;;) {
        const auto comma = text.find(',');
        names.emplace_back(trim(text.substr(0, comma)));
        if (comma == std::string_view::npos) {
            return names;
        }
        text.remove_prefix(comma + 1);
    }
}

bool matchesName(std::string_view pattern, std::string_view name) {
    std::size_t position = 0;
    std::size_t character = 0;
    std::size_t star = std::string_view::npos;
    std::size_t retry = 0;
    while (character < name.size()) {
        if (position < pattern.size() &&
            (pattern[position] == '?' ||
             toUpperAscii(pattern[position]) == toUpperAscii(name[character]))) {
            ++position;
            ++character;
        } else if (position < pattern.size() && pattern[position] == '*') {
            star = position++;
            retry = character;
        } else if (star != std::string_view::npos) {
            position = star + 1;
            character = ++retry;
        } else {
            return false;
        }
    }
    while (position < pattern.size() && pattern[position] == '*') {
        ++position;
    }
    return position == pattern.size();
}

std::vector<core::LayerId> matchingLayers(const core::Document& document,
                                        std::string_view text) {
    const auto patterns = splitNameList(text);
    std::vector<core::LayerId> ids;
    for (const auto& layer : document.layers()) {
        if (std::ranges::any_of(patterns, [&layer](const std::string& pattern) {
                return matchesName(pattern, layer.name);
            })) {
            ids.push_back(layer.id);
        }
    }
    return ids;
}

}  // namespace tg::cmd::detail
