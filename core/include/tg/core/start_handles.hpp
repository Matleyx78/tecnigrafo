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

// The records of the tables, in the document (core::Document::newDrawing).
inline constexpr std::uint64_t kByBlock = 256;
inline constexpr std::uint64_t kByLayer = 255;
inline constexpr std::uint64_t kContinuous = 254;
inline constexpr std::uint64_t kLayerZero = 253;

// The objects of the DXF skeleton a new drawing is written with (io,
// package 0.6-P10): the tables, in the order of the TABLES section ...
inline constexpr std::uint64_t kTableVport = 252;
inline constexpr std::uint64_t kTableLtype = 251;
inline constexpr std::uint64_t kTableLayer = 250;
inline constexpr std::uint64_t kTableStyle = 249;
inline constexpr std::uint64_t kTableView = 248;
inline constexpr std::uint64_t kTableUcs = 247;
inline constexpr std::uint64_t kTableAppid = 246;
inline constexpr std::uint64_t kTableDimstyle = 245;
inline constexpr std::uint64_t kTableBlockRecord = 244;
// ... the records of the other tables ...
inline constexpr std::uint64_t kVportActive = 243;
inline constexpr std::uint64_t kStyleStandard = 242;
inline constexpr std::uint64_t kAppidAcad = 241;
inline constexpr std::uint64_t kDimstyleStandard = 240;
inline constexpr std::uint64_t kBlockRecordModel = 239;
inline constexpr std::uint64_t kBlockRecordPaper = 238;
// ... the BLOCKS section ...
inline constexpr std::uint64_t kBlockModel = 237;
inline constexpr std::uint64_t kEndBlockModel = 236;
inline constexpr std::uint64_t kBlockPaper = 235;
inline constexpr std::uint64_t kEndBlockPaper = 234;
// ... and the OBJECTS section.
inline constexpr std::uint64_t kDictionaryRoot = 233;
inline constexpr std::uint64_t kDictionaryGroups = 232;
inline constexpr std::uint64_t kDictionaryLayouts = 231;
inline constexpr std::uint64_t kLayoutModel = 230;
inline constexpr std::uint64_t kLayoutPaper = 229;

inline constexpr std::uint64_t kLowest = kLayoutPaper;  ///< the next object goes below
static_assert(kLowest >= 1);

}  // namespace tg::core::start_handles
