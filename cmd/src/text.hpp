// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

// Text helpers shared by the sources of tg_cmd. Not installed, not public.
namespace tg::cmd::detail {

inline constexpr std::string_view kWhiteSpace = " \t\n\v\f\r";

[[nodiscard]] inline std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(kWhiteSpace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kWhiteSpace);
    return text.substr(first, last - first + 1);
}

/// ASCII-only on purpose: command names and keywords are ASCII, and the
/// result must not depend on the locale of the system.
[[nodiscard]] constexpr char toUpperAscii(char c) {
    return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

[[nodiscard]] constexpr bool equalsIgnoringCase(std::string_view lhs, std::string_view rhs) {
    return std::ranges::equal(lhs, rhs,
                              [](char a, char b) { return toUpperAscii(a) == toUpperAscii(b); });
}

}  // namespace tg::cmd::detail
