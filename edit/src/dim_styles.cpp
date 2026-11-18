// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/dim_styles.hpp"

#include "tg/edit/editor.hpp"

#include <string>
#include <utility>

namespace tg::edit {

bool saveDimStyle(Editor& editor, std::string_view name) {
    if (!core::isValidTableName(name)) {
        return false;
    }
    auto table = editor.document().dimStyles();
    core::DimStyle style;
    if (const auto* existing = table.find(name)) {
        style = *existing;
    } else {
        style.name = std::string{name};
        if (const auto* current = table.find(table.current())) {
            style.extras = current->extras;
        }
    }
    style.vars = table.currentVariables();
    table.put(std::move(style), editor.m_document);
    table.setCurrent(std::string{name});
    return editor.tables().replaceDimStyles(std::move(table));
}

bool restoreDimStyle(Editor& editor, std::string_view name) {
    auto table = editor.document().dimStyles();
    const auto* style = table.find(name);
    if (style == nullptr) {
        return false;
    }
    const std::string canonical = style->name;
    const core::DimVariables variables = style->vars;
    table.setCurrent(canonical);
    table.setCurrentVariables(variables);
    return editor.tables().replaceDimStyles(std::move(table));
}

bool setCurrentDimVariable(Editor& editor, std::string_view name, const core::DimValue& value) {
    auto table = editor.document().dimStyles();
    auto variables = table.currentVariables();
    if (!core::setDimValue(variables, name, value) ||
        !core::DimStyleTable::inDomain(variables, editor.document().textStyles())) {
        return false;
    }
    table.setCurrentVariables(std::move(variables));
    return editor.tables().replaceDimStyles(std::move(table));
}

}  // namespace tg::edit
