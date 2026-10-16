// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

namespace tg::app {

/// Demo content shown at start-up until file loading exists: a 120 x 80
/// plate with rounded corners, a slot and four holes, all polylines.
[[nodiscard]] core::Document makeSampleDrawing();

}  // namespace tg::app
