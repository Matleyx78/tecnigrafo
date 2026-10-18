// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

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

/// Reads an entity written in the format of docs/verifica/rapporto.md
/// ("LWPOLYLINE closed=no vertex=0,0 vertex=10,0"), as writeEntity()
/// (report.hpp) writes it. Properties with a default value may be left out;
/// derived properties, whose name starts with '~', are accepted and ignored.
[[nodiscard]] std::variant<core::Entity, EntityError> readEntity(std::string_view text);

}  // namespace tg::script
