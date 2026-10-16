// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/coordinate_input.hpp"

#include "tg/core/math.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <system_error>

// libc++ (Apple Clang, macOS) does not implement std::from_chars for floating
// point before LLVM 20, and does not define __cpp_lib_to_chars until it does.
// There the conversion falls back to a stream imbued with the classic "C"
// locale, which is just as independent of the system locale.
#if !defined(__cpp_lib_to_chars)
#include <locale>
#include <sstream>
#include <string>
#endif

namespace tg::cmd {
namespace {

constexpr std::string_view kWhiteSpace = " \t\n\v\f\r";

/// Below this length (drawing units) the pointer is on the last point and
/// gives no usable direction for a Distance entry.
constexpr double kMinDirectionLength = 1e-9;

std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(kWhiteSpace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kWhiteSpace);
    return text.substr(first, last - first + 1);
}

constexpr bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

/// Moves `position` past the decimal digits that start there and returns
/// how many they are.
std::size_t skipDigits(std::string_view text, std::size_t& position) {
    const std::size_t start = position;
    while (position < text.size() && isDigit(text[position])) {
        ++position;
    }
    return position - start;
}

/// True if `text` is exactly `[+-] digits [. digits] [(e|E) [+-] digits]`,
/// with at least one digit before or after the decimal point. Checking the
/// grammar here, instead of relying on the conversion function, makes the
/// accepted syntax the same on every platform: no "inf", "nan", hexadecimal
/// numbers or white space.
bool isDecimalNumber(std::string_view text) {
    std::size_t position = 0;
    if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
        ++position;
    }
    std::size_t mantissaDigits = skipDigits(text, position);
    if (position < text.size() && text[position] == '.') {
        ++position;
        mantissaDigits += skipDigits(text, position);
    }
    if (mantissaDigits == 0) {
        return false;
    }
    if (position < text.size() && (text[position] == 'e' || text[position] == 'E')) {
        ++position;
        if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
            ++position;
        }
        if (skipDigits(text, position) == 0) {
            return false;
        }
    }
    return position == text.size();
}

std::optional<double> parseNumber(std::string_view text) {
    if (!isDecimalNumber(text)) {
        return std::nullopt;
    }
    // std::from_chars does not accept a leading '+'.
    if (text.front() == '+') {
        text.remove_prefix(1);
    }
    double value = 0.0;
#if defined(__cpp_lib_to_chars)
    const char* const end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc{} || stop != end) {
        return std::nullopt;
    }
#else
    std::istringstream stream{std::string{text}};
    stream.imbue(std::locale::classic());
    stream >> value;
    if (stream.fail()) {
        return std::nullopt;
    }
#endif
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

/// The two numbers on either side of `separator`, or std::nullopt.
std::optional<PointSpec> parsePair(std::string_view text, std::size_t separator,
                                   PointSpec::Kind kind) {
    const std::optional<double> a = parseNumber(text.substr(0, separator));
    const std::optional<double> b = parseNumber(text.substr(separator + 1));
    if (!a || !b) {
        return std::nullopt;
    }
    return PointSpec{kind, *a, *b};
}

core::Vec2 polar(double distance, double angleDegrees) {
    const double angle = core::degreesToRadians(angleDegrees);
    return {distance * std::cos(angle), distance * std::sin(angle)};
}

}  // namespace

std::optional<PointSpec> parseCoordinate(std::string_view text) {
    text = trim(text);
    const bool relative = text.starts_with('@');
    if (relative) {
        text.remove_prefix(1);
    }

    if (const std::size_t comma = text.find(','); comma != std::string_view::npos) {
        return parsePair(text, comma,
                         relative ? PointSpec::Kind::Relative : PointSpec::Kind::Absolute);
    }
    if (const std::size_t angle = text.find('<'); angle != std::string_view::npos) {
        return parsePair(
            text, angle,
            relative ? PointSpec::Kind::PolarRelative : PointSpec::Kind::PolarAbsolute);
    }
    // A bare number is a distance; "@" followed by a single number is not a
    // form of ADR 0005.
    if (relative) {
        return std::nullopt;
    }
    const std::optional<double> distance = parseNumber(text);
    if (!distance) {
        return std::nullopt;
    }
    return PointSpec{PointSpec::Kind::Distance, *distance, 0.0};
}

std::optional<core::Vec2> resolve(const PointSpec& spec, std::optional<core::Vec2> lastPoint,
                                  core::Vec2 pointer) {
    switch (spec.kind) {
        case PointSpec::Kind::Absolute:
            return core::Vec2{spec.a, spec.b};
        case PointSpec::Kind::PolarAbsolute:
            return polar(spec.a, spec.b);
        case PointSpec::Kind::Relative:
            if (!lastPoint) {
                return std::nullopt;
            }
            return *lastPoint + core::Vec2{spec.a, spec.b};
        case PointSpec::Kind::PolarRelative:
            if (!lastPoint) {
                return std::nullopt;
            }
            return *lastPoint + polar(spec.a, spec.b);
        case PointSpec::Kind::Distance: {
            if (!lastPoint) {
                return std::nullopt;
            }
            const core::Vec2 direction = pointer - *lastPoint;
            const double length = direction.length();
            if (length < kMinDirectionLength) {
                return std::nullopt;
            }
            return *lastPoint + (direction * (spec.a / length));
        }
    }
    return std::nullopt;  // unreachable: every kind is handled above
}

}  // namespace tg::cmd
