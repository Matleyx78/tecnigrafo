// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The dimension styles in the DXF: the DIMSTYLE table, the $DIM… variables of
// the header and the overrides of a dimension (docs/adr/0047, point 2;
// docs/tappe/0.8/PT22-stili-di-quota.md). The table is read by TableReader,
// with the other tables in dxf_tables.cpp.

#include "dxf_entity_common.hpp"
#include "dxf_tables.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>

namespace tg::io {

using dxf_groups::firstOwn;
using dxf_groups::ownGroups;
using dxf_groups::parseInteger;

namespace {

constexpr std::string_view kStandard = "Standard";
constexpr std::int16_t kHandleCode = 105;
constexpr std::int16_t kTextStyleCode = 340;

// The groups of a DIMSTYLE record in the order AutoCAD writes them.
constexpr std::array kDimStyleOrder{
    2,   70,  3,   4,   5,   6,   7,   40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  140,
    141, 142, 143, 144, 145, 146, 147, 148, 71,  72,  73,  74,  75,  76,  77,  78,  79,  170,
    171, 172, 173, 174, 175, 176, 177, 178, 179, 271, 272, 273, 274, 275, 276, 277, 278, 279,
    280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 340, 341, 342, 343, 344, 371, 372};

/// The XDATA group that carries the value of a variable with `code` in the
/// overrides of a dimension: a text, a real, a handle or an integer.
std::int16_t xdataCode(std::int16_t code) {
    constexpr std::int16_t kText = 1000;
    constexpr std::int16_t kHandle = 1005;
    constexpr std::int16_t kReal = 1040;
    constexpr std::int16_t kInteger = 1070;
    if (code >= 1 && code <= 9) {
        return kText;
    }
    if ((code >= 40 && code <= 59) || (code >= 140 && code <= 149)) {
        return kReal;
    }
    if (code >= 340 && code <= 349) {
        return kHandle;
    }
    return kInteger;
}

/// The value of a group for the essential variable `info`, or std::nullopt
/// when the group gives no value of its kind. DIMTXSTY is not read here.
std::optional<core::DimValue> valueOf(const core::DimVariableInfo& info, std::string_view text) {
    if (info.name == "DIMPOST") {
        return core::DimValue{std::string{text}};
    }
    if (info.name == "DIMDSEP") {
        const auto code = parseInteger(text);
        if (!code) {
            return std::nullopt;
        }
        return core::DimValue{static_cast<double>(*code)};
    }
    const auto number = parseNumber(trimDxf(text));
    if (!number) {
        return std::nullopt;
    }
    return core::DimValue{*number};
}

/// The value text of a variable as the model holds it: the number as DXF
/// writes it, the text as it is; DIMDSEP as its character code.
std::string valueText(const core::DimVariableInfo& info, const core::DimValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        if (info.name == "DIMDSEP" && text->size() == 1) {
            return std::to_string(static_cast<unsigned char>(text->front()));
        }
        return *text;
    }
    const double number = std::get<double>(value);
    if (xdataCode(info.styleCode) == 1070) {
        return std::to_string(static_cast<long long>(number));
    }
    return formatNumber(number);
}

/// True when the group text `read` says `value`.
bool says(const core::DimVariableInfo& info, std::string_view read, const core::DimValue& value) {
    const auto parsed = valueOf(info, read);
    if (!parsed) {
        return false;
    }
    if (info.name == "DIMDSEP") {
        core::DimVariables a;
        core::DimVariables b;
        return core::setDimValue(a, info.name, *parsed) && core::setDimValue(b, info.name, value) &&
               a.decimalSeparator == b.decimalSeparator;
    }
    return *parsed == value;
}

const core::TextStyle* textStyleWithId(const core::TextStyleTable& styles, std::uint64_t id) {
    const auto all = styles.styles();
    const auto found = std::ranges::find(all, id, &core::TextStyle::id);
    return found == all.end() ? nullptr : &*found;
}

bool isDimVariable(std::string_view variable) {
    return variable.starts_with("$DIM") && variable != "$DIMSTYLE";
}

/// The essential variable of a header variable ($DIMTXT), or nullptr.
const core::DimVariableInfo* headerVariable(std::string_view variable) {
    return variable.starts_with('$') ? core::findDimVariable(variable.substr(1)) : nullptr;
}

/// The first own group with `code` after the ACAD application, where the
/// DSTYLE list of a dimension is: [begin, end) of its pairs, if any.
struct DstyleList {
    std::size_t open{0};   ///< the 1000 DSTYLE group
    std::size_t close{0};  ///< the 1002 } group
};

std::optional<DstyleList> findDstyle(const core::RawGroups& extras) {
    bool acad = false;
    for (std::size_t i = 0; i < extras.size(); ++i) {
        const auto& group = extras[i];
        if (group.code == 1001) {
            acad = trimDxf(group.value) == "ACAD";
            continue;
        }
        if (!acad || group.code != 1000 || trimDxf(group.value) != "DSTYLE" ||
            i + 1 >= extras.size() || extras[i + 1].code != 1002 ||
            trimDxf(extras[i + 1].value) != "{") {
            continue;
        }
        for (std::size_t j = i + 2; j < extras.size() && extras[j].code != 1001; ++j) {
            if (extras[j].code == 1002 && trimDxf(extras[j].value) == "}") {
                return DstyleList{i, j};
            }
        }
        return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace

// --- Reading ---------------------------------------------------------------

std::uint64_t TableReader::newRecordHandle(std::string_view type, std::size_t line,
                                           std::vector<std::string>& warnings) {
    const std::uint64_t handle = m_document.nextId();
    m_document.setNextId(handle + 1);
    warnings.push_back("line " + std::to_string(line) + ": " + std::string{type} +
                       " record had no handle; assigned " + handleText(handle));
    return handle;
}

bool TableReader::readDimStyle(core::RawGroups record, std::size_t line,
                               std::vector<std::string>& warnings) {
    const auto at2 = firstOwn(record, ownGroups(record), 2);
    if (!at2 || !core::isValidTableName(record[*at2].value) ||
        m_dimStyles.find(record[*at2].value) != nullptr) {
        return false;
    }
    core::DimStyle style;
    style.name = record[*at2].value;
    // The variables the record lacks take the DXF defaults (PT22, point 3).
    style.vars = core::dxfDefaultDimVariables();
    for (const auto& info : core::dimVariableInfos()) {
        if (info.styleCode == kTextStyleCode) {
            continue;  // DIMTXSTY by handle, in finishDimStyles()
        }
        const auto at = firstOwn(record, ownGroups(record), info.styleCode);
        if (!at) {
            continue;
        }
        // The group stays in the extras, as a witness of its place and of
        // its spelling (as in a STYLE record): the writer gives it the value
        // of the model.
        if (const auto value = valueOf(info, record[*at].value)) {
            static_cast<void>(core::setDimValue(style.vars, info.name, *value));
        }
    }
    const auto handle = firstOwn(record, ownGroups(record), kHandleCode);
    const auto read = handle ? parseHandle(record[*handle].value) : std::nullopt;
    style.id = read && *read != 0 ? *read : newRecordHandle("DIMSTYLE", line, warnings);
    record.erase(record.begin() +
                 static_cast<std::ptrdiff_t>(*firstOwn(record, ownGroups(record), 2)));
    style.extras = std::move(record);
    m_dimStyles.put(std::move(style), m_document);
    return true;
}

bool TableReader::readDimHeader(std::string_view variable, const core::RawGroup& group,
                                bool first) {
    if (variable == "$DIMSTYLE") {
        if (group.code == 2 && first) {
            m_currentDimStyle = group.value;
        }
        return true;
    }
    if (!isDimVariable(variable)) {
        return true;
    }
    if (const auto* info = headerVariable(variable)) {
        if (group.code == info->headerCode && first) {
            const std::optional<core::DimValue> value = info->name == "DIMTXSTY"
                                                            ? core::DimValue{group.value}
                                                            : valueOf(*info, group.value);
            if (value) {
                m_dimHeader.emplace_back(std::string{info->name}, *value);
            }
        }
        return true;
    }
    if (first) {
        m_dimHeaderExtras.push_back({9, std::string{variable}});
    }
    m_dimHeaderExtras.push_back(group);
    return true;
}

void TableReader::finishDimStyles(std::uint64_t standardHandle) {
    const auto& textStyles = m_document.textStyles();
    core::DimStyleTable table;
    for (core::DimStyle style : m_dimStyles.styles()) {
        const auto at = firstOwn(style.extras, ownGroups(style.extras), kTextStyleCode);
        if (at) {
            const auto handle = parseHandle(style.extras[*at].value);
            const core::TextStyle* named = handle ? textStyleWithId(textStyles, *handle) : nullptr;
            // A handle that names no text style: DIMTXSTY names none, and
            // the dimensions of the style are not regenerable (PT5, B07).
            style.vars.textStyle = named != nullptr ? named->name : std::string{};
        } else {
            style.vars.textStyle = std::string{kStandard};
        }
        table.put(std::move(style), m_document);
    }
    // PT5, B06: a missing or unknown $DIMSTYLE gives Standard, created with
    // the values of AutoCAD when the table lacks it.
    const core::DimStyle* current = m_currentDimStyle ? table.find(*m_currentDimStyle) : nullptr;
    if (current == nullptr) {
        if (table.find(kStandard) == nullptr) {
            core::DimStyle standard;
            standard.id = standardHandle;
            standard.name = kStandard;
            standard.vars = core::dxfDefaultDimVariables();
            table.put(std::move(standard), m_document);
        }
        current = table.find(kStandard);
    }
    table.setCurrent(current->name);
    // The header variables the file lacks are those of the current style.
    core::DimVariables variables = current->vars;
    for (const auto& [name, value] : m_dimHeader) {
        static_cast<void>(core::setDimValue(variables, name, value));
    }
    table.setCurrentVariables(std::move(variables));
    table.setCurrentExtras(std::move(m_dimHeaderExtras));
    m_document.replaceDimStyles(std::move(table));
}

// --- Writing ---------------------------------------------------------------

bool needsDimStyleTable(const core::Document& document) {
    const auto styles = document.dimStyles().styles();
    if (styles.size() != 1 || !core::sameTableName(styles.front().name, kStandard)) {
        return !styles.empty();
    }
    const core::DimStyle& standard = styles.front();
    return standard.vars != core::dxfDefaultDimVariables() || !standard.extras.empty();
}

core::RawGroups dimStyleRecord(const core::DimStyle& style, const core::Document& document,
                               std::string_view owner) {
    const bool fresh =
        std::ranges::find(style.extras, 100, &core::RawGroup::code) == style.extras.end();
    core::RawGroups body;
    if (fresh) {
        body = {{330, std::string{owner}},
                {100, "AcDbSymbolTableRecord"},
                {100, "AcDbDimStyleTableRecord"}};
    }
    body.insert(body.end(), style.extras.begin(), style.extras.end());
    // The handle: the 105 read, or a new one at the start.
    const auto handle = firstOwn(body, ownGroups(body), kHandleCode);
    if (!handle) {
        body.insert(body.begin(), {kHandleCode, handleText(style.id)});
    } else if (parseHandle(body[*handle].value) != style.id) {
        body[*handle].value = handleText(style.id);
    }
    if (!firstOwn(body, ownGroups(body), 70)) {
        dxf_groups::insertInOrder(body, {{70, "0"}}, kDimStyleOrder);
    }
    dxf_groups::insertInOrder(body, {{2, style.name}}, kDimStyleOrder);
    const core::DimVariables defaults = core::dxfDefaultDimVariables();
    core::RawGroups additions;
    for (const auto& info : core::dimVariableInfos()) {
        const auto at = firstOwn(body, ownGroups(body), info.styleCode);
        if (info.styleCode == kTextStyleCode) {
            const core::TextStyle* named = document.textStyles().find(style.vars.textStyle);
            if (named == nullptr) {
                continue;  // the 340 read stays as it was
            }
            const std::string text = handleText(named->id);
            if (at) {
                if (parseHandle(body[*at].value) != named->id) {
                    body[*at].value = text;
                }
            } else if (fresh || !core::sameTableName(style.vars.textStyle, kStandard)) {
                additions.push_back({kTextStyleCode, text});
            }
            continue;
        }
        const core::DimValue value = core::dimValue(style.vars, info.name);
        if (at) {
            if (!says(info, body[*at].value, value)) {
                body[*at].value = valueText(info, value);
            }
        } else if (value != core::dimValue(defaults, info.name)) {
            additions.push_back({info.styleCode, valueText(info, value)});
        }
    }
    dxf_groups::insertInOrder(body, additions, kDimStyleOrder);
    core::RawGroups out{{0, "DIMSTYLE"}};
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

core::RawGroups headerDimVariables(const core::RawGroups& header, const core::Document& document,
                                   bool addAll) {
    const auto& table = document.dimStyles();
    const std::string currentName =
        table.current().empty() ? std::string{kStandard} : table.current();
    bool dimStyleSeen = false;
    const core::DimVariables& current = table.currentVariables();
    const core::DimStyle* style = table.find(table.current());
    const core::DimVariables inferred = style != nullptr ? style->vars : core::DimVariables{};
    const core::RawGroups& extras = table.currentExtras();
    /// The value groups of the extra variable `name`, if currentExtras has it.
    const auto extraValues = [&extras](std::string_view name) -> std::optional<core::RawGroups> {
        for (std::size_t i = 0; i < extras.size(); ++i) {
            if (extras[i].code == 9 && trimDxf(extras[i].value) == name) {
                core::RawGroups values;
                for (std::size_t j = i + 1; j < extras.size() && extras[j].code != 9; ++j) {
                    values.push_back(extras[j]);
                }
                return values;
            }
        }
        return std::nullopt;
    };
    std::vector<std::string> seen;
    core::RawGroups out;
    std::size_t i = 0;
    while (i < header.size()) {
        const auto& group = header[i];
        const std::string_view variable = group.code == 9 ? trimDxf(group.value) : "";
        if (variable == "$DIMSTYLE" && !table.styles().empty()) {
            // The name of the current style. A name read that names no
            // style stays as read while the current style is the Standard a
            // reader gives for it (PT22, point 3): the file is written again
            // as it was.
            dimStyleSeen = true;
            out.push_back(group);
            ++i;
            bool written = false;
            while (i < header.size() && header[i].code != 9) {
                const auto& value = header[i++];
                if (value.code != 2 || written) {
                    out.push_back(value);
                    continue;
                }
                written = true;
                const bool keep = core::sameTableName(value.value, currentName) ||
                                  (table.find(value.value) == nullptr &&
                                   core::sameTableName(currentName, kStandard));
                out.push_back(keep ? value : core::RawGroup{2, currentName});
            }
            if (!written) {
                out.push_back({2, currentName});
            }
            continue;
        }
        if (group.code != 9 || !isDimVariable(variable)) {
            out.push_back(group);
            ++i;
            continue;
        }
        seen.emplace_back(variable);
        ++i;
        core::RawGroups read;
        while (i < header.size() && header[i].code != 9) {
            read.push_back(header[i++]);
        }
        const auto* info = headerVariable(variable);
        if (info == nullptr) {
            // Another $DIM… variable: the value of currentExtras, or none.
            if (const auto values = extraValues(variable)) {
                out.push_back(group);
                out.insert(out.end(), values->begin(), values->end());
            }
            continue;
        }
        out.push_back(group);
        const core::DimValue value = core::dimValue(current, info->name);
        bool written = false;
        for (const auto& value_group : read) {
            if (value_group.code == info->headerCode && !written) {
                written = true;
                const bool same = info->name == "DIMTXSTY"
                                      ? value_group.value == std::get<std::string>(value)
                                      : says(*info, value_group.value, value);
                out.push_back(same ? value_group
                                   : core::RawGroup{info->headerCode, valueText(*info, value)});
            } else {
                out.push_back(value_group);
            }
        }
        if (!written) {
            out.push_back({info->headerCode, valueText(*info, value)});
        }
    }
    if (!dimStyleSeen && !table.styles().empty() &&
        (addAll || !core::sameTableName(currentName, kStandard))) {
        out.push_back({9, "$DIMSTYLE"});
        out.push_back({2, currentName});
    }
    for (std::size_t k = 0; k < extras.size(); ++k) {
        if (extras[k].code == 9 &&
            std::ranges::find(seen, trimDxf(extras[k].value)) == seen.end()) {
            out.push_back(extras[k]);
            for (std::size_t j = k + 1; j < extras.size() && extras[j].code != 9; ++j) {
                out.push_back(extras[j]);
            }
        }
    }
    for (const auto& info : core::dimVariableInfos()) {
        const std::string name = "$" + std::string{info.name};
        const core::DimValue value = core::dimValue(current, info.name);
        if (std::ranges::find(seen, name) == seen.end() &&
            value != core::dimValue(inferred, info.name)) {
            out.push_back({9, name});
            out.push_back({info.headerCode, valueText(info, value)});
        }
    }
    return out;
}

// --- Overrides of a dimension ------------------------------------------------

DimOverrides readDimOverrides(const core::RawGroups& extras, const core::TextStyleTable& styles) {
    DimOverrides overrides;
    const auto list = findDstyle(extras);
    if (!list) {
        return overrides;
    }
    for (std::size_t i = list->open + 2; i + 1 < list->close; i += 2) {
        const auto code = parseInteger(extras[i].value);
        if (extras[i].code != 1070 || !code) {
            continue;
        }
        const auto codeValue = static_cast<std::int16_t>(*code);
        const std::string& text = extras[i + 1].value;
        const auto infos = core::dimVariableInfos();
        const auto info = std::ranges::find(infos, codeValue, &core::DimVariableInfo::styleCode);
        if (info != infos.end() && codeValue == kTextStyleCode) {
            const auto handle = parseHandle(text);
            if (const core::TextStyle* named =
                    handle ? textStyleWithId(styles, *handle) : nullptr) {
                overrides.essential.emplace_back(std::string{info->name}, named->name);
                continue;
            }
        } else if (info != infos.end()) {
            const auto value = valueOf(*info, text);
            core::DimVariables check;
            if (value && core::setDimValue(check, info->name, *value)) {
                const std::string shown = info->name == "DIMDSEP"
                                              ? std::string(1, check.decimalSeparator)
                                              : std::string{trimDxf(text)};
                overrides.essential.emplace_back(std::string{info->name}, shown);
                continue;
            }
        }
        overrides.others.push_back({codeValue, text});
    }
    return overrides;
}

void writeDimOverrides(core::RawGroups& extras, const DimOverrides& overrides,
                       const core::TextStyleTable& styles) {
    if (readDimOverrides(extras, styles) == overrides) {
        return;
    }
    // The pairs, in the order of the codes.
    std::vector<std::pair<std::int16_t, core::RawGroup>> pairs;
    for (const auto& [name, text] : overrides.essential) {
        const auto* info = core::findDimVariable(name);
        if (info == nullptr) {
            continue;
        }
        if (info->styleCode == kTextStyleCode) {
            if (const core::TextStyle* named = styles.find(text)) {
                pairs.emplace_back(info->styleCode, core::RawGroup{xdataCode(info->styleCode),
                                                                   handleText(named->id)});
            }
            continue;
        }
        core::DimVariables check;
        const core::DimValue value = info->name == "DIMDSEP" || info->name == "DIMPOST"
                                         ? core::DimValue{text}
                                         : core::DimValue{parseNumber(trimDxf(text)).value_or(0.0)};
        if (!core::setDimValue(check, info->name, value)) {
            continue;
        }
        const std::string written = info->name == "DIMDSEP" || info->name == "DIMPOST"
                                        ? valueText(*info, value)
                                        : std::string{trimDxf(text)};
        pairs.emplace_back(info->styleCode, core::RawGroup{xdataCode(info->styleCode), written});
    }
    for (const auto& group : overrides.others) {
        pairs.emplace_back(group.code, core::RawGroup{xdataCode(group.code), group.value});
    }
    std::ranges::stable_sort(pairs, {}, &std::pair<std::int16_t, core::RawGroup>::first);
    core::RawGroups list;
    if (!pairs.empty()) {
        list = {{1000, "DSTYLE"}, {1002, "{"}};
        for (const auto& [code, value] : pairs) {
            list.push_back({1070, std::to_string(code)});
            list.push_back(value);
        }
        list.push_back({1002, "}"});
    }
    const auto at = [&extras](std::size_t index) {
        return extras.begin() + static_cast<std::ptrdiff_t>(index);
    };
    if (const auto found = findDstyle(extras)) {
        extras.erase(at(found->open), at(found->close + 1));
        extras.insert(at(found->open), list.begin(), list.end());
        return;
    }
    if (list.empty()) {
        return;
    }
    const auto acad = std::ranges::find_if(extras, [](const core::RawGroup& group) {
        return group.code == 1001 && trimDxf(group.value) == "ACAD";
    });
    if (acad != extras.end()) {
        extras.insert(acad + 1, list.begin(), list.end());
        return;
    }
    extras.push_back({1001, "ACAD"});
    extras.insert(extras.end(), list.begin(), list.end());
}

}  // namespace tg::io
