// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The text of a dimension (docs/adr/0047, point 3; docs/tappe/0.8/PT23,
// point 3).

#include "tg/core/dimension.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace tg::core {
namespace {

// DIMZIN 8 and DIMAZIN 2: no trailing zeros.
constexpr int kLengthTrailingZeros = 8;
constexpr int kAngleTrailingZeros = 2;

/// `value` rounded to a multiple of `step` (> 0), half away from zero.
double roundedTo(double value, double step) {
    const double multiple = std::floor((std::fabs(value) / step) + 0.5) * step;
    return value < 0.0 ? -multiple : multiple;
}

/// `value` with `decimals` fixed decimals, `separator` for the point and,
/// with `noTrailingZeros`, without the zeros at the end of the decimals.
std::string formatted(double value, int decimals, char separator, bool noTrailingZeros) {
    // A finite double with at most 8 decimals (DimStyleTable::inDomain())
    // takes at most 309 digits before the point.
    std::array<char, 400> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value,
                                      std::chars_format::fixed, decimals);
    if (result.ec != std::errc{}) {
        return {};
    }
    std::string text(buffer.data(), result.ptr);
    const std::size_t point = text.find('.');
    if (point != std::string::npos) {
        if (noTrailingZeros) {
            while (text.back() == '0') {
                text.pop_back();
            }
            if (text.back() == '.') {
                text.pop_back();
            }
        }
        if (point < text.size()) {
            text[point] = separator;
        }
    }
    // -0 is written 0, as every value whose digits are all zero.
    if (text.front() == '-' && text.find_first_of("123456789") == std::string::npos) {
        text.erase(0, 1);
    }
    return text;
}

}  // namespace

std::string dimensionText(const DimVariables& variables, DimensionKind kind, double measurement,
                          std::string_view textOverride) {
    if (textOverride == " ") {
        return {};
    }
    std::string value;
    if (kind == DimensionKind::Angular) {
        const int decimals =
            variables.angleDecimals == -1 ? variables.decimals : variables.angleDecimals;
        value = formatted(measurement, decimals, variables.decimalSeparator,
                          (variables.angleZeroSuppression & kAngleTrailingZeros) != 0);
        value += "%%d";
    } else {
        double length = measurement * variables.linearFactor;
        if (variables.round > 0.0) {
            length = roundedTo(length, variables.round);
        }
        value = formatted(length, variables.decimals, variables.decimalSeparator,
                          (variables.zeroSuppression & kLengthTrailingZeros) != 0);
        if (kind == DimensionKind::Radius) {
            value.insert(0, "R");
        } else if (kind == DimensionKind::Diameter) {
            value.insert(0, "%%c");
        }
    }
    constexpr std::string_view kPlace = "<>";
    if (const std::size_t at = variables.post.find(kPlace); at != std::string::npos) {
        std::string post = variables.post;
        post.replace(at, kPlace.size(), value);
        value = std::move(post);
    } else {
        value += variables.post;
    }
    if (textOverride.empty()) {
        return value;
    }
    std::string text{textOverride};
    if (const std::size_t at = text.find(kPlace); at != std::string::npos) {
        text.replace(at, kPlace.size(), value);
    }
    return text;
}

}  // namespace tg::core
