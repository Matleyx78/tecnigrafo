// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/color.hpp"
#include "tg/core/linetype_pattern.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/raw_group.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace tg::core {

/// The handle of a LAYER table record (docs/adr/0023, point 1). Like
/// EntityId, it is never reused, and 0 is never a valid identifier.
struct LayerId {
    std::uint64_t value{0};

    friend constexpr bool operator==(const LayerId&, const LayerId&) = default;
};

/// The handle of an LTYPE table record (docs/adr/0023, point 1).
struct LinetypeId {
    std::uint64_t value{0};

    friend constexpr bool operator==(const LinetypeId&, const LinetypeId&) = default;
};

/// A record of the linetype table (docs/adr/0023 and 0025).
struct Linetype {
    LinetypeId id;
    std::string name;
    std::string description;
    LinetypePattern pattern;
    RawGroups extras;  ///< what the model does not represent, as read

    friend bool operator==(const Linetype&, const Linetype&) = default;
};

/// A record of the layer table (docs/adr/0023, point 1).
struct Layer {
    LayerId id;
    std::string name;
    Color color{Color::fromIndex(7)};      ///< Index or Rgb only
    LinetypeId linetype;                   ///< never ByLayer nor ByBlock
    int lineweight{lineweight::kDefault};  ///< allowed value or kDefault
    bool on{true};
    bool frozen{false};
    bool locked{false};
    bool plot{true};
    RawGroups extras;  ///< what the model does not represent, as read

    friend bool operator==(const Layer&, const Layer&) = default;
};

/// The properties every entity has (docs/adr/0023, point 2).
struct EntityProperties {
    LayerId layer;
    Color color{Color::byLayer()};
    LinetypeId linetype;  ///< may be the ByLayer or ByBlock record
    int lineweight{lineweight::kByLayer};
    double linetypeScale{1.0};  ///< DXF 48

    friend bool operator==(const EntityProperties&, const EntityProperties&) = default;
};

/// $CLAYER, $CECOLOR, $CELTYPE, $CELWEIGHT, $CELTSCALE (docs/adr/0023, point 3).
using CurrentProperties = EntityProperties;

/// docs/adr/0023, point 1: not empty, at most 255 characters, none of
/// < > / \ " : ; ? * | , = `, no leading or trailing blank.
[[nodiscard]] bool isValidTableName(std::string_view name);

/// Names compare without case for ASCII letters, as in AutoCAD.
[[nodiscard]] bool sameTableName(std::string_view a, std::string_view b);

}  // namespace tg::core
