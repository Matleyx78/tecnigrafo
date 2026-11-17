// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/attribute_definition.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/text.hpp"
#include "tg/core/transform2d.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {

class Document;
class Similarity;
struct EntityRecord;
struct GeneratedItem;
struct GripResult;
struct SnapModes;
struct SnapPoint;
struct TransformOutcome;
enum class MirrorText : std::uint8_t;

/// DXF ATTRIB, owned by its Insert (docs/adr/0045, point 2): it has its own
/// handle, extras and properties, and leaves and comes back with the insert.
/// The flags the model does not represent (verify 4, preset 8) stay in a
/// group 70 of extras.specific, with the whole value of the flags; the
/// locked position (280) in a group 280 there (docs/DECISIONI.md,
/// 2026-11-14, "Revisione delle interfacce PT4", P21).
struct Attribute {
    EntityId id;                  ///< its handle
    Text text;                    ///< text.value is the value (1)
    std::string tag;              ///< 2
    bool invisible{false};        ///< 70 bit 1
    EntityExtras extras;          ///< what the model does not represent
    EntityProperties properties;  ///< layer, colour, linetype, …

    friend bool operator==(const Attribute&, const Attribute&) = default;
};

/// DXF INSERT (docs/adr/0045, points 2 and 4). Angles in radians.
struct Insert {
    std::string block;                  ///< 2, the name of the block definition
    Vec2 insertion;                     ///< 10
    double scaleX{1.0};                 ///< 41
    double scaleY{1.0};                 ///< 42
    double rotation{0.0};               ///< 50
    std::vector<Attribute> attributes;  ///< the ATTRIB that follow it, in order

    friend bool operator==(const Insert&, const Insert&) = default;
};

/// An attribute of an insert in drawing coordinates (docs/adr/0045, point 6):
/// an ATTRIB of the insert, or a constant ATTDEF of its definition.
struct PlacedAttribute {
    std::string tag;
    std::string value;
    AttributeFlags flags;
    Text text;                   ///< in drawing coordinates; text.value is the value
    bool fromDefinition{false};  ///< a constant ATTDEF of the definition

    friend bool operator==(const PlacedAttribute&, const PlacedAttribute&) = default;
};

/// The flags of an ATTRIB: invisible from the model, verify and preset from
/// the group 70 of extras.specific (Attribute); never constant.
[[nodiscard]] AttributeFlags attributeFlags(const Attribute& attribute);

/// True when the text and the tag of the ATTRIB are valid, as for an ATTDEF
/// (validAttributeText(), validAttributeTag()).
[[nodiscard]] bool supported(const Attribute& attribute);

/// The ATTRIBs an insert of `definition` gets with ATTREQ = 0: one per
/// non-constant ATTDEF, in order, with the default value, placed by the
/// insert's transform (transformedText), with layer, color, linetype,
/// lineweight and flags of its ATTDEF. Ids are 0: the editor assigns them.
///
/// The properties are those of the ATTDEF as a whole (its linetype scale
/// too); verify and preset go in a group 70 of extras.specific with the
/// whole value of the flags, and a group 280 of the ATTDEF (locked
/// position) is copied there (Attribute). Nothing without the definition;
/// an ATTDEF that is not supported gives nothing either.
[[nodiscard]] std::vector<Attribute> defaultAttributes(const Document& document,
                                                       const Insert& insert);

/// Every attribute of an insert in drawing coordinates: its ATTRIBs, then
/// the constant ATTDEFs of its definition (fromDefinition), in order.
///
/// A constant ATTDEF is placed by transformedText() under insertTransform(),
/// with its default value as value. The ATTDEFs that are not supported are
/// left out.
[[nodiscard]] std::vector<PlacedAttribute> attributesOf(const Document& document,
                                                        const Insert& insert);

/// The first attribute of attributesOf() whose tag is `tag`, compared without
/// case for ASCII letters (sameTableName()); std::nullopt when none has it.
[[nodiscard]] std::optional<PlacedAttribute> findAttribute(const Document& document,
                                                           const Insert& insert,
                                                           std::string_view tag);

