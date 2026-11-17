// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text.hpp"

#include <cstdint>
#include <variant>

// The entity and its record, apart from document.hpp so that a block
// definition (block_table.hpp), which the document holds, can hold records.
namespace tg::core {

/// Every drawable entity. New entity types are added to this variant;
/// code that visits entities then fails to compile until it handles them,
/// which is intentional (see docs/adr/0004). The types after Text arrive in
/// the 0.8 (docs/adr/0044, point 1).
using Entity = std::variant<Polyline, Line, Circle, Arc, Text, Solid, Ellipse, MText,
                            AttributeDefinition, Insert, Dimension, Hatch>;

struct EntityRecord {
    EntityId id;
    Entity entity;
    /// What the file brought that the model does not represent; empty for
    /// entities made by commands (docs/adr/0018, point 4).
    EntityExtras extras{};
    /// Order key, shared with the unhandled entities: the drawing and file
    /// order (docs/adr/0018, point 2).
    std::uint64_t order{0};
    /// Layer, colour, linetype, lineweight and linetype scale
    /// (docs/adr/0023, point 2). Last, so that the records built by position
    /// as {id, entity, extras, order} keep their meaning.
    EntityProperties properties{};

    friend bool operator==(const EntityRecord&, const EntityRecord&) = default;
};

}  // namespace tg::core
