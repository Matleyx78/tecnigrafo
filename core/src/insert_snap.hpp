// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/generated.hpp"
#include "tg/core/insert.hpp"

#include <vector>

/// The shapes of an INSERT that give snap points. Private to core.
namespace tg::core {

class Document;
struct EntityRecord;

/// The items of generate(document, record) for `insert`, the entity of
/// `record`, in the same order, without those of the hatches of its block
/// and of the nested blocks: the lines of a hatch give no snap points, not
/// even through an insert, as AutoCAD with OSNAPHATCH 0 (docs/adr/0048,
/// point 4). Precondition: supported(insert).
[[nodiscard]] std::vector<GeneratedItem> snapItemsOf(const Document& document,
                                                     const EntityRecord& record,
                                                     const Insert& insert);

}  // namespace tg::core
