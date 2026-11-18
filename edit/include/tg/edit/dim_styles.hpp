// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/dim_style_table.hpp"

#include <string_view>

namespace tg::edit {

class Editor;

/// Save the current dimension variables in a new or existing style.
bool saveDimStyle(Editor& editor, std::string_view name);

/// Select an existing style and restore its variables.
bool restoreDimStyle(Editor& editor, std::string_view name);

/// Change one current essential dimension variable.
bool setCurrentDimVariable(Editor& editor, std::string_view name, const core::DimValue& value);

}  // namespace tg::edit
