// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace tg::core {
struct Rgb {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    friend constexpr bool operator==(const Rgb&, const Rgb&) = default;
};

/// Entity or layer colour (ADR 0024).
class Color {
public:
    enum class Kind : std::uint8_t { ByLayer, ByBlock, Index, Rgb };

    [[nodiscard]] static constexpr Color byLayer() { return Color(Kind::ByLayer); }

    [[nodiscard]] static constexpr Color byBlock() { return Color(Kind::ByBlock); }

    /// Precondition: index is in 1..255.
    [[nodiscard]] static Color fromIndex(int index);

    [[nodiscard]] static constexpr Color fromRgb(Rgb rgb) {
        Color color(Kind::Rgb);
        color.m_rgb = rgb;
        return color;
    }

    [[nodiscard]] Kind kind() const { return m_kind; }

    /// Precondition: kind() == Kind::Index.
    [[nodiscard]] int index() const {
        assert(m_kind == Kind::Index);
        return m_index;
    }

    /// Precondition: kind() == Kind::Rgb.
    [[nodiscard]] Rgb rgb() const {
        assert(m_kind == Kind::Rgb);
        return m_rgb;
    }

    friend constexpr bool operator==(const Color&, const Color&) = default;

private:
    constexpr explicit Color(Kind kind) : m_kind(kind) {}

    Kind m_kind;
    int m_index{0};
    Rgb m_rgb;
};

/// AutoCAD Color Index RGB; precondition: index is in 1..255.
[[nodiscard]] Rgb aciToRgb(int index);
/// Nearest index, excluding 7; ties select the smaller index (ADR 0024).
[[nodiscard]] int nearestAci(Rgb rgb);
[[nodiscard]] std::string formatColor(const Color& color);
[[nodiscard]] std::optional<Color> parseColor(std::string_view text);
}  // namespace tg::core
