// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/color.hpp"
#include "tg/core/document.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/linetype_pattern.hpp"
#include "tg/core/lineweight.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace tg::script {

/// Why readEntity() could not read an entity.
struct EntityError {
    std::string message;
    /// True for a type that docs/verifica/rapporto.md describes but the
    /// program does not have yet.
    bool notImplemented{false};

    friend bool operator==(const EntityError&, const EntityError&) = default;
};

/// The properties of stage 0.6 as a definition writes them
/// (docs/verifica/rapporto.md): layer and linetype by name, because the
/// reader has no document. A property left out is std::nullopt: its default.
struct WrittenProperties {
    std::optional<std::string> layer;
    std::optional<core::Color> color;
    std::optional<std::string> linetype;
    std::optional<double> linetypeScale;
    std::optional<int> lineweight;  ///< an allowed value, kByLayer, kByBlock or kDefault

    friend bool operator==(const WrittenProperties&, const WrittenProperties&) = default;
};

/// An entity of a definition with the properties written after its geometry.
struct EntityDefinition {
    core::Entity entity;
    WrittenProperties properties;
    /// `linetype-generation` of an LWPOLYLINE (docs/adr/0025, point 4).
    /// Read and checked here; core::Polyline receives it with package
    /// 0.6-P10, and until then it cannot be stored nor written.
    std::optional<bool> linetypeGeneration;
};

/// Reads an entity written in the format of docs/verifica/rapporto.md
/// ("LWPOLYLINE closed=no vertex=0,0 vertex=10,0"), as writeEntity()
/// (report.hpp) writes it. Properties with a default value may be left out;
/// derived properties, whose name starts with '~', are accepted and ignored.
/// The properties of stage 0.6 (layer, colour…) are checked and accepted.
[[nodiscard]] std::variant<EntityDefinition, EntityError> readEntityDefinition(
    std::string_view text);

/// readEntityDefinition() without the properties of stage 0.6.
[[nodiscard]] std::variant<core::Entity, EntityError> readEntity(std::string_view text);

/// Reads the properties of stage 0.6 only, in any order, each at most once
/// ("layer=\"Muri\" color=2"): the arguments of the directive !current of a
/// question file (docs/verifica/domande.md).
[[nodiscard]] std::variant<WrittenProperties, EntityError> readProperties(std::string_view text);

/// `written` with the names looked up in `document`: a layer or linetype
/// left out is the default of docs/verifica/rapporto.md ("0", "ByLayer"),
/// as are the other properties. An unknown name is an error.
[[nodiscard]] std::variant<core::EntityProperties, EntityError> resolveProperties(
    const WrittenProperties& written, const core::Document& document);

/// `written` applied over `base`: only the properties written change, as
/// the directive !current does. An unknown name is an error.
[[nodiscard]] std::variant<core::EntityProperties, EntityError> applyProperties(
    const WrittenProperties& written, const core::EntityProperties& base,
    const core::Document& document);

/// A layer as a definition writes it ("LAYER name=\"Muri\" color=1 off"):
/// the values left out are those of a new layer 0 (docs/adr/0023, point 5).
struct LayerDefinition {
    std::string name;
    core::Color color{core::Color::fromIndex(7)};
    std::string linetype{"Continuous"};
    int lineweight{core::lineweight::kDefault};
    bool off{false};
    bool frozen{false};
    bool locked{false};
    bool noplot{false};

    friend bool operator==(const LayerDefinition&, const LayerDefinition&) = default;
};

/// Reads "LAYER name=… [color=…] [linetype=…] [lineweight=…] [off] [frozen]
/// [locked] [noplot]" (docs/verifica/domande.md). The name is checked by
/// core::isValidTableName(); whether it is free is up to the caller.
[[nodiscard]] std::variant<LayerDefinition, EntityError> readLayerDefinition(std::string_view text);

/// A linetype as a definition writes it ("LTYPE name=… pattern=…
/// description=…"). Without pattern, the name must be a standard linetype
/// (core::standardLinetype()), whose pattern and description it takes.
[[nodiscard]] std::variant<core::LinetypeDefinition, EntityError> readLinetypeDefinition(
    std::string_view text);

}  // namespace tg::script
