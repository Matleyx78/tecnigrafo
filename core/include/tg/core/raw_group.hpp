// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tg::core {

/// A DXF group as read, never converted (docs/adr/0018, point 1).
///
/// `core` keeps these groups and gives them back unchanged; it never
/// interprets them. Only module `io` knows what a code means.
struct RawGroup {
    std::int16_t code{0};
    std::string value;  ///< UTF-8, exactly as written (numbers stay text)

    friend bool operator==(const RawGroup&, const RawGroup&) = default;
};

/// An ordered sequence of groups, in file order.
using RawGroups = std::vector<RawGroup>;

}  // namespace tg::core
