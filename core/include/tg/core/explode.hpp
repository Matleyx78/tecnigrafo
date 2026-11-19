// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/raw_drawing.hpp"

#include <string>
#include <string_view>
#include <vector>

// EXPLODE of block references and dimensions (docs/adr/0045, point 4;
// docs/adr/0047, point 4; docs/tappe/0.8/PT29-explode-blocchi-quote.md).
namespace tg::core {

/// What an entity explodes into. The records and the raw entities keep the
/// order key they have in the block definition, or in the graphics of the
/// dimension, so that the caller can put them in that order (the two lists
/// merged by key, a record first on a tie); the editor gives them new
/// identifiers and order keys (Editor::addRecord(), Editor::addRaw()).
struct Exploded {
    std::vector<EntityRecord> entities;  // ids 0; properties of their own
    std::vector<RawEntity> raw;          // unhandled entities of the block, transformed
    std::string refusal;                 // not empty: nothing explodes
};

/// The message of a block that cannot be exploded: its BLOCK_RECORD has
/// 280 = 0, or the drawing has no definition with its name.
[[nodiscard]] std::string cannotExplodeBlock(const std::string& name);

/// The message of a block with an entity that does not survive the
/// transformation: under a non-uniform scale an MTEXT, a dimension or a
/// hatch; under any map an unhandled entity with coordinates (DXF groups 10
/// to 37), a dimension that cannot be regenerated, an entity that is not
/// supported.
[[nodiscard]] std::string blockHasUnexplodableObjects(const std::string& name);

/// The message of a block with a nested insert whose composed map is no
/// longer a scale and a rotation.
[[nodiscard]] std::string blockHasUnrepresentableNestedBlock(const std::string& name);

/// The message of a dimension whose graphics hold unhandled entities.
inline constexpr std::string_view kDimensionHasUnexplodableObjects =
    "Dimension contains objects Tecnigrafo cannot explode.";

/// The entities of the block of `insert` mapped by insertTransform(), in the
/// order of the definition (docs/adr/0045, point 4), each with its own
/// properties and extras, without the complete 102 groups of
/// extras.beforeEntity (reactors and dictionaries of the original).
///
/// Under a similarity (|X| == |Y|) each entity goes through transformedIn()
/// with MIRRTEXT 1, as the insert draws it; a nested insert stays an insert
/// with the composed transformation. Under a non-uniform scale each shape
/// goes through transformedShape(): circles and arcs become ellipses, texts
/// are deformed, a polyline with arcs splits into lines and elliptical arcs
/// and every piece loses its width; an attribute definition stays an
/// attribute definition with its text through transformedText(); a nested
/// insert stays an insert when the columns of the linear part of the
/// composed map are orthogonal, written as rotation, X scale (negative for a
/// reflection) and positive Y scale, and an MTEXT, a dimension or a hatch
/// refuse. The attributes of a nested insert are transformed with it and get
/// id 0 (insertForCopy()); those of `insert` disappear, and the attribute
/// definitions of the block, constant ones included, take their place. A
/// dimension of the block gets graphics with id 0.
///
/// The unhandled entities of the block without coordinates are copied, with
/// id 0; one with coordinates refuses. Refusals, in this order of priority:
/// the block missing or not explodable (cannotExplodeBlock()), an object
/// that cannot be exploded (blockHasUnexplodableObjects()), whatever the
/// order of the entities, and a nested insert that cannot stay one
/// (blockHasUnrepresentableNestedBlock()). Precondition: supported(insert).
[[nodiscard]] Exploded explodeInsert(const Document& document, const Insert& insert);

/// The records of the graphics of `dimension` as they are, in order, with
/// id 0 and without the complete 102 groups of extras.beforeEntity. Their
/// properties are those of the graphics: the caller resolves them with
/// propertiesInBlock() and the properties of the dimension, as AutoCAD (layer
/// 0 and ByBlock become those of the dimension). Graphics with unhandled
/// entities refuse with kDimensionHasUnexplodableObjects, and the
/// dimension stays (docs/DECISIONI.md, 2026-11-14, "Revisione delle
/// interfacce PT4", P24).
[[nodiscard]] Exploded explodeDimension(const Document& document, const Dimension& dimension);

}  // namespace tg::core
