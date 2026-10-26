// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstdint>
#include <string_view>

namespace tg::edit {

enum class LayerProblem : std::uint8_t {
    None,
    InvalidName,
    NameInUse,
    LayerZero,
    Current,
    InUse,
    NotFound,
};

[[nodiscard]] LayerProblem checkNewLayer(const core::Document& doc, std::string_view name);
[[nodiscard]] LayerProblem checkRename(const core::Document& doc, core::LayerId id,
                                       std::string_view newName);
[[nodiscard]] LayerProblem checkRemove(const core::Document& doc, core::LayerId id);
[[nodiscard]] LayerProblem checkFreeze(const core::Document& doc, core::LayerId id);

/// A rename preserves these other references and the caller must warn about them.
/// Entity layer groups (8), including those in raw BLOCKS, are updated instead.
[[nodiscard]] bool namedInOtherUnhandledData(const core::Document& doc, core::LayerId id);

}  // namespace tg::edit
