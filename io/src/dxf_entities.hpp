// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/io.hpp"

#include <optional>
#include <string_view>

namespace tg::io {

struct EntityReadContext {
    std::string_view version;
    std::string_view codePage;
    core::EntityId modelSpace;
    core::EntityId id;
    std::uint64_t order{0};
};

struct EntityWriteContext {
    std::string_view version;
    std::string_view codePage;
    core::EntityId modelSpace;
};

struct EntityCodec {
    std::string_view type;
    std::optional<core::EntityRecord> (*read)(const core::RawGroups&, EntityReadContext&);
    void (*write)(const core::EntityRecord&, EntityWriteContext&, core::RawGroups&);
};

[[nodiscard]] const EntityCodec* findCodec(std::string_view type);
[[nodiscard]] const EntityCodec* findCodec(const core::Entity& entity);

/// Applies the shared placement/OCS gate before calling even a known codec.
/// Accepts an explicit codec so the empty registry can be tested independently.
[[nodiscard]] std::optional<core::EntityRecord> decodeEntity(const EntityCodec* codec,
                                                             const core::RawGroups& groups,
                                                             EntityReadContext& context);

[[nodiscard]] std::string_view trimDxf(std::string_view text);
[[nodiscard]] std::optional<std::uint64_t> parseHandle(std::string_view text);
[[nodiscard]] std::string handleText(std::uint64_t value);
[[nodiscard]] std::string_view versionName(DxfVersion version);
[[nodiscard]] core::EntityId modelSpaceHandle(const core::RawDrawing& raw);

struct DxfSkeleton {
    core::RawDrawing raw;
    std::uint64_t nextId{0};
};

/// Allocates every skeleton handle above the document's existing handles.
[[nodiscard]] Result<DxfSkeleton> makeSkeleton(std::uint64_t firstId);

}  // namespace tg::io
