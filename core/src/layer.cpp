// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/layer.hpp"

#include <algorithm>
#include <cstddef>

namespace tg::core {

namespace {

/// The longest name, in characters (docs/adr/0023, point 1).
constexpr std::size_t kMaxNameLength = 255;

/// Characters a table name cannot contain (docs/adr/0023, point 1).
constexpr std::string_view kForbidden = "<>/\\\":;?*|,=`";

/// Names are UTF-8: a character is every byte that does not continue one.
std::size_t characterCount(std::string_view text) {
    constexpr unsigned char kContinuationMask = 0xC0;
    constexpr unsigned char kContinuation = 0x80;
    return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) {
        return (static_cast<unsigned char>(c) & kContinuationMask) != kContinuation;
    }));
}

char asciiLower(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

}  // namespace

bool isValidTableName(std::string_view name) {
    if (name.empty() || characterCount(name) > kMaxNameLength) {
        return false;
    }
    if (name.front() == ' ' || name.back() == ' ') {
        return false;
    }
    return name.find_first_of(kForbidden) == std::string_view::npos;
}

bool sameTableName(std::string_view a, std::string_view b) {
    return std::ranges::equal(a, b, [](char x, char y) { return asciiLower(x) == asciiLower(y); });
}

}  // namespace tg::core
