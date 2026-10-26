// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "dxf_entities.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

/// The LAYER and LTYPE tables, the header variables of the current values and
/// the properties of the entities in the DXF (docs/adr/0023, 0024 and 0025;
/// package 0.6-P10).
namespace tg::io {

/// The group left in the raw TABLES section where a LAYER or LTYPE record
/// was read: the writer puts the records of the document back there, in
/// table order (docs/adr/0018, point 2). Its code is outside the DXF range
/// (-5 to 1071), so no file group can be taken for it; its value is empty,
/// so it never matches a table name.
inline constexpr std::int16_t kRecordPlaceholder = -1000;

/// Builds the tables of a document read from a file. It is used in this
/// order: readTables(), readHeader(), entityProperties() for every handled
/// entity, finish().
class TableReader {
public:
    explicit TableReader(core::Document& document)
        : m_document(document), m_seed(document.nextId()) {}

    /// Takes the LAYER and LTYPE records out of the groups of the TABLES
    /// section, leaving a kRecordPlaceholder group in their place. The
    /// linetypes enter the document at once, with their handles; the layers
    /// at finish(). A record with a name that is not valid, or already used
    /// by an earlier record of the same table, or with a colour that is not
    /// valid, stays raw where it is: its handle is reserved and it is
    /// counted. `lines` gives the line of every group, for the warnings.
    void readTables(core::RawGroups& groups, std::span<const std::size_t> lines,
                    std::vector<std::string>& warnings);

    /// Reads $CLAYER, $CECOLOR, $CELTYPE, $CELWEIGHT, $CELTSCALE, $LTSCALE,
    /// $LWDISPLAY and $LWDEFAULT. The value groups of $CLAYER and $CELTYPE,
    /// which are names, leave the raw HEADER: the writer puts the name of
    /// the record back after the variable (renaming a layer must not leave
    /// its old name there). The numbers stay, and are written again as they
    /// were while the document keeps their value, as $INSUNITS.
    void readHeader(core::RawGroups& header);

    /// Takes the properties out of the extras of a handled entity read from
    /// the file, before it enters the document, and gives it the records of
    /// the file it names; finish() gives it those the file lacks. False if
    /// the entity must stay unhandled (a name no table can hold).
    [[nodiscard]] bool entityProperties(core::EntityRecord& record);

    /// Adds the layers of the file, then completes the tables: the
    /// linetypes and the layer every drawing has (ByBlock, ByLayer,
    /// Continuous, layer 0) when the file lacks them, then the records that are named but missing
    /// (counted); gives the entities and the current values their records. Appends one warning per
    /// kind of inconsistency (docs/adr/0023, point 6).
    ///
    /// A file without LAYER and LTYPE tables that names only layer 0 and
    /// ByLayer, with the default current values, gets no record at all:
    /// its entities keep the identifier 0, which stands for the default
    /// values, and the handles of its entities stay those of a file read
    /// before the tables (docs/verifica/domande.md, `layers`: `none` for a
    /// drawing without a layer table).
    ///
    /// `usedHandles` are the handles the file spends. The standard linetypes
    /// the file lacks exist only in the model: they take the lowest free
    /// handles below the first free one, so that the next entity of the
    /// drawing still takes `$HANDSEED`, as in AutoCAD.
    void finish(std::vector<std::string>& warnings,
                const std::unordered_set<std::uint64_t>& usedHandles);

private:
    struct PendingLayer {
        core::Layer layer;
        std::string linetype;
    };

    /// An entity that names a record the file lacks.
    struct PendingEntity {
        core::EntityId id;
        core::EntityProperties properties;
        std::string layer;
        std::optional<std::string> linetype;  ///< std::nullopt for ByLayer
    };

    /// True when the file or what it names asks for the tables.
    [[nodiscard]] bool needsRecords() const;
    [[nodiscard]] std::optional<core::LayerId> knownLayer(std::string_view name) const;

