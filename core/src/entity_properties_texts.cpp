// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The own properties of MTEXT, ATTDEF and INSERT
// (docs/tappe/0.8.1/PT24-proprieta-testi-inserimenti.md).

#include "tg/core/attribute_definition.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_properties.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/text_style_table.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

PropertyChange refused(std::string_view reason) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{reason}};
}

PropertyChange invalid() {
    return refused(kInvalidPropertyValue);
}

PropertyChange changed(Entity entity) {
    return PropertyChange{std::move(entity), std::nullopt, {}};
}

/// The number held by `value`, if it is one and finite.
std::optional<double> finiteNumber(const PropertyValue& value) {
    const auto* number = std::get_if<double>(&value);
    if (number == nullptr || !std::isfinite(*number)) {
        return std::nullopt;
    }
    return *number;
}

/// The point held by `value`, if it is one and finite.
std::optional<Vec2> finitePoint(const PropertyValue& value) {
    const auto* point = std::get_if<Vec2>(&value);
    if (point == nullptr || !std::isfinite(point->x) || !std::isfinite(point->y)) {
        return std::nullopt;
    }
    return *point;
}

/// The text held by `value`, if it is one.
std::optional<std::string> textOf(const PropertyValue& value) {
    const auto* text = std::get_if<std::string>(&value);
    if (text == nullptr) {
        return std::nullopt;
    }
    return *text;
}

/// The flag held by `value`, if it is one.
std::optional<bool> flagOf(const PropertyValue& value) {
    const auto* flag = std::get_if<bool>(&value);
    if (flag == nullptr) {
        return std::nullopt;
    }
    return *flag;
}

/// The name of the text style `name` as the table writes it, if it exists.
std::optional<std::string> existingStyle(const Document& document, std::string_view name) {
    const TextStyle* style = document.textStyles().find(name);
    if (style == nullptr) {
        return std::nullopt;
    }
    return style->name;
}

// ---------------------------------------------------------------------------
// MTEXT

/// The names of the attachments, in the order of MTextAttachment (1 to 9),
/// as the report writes them (docs/verifica/rapporto.md).
constexpr std::array<std::string_view, 9> kAttachmentNames{
    "top-left",     "top-center",  "top-right",     "middle-left", "middle-center",
    "middle-right", "bottom-left", "bottom-center", "bottom-right"};

constexpr std::string_view kAtLeast = "at-least";
constexpr std::string_view kExactly = "exactly";

std::vector<std::string> attachmentChoices() {
    return {kAttachmentNames.begin(), kAttachmentNames.end()};
}

std::string_view attachmentName(MTextAttachment attachment) {
    return kAttachmentNames.at(static_cast<std::size_t>(attachment) - 1);
}

