// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "name_list.hpp"

#include "text.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

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

namespace {

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isLetter(char c) {
    const char upper = toUpperAscii(c);
    return upper >= 'A' && upper <= 'Z';
}

/// One element of a pattern: '*', or what a single character must be.
struct Element {
    enum class Kind : std::uint8_t { Star, Any, Digit, Letter, Other, Literal, Class };
    Kind kind{Kind::Literal};
    char literal{'\0'};
    std::vector<std::pair<char, char>> ranges;  ///< Class: uppercase, inclusive
    bool negated{false};                        ///< Class: "[~...]"

    [[nodiscard]] bool admits(char c) const {
        switch (kind) {
            case Kind::Star:
            case Kind::Any:
                return true;
            case Kind::Digit:
                return isDigit(c);
            case Kind::Letter:
                return isLetter(c);
            case Kind::Other:
                return !isDigit(c) && !isLetter(c);
            case Kind::Literal:
                return toUpperAscii(c) == toUpperAscii(literal);
            case Kind::Class:
                break;
        }
        const char upper = toUpperAscii(c);
        const bool listed = std::ranges::any_of(ranges, [upper](const auto& range) {
            return upper >= range.first && upper <= range.second;
        });
        return listed != negated;
    }
};

/// The class that begins at pattern[open] == '[', and the position after its
/// ']'; std::nullopt without a ']'.
std::optional<std::pair<Element, std::size_t>> parseClass(std::string_view pattern,
                                                          std::size_t open) {
    Element element;
    element.kind = Element::Kind::Class;
    std::size_t position = open + 1;
    if (position + 1 < pattern.size() && pattern[position] == '~' && pattern[position + 1] != ']') {
        element.negated = true;
        ++position;
    }
    while (position < pattern.size() && pattern[position] != ']') {
        const char first = toUpperAscii(pattern[position]);
        if (position + 2 < pattern.size() && pattern[position + 1] == '-' &&
            pattern[position + 2] != ']') {
            element.ranges.emplace_back(first, toUpperAscii(pattern[position + 2]));
            position += 3;
        } else {
            element.ranges.emplace_back(first, first);
            ++position;
        }
    }
    if (position == pattern.size()) {
        return std::nullopt;
    }
    return std::pair{std::move(element), position + 1};
}

std::vector<Element> parsePattern(std::string_view pattern) {
    std::vector<Element> elements;
    std::size_t position = 0;
    while (position < pattern.size()) {
        const char c = pattern[position];
        Element element;
        switch (c) {
            case '*':
                element.kind = Element::Kind::Star;
                break;
            case '?':
                element.kind = Element::Kind::Any;
                break;
            case '#':
                element.kind = Element::Kind::Digit;
                break;
            case '@':
                element.kind = Element::Kind::Letter;
                break;
            case '.':
                element.kind = Element::Kind::Other;
                break;
            case '`':
                if (position + 1 < pattern.size()) {
                    ++position;
                }
                element.literal = pattern[position];
                break;
            case '[':
                if (auto parsed = parseClass(pattern, position)) {
                    elements.push_back(std::move(parsed->first));
                    position = parsed->second;
                    continue;
                }
                element.literal = c;
                break;
            default:
                element.literal = c;
                break;
        }
        elements.push_back(std::move(element));
        ++position;
    }
    return elements;
}

/// The patterns of `text`, split at the commas that are not after a '`'
/// (docs/adr/0056, point 4): each one keeps its '`', and loses the white
/// space around it except what a '`' makes part of it.
std::vector<std::string_view> splitPatterns(std::string_view text) {
    std::vector<std::string_view> patterns;
    std::size_t first = std::string_view::npos;  // the first character kept
    std::size_t end = 0;                         // after the last one kept
    for (std::size_t position = 0; position <= text.size(); ++position) {
        if (position == text.size() || text[position] == ',') {
            patterns.push_back(first == std::string_view::npos ? std::string_view{}
                                                               : text.substr(first, end - first));
            first = std::string_view::npos;
            continue;
        }
        const bool escape = text[position] == '`' && position + 1 < text.size();
        const std::size_t last = escape ? position + 1 : position;
        if (escape || kWhiteSpace.find(text[position]) == std::string_view::npos) {
            if (first == std::string_view::npos) {
                first = position;
            }
            end = last + 1;
        }
        position = last;
    }
    return patterns;
}

bool matchesElements(const std::vector<Element>& elements, std::string_view name) {
    std::size_t position = 0;
    std::size_t character = 0;
    std::size_t star = std::string_view::npos;
    std::size_t retry = 0;
    while (character < name.size()) {
        if (position < elements.size() && elements[position].kind == Element::Kind::Star) {
            star = position++;
            retry = character;
        } else if (position < elements.size() && elements[position].admits(name[character])) {
            ++position;
            ++character;
        } else if (star != std::string_view::npos) {
            position = star + 1;
            character = ++retry;
        } else {
            return false;
        }
    }
    while (position < elements.size() && elements[position].kind == Element::Kind::Star) {
        ++position;
    }
    return position == elements.size();
}

}  // namespace

bool matchesName(std::string_view pattern, std::string_view name) {
    if (pattern.size() > 1 && pattern.front() == '~') {
        return !matchesElements(parsePattern(pattern.substr(1)), name);
    }
    return matchesElements(parsePattern(pattern), name);
}

bool matchesNameList(std::string_view text, std::string_view name) {
    return std::ranges::any_of(splitPatterns(text), [name](std::string_view pattern) {
        return matchesName(pattern, name);
    });
}

std::vector<core::LayerId> matchingLayers(const core::Document& document, std::string_view text) {
    std::vector<core::LayerId> ids;
    for (const auto& layer : document.layers()) {
        if (matchesNameList(text, layer.name)) {
            ids.push_back(layer.id);
        }
    }
    return ids;
}

}  // namespace tg::cmd::detail
