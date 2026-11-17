// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/text_styles.hpp"

#include "tg/core/start_handles.hpp"
#include "tg/edit/editor.hpp"

#include <cmath>
#include <utility>

namespace tg::edit {

bool putTextStyle(Editor& editor, core::TextStyle style) {
    if (!core::isValidTableName(style.name) || !std::isfinite(style.fixedHeight) ||
        style.fixedHeight < 0.0 || !std::isfinite(style.widthFactor) || style.widthFactor <= 0.0 ||
        !std::isfinite(style.oblique)) {
        return false;
    }
    auto table = editor.document().textStyles();
    if (table.find(style.name) == nullptr && style.id == 0) {
        // New table records use the reserved object-handle range, leaving the
        // first entity handle unchanged (docs/adr/0023, point 1).
        style.id = core::start_handles::kLowest - table.styles().size();
    }
    // TextStyleTable registers explicit handles through its owning document.
    table.put(std::move(style), editor.m_document);
    return editor.tables().replaceTextStyles(std::move(table));
}

bool setCurrentTextStyle(Editor& editor, std::string_view name) {
    auto table = editor.document().textStyles();
    const auto* style = table.find(name);
    if (style == nullptr) {
        return false;
    }
    table.setCurrent(style->name);
    return editor.tables().replaceTextStyles(std::move(table));
}

}  // namespace tg::edit
