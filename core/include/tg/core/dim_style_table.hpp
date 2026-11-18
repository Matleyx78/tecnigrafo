// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/color.hpp"
#include "tg/core/raw_group.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {

class Document;
class TextStyleTable;

/// The essential variables of docs/adr/0047, point 2, with their DXF codes
/// in a DIMSTYLE record. The defaults are the values of ISO-25 in a new
/// drawing, confirmed in AutoCAD LT (docs/tappe/0.8/dxf-di-prova.md, "Prove
/// a mano", point 1; the STatus of the case dimstyle/stato-elenco).
struct DimVariables {
    double scale{1.0};                  ///< DIMSCALE, 40
    double arrowSize{2.5};              ///< DIMASZ, 41
    double extOffset{0.625};            ///< DIMEXO, 42
    double extExtension{1.25};          ///< DIMEXE, 44
    double textHeight{2.5};             ///< DIMTXT, 140
    double gap{0.625};                  ///< DIMGAP, 147
    int textAbove{1};                   ///< DIMTAD, 77
    bool textInsideHorizontal{false};   ///< DIMTIH, 73
    bool textOutsideHorizontal{false};  ///< DIMTOH, 74
    int decimals{2};                    ///< DIMDEC, 271
    int angleDecimals{0};               ///< DIMADEC, 179
    char decimalSeparator{','};         ///< DIMDSEP, 278 (the character code)
    int zeroSuppression{8};             ///< DIMZIN, 78
    int angleZeroSuppression{0};        ///< DIMAZIN, 79
    double linearFactor{1.0};           ///< DIMLFAC, 144
    double round{0.0};                  ///< DIMRND, 45
    std::string post;                   ///< DIMPOST, 3
    std::string textStyle{"Standard"};  ///< DIMTXSTY, 340 (the handle of the STYLE record)
    double centerMark{2.5};             ///< DIMCEN, 141
    bool suppressExt1{false};           ///< DIMSE1, 75
    bool suppressExt2{false};           ///< DIMSE2, 76
    bool suppressDim1{false};           ///< DIMSD1, 281
    bool suppressDim2{false};           ///< DIMSD2, 282
    Color dimColor{Color::byBlock()};   ///< DIMCLRD, 176
    Color extColor{Color::byBlock()};   ///< DIMCLRE, 177
    Color textColor{Color::byBlock()};  ///< DIMCLRT, 178
    int dimLineweight{-2};              ///< DIMLWD, 371 (ByBlock)
    int extLineweight{-2};              ///< DIMLWE, 372 (ByBlock)

    friend bool operator==(const DimVariables&, const DimVariables&) = default;
};

/// The values a DIMSTYLE record takes for the groups it lacks: the DXF
/// defaults of AutoCAD for DIMASZ, DIMCEN, DIMDEC, DIMDSEP, DIMEXE, DIMEXO,
/// DIMGAP, DIMTAD, DIMTIH, DIMTOH, DIMTXT and DIMZIN, ISO-25 for the others
/// (docs/tappe/0.8/PT22-stili-di-quota.md, point 3). They are also the values
/// of Standard in a new drawing (dxf-di-prova.md, "Prove a mano", point 1).
[[nodiscard]] DimVariables dxfDefaultDimVariables();

/// The value of an essential variable: a number for the reals, the
/// integers, the switches (0 or 1), the colours (the ACI number, 0 for
/// ByBlock and 256 for ByLayer) and the lineweights; a text for DIMPOST,
/// DIMTXSTY (the name of the text style) and DIMDSEP (one character).
using DimValue = std::variant<double, std::string>;

/// An essential variable: its name and its DXF groups.
struct DimVariableInfo {
    std::string_view name;    ///< "DIMTXT"
    std::int16_t styleCode;   ///< in a DIMSTYLE record and in the overrides of a dimension
    std::int16_t headerCode;  ///< after $DIM… in the HEADER section
};

/// The 28 essential variables, in alphabetical order.
[[nodiscard]] std::span<const DimVariableInfo> dimVariableInfos();

/// The essential variable with that name (DIMTXT, upper case), or nullptr.
[[nodiscard]] const DimVariableInfo* findDimVariable(std::string_view name);

/// The value of the essential variable `name`. Precondition:
/// findDimVariable(name) != nullptr.
[[nodiscard]] DimValue dimValue(const DimVariables& variables, std::string_view name);