std::optional<MTextAttachment> attachmentNamed(std::string_view name) {
    for (std::size_t i = 0; i < kAttachmentNames.size(); ++i) {
        if (kAttachmentNames.at(i) == name) {
            return static_cast<MTextAttachment>(i + 1);
        }
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// INSERT: the scale Z, group 43 of AcDbBlockReference in extras.specific

constexpr std::int16_t kScaleZCode = 43;
constexpr std::string_view kSeqendMarker = "SEQEND";
constexpr std::string_view kReferenceMarker = "AcDbBlockReference";

/// The groups a new 43 goes after: those that come before it in
/// the order of the DXF reference (100, 66, 2, 10, 20, 41, 42).
constexpr std::array<std::int16_t, 7> kBeforeScaleZ{100, 66, 2, 10, 20, 41, 42};

/// For every group of `specific`, true when it is an own group of the
/// insert: outside the 102 {…} groups, before the first XDATA group and
/// before the groups of the SEQEND, which io keeps after a group 0 "SEQEND"
/// (io/src/dxf_insert.cpp).
std::vector<bool> ownGroups(const RawGroups& specific) {
    std::vector<bool> own(specific.size(), false);
    std::size_t depth = 0;
    for (std::size_t i = 0; i < specific.size(); ++i) {
        const RawGroup& group = specific[i];
        if (group.code == 1001 || (group.code == 0 && group.value == kSeqendMarker)) {
            break;
        }
        if (group.code == 102) {
            if (group.value.starts_with('{')) {
                ++depth;
            } else if (group.value == "}" && depth != 0) {
                --depth;
            }
            continue;
        }
        own[i] = depth == 0;
    }
    return own;
}

/// The index of the own group 43 of the insert in `specific`.
std::optional<std::size_t> scaleZIndex(const RawGroups& specific) {
    const std::vector<bool> own = ownGroups(specific);
    for (std::size_t i = 0; i < specific.size(); ++i) {
        if (own[i] && specific[i].code == kScaleZCode) {
            return i;
        }
    }
    return std::nullopt;
}

/// The number written in a group, without the blanks a DXF may put around it.
std::optional<double> groupNumber(std::string_view text) {
    constexpr std::string_view kBlanks = " \t\r\n";
    const auto first = text.find_first_not_of(kBlanks);
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    text = text.substr(first, text.find_last_not_of(kBlanks) - first + 1);
    if (text.starts_with('+')) {
        text.remove_prefix(1);
    }
    double number = 0.0;
    const auto [stop, error] = std::from_chars(text.data(), text.data() + text.size(), number);
    if (error != std::errc{} || stop != text.data() + text.size()) {
        return std::nullopt;
    }
    return number;
}

/// The shortest text that reads back as `value`, for a DXF group (as -INSERT
/// writes the 43).
std::string groupText(double value) {
    constexpr std::size_t kSize = 32;
    std::array<char, kSize> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    std::string text = error == std::errc{} ? std::string(buffer.data(), end) : std::string{"1"};
    return text == "-0" ? std::string{"0"} : text;
}

/// The scale Z of the insert: its group 43, 1 when it is missing or is not a
/// number (as io reads a group of the model that is missing).
double scaleZ(const EntityExtras& extras) {
    const auto index = scaleZIndex(extras.specific);
    if (!index) {
        return 1.0;
    }
    return groupNumber(extras.specific[*index].value).value_or(1.0);
}

/// The extras with the group 43 set to `value`, every other group as it is.
/// A 43 that is missing goes after the last own group that comes before it
/// in the DXF reference, or with the marker of the subclass when there is
/// none, as -INSERT writes it.
EntityExtras withScaleZ(EntityExtras extras, double value) {
    RawGroups& specific = extras.specific;
    if (const auto index = scaleZIndex(specific)) {
        specific[*index].value = groupText(value);
        return extras;
    }
    const RawGroup group{kScaleZCode, groupText(value)};
    const std::vector<bool> own = ownGroups(specific);
    std::size_t at = 0;
    bool found = false;
    for (std::size_t i = 0; i < specific.size(); ++i) {
        if (own[i] && std::ranges::find(kBeforeScaleZ, specific[i].code) != kBeforeScaleZ.end()) {
            at = i + 1;
            found = true;
        }
    }
    if (!found) {
        specific.insert(specific.begin(), {RawGroup{100, std::string{kReferenceMarker}}, group});
        return extras;
    }
    specific.insert(specific.begin() + static_cast<std::ptrdiff_t>(at), group);
    return extras;
}

// ---------------------------------------------------------------------------
// INSERT: the attributes, written "<TAG>":"<value>" as the report writes texts

constexpr std::string_view kAttributeName = "attribute";

/// `text` between double quotes, as the report writes a text
/// (docs/verifica/rapporto.md): '"' and '\' after a '\', a control
/// character as \u{XX}.
std::string quoted(std::string_view text) {
    constexpr std::string_view kHexDigits = "0123456789ABCDEF";
    constexpr unsigned char kFirstPrintable = 0x20;
    constexpr unsigned char kDelete = 0x7F;
    constexpr unsigned kNibble = 4U;
    constexpr unsigned kLowNibble = 0x0FU;
    std::string out = "\"";
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (byte < kFirstPrintable || byte == kDelete) {
            out += "\\u{";
            out += kHexDigits[byte >> kNibble];
            out += kHexDigits[byte & kLowNibble];
            out += '}';
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

void appendUtf8(std::uint32_t codePoint, std::string& out) {
    constexpr std::uint32_t kOneByte = 0x80;
    constexpr std::uint32_t kTwoBytes = 0x800;
    constexpr std::uint32_t kThreeBytes = 0x10000;
    constexpr std::uint32_t kContinuation = 0x80;
    constexpr std::uint32_t kSixBits = 0x3F;
    constexpr unsigned kShift = 6U;
    const auto byte = [](std::uint32_t value) {
        return static_cast<char>(value);
    };
    if (codePoint < kOneByte) {
        out += byte(codePoint);
    } else if (codePoint < kTwoBytes) {
        out += byte(0xC0U | (codePoint >> kShift));
        out += byte(kContinuation | (codePoint & kSixBits));
    } else if (codePoint < kThreeBytes) {
        out += byte(0xE0U | (codePoint >> (2 * kShift)));
        out += byte(kContinuation | ((codePoint >> kShift) & kSixBits));
        out += byte(kContinuation | (codePoint & kSixBits));
    } else {
        out += byte(0xF0U | (codePoint >> (3 * kShift)));
        out += byte(kContinuation | ((codePoint >> (2 * kShift)) & kSixBits));
        out += byte(kContinuation | ((codePoint >> kShift) & kSixBits));
        out += byte(kContinuation | (codePoint & kSixBits));
    }
}

std::optional<std::uint32_t> hexValue(char c) {
    constexpr std::uint32_t kTen = 10;
    if (c >= '0' && c <= '9') {
        return static_cast<std::uint32_t>(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<std::uint32_t>(c - 'A') + kTen;
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<std::uint32_t>(c - 'a') + kTen;
    }
    return std::nullopt;
}

/// The text quoted() wrote at the start of `text`, which advances past it;
/// std::nullopt when it is not written so.
std::optional<std::string> unquoted(std::string_view& text) {
    constexpr std::size_t kMaxHexDigits = 6;
    constexpr std::uint32_t kMaxCodePoint = 0x10FFFF;
    constexpr std::uint32_t kFirstSurrogate = 0xD800;
    constexpr std::uint32_t kLastSurrogate = 0xDFFF;
    constexpr unsigned kNibble = 4U;
    if (text.empty() || text.front() != '"') {
        return std::nullopt;
    }
    std::string out;
    std::size_t i = 1;
    while (i < text.size()) {
        const char c = text[i++];
        if (c == '"') {
            text.remove_prefix(i);
            return out;
        }
        if (c != '\\') {
            out += c;
            continue;
        }
        if (i == text.size()) {
            return std::nullopt;
        }
        const char escaped = text[i++];
        if (escaped == '"' || escaped == '\\') {
            out += escaped;
            continue;
        }
        if (escaped != 'u' || i == text.size() || text[i] != '{') {
            return std::nullopt;
        }
        ++i;
        std::uint32_t codePoint = 0;
        std::size_t digits = 0;
        while (i < text.size() && text[i] != '}') {
            const auto digit = hexValue(text[i]);
            if (!digit || ++digits > kMaxHexDigits) {
                return std::nullopt;
            }
            codePoint = (codePoint << kNibble) | *digit;
            ++i;
        }
        if (i == text.size() || digits == 0 || codePoint > kMaxCodePoint ||
            (codePoint >= kFirstSurrogate && codePoint <= kLastSurrogate)) {
            return std::nullopt;
        }
        ++i;  // the '}'
        appendUtf8(codePoint, out);
    }
    return std::nullopt;
}

/// The tag and value of `"<TAG>":"<value>"`.
std::optional<std::pair<std::string, std::string>> tagAndValue(std::string_view text) {
    auto tag = unquoted(text);
    if (!tag || text.empty() || text.front() != ':') {
        return std::nullopt;
    }
    text.remove_prefix(1);
    auto value = unquoted(text);
    if (!value || !text.empty()) {
        return std::nullopt;
    }
    return std::pair{std::move(*tag), std::move(*value)};
}

/// The index of the first ATTRIB of `insert` with tag `tag`, compared
/// without case as findAttribute() compares it (sameTableName()).
std::optional<std::size_t> firstWithTag(const Insert& insert, std::string_view tag) {
    for (std::size_t i = 0; i < insert.attributes.size(); ++i) {
        if (sameTableName(insert.attributes[i].tag, tag)) {
            return i;
        }
    }
    return std::nullopt;
}

/// True when the insert `insert`, which replaces the entity of `record`,
/// would make a block contain itself: only an insert among the entities of
/// a block definition can (docs/adr/0045, point 1).
bool wouldCycle(const Document& document, const EntityRecord& record, const Insert& insert) {
    for (const BlockDefinition& definition : document.blocks().definitions()) {
        for (std::size_t i = 0; i < definition.entities.size(); ++i) {
            if (definition.entities[i].id != record.id) {
                continue;
            }
            BlockDefinition changedDefinition = definition;
            changedDefinition.entities[i].entity = insert;
            return document.blocks().wouldCycle(changedDefinition);
        }
    }
    return false;
}

/// A changed insert that the model supports.
PropertyChange checked(const Insert& insert) {
    if (!supported(insert)) {
        return invalid();
    }
    return changed(insert);
}

}  // namespace

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/, const MText& mtext) {
    return {
        {"value", PropertyKind::Text, mtext.value, {}, false},
        {"style", PropertyKind::Text, mtext.style, {}, false},
        {"attachment", PropertyKind::Choice, std::string{attachmentName(mtext.attachment)},
         attachmentChoices(), false},
        {"width", PropertyKind::Distance, mtext.width, {}, false},
        {"height", PropertyKind::Distance, mtext.height, {}, false},
        {"rotation", PropertyKind::Angle, mtext.rotation, {}, false},
        {"line-spacing", PropertyKind::Number, mtext.lineSpacing, {}, false},
        {"line-spacing-style",
         PropertyKind::Choice,
         std::string{mtext.exactSpacing ? kExactly : kAtLeast},
         {std::string{kAtLeast}, std::string{kExactly}},
         false},
    };
}

PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& /*record*/,
                                 const MText& mtext, std::string_view name,
                                 const PropertyValue& value) {
    MText result = mtext;
    if (name == "value" || name == "style" || name == "attachment" ||
        name == "line-spacing-style") {
        const std::optional<std::string> text = textOf(value);
        if (!text) {
            return invalid();
        }
        if (name == "value") {
            result.value = *text;
        } else if (name == "style") {
            const auto style = existingStyle(document, *text);
            if (!style) {
                return invalid();
            }
            result.style = *style;
        } else if (name == "attachment") {
            const auto attachment = attachmentNamed(*text);
            if (!attachment) {
                return invalid();
            }
            result.attachment = *attachment;
        } else if (*text == kAtLeast || *text == kExactly) {
            result.exactSpacing = *text == kExactly;
        } else {
            return invalid();
        }
    } else {
        const std::optional<double> number = finiteNumber(value);
        if (!number) {
            return invalid();
        }
        if (name == "width") {
            if (*number < 0.0) {
                return invalid();
            }
            result.width = *number + 0.0;
        } else if (name == "height") {
            if (!(*number > 0.0)) {
                return invalid();
            }
            result.height = *number;
        } else if (name == "rotation") {
            result.rotation = *number;
        } else {
            // line-spacing: finite and positive, as the command MTEXT takes
            // it (#927, [decisione] 8).
            if (!(*number > 0.0)) {
                return invalid();
            }
            result.lineSpacing = *number;
        }
    }
    if (!supported(result)) {
        return invalid();
    }
    return changed(result);
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/,
                                         const AttributeDefinition& definition) {
    const AttributeFlags& flags = definition.flags;
    return {
        {"tag", PropertyKind::Text, definition.tag, {}, false},
        {"prompt", PropertyKind::Text, definition.prompt, {}, false},
        {"default", PropertyKind::Text, definition.text.value, {}, false},
        {"invisible", PropertyKind::Flag, flags.invisible, {}, false},
        {"constant", PropertyKind::Flag, flags.constant, {}, false},
        {"verify", PropertyKind::Flag, flags.verify, {}, false},
        {"preset", PropertyKind::Flag, flags.preset, {}, false},
        {"lock-position", PropertyKind::Flag, definition.lockPosition, {}, false},
        {"height", PropertyKind::Distance, definition.text.height, {}, false},
        {"rotation", PropertyKind::Angle, definition.text.rotation, {}, false},
        {"style", PropertyKind::Text, definition.text.style, {}, false},
    };
}

PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& /*record*/,
                                 const AttributeDefinition& definition, std::string_view name,
                                 const PropertyValue& value) {
    AttributeDefinition result = definition;
    if (name == "tag" || name == "prompt" || name == "default" || name == "style") {
        const std::optional<std::string> text = textOf(value);
        if (!text) {
            return invalid();
        }
        if (name == "tag") {
            if (!validAttributeTag(*text)) {
                return invalid();
            }
            result.tag = *text;
        } else if (name == "prompt") {
            result.prompt = *text;
        } else if (name == "default") {
            result.text.value = *text;
        } else {
            const auto style = existingStyle(document, *text);
            if (!style) {
                return invalid();
            }
            result.text.style = *style;
        }
    } else if (name == "height" || name == "rotation") {
        const std::optional<double> number = finiteNumber(value);
        if (!number) {
            return invalid();
        }
        if (name == "height") {
            if (!(*number > 0.0)) {
                return invalid();
            }
            result.text.height = *number;
        } else {
            result.text.rotation = *number;
        }
    } else {
        // The five flags: invisible, constant, verify, preset, lock-position.
        const std::optional<bool> flag = flagOf(value);
        if (!flag) {
            return invalid();
        }
        if (name == "invisible") {
            result.flags.invisible = *flag;
        } else if (name == "constant") {
            result.flags.constant = *flag;
        } else if (name == "verify") {
            result.flags.verify = *flag;
        } else if (name == "preset") {
            result.flags.preset = *flag;
        } else {
            result.lockPosition = *flag;
        }
    }
    if (!supported(result)) {
        return invalid();
    }
    return changed(result);
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/, const EntityRecord& record,
                                         const Insert& insert) {
    std::vector<OwnProperty> properties{
        {"block", PropertyKind::Text, insert.block, {}, false},
        {"insertion", PropertyKind::Point, insert.insertion, {}, false},
        {"scale", PropertyKind::Point, Vec2{insert.scaleX, insert.scaleY}, {}, false},
        {"scale-z", PropertyKind::Number, scaleZ(record.extras), {}, false},
        {"rotation", PropertyKind::Angle, insert.rotation, {}, false},
    };
    // One per tag, that of the first ATTRIB with it (findAttribute();
    // docs/DECISIONI.md, 2026-11-14, "Revisione delle interfacce PT4", P21).
    for (std::size_t i = 0; i < insert.attributes.size(); ++i) {
        const Attribute& attribute = insert.attributes[i];
        if (firstWithTag(insert, attribute.tag) != i) {
            continue;
        }
        properties.push_back({std::string{kAttributeName},
                              PropertyKind::Choice,
                              quoted(attribute.tag) + ":" + quoted(attribute.text.value),
                              {},
                              false});
    }
    return properties;
}

PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                 const Insert& insert, std::string_view name,
                                 const PropertyValue& value) {
    Insert result = insert;
    if (name == "block") {
        const std::optional<std::string> text = textOf(value);
        if (!text) {
            return invalid();
        }
        const BlockDefinition* definition = document.blocks().find(*text);
        if (definition == nullptr) {
            return invalid();
        }
        // Point, scales, rotation and ATTRIBs as they are, even when the new
        // block has other tags: ATTSYNC aligns them (PT24, point 3).
        result.block = definition->name;
        if (wouldCycle(document, record, result)) {
            return invalid();
        }
        return checked(result);
    }
    if (name == kAttributeName) {
        const std::optional<std::string> text = textOf(value);
        const auto parts = text ? tagAndValue(*text) : std::nullopt;
        if (!parts) {
            return invalid();
        }
        const auto index = firstWithTag(insert, parts->first);
        if (!index) {
            return invalid();
        }
        result.attributes[*index].text.value = parts->second;
        return checked(result);
    }
    if (name == "insertion" || name == "scale") {
        const std::optional<Vec2> point = finitePoint(value);
        if (!point) {
            return invalid();
        }
        if (name == "insertion") {
            result.insertion = *point;
        } else {
            if (point->x == 0.0 || point->y == 0.0) {
                return invalid();
            }
            result.scaleX = point->x;
            result.scaleY = point->y;
        }
        return checked(result);
    }
    const std::optional<double> number = finiteNumber(value);
    if (!number) {
        return invalid();
    }
    if (name == "rotation") {
        result.rotation = *number;
        return checked(result);
    }
    // scale-z: finite and not 0, as the scales of -INSERT (#927,
    // [decisione] 8); only the 43 of the extras changes.
    if (*number == 0.0) {
        return invalid();
    }
    if (!supported(result)) {
        return invalid();
    }
    return PropertyChange{result, withScaleZ(record.extras, *number), {}};
}

}  // namespace tg::core