    void readLinetype(core::RawGroups record, std::size_t line, std::vector<std::string>& warnings);
    void readLayer(core::RawGroups record, std::size_t line, std::vector<std::string>& warnings);
    /// The handle of a record, or a new one from the counter, with a warning.
    [[nodiscard]] std::uint64_t recordHandle(core::RawGroups& record, std::string_view type,
                                             std::size_t line, std::vector<std::string>& warnings);
    [[nodiscard]] core::LinetypeId linetypeNamed(std::string_view name);
    [[nodiscard]] core::LayerId layerNamed(std::string_view name);
    [[nodiscard]] bool layerPending(std::string_view name) const;

    core::Document& m_document;
    std::uint64_t m_seed;  ///< the first free handle of the file, before any entity
    std::vector<PendingLayer> m_layers;
    std::vector<PendingEntity> m_entities;
    std::optional<std::string> m_currentLayer;
    std::optional<std::string> m_currentLinetype;
    core::CurrentProperties m_current;
    bool m_layerTable{false};     ///< the file has a LAYER table
    bool m_linetypeTable{false};  ///< the file has an LTYPE table
    int m_missingLayers{0};
    int m_missingLinetypes{0};
    int m_duplicateLayers{0};
    int m_duplicateLinetypes{0};
    int m_invalidLayers{0};
    int m_invalidLinetypes{0};
    int m_roundedLineweights{0};
    int m_replacedLayerLinetypes{0};
};

/// The groups of the TABLES section `tables` with the LAYER and LTYPE
/// records of the document at their placeholders, in table order, and the
/// records without a placeholder at the end of their table; the count (70)
/// of the two tables follows the records added or removed. A LAYER or
/// LTYPE table the section lacks is added when the document needs it
/// (needsLayerTable, needsLinetypeTable), with a handle from `nextHandle`.
void writeTables(const core::RawGroups& tables, const core::Document& document,
                 std::uint64_t& nextHandle, core::RawGroups& out);

/// A TABLES section, without its SECTION and ENDSEC groups, for a file that
/// has none; empty when the document needs none.
[[nodiscard]] core::RawGroups newTables(const core::Document& document, std::uint64_t& nextHandle);

/// How many tables writeTables() or newTables() add to `raw` for
/// `document`, so that their handles can be counted in $HANDSEED before
/// the header is written: 0, 1 or 2.
[[nodiscard]] std::uint64_t tablesToAdd(const core::RawDrawing& raw,
                                        const core::Document& document);

/// True unless the layer table holds only the layer 0 a reader adds to a
/// file without one: then a file without a LAYER table is written without.
[[nodiscard]] bool needsLayerTable(const core::Document& document);

/// True when the document has a linetype other than ByBlock, ByLayer and
/// Continuous, or one of these with a pattern or extras.
[[nodiscard]] bool needsLinetypeTable(const core::Document& document);

/// The HEADER groups `header` with the values of the document for
/// $CLAYER, $CECOLOR, $CELTYPE, $CELWEIGHT, $CELTSCALE, $LTSCALE,
/// $LWDISPLAY and $LWDEFAULT. A value read and not changed keeps its
/// spelling; a variable the header lacks is added at the end when `addAll`
/// (a new drawing or a conversion) or when the document's value differs
/// from that of a new drawing.
[[nodiscard]] core::RawGroups headerProperties(const core::RawGroups& header,
                                               const core::Document& document, bool addAll);

/// The objects of the DXF skeleton of a new drawing, with the fixed handles
/// of core::start_handles when they are free in `document`, otherwise with
/// handles from document.nextId() up, as for a document that already holds
/// those numbers. The LTYPE and LAYER tables hold the records of the
/// document; the records every drawing has are added raw when the document
/// lacks them. nextId is at least start_handles::kFirstEntity.
[[nodiscard]] Result<DxfSkeleton> makeStartSkeleton(const core::Document& document);

}  // namespace tg::io
