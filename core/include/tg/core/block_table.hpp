// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {

/// A block definition (docs/adr/0045, point 1). Only its name for now: the
/// package of the blocks adds the other fields in this file.
struct BlockDefinition {
    std::string name;  ///< 2

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

    friend bool operator==(const BlockTable&, const BlockTable&) = default;

private:
    friend class Document;

    std::vector<BlockDefinition> m_definitions;  ///< in table order
};

}  // namespace tg::core
