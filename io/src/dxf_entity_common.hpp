// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "dxf_entities.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

/// The properties an entity brings in its AcDbEntity groups, before the
/// names are looked up in the tables (docs/adr/0023, point 2).
struct EntityPropertyGroups {
    std::string layer{"0"};               ///< group 8; "0" when absent
    std::optional<std::string> linetype;  ///< group 6; std::nullopt for ByLayer
    core::Color color{core::Color::byLayer()};
    int lineweight{core::lineweight::kByLayer};
    double linetypeScale{1.0};
    bool lineweightRounded{false};  ///< a 370 that was not an allowed value
};

/// Takes the groups 8, 6, 62, 420, 370 and 48 out of extras.common and
/// returns their values. A group that only repeats the default value
/// (62 256, 6 ByLayer, 370 -1, 48 1.0), and the 62 that goes with a 420,
/// stay in extras.common where they are, so that they are written again as
/// they were while the model keeps that value (docs/adr/0024, point 2; as
/// the zero width 43 of LWPOLYLINE). A 370 that is not allowed is rounded
/// (docs/adr/0025, point 6). With `keepLayer` and `keepLinetype` the 8 and
/// the 6 stay too: they spell the name of their record differently, and are
/// written again as read while they name it. std::nullopt for a 62 outside
/// 0-256, or a 62, 420, 370 or 48 that is not a valid number: the entity
/// stays unhandled.
[[nodiscard]] std::optional<EntityPropertyGroups> takeEntityProperties(core::EntityExtras& extras,
                                                                       bool keepLayer = false,
                                                                       bool keepLinetype = false);

/// The AcDbEntity groups of `record` as the writer puts them out: those of
/// extras.common, with the properties of the model put in by the order of
/// the DXF reference (8, 6, 347, 62, 370, 48, 60, 420, 430, 440, 390, 284).
/// The layer (8) is always written, the other properties only when they
/// differ from the default value; a colour index for an RGB colour is the
/// 62 read with it, if any, otherwise the nearest index. An 8 or 6 kept in
/// extras.common is written as it is while it names the record of the model
/// (other capitals), and replaced by its name otherwise.
[[nodiscard]] core::RawGroups entityCommonGroups(const core::EntityRecord& record,
                                                 const core::Document& document);

/// Group helpers shared by the tables and the entities.
namespace dxf_groups {

/// For every group, true when it belongs to the object itself: outside the
/// 102 {…} groups and before the first XDATA group (1001).
[[nodiscard]] std::vector<bool> ownGroups(const core::RawGroups& groups);

/// The index of the first own group with `code`, if any.
[[nodiscard]] std::optional<std::size_t> firstOwn(const core::RawGroups& groups,
                                                  const std::vector<bool>& own, int code);

/// An integer written as a DXF number ("     7", "-3"), or std::nullopt.
[[nodiscard]] std::optional<long long> parseInteger(std::string_view text);

/// The value of a group 420: 0xRRGGBB (docs/adr/0024, point 2).
[[nodiscard]] int rgbValue(core::Rgb rgb);

/// The colour of a group 420; precondition: 0 <= value <= 0xFFFFFF.
[[nodiscard]] core::Rgb rgbOf(long long value);

/// Puts each group of `additions`, in order, after the last own group of
/// `groups` that comes earlier in `order`; when there is none, after the
/// last own subclass marker (100), or at the start.
void insertInOrder(core::RawGroups& groups, const core::RawGroups& additions,
                   std::span<const int> order);

}  // namespace dxf_groups

extern const EntityCodec kLwpolylineCodec;
extern const EntityCodec kTextCodec;

}  // namespace tg::io
