// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_record.hpp"
#include "tg/core/raw_drawing.hpp"
#include "tg/core/raw_group.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {

/// A block definition (docs/adr/0045, point 1): DXF BLOCK, its entities,
/// ENDBLK and its BLOCK_RECORD. The handles of its entities are in the same
/// space as those of the drawing: Document never gives them again.
struct BlockDefinition {
    std::string name;  ///< 2
    // Every other member has an initializer, so that {"A"} names a block
    // without -Wmissing-field-initializers, which ignores the members that
    // have one; tidy calls {} redundant for those with a constructor.
    // NOLINTBEGIN(readability-redundant-member-init)
    Vec2 base{};                           ///< 10, the point that the insertion point takes
    std::string description{};             ///< 4
    int units{0};                          ///< BLOCK_RECORD 70, INSUNITS of the block
    bool explodable{true};                 ///< BLOCK_RECORD 280
    bool uniformScale{false};              ///< BLOCK_RECORD 281
    bool anonymous{false};                 ///< BLOCK 70 bit 1
    bool hasAttributes{false};             ///< BLOCK 70 bit 2
    std::vector<EntityRecord> entities{};  ///< handled, with their handles, in order
    std::vector<RawEntity> rawEntities{};  ///< unhandled, in order with them (order key)
    std::uint64_t recordId{0};             ///< BLOCK_RECORD handle
    std::uint64_t blockId{0};              ///< BLOCK handle
    std::uint64_t endBlockId{0};           ///< ENDBLK handle
    RawGroups blockExtras{};               ///< groups of BLOCK the model does not use
    RawGroups endBlockExtras{};            ///< groups of ENDBLK the model does not use
    RawGroups recordExtras{};              ///< groups of BLOCK_RECORD the model does not use
    // NOLINTEND(readability-redundant-member-init)

    friend bool operator==(const BlockDefinition&, const BlockDefinition&) = default;
};

/// The block definitions of a drawing, owned by Document, which changes them
/// with Document::replaceBlock() (docs/adr/0044, point 3).
class BlockTable {
public:
    /// The definitions, in table order.
    [[nodiscard]] std::span<const BlockDefinition> definitions() const { return m_definitions; }

    /// The definition named `name` (sameTableName), or nullptr. The pointer
    /// is valid until the table is next changed.
    [[nodiscard]] const BlockDefinition* find(std::string_view name) const;

    /// True when putting `definition` in the table would make a block contain
    /// itself, directly or through nested inserts (docs/adr/0045, point 1):
    /// an insert among its entities names it, or names a block of the table
    /// that contains such an insert at any depth. The definition of the
    /// table with the same name (sameTableName) is the one `definition`
    /// replaces, so its entities do not count.
    [[nodiscard]] bool wouldCycle(const BlockDefinition& definition) const;

    friend bool operator==(const BlockTable&, const BlockTable&) = default;

private:
    friend class Document;

    std::vector<BlockDefinition> m_definitions;  ///< in table order
};

}  // namespace tg::core