/// The insert ready to be added as a copy (ADR 0031, point 6): every ATTRIB
/// with id 0, so that the editor assigns new handles, and without the 102
/// groups of extras.beforeEntity (reactors and dictionaries of the
/// original), as Editor::addCopy() does for the insert.
[[nodiscard]] Insert insertForCopy(const Insert& insert);

/// The deepest insert that is drawn: the insert of the drawing is level 1,
/// an insert among the entities of its block level 2, and so on. A deeper one
/// is like a reference to a block that is not there: nothing from it down
/// (docs/DECISIONI.md, 2026-11-14, "Revisione delle interfacce PT4", P17).
inline constexpr int kMaxInsertDepth = 64;

/// True when `record` depends on the block definition `name`: an insert of
/// it, or of a block that contains it at any depth. Document::replaceBlock()
/// re-indexes the entities for which it is true.
[[nodiscard]] bool dependsOnBlock(const Document& document, const EntityRecord& record,
                                  std::string_view name);

/// True when the block definition `name` is used: an entity of the drawing,
/// or of another block definition, depends on it (dependsOnBlock()). A block
/// in use is not removed, as AutoCAD's PURGE does not remove it
/// (docs/tappe/0.8/PT16-blocchi-core.md, point 1).
[[nodiscard]] bool isBlockUsed(const Document& document, std::string_view name);

/// The map from the block to the drawing (docs/adr/0045, point 2): subtract
/// the base point of the definition, scale by (X, Y), rotate, move to the
/// insertion point. Without the definition the base point is the origin.
[[nodiscard]] Transform2D insertTransform(const Document& document, const Insert& insert);

/// True when insertTransform() is a similarity: |X| == |Y|.
[[nodiscard]] bool isSimilarity(const Insert& insert);

// The functions of Insert that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(insert).

/// True when both scales are finite and not zero, and every attribute is
/// supported (supported(const Attribute&)).
[[nodiscard]] bool supported(const Insert& insert);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
///
/// For each entity of the definition, in order, its generated geometry
/// mapped by insertTransform(); a nested insert gives that of its own block,
/// the maps composed first and applied once to each shape, so that a circle
/// under a scaling (0.5, 1) and then (2, 1) stays a circle. Its properties
/// are those of propertiesInBlock() and layerState() (docs/adr/0045,
/// point 3). The attribute definitions of the block are not drawn: the
/// attributes of the insert are, after the shapes of its block, as texts with
/// the properties propertiesInBlock() gives them (layer 0 and ByBlock as the
/// entities of the block); an invisible one is not drawn. The ATTRIBs of a
/// nested insert are mapped as the entities of the block that holds it.
/// Nothing from a block that is not there, its attributes included, nor
/// from an insert deeper than kMaxInsertDepth.
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record,
                                                    const Insert& insert);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not, and of the insertion point, so that
/// the snap search finds it even far from the shapes (docs/tappe/0.8,
/// PT16, point 6). Empty on a frozen layer.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const Insert& insert);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const Insert& insert, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one;
/// without a drawn shape (an empty block, or every shape hidden), the
/// insertion point inside `rectangle`, when the layer of the insert is
/// shown (docs/DECISIONI.md, 2026-11-14, "Revisione delle interfacce PT4", P3).
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const Insert& insert, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`; without a
/// drawn shape, the insertion point inside `rectangle`, as isInsideOf().
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const Insert& insert, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives: the insertion
/// point (SnapKind::Insertion), when that mode is enabled, always, even far
/// from the shapes (docs/adr/0045, point 4).
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const Insert& insert, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The entity transformed by `similarity` (docs/adr/0045, point 4): the
/// insertion point mapped, the scales multiplied by its factor, its angle
/// added to the rotation. A reflection across an axis at angle φ turns
/// T(p)·R(r)·S(sx, sy) into T(p')·R(2φ − r + 180°)·S(−sx, sy), as AutoCAD
/// writes it: the X scale changes sign and the rotation is brought into
/// [0°, 360°). Never refused. Each ATTRIB is transformed in the same step as
/// a text, with `mirrorText` (ADR 0031, point 3).
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const Insert& insert,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips: the insertion point alone.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Insert& insert);

/// Moves the insertion point to `target`; Invalid for a target that is not
/// finite. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Insert& insert,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
