// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_id.hpp"
#include "tg/core/raw_group.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tg::core {

/// A section kept as read; the ENTITIES section is a placeholder with no
/// groups, which marks where the entities go (docs/adr/0018, point 2).
struct RawSection {
    std::string name;  ///< "HEADER", "TABLES", …
    RawGroups groups;  ///< without the 0 SECTION / 2 name / 0 ENDSEC groups

    friend bool operator==(const RawSection&, const RawSection&) = default;
};

/// An entity Tecnigrafo does not handle (docs/adr/0018, point 3). It is
/// never drawn nor selected, and stays out of the spatial index.
struct RawEntity {
    EntityId id;  ///< its handle
    std::uint64_t order{0};
    std::string type;  ///< group 0, "HATCH"
    RawGroups groups;  ///< every group after group 0, handle included

    friend bool operator==(const RawEntity&, const RawEntity&) = default;
};

/// A part of a .cadx archive other than mimetype, manifest.json and
/// drawing.dxf, kept byte for byte (extensions.json, thumbnail.png, …).
struct NativePart {
    std::string name;
    std::string bytes;

    friend bool operator==(const NativePart&, const NativePart&) = default;
};

/// What a file brought that the model does not represent.
struct RawDrawing {
    std::string version;               ///< $ACADVER as read ("AC1015"), empty for a new drawing
    std::string codePage;              ///< $DWGCODEPAGE as read, empty if absent
    std::vector<RawSection> sections;  ///< in file order; empty for a new drawing
    std::vector<NativePart> nativeParts;

    friend bool operator==(const RawDrawing&, const RawDrawing&) = default;
};

}  // namespace tg::core
