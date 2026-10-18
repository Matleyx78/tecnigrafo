// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/compare.hpp"

#include "tg/script/number_format.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>

namespace tg::script {
namespace {

constexpr std::string_view kByteOrderMark = "\xEF\xBB\xBF";

constexpr bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool isWordCharacter(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '.';
}

/// A piece of a line: either a number or the text between two numbers.
struct Part {
    std::string_view text;
    std::optional<double> number;
};

/// Length of the number starting at `start`, or 0 if none starts there.
std::size_t numberLength(std::string_view line, std::size_t start) {
    if (start > 0 && isWordCharacter(line[start - 1])) {
        return 0;
    }
    std::size_t position = start;
    if (line[position] == '+' || line[position] == '-') {
        ++position;
    }
    std::size_t digits = 0;
    while (position < line.size() && isDigit(line[position])) {
        ++position;
        ++digits;
    }
    if (position + 1 < line.size() && line[position] == '.' && isDigit(line[position + 1])) {
        ++position;
        while (position < line.size() && isDigit(line[position])) {
            ++position;
            ++digits;
        }
    }
    if (digits == 0) {
        return 0;
    }
    // An exponent counts only if digits follow it: "5e" is a number and a
    // letter.
    if (position < line.size() && (line[position] == 'e' || line[position] == 'E')) {
        std::size_t exponent = position + 1;
        if (exponent < line.size() && (line[exponent] == '+' || line[exponent] == '-')) {
            ++exponent;
        }
        if (exponent < line.size() && isDigit(line[exponent])) {
            while (exponent < line.size() && isDigit(line[exponent])) {
                ++exponent;
            }
            position = exponent;
        }
    }
    return position - start;
}

std::vector<Part> split(std::string_view line) {
    std::vector<Part> parts;
    std::size_t textStart = 0;
    std::size_t position = 0;
    while (position < line.size()) {
        const std::size_t length = numberLength(line, position);
        const std::optional<double> value =
            length > 0 ? parseNumber(line.substr(position, length)) : std::nullopt;
        if (!value) {
            ++position;
            continue;
        }
        if (position > textStart) {
            parts.push_back({line.substr(textStart, position - textStart), std::nullopt});
        }
        parts.push_back({line.substr(position, length), value});
        position += length;
        textStart = position;
    }
    if (textStart < line.size()) {
        parts.push_back({line.substr(textStart), std::nullopt});
    }
    return parts;
}

}  // namespace

bool numbersMatch(double expected, double actual, const Tolerances& tolerances) {
    const double difference = std::abs(expected - actual);
    return difference <= tolerances.absolute ||
           difference <= tolerances.relative * std::max(std::abs(expected), std::abs(actual));
}

bool linesMatch(std::string_view expected, std::string_view actual, const Tolerances& tolerances) {
    if (expected == actual) {
        return true;
    }
    const std::vector<Part> expectedParts = split(expected);
    const std::vector<Part> actualParts = split(actual);
    if (expectedParts.size() != actualParts.size()) {
        return false;
    }
    for (std::size_t i = 0; i < expectedParts.size(); ++i) {
        const Part& wanted = expectedParts[i];
        const Part& got = actualParts[i];
        if (wanted.number.has_value() != got.number.has_value()) {
            return false;
        }
        const bool same = wanted.number ? numbersMatch(*wanted.number, *got.number, tolerances)
                                        : wanted.text == got.text;
        if (!same) {
            return false;
        }
    }
    return true;
}

std::vector<std::string_view> splitLines(std::string_view text) {
    if (text.starts_with(kByteOrderMark)) {
        text.remove_prefix(kByteOrderMark.size());
    }
    std::vector<std::string_view> lines;
    while (!text.empty()) {
        const std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }
        lines.push_back(line);
        if (end == std::string_view::npos) {
            break;
        }
        text.remove_prefix(end + 1);
    }
    return lines;
}

std::vector<Difference> compareReports(std::string_view expected, std::string_view actual,
                                       const Tolerances& tolerances) {
    const std::vector<std::string_view> expectedLines = splitLines(expected);
    const std::vector<std::string_view> actualLines = splitLines(actual);
    std::vector<Difference> differences;
    const std::size_t count = std::max(expectedLines.size(), actualLines.size());
    for (std::size_t i = 0; i < count; ++i) {
        const std::string_view wanted = i < expectedLines.size() ? expectedLines[i] : kNoLine;
        const std::string_view got = i < actualLines.size() ? actualLines[i] : kNoLine;
        const bool bothPresent = i < expectedLines.size() && i < actualLines.size();
        if (!bothPresent || !linesMatch(wanted, got, tolerances)) {
            differences.push_back({static_cast<int>(i) + 1, std::string{wanted}, std::string{got}});
        }
    }
    return differences;
}

}  // namespace tg::script
