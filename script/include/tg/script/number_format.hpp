// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <optional>
#include <string>
#include <string_view>

/// Numbers of reports and question files (docs/verifica/rapporto.md). The
/// text depends neither on the locale nor on the platform: the same double
/// is written the same way everywhere.
namespace tg::script {

/// Shortest text that reads back as the same double; "0" for -0.
/// Independent of the locale.
///
/// Fixed notation, without a trailing ".0" ("100", "0.1", "-2.5"), for
/// decimal exponents from -4 to 15; scientific notation otherwise, with a
/// sign and at least two exponent digits ("1e+16", "1e-05", "1.5e+300"),
/// as Python's repr() does. Infinities are "inf" and "-inf", NaN is "nan".
[[nodiscard]] std::string formatNumber(double value);

/// `point` as "x,y", each number written by formatNumber().
[[nodiscard]] std::string formatPoint(const core::Vec2& point);

/// Reads a finite number written as `[+-] digits [. digits] [(e|E) [+-]
/// digits]`, with at least one digit before or after the point: the syntax
/// of coordinates on the command line (docs/adr/0005). No white space,
/// "inf", "nan" or hexadecimal numbers. Independent of the locale.
[[nodiscard]] std::optional<double> parseNumber(std::string_view text);

/// Reads "x,y" with two numbers of parseNumber() and no white space.
[[nodiscard]] std::optional<core::Vec2> parsePoint(std::string_view text);

}  // namespace tg::script
