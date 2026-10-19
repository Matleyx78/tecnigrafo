// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <string>

namespace tg::cmd {

/// "2.5000": 4 decimals, as AutoCAD shows lengths by default.
/// Independent of the locale; values rounded to zero have no minus sign.
[[nodiscard]] std::string formatLength(double value);

/// Degrees, up to 4 decimals without trailing zeros: "0", "37.5".
/// Independent of the locale; values rounded to zero have no minus sign.
[[nodiscard]] std::string formatAngle(double radians);

}  // namespace tg::cmd
