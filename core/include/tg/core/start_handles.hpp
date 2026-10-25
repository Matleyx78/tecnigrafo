// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cstdint>

/// Handles of the objects a new drawing starts with (docs/adr/0023, point 1).
/// Fixed for ever, given downwards from kLastReserved; a new object takes the
/// first free handle below kLowest and becomes the new kLowest.
namespace tg::core::start_handles {

/// Handles 1 to 256 (0x1 to 0x100) belong to the start objects; the
/// entities of a new drawing come after them.
inline constexpr std::uint64_t kLastReserved = 256;
inline constexpr std::uint64_t kFirstEntity = kLastReserved + 1;  // 257
inline constexpr std::uint64_t kByBlock = 256;
inline constexpr std::uint64_t kByLayer = 255;
inline constexpr std::uint64_t kContinuous = 254;
inline constexpr std::uint64_t kLayerZero = 253;
inline constexpr std::uint64_t kLowest = kLayerZero;  ///< P10 goes below
static_assert(kLowest >= 1);

}  // namespace tg::core::start_handles
