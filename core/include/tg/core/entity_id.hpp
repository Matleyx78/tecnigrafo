// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cstdint>

namespace tg::core {

/// Stable identifier of an entity inside its document. Never reused.
/// Corresponds to the DXF handle (docs/adr/0006). The value 0 is never a
/// valid identifier.
struct EntityId {
    std::uint64_t value{0};

    friend constexpr bool operator==(const EntityId&, const EntityId&) = default;
};

}  // namespace tg::core
