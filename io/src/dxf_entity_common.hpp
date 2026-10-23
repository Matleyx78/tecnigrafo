// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "dxf_entities.hpp"

#include <span>

namespace tg::io {

/// Subclass groups are returned separately so each codec can consume its own
/// properties and move only unrepresented groups into extras.specific.
struct EntityParts {
    core::EntityExtras extras;
    core::RawGroups geometry;
};

[[nodiscard]] EntityParts splitEntityGroups(const core::RawGroups& groups);
/// Writes handle, owner/reactors and AcDbEntity. Does not write group 0.
void writeEntityCommon(const core::EntityRecord& record, const EntityWriteContext& context,
                       core::RawGroups& out);
/// Emits preserved placement groups at the position required by the codec.
void writePlacement(const core::EntityExtras& extras, std::span<const int> codes,
                    core::RawGroups& out);
/// Copies unknown subclass groups and XDATA, optionally skipping a flag group
/// already merged into the model's flags by the codec.
void writeEntityTail(const core::EntityExtras& extras, core::RawGroups& out, int flagCode = 0);
[[nodiscard]] int preservedFlags(const core::EntityExtras& extras, int code);

extern const EntityCodec kLwpolylineCodec;
extern const EntityCodec kTextCodec;

}  // namespace tg::io
