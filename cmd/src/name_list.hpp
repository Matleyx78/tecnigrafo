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
/// ASCII case insensitive, with only '*' and '?' as wildcard operators.
[[nodiscard]] bool matchesName(std::string_view pattern, std::string_view name);
/// Table order, each layer at most once even when patterns overlap.
[[nodiscard]] std::vector<core::LayerId> matchingLayers(const core::Document& document,
                                                      std::string_view text);

}  // namespace tg::cmd::detail
