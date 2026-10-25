// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/linetype_pattern.hpp"

#include <array>
#include <cmath>

namespace tg::core {
namespace {
constexpr std::array<std::string_view, 2> kNames{"ACAD_ISO02W100", "ACAD_ISO10W100"};
}  // namespace

double LinetypePattern::length() const {
    double total = 0;
    for (const double element : elements) {
        total += std::abs(element);
    }
    return total;
}

std::optional<LinetypeDefinition> standardLinetype(std::string_view name) {
    std::string upper(name);
    for (auto& ch : upper) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    if (upper == kNames.at(0)) {
        return LinetypeDefinition{
            upper, "ISO dash __ __ __ __ __ __ __ __ __ __ __ __ __", {{12, -3}}};
    }
    if (upper == kNames.at(1)) {
        return LinetypeDefinition{
            upper, "ISO dash dot __ . __ . __ . __ . __ . __ . __", {{12, -3, 0, -3}}};
    }
    return std::nullopt;
}

std::span<const std::string_view> standardLinetypeNames() {
    return kNames;
}
}  // namespace tg::core
