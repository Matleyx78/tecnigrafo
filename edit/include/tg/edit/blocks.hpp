// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/block_table.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/raw_group.hpp"
#include "tg/core/text_style_table.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace tg::edit {

class Editor;

/// Defines or redefines a block from drawing entities, preserving their
/// records and handles. Returns false without changes if the definition would
/// create a cycle. Precondition: a transaction is open.
bool defineBlock(Editor& editor, std::string name, const core::Vec2& base,
                 std::span<const core::EntityId> ids);

/// What a file brings as a block, already renumbered for the drawing it
/// enters: the fields of io::BlockImport (docs/tappe/0.8/PT19) but its unit
/// factor, which only the command uses. `edit` does not depend on `io`, so
/// the command copies them here.
struct BlockImport {
    /// The definition to define or redefine, named as the command wants it.
    core::BlockDefinition definition;
    /// The blocks of the file that the drawing lacks, in file order.
    std::vector<core::BlockDefinition> nested;
    /// The table records that the drawing lacks, with their new handles.
    std::vector<core::Layer> layers;
    std::vector<core::Linetype> linetypes;
    std::vector<core::TextStyle> textStyles;
    std::vector<core::DimStyle> dimStyles;
    /// The handle counter after every handle above.
    std::uint64_t nextId{0};
};

/// Adds `insert` with the attributes of its definition (core::defaultAttributes(),
/// ATTREQ = 0), replacing those it has, and returns its identifier. The insert
/// takes the next handle, then each attribute the following ones, in order.
/// `specific` becomes its extras.specific: the groups the model does not
/// represent, such as the Z scale (43) that -INSERT accepts and keeps.
/// Precondition: a transaction is open; core::supported(insert).
core::EntityId addInsert(Editor& editor, core::Insert insert, core::RawGroups specific = {});

/// Applies `import` in the open transaction, so that one undo step takes it
/// all back (docs/adr/0045, point 7): the linetypes, layers, text styles and
/// dimension styles whose names the drawing lacks, with their handles; the
/// nested blocks the drawing lacks; then the definition, new or, with
/// `redefine`, in place of the block with its name, which keeps its position,
/// its spelling and the handles of its own records (BLOCK_RECORD, BLOCK,
/// ENDBLK). The handle counter moves past import.nextId, and is not moved
/// back by undo: handles are never reused.
///
/// Returns false, leaving the transaction as it was, if the block exists and
/// `redefine` is false, or if the definition would contain itself.
/// Precondition: a transaction is open.
bool applyImport(Editor& editor, BlockImport import, bool redefine);

}  // namespace tg::edit
