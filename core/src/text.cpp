// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text.hpp"

#include "tg/core/text_metrics.hpp"

#include <cmath>
#include <cstddef>
#include <optional>

namespace tg::core {
namespace {

// A DXF control code is "%%" followed by one character (docs/adr/0013,
// point 5).
constexpr std::size_t kControlCodeLength = 3;

/// The character that "%%<code>" stands for, or std::nullopt if the code is
/// not one of those converted in tappa 0.4. AutoCAD accepts both cases.
std::optional<char32_t> controlCharacter(char32_t code) {
    switch (code) {
        case U'c':
        case U'C':
            return U'⌀';  // diameter sign
        case U'd':
        case U'D':
            return U'°';  // degree sign
        case U'p':
        case U'P':
            return U'±';  // plus-minus sign
        case U'%':
            return U'%';
        default:
            return std::nullopt;
    }
}

}  // namespace

BBox Text::boundingBox() const {
    BBox box;
    for (const Vec2& corner : textBox(*this)) {
        box.expand(corner);
    }
    return box;
}

std::array<Vec2, 4> textBox(const Text& text) {
    const FontMetrics& metrics = fontMetrics();
    const double width = textWidth(displayText(text.value)) * text.height * text.widthFactor;
    const double bottom = metrics.descent * text.height;
    const double top = metrics.ascent * text.height;
    const Vec2 along{std::cos(text.rotation), std::sin(text.rotation)};
    const Vec2 across = along.perpendicular();
    const auto place = [&](double x, double y) {
        return text.position + along * x + across * y;
    };
    return {place(0.0, bottom), place(width, bottom), place(width, top), place(0.0, top)};
}

std::u32string displayText(std::string_view value) {
    const std::u32string decoded = decodeUtf8(value);
    std::u32string result;
    result.reserve(decoded.size());
    std::size_t i = 0;
    while (i < decoded.size()) {
        if (i + kControlCodeLength <= decoded.size() && decoded[i] == U'%' &&
            decoded[i + 1] == U'%') {
            if (const std::optional<char32_t> converted = controlCharacter(decoded[i + 2])) {
                result += *converted;
                i += kControlCodeLength;
                continue;
            }
        }
        result += decoded[i];
        ++i;
    }
    return result;
}

}  // namespace tg::core
