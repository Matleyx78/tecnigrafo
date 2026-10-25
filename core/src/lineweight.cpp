// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/lineweight.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>

namespace tg::core {
bool isAllowedLineweight(int value) {
    return value == lineweight::kByLayer || value == lineweight::kByBlock ||
           value == lineweight::kDefault ||
           std::find(lineweight::kAllowed.begin(), lineweight::kAllowed.end(), value) !=
               lineweight::kAllowed.end();
}

int nearestLineweight(int value) {
    assert(value >= 0);
    int best = 0;
    for (const int candidate : lineweight::kAllowed) {
        if (std::abs(value - candidate) < std::abs(value - best)) {
            best = candidate;
        }
    }
    return best;
}

std::string formatLineweight(int value) {
    assert(isAllowedLineweight(value));
    if (value == lineweight::kByLayer) {
        return "bylayer";
    }
    if (value == lineweight::kByBlock) {
        return "byblock";
    }
    if (value == lineweight::kDefault) {
        return "default";
    }
    if (value == 0) {
        return "0";
    }
    constexpr int kHundredthsPerMillimetre = 100;
    const int fraction = value % kHundredthsPerMillimetre;
    return std::to_string(value / kHundredthsPerMillimetre) + "." + (fraction < 10 ? "0" : "") +
           std::to_string(fraction);
}

std::optional<int> parseLineweight(std::string_view text) {
    constexpr std::string_view kWhitespace = " \t\r\n\f\v";
    const auto first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    text = text.substr(first, text.find_last_not_of(kWhitespace) - first + 1);
    std::string lower(text);
    for (auto& ch : lower) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    if (lower == "bylayer") {
        return lineweight::kByLayer;
    }
    if (lower == "byblock") {
        return lineweight::kByBlock;
    }
    if (lower == "default") {
        return lineweight::kDefault;
    }
    text = lower;
    if (text.ends_with("mm")) {
        text.remove_suffix(2);
    }
    if (text.empty()) {
        return std::nullopt;
    }
    // Use the fixed grammar: command values are decimal millimetres, not exponents.
    bool hasDigit = false;
    bool hasDot = false;
    for (const char ch : text) {
        if (ch >= '0' && ch <= '9') {
            hasDigit = true;
        } else if (ch == '.' && !hasDot) {
            hasDot = true;
        } else {
            return std::nullopt;
        }
    }
    if (!hasDigit) {
        return std::nullopt;
    }
    // Floating-point from_chars is unavailable in the macOS 14 standard library.
    std::istringstream stream{std::string(text)};
    stream.imbue(std::locale::classic());
    double millimetres = 0;
    stream >> std::noskipws >> millimetres;
    if (!stream || !stream.eof() || !std::isfinite(millimetres)) {
        return std::nullopt;
    }
    constexpr double kHundredthsPerMillimetre = 100.0;
    constexpr double kRoundingTolerance = 0.005;
    constexpr double kRoundoff = 8 * std::numeric_limits<double>::epsilon();
    int best = 0;
    double bestDistance = millimetres;
    for (const int candidate : lineweight::kAllowed) {
        const double distance = std::abs(millimetres - candidate / kHundredthsPerMillimetre);
        if (distance < bestDistance - kRoundoff) {
            bestDistance = distance;
            best = candidate;
        }
    }
    if (bestDistance > kRoundingTolerance + kRoundoff) {
        return std::nullopt;
    }
    return best;
}
}  // namespace tg::core
