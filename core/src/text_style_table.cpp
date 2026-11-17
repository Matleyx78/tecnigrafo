// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text_style_table.hpp"

#include "tg/core/document.hpp"
#include "tg/core/layer.hpp"

#include <algorithm>
#include <cassert>

namespace tg::core {

const TextStyle* TextStyleTable::find(std::string_view name) const {
    const auto found = std::ranges::find_if(
        m_styles, [name](const TextStyle& style) { return sameTableName(style.name, name); });
    return found == m_styles.end() ? nullptr : &*found;
}

std::uint64_t TextStyleTable::put(TextStyle style, Document& owner) {
    assert(isValidTableName(style.name));
    assert(style.widthFactor > 0.0);
    const auto found = std::ranges::find_if(m_styles, [&](const TextStyle& current) {
        return sameTableName(current.name, style.name);
    });
    if (found != m_styles.end()) {
        style.id = found->id;
        style.name = found->name;
        *found = std::move(style);
        return found->id;
    }
    if (style.id == 0) {
        style.id = owner.nextId();
        owner.setNextId(style.id + 1);
    } else {
        owner.reserveHandle(EntityId{style.id});
    }
    const std::uint64_t id = style.id;
    m_styles.push_back(std::move(style));
    return id;
}

void TextStyleTable::setCurrent(std::string name) {
    assert(find(name) != nullptr);
    m_current = std::move(name);
}

}  // namespace tg::core
