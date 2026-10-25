// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {
/// ADR 0025: positive dash, negative gap, zero dot, in drawing units.
struct LinetypePattern {
    std::vector<double> elements;

    [[nodiscard]] bool continuous() const { return elements.empty(); }

    [[nodiscard]] double length() const;
    friend bool operator==(const LinetypePattern&, const LinetypePattern&) = default;
};

struct LinetypeDefinition {
    std::string name;
    std::string description;
    LinetypePattern pattern;
};

/// Case-insensitive lookup of the ISO definitions offered for loading.
[[nodiscard]] std::optional<LinetypeDefinition> standardLinetype(std::string_view name);
/// Standard definition names in alphabetical order.
[[nodiscard]] std::span<const std::string_view> standardLinetypeNames();
}  // namespace tg::core
