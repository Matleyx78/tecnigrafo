// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_id.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/text.hpp"

#include <optional>
#include <string>
#include <variant>

namespace tg::cmd {

/// What the in-window editor opens on (docs/adr/0054, point 5).
struct TextEditRequest {
    std::optional<core::EntityId> target;
    std::variant<core::Text, core::MText> shown;
    std::string value;
    std::optional<std::string> measurement;

    friend bool operator==(const TextEditRequest&, const TextEditRequest&) = default;
};

}  // namespace tg::cmd
