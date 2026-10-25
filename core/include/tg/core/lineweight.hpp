// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace tg::core {
namespace lineweight {
inline constexpr int kByLayer = -1;
inline constexpr int kByBlock = -2;
inline constexpr int kDefault = -3;
/// DXF group 370 values, in hundredths of a millimetre (ADR 0025).
inline constexpr std::array<int, 24> kAllowed{0,  5,   9,   13,  15,  18,  20,  25,
                                              30, 35,  40,  50,  53,  60,  70,  80,
                                              90, 100, 106, 120, 140, 158, 200, 211};
}  // namespace lineweight

[[nodiscard]] bool isAllowedLineweight(int value);
/// Precondition: value >= 0. Ties select the smaller allowed value.
[[nodiscard]] int nearestLineweight(int value);
/// Precondition: isAllowedLineweight(value).
[[nodiscard]] std::string formatLineweight(int value);
/// Millimetres, optionally suffixed by mm; maximum rounding distance 0.005 mm.
[[nodiscard]] std::optional<int> parseLineweight(std::string_view text);
}  // namespace tg::core
