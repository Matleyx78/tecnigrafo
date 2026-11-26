// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd::detail {

/// Keeps empty elements so callers can reject invalid creation lists atomically.
[[nodiscard]] std::vector<std::string> splitNameList(std::string_view text);
/// ASCII case insensitive, with the wildcards of AutoCAD (docs/adr/0056,
/// point 4): '*' any sequence, '?' any character, '#' a digit, '@' a letter,
/// '.' a character that is neither, "[...]" one of the characters listed or
/// in a range ("[A-C]"), "[~...]" one that is not, '`' the next character as
/// it is, and '~' in front of the whole pattern for the names it does not
/// match. A '[' without its ']' is an ordinary character. One pattern: the
/// commas are split by matchesNameList().
[[nodiscard]] bool matchesName(std::string_view pattern, std::string_view name);
/// True when `name` matches one of the patterns of the list `text`
/// (matchesName()). The list is split at the commas, as splitNameList(),
/// except those after a '`': "`," is a pattern for a comma.
[[nodiscard]] bool matchesNameList(std::string_view text, std::string_view name);
/// Table order, each layer at most once even when patterns overlap.
[[nodiscard]] std::vector<core::LayerId> matchingLayers(const core::Document& document,
                                                        std::string_view text);

}  // namespace tg::cmd::detail