/// Sets the essential variable `name`. False, and `variables` unchanged,
/// when the value has not the kind of the variable: a text for a number, a
/// number that is not an integer for an integer, a switch other than 0 and
/// 1, a colour outside 0–256, a DIMDSEP of more or less than one character
/// (or a number outside 1–255). The range of the value is not checked:
/// that is DimStyleTable::inDomain(). Precondition:
/// findDimVariable(name) != nullptr.
bool setDimValue(DimVariables& variables, std::string_view name, const DimValue& value);

/// `variables` with the overrides of a dimension, by name and value text
/// (DimensionPresentation::overrides): a number as DXF writes it, a text
/// as it is. An override of a variable that is not essential, or whose text
/// does not give a value of its kind, is ignored.
[[nodiscard]] DimVariables withOverrides(
    DimVariables variables, std::span<const std::pair<std::string, std::string>> overrides);

struct DimStyle {
    std::uint64_t id{0};
    std::string name;
    DimVariables vars;
    RawGroups extras;  ///< the other variables, as read (groups of the DIMSTYLE record)

    friend bool operator==(const DimStyle&, const DimStyle&) = default;
};

/// ISO-25 of a new drawing, with the handle start_handles::kDimstyleIso25:
/// the essential variables of DimVariables{} and, in `extras`, the other
/// variables whose value in ISO-25 differs from the DXF default, as AutoCAD
/// writes them for acadiso.dwt (DIMDLI, DIMALTF, DIMALTD, DIMTOFL, DIMTDEC,
/// DIMALTTD, DIMTOLJ, DIMTZIN).
[[nodiscard]] DimStyle newIso25DimStyle();

/// The dimension styles of a drawing, owned by Document (docs/adr/0044,
/// point 3; docs/adr/0047, point 2), and the current dimension variables of
/// the header.
class DimStyleTable {
public:
    [[nodiscard]] std::span<const DimStyle> styles() const { return m_styles; }

    /// The style with that name (sameTableName), or nullptr.
    [[nodiscard]] const DimStyle* find(std::string_view name) const;

    /// Adds `style` at the end, or replaces the style with the same name
    /// (sameTableName), which keeps its id and the name as written. A new
    /// style with id 0 takes the next id of `owner`; otherwise its id is
    /// reserved in `owner`. Returns the id. Precondition: valid table name.
    std::uint64_t put(DimStyle style, Document& owner);

    /// $DIMSTYLE.
    [[nodiscard]] const std::string& current() const { return m_current; }

    /// Precondition: find(name) != nullptr.
    void setCurrent(std::string name);

    /// The header $DIM… variables: the essential ones.
    [[nodiscard]] const DimVariables& currentVariables() const { return m_currentVariables; }

    void setCurrentVariables(DimVariables variables) { m_currentVariables = std::move(variables); }

    /// The other $DIM… variables of the header, as read: each one a group 9
    /// with its name followed by its value groups.
    [[nodiscard]] const RawGroups& currentExtras() const { return m_currentExtras; }

    void setCurrentExtras(RawGroups extras) { m_currentExtras = std::move(extras); }

    /// True when every variable that blocks regeneration (docs/adr/0047,
    /// point 2) has the ISO-25 value in `extras` (absent counts as ISO-25).
    /// `extras` are groups with the codes of a DIMSTYLE record: the extras of
    /// a style, or the other overrides of a dimension.
    [[nodiscard]] static bool regenerable(const RawGroups& extras);

    /// True when the effective essential variables (style plus overrides)
    /// are in the ranges DIMSTYLE accepts and DIMTXSTY names a text style
    /// of `styles` (PT5, B07; checked after the overrides).
    [[nodiscard]] static bool inDomain(const DimVariables& effective, const TextStyleTable& styles);

    friend bool operator==(const DimStyleTable&, const DimStyleTable&) = default;

private:
    std::vector<DimStyle> m_styles;
    std::string m_current;
    DimVariables m_currentVariables;
    RawGroups m_currentExtras;
};

/// The variables of a dimension whose style (group 3) is `style`: those of
/// the style or, when the table has none with that name, those of Standard
/// (PT5, B06; dxfDefaultDimVariables() without Standard either), with the
/// overrides of the dimension (withOverrides()). The dimension keeps its
/// style name.
[[nodiscard]] DimVariables dimensionVariables(
    const DimStyleTable& table, std::string_view style,
    std::span<const std::pair<std::string, std::string>> overrides);

}  // namespace tg::core
