// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/text.hpp"
#include "tg/script/entity_reader.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// The lines of the entity types of stage 0.8 (docs/verifica/rapporto.md,
// "Tappa 0.8"; docs/tappe/0.8/PT9-rapporto-entita-nuove.md): SOLID, ELLIPSE,
// MTEXT, ATTDEF, INSERT, DIMENSION, HATCH, and the ATTRIB of an INSERT.
// Not installed, not public: report.cpp and entity_reader.cpp use them.
namespace tg::script::detail {

/// Appends the properties of a TEXT, without the type, in the order of
/// docs/verifica/rapporto.md ("position=0,0 … value=\"A\""): shared by TEXT,
/// ATTDEF and ATTRIB.
void writeTextProperties(const core::Text& text, std::string& out);

/// Appends the derived ` ~box=…` of a TEXT (docs/verifica/rapporto.md): the
/// corners of core::textBox(), separated by ';'. Shared by TEXT, ATTDEF and
/// ATTRIB.
void writeTextBox(const core::Text& text, std::string& out);

/// Appends the type and the properties of the model of an entity of the
/// seven types (SOLID, ELLIPSE, MTEXT, ATTDEF, INSERT, DIMENSION, HATCH),
/// without the derived ones; nothing for the types of before the 0.8.
void writeNewEntity(const core::Entity& entity, std::string& out);

/// Appends the line of an ATTRIB of the INSERT `owner`, without the derived
/// properties: "ATTRIB owner=300 position=… value=\"-K1\" tag=\"TG_REF\"".
void writeAttribute(const core::Attribute& attribute, core::EntityId owner, std::string& out);

/// Appends the derived properties of the seven types that need no document
/// (the ~box of an ATTDEF), and nothing for the others.
void writeNewDerived(const core::Entity& entity, std::string& out);

/// Appends the derived properties that come from the generated geometry
/// (docs/adr/0044, point 2): ~lines of an MTEXT, ~text of a DIMENSION. They
/// are written only when the type is supported (core::isSupported()), since
/// core::generate() answers from then on; nothing for the other types.
void writeGeneratedDerived(const core::Document& document, const core::EntityRecord& record,
                           std::string& out);

/// The properties of an entity line, after the type ("vertex=0,0").
using PropertyTokens = std::span<const std::string_view>;

/// Reads the properties of an entity of the seven types, without those of
/// stages 0.5 and 0.6, which the caller has taken out. std::nullopt if
/// `type` is none of the seven. The entity is read even when
/// core::isSupported() is false: the caller refuses it.
[[nodiscard]] std::optional<std::variant<core::Entity, EntityError>> readNewEntity(
    std::string_view type, PropertyTokens properties);

/// Reads the properties of a TEXT, without the type, as readEntity() does:
/// shared by TEXT, ATTDEF and ATTRIB. Defined in entity_reader.cpp.
[[nodiscard]] std::variant<core::Text, EntityError> readTextProperties(PropertyTokens properties);

/// Appends one shape of the generated geometry, as the question `generated`
/// answers it (docs/verifica/domande.md): the shape in the format of the
/// entities, then the effective properties that differ from those of an
/// entity with the default properties in a new session, then the derived
/// ones. Defined in report.cpp.
void writeGeneratedItem(const core::GeneratedItem& item, std::string& out);

/// One line of the list "entities:", with the identifier it is sorted by.
struct EntityLine {
    std::uint64_t id{0};
    std::string text;  ///< "257 LINE start=0,0 end=10,0", without indentation
};

/// The lines of "entities:" for `records`, sorted by identifier: every
/// record, and the ATTRIB of every INSERT with their own identifier
/// (docs/verifica/rapporto.md). `document` gives the names of the layers and
/// linetypes and the generated geometry; the records need not be in it.
/// Defined in report.cpp.
[[nodiscard]] std::vector<EntityLine> entityLines(
    std::span<const core::EntityRecord* const> records, const core::Document& document);

/// The items of "statistics:" for `records`: every DXF type, ATTRIB
/// included, in alphabetical order, with its count ("ATTRIB 2").
/// Defined in report.cpp.
[[nodiscard]] std::vector<std::string> statisticsLines(
    std::span<const core::EntityRecord* const> records);

}  // namespace tg::script::detail
