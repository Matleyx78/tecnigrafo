// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_extras.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// The names of the entity types and the own properties of the types of the
// 0.8 (docs/tappe/0.8.1/PT22-proprieta-core.md): the panel (PT25), the
// questions "properties" and "set-property" and -HATCHEDIT (PT26) read and
// change them through these functions. The functions of each type are in
// entity_properties_shapes.cpp (ELLIPSE, SOLID),
// entity_properties_annotations.cpp (HATCH, DIMENSION) and
// entity_properties_texts.cpp (MTEXT, ATTDEF, INSERT).
namespace tg::core {

class Document;

/// The DXF name of every alternative of core::Entity ("LWPOLYLINE",
/// "ELLIPSE", "ATTDEF", "INSERT"…). An alternative without a name does not
/// compile (#913).
[[nodiscard]] std::string_view entityTypeName(const Entity& entity);

/// How a property is shown and edited.
enum class PropertyKind : std::uint8_t { Number, Distance, Angle, Point, Text, Choice, Flag };

/// Number, Distance and Angle hold a double, Point a Vec2, Text and Choice a
/// string, Flag a bool.
using PropertyValue = std::variant<double, Vec2, std::string, bool>;

struct OwnProperty {
    /// The names of docs/verifica/domande.md ("major-radius"); a derived,
    /// read-only one keeps its prefix "~" ("~area").
    std::string name;
    PropertyKind kind{PropertyKind::Number};
    PropertyValue value;               ///< angles in radians
    std::vector<std::string> choices;  ///< for Choice
    bool readOnly{false};

    friend bool operator==(const OwnProperty&, const OwnProperty&) = default;
};

/// The refusal of a value that is not valid: a radius not greater than 0, a
/// ratio outside (0, 1], a number or point that is not finite, a value of
/// the wrong alternative of PropertyValue.
inline constexpr std::string_view kInvalidPropertyValue = "Invalid value.";
/// The refusal of a name that ownProperties() does not give for the entity.
inline constexpr std::string_view kUnknownProperty = "Unknown property.";
/// The refusal of a change to a read-only property ("~area").
inline constexpr std::string_view kReadOnlyProperty = "Read-only property.";

/// The own properties of the entity, in the order of
/// docs/verifica/domande.md. None for the types of before the 0.8, for which
/// the panel keeps its Geometry group, and for a type whose package has not
/// written them yet (MTEXT, ATTDEF and INSERT until PT24). Precondition:
/// isSupported(record.entity).
[[nodiscard]] std::vector<OwnProperty> ownProperties(const Document& document,
                                                     const EntityRecord& record);

struct PropertyChange {
    std::optional<Entity> entity;
    /// When the change touches them (the origin of a hatch, PT23; the scale
    /// Z of an insert, PT24): the extras of the record after the change.
    std::optional<EntityExtras> extras;
    std::string refusal;  ///< empty when entity has a value
};

/// The entity with the property `name` set to `value`, or a refusal and no
/// entity: kUnknownProperty for a name ownProperties() does not give,
/// kReadOnlyProperty for a read-only one, kInvalidPropertyValue for a value
/// that is not valid. A valid value equal to the current one gives the
/// entity unchanged. Precondition: isSupported(record.entity).
[[nodiscard]] PropertyChange withOwnProperty(const Document& document, const EntityRecord& record,
                                             std::string_view name, const PropertyValue& value);

// The functions of each type of the 0.8 that ownProperties() and
// withOwnProperty() call, one line per type (as docs/adr/0044, point 1).
// withOwnProperty() refuses the unknown and read-only names first: a
// withOwnPropertyOf() receives only the name of a property it can change.

/// ELLIPSE: center, major-radius, minor-radius, ratio, start-angle,
/// end-angle, ~area (docs/tappe/0.8.1/PT22-proprieta-core.md, point 2).
/// The angles are true angles, measured from the major axis as the command
/// ELLIPSE measures them: angle θ is the parameter
/// t = atan2(a · sin θ, b · cos θ), a and b the half axes. The end angle of a
/// whole ellipse is its start angle plus 360°. ~area is π · a · b for a
/// whole ellipse, a · b / 2 · (Δt − sin Δt) for an arc, the segment closed
/// by its chord, with Δt = parameterSpan().
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const Ellipse& ellipse);
/// center moves the ellipse. major-radius sets the length of the major half
/// axis and keeps its direction and the minor one; minor-radius sets the
/// minor half axis and keeps the major one; ratio, in (0, 1], keeps the major
/// one. When the minor half axis becomes longer than the major one, the axes
/// swap as moveGripOf() swaps them (docs/REGOLE-NUMERICHE.md, section 2,
/// point 7): the new major axis has the direction of the old minor one and
/// the parameters of an arc move by -90°. With ratio 1 exactly the major
/// axis goes along +X, as in moveGripOf(). start-angle and end-angle change
/// only that parameter, brought into [0, 2π); an angle whose parameter is
/// the current one modulo 2π gives the ellipse unchanged.
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const Ellipse& ellipse, std::string_view name,
                                               const PropertyValue& value);

/// SOLID: vertex1 to vertex4, in DXF order (groups 10 to 13).
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const Solid& solid);
/// Each vertex moves alone: a fourth vertex equal to the third stays a
/// triangle. A fourth vertex omitted in the file (group 13) becomes present
/// when it no longer equals the third.
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const Solid& solid, std::string_view name,
                                               const PropertyValue& value);

/// HATCH: pattern, scale, angle, origin, islands, ~area
/// (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md, point 2). origin is
/// patternOrigin() of the extras; ~area is filledArea().
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const Hatch& hatch);
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const Hatch& hatch, std::string_view name,
                                               const PropertyValue& value);

/// DIMENSION: style, text, text-rotation (0 for the automatic angle),
/// ~measurement (PT23, point 3). style, text and text-rotation regenerate
/// the dimension with regenerated(); one that cannot be regenerated refuses
/// them with kDimensionNotRegenerable and stays as it was.
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const Dimension& dimension);
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const Dimension& dimension, std::string_view name,
                                               const PropertyValue& value);

/// MTEXT: none until PT24.
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const MText& mtext);
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const MText& mtext, std::string_view name,
                                               const PropertyValue& value);

/// ATTDEF: none until PT24.
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const AttributeDefinition& definition);
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const AttributeDefinition& definition,
                                               std::string_view name, const PropertyValue& value);

/// INSERT: none until PT24.
[[nodiscard]] std::vector<OwnProperty> ownPropertiesOf(const Document& document,
                                                       const EntityRecord& record,
                                                       const Insert& insert);
[[nodiscard]] PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                               const Insert& insert, std::string_view name,
                                               const PropertyValue& value);

}  // namespace tg::core
