// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/block_table.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/text_style_table.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/result.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace tg::io {

/// What inserting `source` into `target` as a block brings (docs/adr/0045,
/// point 7). Nothing changes in `target`: the command applies it.
struct BlockImport {
    /// Named as the file without its extension, as written. Its base is
    /// (0,0) and its entities are those of the model space of the file,
    /// in order, moved by -$INSBASE; if $INSBASE is not zero and the model
    /// space has an unhandled entity, the base is $INSBASE and every entity
    /// stays as read. Its units are $INSUNITS of the file. Its own handles
    /// (recordId, blockId, endBlockId) are 0: the writer gives them.
    core::BlockDefinition definition;
    /// The block definitions of the file whose name `target` lacks, in file
    /// order; a block named as `definition` does not enter either.
    std::vector<core::BlockDefinition> nested;
    /// The records of the file whose name `target` lacks (sameTableName), in
    /// table order. The entities and records that used a record `target`
    /// has use the record of `target`.
    std::vector<core::Layer> layers;
    std::vector<core::Linetype> linetypes;
    std::vector<core::TextStyle> textStyles;
    std::vector<core::DimStyle> dimStyles;
    /// The ratio of the units of the file to those of `target` when both
    /// $INSUNITS are not zero and differ, else 1.
    double unitFactor{1.0};
    /// target.nextId() after the handles of everything above, renumbered
    /// from target.nextId() in the order of the file.
    std::uint64_t nextId{0};
};

/// Reads `source` through `files` with readDxf() (same limits, same error
/// messages) and builds what it brings as a block into `target`
/// (docs/tappe/0.8/PT19-file-come-blocco.md).
///
/// Every handle of what enters (entities, attributes, records, blocks) is
/// renumbered from target.nextId() in the order of the file; the groups 5,
/// 330, 340 and 360 of the entities, and 330, 340 and 360 of the records and
/// blocks, that name a renumbered handle take the new one, the others stay
/// as read. The name of the file is not checked as a table name: the
/// command can give the definition another name (`name=path`).
///
/// A file that inserts a block with its own name, directly or through its
/// blocks or those of `target`, is refused with
/// `Block "<name>" references itself.`
[[nodiscard]] Result<BlockImport> importBlock(const std::filesystem::path& source,
                                              const core::Document& target, FileAccess& files);

}  // namespace tg::io
