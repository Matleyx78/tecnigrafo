// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/number_format.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <system_error>

// libc++ (Apple Clang, macOS) does not implement std::from_chars for floating
// point before LLVM 20, and does not define __cpp_lib_to_chars until it does.
// There the conversion falls back to a stream imbued with the classic "C"
// locale, as in cmd/src/coordinate_input.cpp. std::to_chars is not used for
// the same reason: the text is built from the digits of a classic stream,
// so it is identical on every platform.

namespace tg::script {
namespace {

// Seventeen significant digits always read back as the same double
// (IEEE 754 binary64), so the search for the shortest text stops there.
constexpr int kMaxSignificantDigits = 17;

// Decimal exponents written in fixed notation, as Python's repr() does:
// 0.0001 is fixed, 0.00001 is "1e-05"; 1e15 is fixed, 1e16 is "1e+16".
constexpr int kMinFixedExponent = -4;
constexpr int kMaxFixedExponent = 15;

constexpr bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

std::size_t skipDigits(std::string_view text, std::size_t& position) {
    const std::size_t start = position;
    while (position < text.size() && isDigit(text[position])) {
        ++position;
    }
    return position - start;
}

/// True if `text` is exactly `[+-] digits [. digits] [(e|E) [+-] digits]`,
/// with at least one mantissa digit. Checking the grammar here makes the
/// accepted syntax the same on every platform.
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

/// `value` in scientific notation with `significantDigits` digits, as
/// "-d.ddde+XX", in the classic locale.
std::string scientific(double value, int significantDigits) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::scientific << std::setprecision(significantDigits - 1) << value;
    return stream.str();
}

/// The decimal digits and the exponent of a text written by scientific():
/// the value is 0.d1 d2 ... dn times 10^(exponent + 1), that is
/// d1.d2...dn times 10^exponent. Trailing zeros are dropped.
struct Decimal {
    bool negative{false};
    std::string digits;
    int exponent{0};
};

Decimal decompose(std::string_view text) {
    Decimal decimal;
    std::size_t position = 0;
    if (text[position] == '-') {
        decimal.negative = true;
        ++position;
    }
    while (position < text.size() && text[position] != 'e') {
        if (isDigit(text[position])) {
            decimal.digits.push_back(text[position]);
        }
        ++position;
    }
    while (decimal.digits.size() > 1 && decimal.digits.back() == '0') {
        decimal.digits.pop_back();
    }
    // The exponent written by the stream: 'e', a sign and the digits.
    int exponent = 0;
    const bool negativeExponent = text[position + 1] == '-';
    for (position += 2; position < text.size(); ++position) {
        exponent = (exponent * 10) + (text[position] - '0');
    }
    decimal.exponent = negativeExponent ? -exponent : exponent;
    return decimal;
}

std::string render(const Decimal& decimal) {
    std::string out = decimal.negative ? "-" : "";
    const std::string& digits = decimal.digits;
    const int exponent = decimal.exponent;
    if (exponent < kMinFixedExponent || exponent > kMaxFixedExponent) {
        out += digits.front();
        if (digits.size() > 1) {
            out += '.';
            out.append(digits, 1);
        }
        out += exponent < 0 ? "e-" : "e+";
        const int magnitude = exponent < 0 ? -exponent : exponent;
        if (magnitude < 10) {
            out += '0';
        }
        out += std::to_string(magnitude);
        return out;
    }
    if (exponent < 0) {
        out += "0.";
        out.append(static_cast<std::size_t>(-exponent - 1), '0');
        out += digits;
        return out;
    }
    const auto integerDigits = static_cast<std::size_t>(exponent) + 1;
    if (digits.size() <= integerDigits) {
        out += digits;
        out.append(integerDigits - digits.size(), '0');
        return out;
    }
    out.append(digits, 0, integerDigits);
    out += '.';
    out.append(digits, integerDigits);
    return out;
}

}  // namespace

std::string formatNumber(double value) {
    if (std::isnan(value)) {
        return "nan";
    }
    if (std::isinf(value)) {
        return value > 0.0 ? "inf" : "-inf";
    }
    if (value == 0.0) {
        return "0";  // both +0 and -0
    }
    // The fewest significant digits that read back as the same value. The
    // stream rounds correctly on every supported platform, so the text with
    // 17 digits always reads back exactly.
    std::string text;
    for (int digits = 1; digits <= kMaxSignificantDigits; ++digits) {
        text = scientific(value, digits);
        const std::optional<double> back = parseNumber(text);
        if (back && *back == value) {
            break;
        }
    }
    return render(decompose(text));
}

std::string formatPoint(const core::Vec2& point) {
    return formatNumber(point.x) + "," + formatNumber(point.y);
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

std::optional<core::Vec2> parsePoint(std::string_view text) {
    const std::size_t comma = text.find(',');
    if (comma == std::string_view::npos) {
        return std::nullopt;
    }
    const std::optional<double> x = parseNumber(text.substr(0, comma));
    const std::optional<double> y = parseNumber(text.substr(comma + 1));
    if (!x || !y) {
        return std::nullopt;
    }
    return core::Vec2{*x, *y};
}

}  // namespace tg::script
