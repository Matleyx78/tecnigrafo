// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dimension_input.hpp"

#include "tg/core/math.hpp"

#include <variant>

namespace tg::cmd::detail {

bool acceptDimensionText(const Input& input, std::string& textOverride) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        textOverride = text->text;
        return true;
    }
    return std::holds_alternative<ConfirmInput>(input);
}

bool acceptDimensionAngle(const Input& input, const core::Vec2& base,
                          std::optional<double>& rotation) {
    if (const auto* value = std::get_if<ValueInput>(&input)) {
        rotation = value->value;
        return true;
    }
    if (const auto* point = std::get_if<PointInput>(&input)) {
        const core::Vec2 delta = point->point - base;
        rotation = core::atan2(delta.y, delta.x);
        return true;
    }
    return std::holds_alternative<ConfirmInput>(input);
}

}  // namespace tg::cmd::detail
