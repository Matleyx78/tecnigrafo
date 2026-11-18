// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/input.hpp"

#include <optional>
#include <string>

namespace tg::cmd::detail {

[[nodiscard]] bool acceptDimensionText(const Input& input, std::string& textOverride);
[[nodiscard]] bool acceptDimensionAngle(const Input& input, const core::Vec2& base,
                                        std::optional<double>& rotation);

}  // namespace tg::cmd::detail
