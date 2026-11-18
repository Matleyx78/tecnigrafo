// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/dim_style_table.hpp"

#include "tg/core/document.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/lineweight.hpp"
#include "tg/core/start_handles.hpp"
#include "tg/core/text_style_table.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <optional>
#include <system_error>

namespace tg::core {
namespace {

// Group codes of the essential variables (DXF reference, DIMSTYLE).
constexpr std::array<DimVariableInfo, 28> kInfos{{
    {"DIMADEC", 179, 70}, {"DIMASZ", 41, 40},   {"DIMAZIN", 79, 70},  {"DIMCEN", 141, 40},
    {"DIMCLRD", 176, 70}, {"DIMCLRE", 177, 70}, {"DIMCLRT", 178, 70}, {"DIMDEC", 271, 70},
    {"DIMDSEP", 278, 70}, {"DIMEXE", 44, 40},   {"DIMEXO", 42, 40},   {"DIMGAP", 147, 40},
    {"DIMLFAC", 144, 40}, {"DIMLWD", 371, 70},  {"DIMLWE", 372, 70},  {"DIMPOST", 3, 1},
    {"DIMRND", 45, 40},   {"DIMSCALE", 40, 40}, {"DIMSD1", 281, 70},  {"DIMSD2", 282, 70},
    {"DIMSE1", 75, 70},   {"DIMSE2", 76, 70},   {"DIMTAD", 77, 70},   {"DIMTIH", 73, 70},
    {"DIMTOH", 74, 70},   {"DIMTXSTY", 340, 7}, {"DIMTXT", 140, 40},  {"DIMZIN", 78, 70},
}};

/// A member of DimVariables, by its type.
using Member =
    std::variant<double DimVariables::*, int DimVariables::*, bool DimVariables::*,
                 char DimVariables::*, std::string DimVariables::*, Color DimVariables::*>;

/// The member of an essential variable, in the order of kInfos.
constexpr std::array<Member, kInfos.size()> kMembers{
    &DimVariables::angleDecimals,
    &DimVariables::arrowSize,
    &DimVariables::angleZeroSuppression,
    &DimVariables::centerMark,
    &DimVariables::dimColor,
    &DimVariables::extColor,
    &DimVariables::textColor,
    &DimVariables::decimals,
    &DimVariables::decimalSeparator,
    &DimVariables::extExtension,
    &DimVariables::extOffset,
    &DimVariables::gap,
    &DimVariables::linearFactor,
    &DimVariables::dimLineweight,
    &DimVariables::extLineweight,
    &DimVariables::post,
    &DimVariables::round,
    &DimVariables::scale,
    &DimVariables::suppressDim1,
    &DimVariables::suppressDim2,
    &DimVariables::suppressExt1,
    &DimVariables::suppressExt2,
    &DimVariables::textAbove,
    &DimVariables::textInsideHorizontal,
    &DimVariables::textOutsideHorizontal,
    &DimVariables::textStyle,
    &DimVariables::textHeight,
    &DimVariables::zeroSuppression,
};

constexpr int kColorByLayer = 256;
constexpr int kMaxCharCode = 255;

const Member& memberOf(std::string_view name) {
    const auto* info = findDimVariable(name);
    assert(info != nullptr);
    return kMembers.at(static_cast<std::size_t>(info - kInfos.data()));
}

std::optional<int> integerOf(double value) {
    if (!std::isfinite(value) || value != std::trunc(value) || std::fabs(value) > 1e9) {
        return std::nullopt;
    }
    return static_cast<int>(value);
}

std::string_view trimmed(std::string_view text) {
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

std::optional<double> numberOf(std::string_view text) {
    text = trimmed(text);
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
    }
    double value = 0.0;
    const auto* end = text.data() + text.size();
    const auto [ptr, error] = std::from_chars(text.data(), end, value);
    if (text.empty() || error != std::errc{} || ptr != end || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

/// The value of an override text for `name`.
std::optional<DimValue> overrideValue(std::string_view name, std::string_view text) {
    const Member& member = memberOf(name);
    if (std::holds_alternative<std::string DimVariables::*>(member) ||
        std::holds_alternative<char DimVariables::*>(member)) {
        return DimValue{std::string{text}};
    }
    if (const auto number = numberOf(text)) {
        return DimValue{*number};
    }
    return std::nullopt;
}

}  // namespace

DimVariables dxfDefaultDimVariables() {
    DimVariables variables;
    variables.arrowSize = 0.18;
    variables.centerMark = 0.09;
    variables.decimals = 4;
    variables.decimalSeparator = '.';
    variables.extExtension = 0.18;
    variables.extOffset = 0.0625;
    variables.gap = 0.09;
    variables.textAbove = 0;
    variables.textInsideHorizontal = true;
    variables.textOutsideHorizontal = true;
    variables.textHeight = 0.18;
    variables.zeroSuppression = 0;
    return variables;
}

DimStyle newIso25DimStyle() {
    DimStyle style;
    style.id = start_handles::kDimstyleIso25;
    style.name = "ISO-25";
    style.extras = {{43, "3.75"}, {143, "0.03937007874016"},
                    {171, "3"},   {172, "1"},
                    {272, "2"},   {274, "3"},
                    {283, "0"},   {284, "8"}};
    return style;
}

std::span<const DimVariableInfo> dimVariableInfos() {
    return kInfos;
}

const DimVariableInfo* findDimVariable(std::string_view name) {
    // A loop rather than std::ranges::find: the iterator of std::array is a
    // pointer with libstdc++ but a class with MSVC, and clang-tidy and MSVC
    // disagree on how to declare it.
    for (const auto& info : kInfos) {
        if (info.name == name) {
            return &info;
        }
    }
    return nullptr;
}

DimValue dimValue(const DimVariables& variables, std::string_view name) {
    return std::visit(
        [&variables](auto member) -> DimValue {
            const auto& value = variables.*member;
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return value;
            } else if constexpr (std::is_same_v<T, char>) {
                return std::string(1, value);
            } else if constexpr (std::is_same_v<T, Color>) {
                switch (value.kind()) {
                    case Color::Kind::ByBlock:
                        return 0.0;
                    case Color::Kind::ByLayer:
                        return static_cast<double>(kColorByLayer);
                    case Color::Kind::Index:
                        return static_cast<double>(value.index());
                    case Color::Kind::Rgb:
                        return static_cast<double>(nearestAci(value.rgb()));
                }
                return 0.0;
            } else if constexpr (std::is_same_v<T, bool>) {
                return value ? 1.0 : 0.0;
            } else {
                return static_cast<double>(value);
            }
        },
        memberOf(name));
}

bool setDimValue(DimVariables& variables, std::string_view name, const DimValue& value) {
    return std::visit(
        [&variables, &value](auto member) -> bool {
            auto& target = variables.*member;
            using T = std::decay_t<decltype(target)>;
            if constexpr (std::is_same_v<T, std::string>) {
                const auto* text = std::get_if<std::string>(&value);
                if (text == nullptr) {
                    return false;
                }
                target = *text;
                return true;
            } else if constexpr (std::is_same_v<T, char>) {
                const auto* text = std::get_if<std::string>(&value);
                if (text != nullptr) {
                    if (text->size() != 1) {
                        return false;
                    }
                    target = text->front();
                    return true;
                }
                const auto code = integerOf(std::get<double>(value));
                if (!code || *code < 1 || *code > kMaxCharCode) {
                    return false;
                }
                target = static_cast<char>(static_cast<unsigned char>(*code));
                return true;
            } else {
                const auto* number = std::get_if<double>(&value);
                if (number == nullptr || !std::isfinite(*number)) {
                    return false;
                }
                if constexpr (std::is_same_v<T, double>) {
                    target = *number;
                    return true;
                } else {
                    const auto integer = integerOf(*number);
                    if (!integer) {
                        return false;
                    }
                    if constexpr (std::is_same_v<T, bool>) {
                        if (*integer != 0 && *integer != 1) {
                            return false;
                        }
                        target = *integer == 1;
                    } else if constexpr (std::is_same_v<T, Color>) {
                        if (*integer == 0) {
                            target = Color::byBlock();
                        } else if (*integer == kColorByLayer) {
                            target = Color::byLayer();
                        } else if (*integer > 0 && *integer < kColorByLayer) {
                            target = Color::fromIndex(*integer);
                        } else {
                            return false;
                        }
                    } else {
                        target = *integer;
                    }
                    return true;
                }
            }
        },
        memberOf(name));
}

DimVariables withOverrides(DimVariables variables,
                           std::span<const std::pair<std::string, std::string>> overrides) {
    for (const auto& [name, text] : overrides) {
        if (findDimVariable(name) == nullptr) {
            continue;
        }
        if (const auto value = overrideValue(name, text)) {
            static_cast<void>(setDimValue(variables, name, *value));
        }
    }
    return variables;
}

const DimStyle* DimStyleTable::find(std::string_view name) const {
    const auto found = std::ranges::find_if(
        m_styles, [name](const DimStyle& style) { return sameTableName(style.name, name); });
    return found == m_styles.end() ? nullptr : &*found;
}

std::uint64_t DimStyleTable::put(DimStyle style, Document& owner) {
    assert(isValidTableName(style.name));
    const auto found = std::ranges::find_if(
        m_styles, [&](const DimStyle& current) { return sameTableName(current.name, style.name); });
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

void DimStyleTable::setCurrent(std::string name) {
    assert(find(name) != nullptr);
    m_current = std::move(name);
}

bool DimStyleTable::regenerable(const RawGroups& extras) {
    // The groups of the variables of docs/adr/0047, point 2, and their
    // value in ISO-25: numbers, or the closed filled arrow.
    struct Number {
        std::int16_t code;
        double iso25;
    };

    constexpr std::array<Number, 15> kNumbers{{
        {173, 0},  // DIMSAH
        {71, 0},   // DIMTOL
        {72, 0},   // DIMLIM
        {170, 0},  // DIMALT
        {277, 2},  // DIMLUNIT
        {275, 0},  // DIMAUNIT
        {280, 0},  // DIMJUST
        {145, 0},  // DIMTVP
        {276, 0},  // DIMFRAC
        {142, 0},  // DIMTSZ
        {289, 3},  // DIMATFIT
        {279, 0},  // DIMTMOVE
        {174, 0},  // DIMTIX
        {175, 0},  // DIMSOXD
        {172, 1},  // DIMTOFL
    }};
    // DIMBLK, DIMBLK1, DIMBLK2 as names (R12) and DIMLDRBLK, DIMBLK,
    // DIMBLK1, DIMBLK2 as handles of a BLOCK_RECORD: empty, or the handle 0,
    // is the closed filled arrow.
    constexpr std::array<std::int16_t, 3> kNames{5, 6, 7};
    constexpr std::array<std::int16_t, 4> kHandles{341, 342, 343, 344};
    for (const auto& group : extras) {
        // A loop rather than std::ranges::find, as in findDimVariable.
        const Number* number = nullptr;
        for (const auto& candidate : kNumbers) {
            if (candidate.code == group.code) {
                number = &candidate;
                break;
            }
        }
        if (number != nullptr) {
            const auto value = numberOf(group.value);
            if (!value || *value != number->iso25) {
                return false;
            }
            continue;
        }
        const std::string_view value = trimmed(group.value);
        if (std::ranges::find(kNames, group.code) != kNames.end() && !value.empty() &&
            !sameTableName(value, "_ClosedFilled")) {
            return false;
        }
        if (std::ranges::find(kHandles, group.code) != kHandles.end() && !value.empty() &&
            value.find_first_not_of('0') != std::string_view::npos) {
            return false;
        }
    }
    return true;
}

bool DimStyleTable::inDomain(const DimVariables& effective, const TextStyleTable& styles) {
    // The ranges of -DIMSTYLE (PT28, from the history of AutoCAD LT).
    constexpr int kMaxDecimals = 8;
    constexpr int kMaxTextAbove = 4;
    constexpr int kMaxZeroSuppression = 15;
    constexpr int kMaxAngleZeroSuppression = 3;
    const auto& v = effective;
    const auto lineweight = [](int value) {
        return value == lineweight::kByLayer || value == lineweight::kByBlock ||
               value == lineweight::kDefault || isAllowedLineweight(value);
    };
    const auto finite =
        std::isfinite(v.scale) && std::isfinite(v.arrowSize) && std::isfinite(v.extOffset) &&
        std::isfinite(v.extExtension) && std::isfinite(v.textHeight) && std::isfinite(v.gap) &&
        std::isfinite(v.linearFactor) && std::isfinite(v.round) && std::isfinite(v.centerMark);
    return finite && v.scale >= 0.0 && v.arrowSize >= 0.0 && v.extOffset >= 0.0 &&
           v.extExtension >= 0.0 && v.textHeight > 0.0 && v.textAbove >= 0 &&
           v.textAbove <= kMaxTextAbove && v.decimals >= 0 && v.decimals <= kMaxDecimals &&
           v.angleDecimals >= -1 && v.angleDecimals <= kMaxDecimals &&
           static_cast<unsigned char>(v.decimalSeparator) > ' ' && v.zeroSuppression >= 0 &&
           v.zeroSuppression <= kMaxZeroSuppression && v.angleZeroSuppression >= 0 &&
           v.angleZeroSuppression <= kMaxAngleZeroSuppression && v.linearFactor != 0.0 &&
           v.round >= 0.0 && lineweight(v.dimLineweight) && lineweight(v.extLineweight) &&
           styles.find(v.textStyle) != nullptr;
}

DimVariables dimensionVariables(const DimStyleTable& table, std::string_view style,
                                std::span<const std::pair<std::string, std::string>> overrides) {
    const DimStyle* found = table.find(style);
    if (found == nullptr) {
        found = table.find("Standard");
    }
    return withOverrides(found != nullptr ? found->vars : dxfDefaultDimVariables(), overrides);
}

}  // namespace tg::core
