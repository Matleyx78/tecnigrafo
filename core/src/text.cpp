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

TextFrame textFrame(const Text& text) {
    TextFrame frame;
    frame.height = text.height;
    frame.widthFactor = text.widthFactor;
    frame.rotation = text.rotation;
    frame.insertion = text.position;
    const double lineWidth = textWidth(displayText(text.value));
    Vec2 offset;
    if (text.horizontal == TextHorizontal::Aligned || text.horizontal == TextHorizontal::Fit) {
        const Vec2 delta = text.alignmentPoint - text.position;
        const double length = delta.length();
        // Guard exact degeneracy only: arbitrarily short baselines remain valid.
        if (length > 0.0 && lineWidth > 0.0) {
            frame.rotation = std::atan2(delta.y, delta.x);
            if (text.horizontal == TextHorizontal::Aligned) {
                frame.height = length / (lineWidth * text.widthFactor);
            } else {
                frame.widthFactor = length / (lineWidth * text.height);
            }
        }
    } else {
        if (text.horizontal != TextHorizontal::Left || text.vertical != TextVertical::Baseline) {
            frame.insertion = text.alignmentPoint;
        }
        const double width = lineWidth * frame.height * frame.widthFactor;
        switch (text.horizontal) {
            case TextHorizontal::Center:
            case TextHorizontal::Middle:
                offset.x = width / 2.0;
                break;
            case TextHorizontal::Right:
                offset.x = width;
                break;
            default:
                break;
        }
        if (text.horizontal == TextHorizontal::Middle) {
            offset.y = frame.height * (1.0 + fontMetrics().descent) / 2.0;
        } else {
            switch (text.vertical) {
                case TextVertical::Baseline:
                    break;
                case TextVertical::Bottom:
                    offset.y = fontMetrics().descent * frame.height;
                    break;
                case TextVertical::Middle:
                    offset.y = frame.height / 2.0;
                    break;
                case TextVertical::Top:
                    offset.y = frame.height;
                    break;
            }
        }
    }
    const Vec2 along{std::cos(frame.rotation), std::sin(frame.rotation)};
    frame.xAxis = along * (text.mirroredX ? -1.0 : 1.0);
    frame.yAxis = frame.xAxis * std::tan(text.obliqueAngle) +
                  along.perpendicular() * (text.mirroredY ? -1.0 : 1.0);
    frame.origin = frame.insertion - frame.xAxis * offset.x - frame.yAxis * offset.y;
    return frame;
}

std::array<Vec2, 4> textBox(const Text& text) {
    const TextFrame frame = textFrame(text);
    const FontMetrics& metrics = fontMetrics();
    const double width = textWidth(displayText(text.value)) * frame.height * frame.widthFactor;
    const double bottom = metrics.descent * frame.height;
    const double top = metrics.ascent * frame.height;
    const auto place = [&](double x, double y) {
        return frame.origin + frame.xAxis * x + frame.yAxis * y;
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
