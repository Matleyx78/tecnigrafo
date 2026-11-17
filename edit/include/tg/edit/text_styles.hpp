// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/text_style_table.hpp"

#include <string_view>

namespace tg::edit {

class Editor;

/// Put a text style in the document table in the current transaction.
bool putTextStyle(Editor& editor, core::TextStyle style);

/// Select an existing text style in the current transaction.
bool setCurrentTextStyle(Editor& editor, std::string_view name);

}  // namespace tg::edit
