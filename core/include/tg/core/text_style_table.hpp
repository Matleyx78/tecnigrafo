// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/raw_group.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {

class Document;

struct TextStyle {
    std::uint64_t id{0};
    std::string name;
    std::string font;
    std::string bigFont;
    double fixedHeight{0.0};
    double widthFactor{1.0};
    double oblique{0.0};
    bool backwards{false};
    bool upsideDown{false};
    double lastHeight{2.5};
    RawGroups extras;

    friend bool operator==(const TextStyle&, const TextStyle&) = default;
};

class TextStyleTable {
public:
    [[nodiscard]] std::span<const TextStyle> styles() const { return m_styles; }

    [[nodiscard]] const TextStyle* find(std::string_view name) const;
    /// Precondition: valid table name and widthFactor > 0.
    std::uint64_t put(TextStyle style, Document& owner);

    [[nodiscard]] const std::string& current() const { return m_current; }

    /// Precondition: find(name) != nullptr.
    void setCurrent(std::string name);

    [[nodiscard]] std::span<const RawGroups> rawRecords() const { return m_rawRecords; }

    /// Used by the DXF codec; raw records are kept in their table positions
    /// by placeholders in RawDrawing.
    void addRawRecord(RawGroups record) { m_rawRecords.push_back(std::move(record)); }

    friend bool operator==(const TextStyleTable&, const TextStyleTable&) = default;

private:
    std::vector<TextStyle> m_styles;
    std::string m_current;
    std::vector<RawGroups> m_rawRecords;
};

}  // namespace tg::core
