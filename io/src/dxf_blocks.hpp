// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "dxf_entities.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/// Blocks, inserts and attributes in the DXF (docs/adr/0045, points 1, 2 and
/// 6; package 0.8-PT18): the BLOCKS section and the BLOCK_RECORD table read
/// into core::BlockTable and written from it, INSERT with its ATTRIB and
/// SEQEND, ATTDEF.
namespace tg::io {

// --- Groups kept as read, values from the model -----------------------------

/// A value the model writes in a group of INSERT, ATTDEF or ATTRIB. These
/// codecs keep the groups of their subclasses as read, in extras.specific, so
/// that a file opened and saved is written again with the same groups, in
/// the same order and spelling (docs/adr/0018, point 6); the groups of the
/// model stay there as witnesses of their place and spelling, as the groups
/// of a LAYER record do.
struct ModelGroup {
    enum class Kind : std::uint8_t {
        Text,     ///< written as `text`
        Number,   ///< a number equal to `value`
        Integer,  ///< an integer equal to `value`
        Angle,    ///< degrees in the file, `value` in radians
    };
    std::int16_t code{0};
    Kind kind{Kind::Text};
    std::string text;     ///< what the model writes
    double value{0.0};    ///< the value a group read must say to stay as it is
    bool omitted{false};  ///< a default value: not added when the groups lack it
};

/// Puts the values of the model in `groups`: the first own group of each code
/// keeps its text when it says the value of the model, and takes the text of
/// the model otherwise; the groups that lack a value which is not `omitted`
/// get it after the last own group of an earlier code of `order`
/// (dxf_groups::insertInOrder).
void fillGroups(core::RawGroups& groups, std::span<const ModelGroup> values,
                std::span<const int> order);

/// The group {0, "SEQEND"} that, in the extras.specific of an INSERT, starts
/// the groups of its SEQEND (every group after its group 0, the handle
/// included). Group 0 never belongs to a subclass, so no group read can be
/// taken for it.
inline constexpr std::string_view kSeqendMarker = "SEQEND";

/// The position of the marker of the SEQEND in `specific`, or its size.
[[nodiscard]] std::size_t seqendStart(const core::RawGroups& specific);

// --- Attributes (dxf_attdef.cpp) ---------------------------------------------

/// An ATTRIB from every group after its group 0, as decodeEntity() reads an
/// entity: std::nullopt when it must stay unhandled, and then its INSERT too
/// (docs/adr/0044, point 4): in paper space (67), with an extrusion other
/// than (0,0,±1), multiline (101), or not valid (core::supported). Its
/// owner (330) is not checked: some programs name the layout there.
[[nodiscard]] std::optional<core::Attribute> readAttribute(const core::RawGroups& groups,
                                                           const EntityReadContext& context);

/// The groups of `attribute` after its group 0. extras.common must already
/// hold the AcDbEntity groups to write (entityCommonGroups()); `owner` is
/// written as 330 when the attribute brings none.
void writeAttribute(const core::Attribute& attribute, core::EntityId owner,
                    const EntityWriteContext& context, core::RawGroups& out);

// --- Entities as they stand in a file ----------------------------------------

/// An entity of the file: its type (group 0, as read), its other groups and
/// the line of its group 0.
struct FileEntity {
    std::string type;
    core::RawGroups groups;
    std::size_t line{0};
};

/// An entity of ENTITIES or of a block as the reader takes it: an INSERT
/// with the ATTRIB and the SEQEND that follow it, or any other entity alone.
using EntityItem = std::vector<FileEntity>;

/// Splits `entities` into items: an INSERT takes the ATTRIB that follow it
/// and the SEQEND that ends them. An INSERT whose ATTRIB are not ended by a
/// SEQEND takes them all the same, and stays unhandled (readItem()).
[[nodiscard]] std::vector<EntityItem> groupItems(std::vector<FileEntity> entities);

/// The groups of an item kept unhandled as a whole (core::RawEntity::groups):
/// those of its first entity, then a group 0 and the groups of each other.
[[nodiscard]] core::RawGroups rawGroupsOf(const EntityItem& item);

/// The handled record of an item, or std::nullopt. `context.modelSpace` is
/// the owner its entities must have (*Model_Space, or the BLOCK_RECORD of
/// their block); `blocks` the names of the block definitions (lower case
/// ASCII, as sameTableName compares them). An INSERT stays unhandled with
/// rows or columns (70 or 71 > 1), a block that is not in `blocks`, a scale
/// of zero, or an ATTRIB that is not handled.
[[nodiscard]] std::optional<core::EntityRecord> readItem(
    const EntityItem& item, EntityReadContext& context,
    const std::unordered_set<std::string>& blocks);

/// The key of a block name in the set of readItem(): its ASCII letters in
/// lower case, as sameTableName compares names.
[[nodiscard]] std::string blockKey(std::string_view name);

/// Every handle an item spends: those of its entities, ATTRIB and SEQEND, one
/// for each, in their order (0 for an entity without a valid group 5).
[[nodiscard]] std::vector<std::uint64_t> itemHandles(const EntityItem& item);

// --- Writing -------------------------------------------------------------------

/// The new handles a document needs to be written: a block definition, an
/// entity of a block or an ATTRIB without one (0), and the SEQEND of an
/// INSERT that has attributes and no SEQEND of its own, or that shares it
/// with another (a copy). Built once, before $HANDSEED is written, by
/// planBlocks(); then the writers take the handles from it.
struct BlockWritePlan {
    /// The definitions, in table order, with every handle given.
    std::vector<core::BlockDefinition> blocks;
    /// The INSERT of the drawing whose attributes or SEQEND took a handle,
    /// by identifier, ready to be written.
    std::unordered_map<std::uint64_t, core::Insert> inserts;
    /// The extras.specific of those INSERT, with their SEQEND.
    std::unordered_map<std::uint64_t, core::RawGroups> insertSpecific;
    /// The handle of the BLOCK_RECORD table when the file lacks it and the
    /// document has definitions; 0 otherwise.
    std::uint64_t recordTable{0};
};

/// Gives handles from `nextHandle` on to everything of `document` that needs
/// one (BlockWritePlan) and moves `nextHandle` past them. `raw` is the
/// drawing written (the file read, or the skeleton of a new drawing).
[[nodiscard]] BlockWritePlan planBlocks(const core::Document& document, const core::RawDrawing& raw,
                                        std::uint64_t& nextHandle);

/// The record of a handled entity as the writer puts it out: the AcDbEntity
/// groups of the model in extras.common, its attributes and SEQEND (for an
/// INSERT) from the plan, with their own AcDbEntity groups.
[[nodiscard]] core::EntityRecord recordForWriting(const core::EntityRecord& record,
                                                  const core::Document& document,
                                                  const BlockWritePlan& plan);

/// The groups of the BLOCKS section, without its SECTION and ENDSEC groups:
/// the groups of `section` (as read, or of the skeleton) with the definitions
/// of the plan where the reader took them out, in table order, and the
/// others at the end; the blocks the model does not hold where they were.
[[nodiscard]] core::RawGroups writeBlocks(const core::RawGroups& section,
                                          const core::Document& document,
                                          const BlockWritePlan& plan, std::string_view version,
                                          std::string_view codePage);

/// Puts the BLOCK_RECORD records of the plan in the TABLES groups `tables`
/// (already written): each at the placeholder the reader left with its
/// handle as value, the others at the end of the table, in table order; the
/// count of the table (70) follows. Adds the table when `tables` lacks it
/// and the plan has definitions.
void writeBlockRecords(core::RawGroups& tables, const BlockWritePlan& plan);

/// The handles the document spends in its block definitions and in the
/// attributes and SEQEND of its INSERT, for $HANDSEED (docs/tappe/0.8,
/// PT18, point 6).
[[nodiscard]] std::uint64_t blockHandlesEnd(const core::Document& document);

// --- Reading -------------------------------------------------------------------

/// The BLOCKS section and the BLOCK_RECORD table read into block definitions
/// (docs/tappe/0.8/PT18-blocchi-dxf.md, point 1).
struct BlocksRead {
    /// The definitions, in file order, with their entities read.
    std::vector<core::BlockDefinition> definitions;
    /// For every definition, the items of its INSERT entities, by handle, so
    /// that an INSERT that would make a cycle goes back to its groups.
    std::vector<std::unordered_map<std::uint64_t, EntityItem>> inserts;
    /// The names of the definitions (blockKey()).
    std::unordered_set<std::string> names;
};

/// Takes the properties of a handled entity read in a block, as the reader
/// of ENTITIES does with the tables (TableReader::entityProperties()).
struct BlockEntityReader {
    virtual ~BlockEntityReader() = default;
    BlockEntityReader() = default;
    BlockEntityReader(const BlockEntityReader&) = delete;
    BlockEntityReader& operator=(const BlockEntityReader&) = delete;
    BlockEntityReader(BlockEntityReader&&) = delete;
    BlockEntityReader& operator=(BlockEntityReader&&) = delete;
    /// False if the entity must stay unhandled.
    [[nodiscard]] virtual bool properties(core::EntityRecord& record) = 0;
};

/// Reads the block definitions of the BLOCKS groups `blocks` (without SECTION
/// and ENDSEC) and their records in the TABLES groups `tables`. The groups of
/// a definition stay in `blocks` after a placeholder (kRecordPlaceholder), as
/// the source of what the writer gives back unchanged; its record leaves
/// `tables`, with a placeholder in its place whose value is the handle of
/// the record. The entities of a definition
/// are read as those of the drawing, with `reader` for their properties; an
/// entity without a handle stays unhandled, as written.
///
/// Not in the table, and kept where they are: *Model_Space, *Paper_Space…,
/// the external blocks (70 bit 4, or a path in group 1), the anonymous
/// blocks *D… of the dimensions, and a block without a BLOCK_RECORD, with a
/// name that is not valid or already used, or whose groups cannot be read.
[[nodiscard]] BlocksRead readBlocks(core::RawGroups& blocks, core::RawGroups& tables,
                                    std::string_view version, std::string_view codePage, int units,
                                    BlockEntityReader& reader);

/// Takes the properties of a handled record and of its attributes with
/// `reader`; false if one must stay unhandled. The AcDbEntity groups of an
/// attribute stay in its extras.common as witnesses of their spelling: an
/// ATTRIB is written again as it was read while its properties do not change
/// (attributeCommonGroups()).
[[nodiscard]] bool takeItemProperties(core::EntityRecord& record, BlockEntityReader& reader);

/// The AcDbEntity groups of an attribute as the writer puts them out: those
/// read, while they say the properties of the model, otherwise those of
/// entityCommonGroups().
[[nodiscard]] core::RawGroups attributeCommonGroups(const core::Attribute& attribute,
                                                    const core::Document& document);

/// A warning when an INSERT of the drawing nests blocks deeper than
/// core::kMaxInsertDepth levels: nothing is drawn from there down
/// (docs/tappe/0.8/PT16-blocchi-core.md, point 1).
[[nodiscard]] std::optional<std::string> depthWarning(const core::Document& document);

/// Puts the definitions into `document` in table order. An INSERT that
/// would make a block contain itself goes back to its groups, unhandled
/// (docs/adr/0045, point 1); a warning says how many.
void putBlocks(BlocksRead& read, core::Document& document, std::vector<std::string>& warnings);

}  // namespace tg::io
