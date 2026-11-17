// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_id.hpp"
#include "tg/core/vec2.hpp"

#include <span>
#include <string>

namespace tg::edit {

class Editor;

/// Defines or redefines a block from drawing entities, preserving their
/// records and handles. Returns false without changes if the definition would
/// create a cycle. Precondition: a transaction is open.
bool defineBlock(Editor& editor, std::string name, const core::Vec2& base,
                 std::span<const core::EntityId> ids);

}  // namespace tg::edit
