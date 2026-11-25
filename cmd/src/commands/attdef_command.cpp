// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/attdef_command.hpp"

#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"
#include "tg/edit/text_styles.hpp"

#include <charconv>
#include <cmath>

namespace tg::cmd {
namespace {

constexpr int kInvisible = 1;
constexpr int kConstant = 2;
constexpr int kVerify = 4;
constexpr int kPreset = 8;
constexpr int kLockPosition = 16;

std::string yesNo(bool value) {
    return value ? "Y" : "N";
}

}  // namespace

CommandDescription AttdefCommand::description() {
    const std::vector<Keyword> justify = {
        {"Left", "L"}, {"Center", "C"}, {"Right", "R"}, {"Align", "A"}, {"Middle", "M"},
        {"Fit", "F"},  {"TL", "TL"},    {"TC", "TC"},   {"TR", "TR"},   {"ML", "ML"},
        {"MC", "MC"},  {"MR", "MR"},    {"BL", "BL"},   {"BC", "BC"},   {"BR", "BR"}};
    return {
        "-ATTDEF",
        {"ATT", "ATTDEF"},
        "Creates an attribute definition.",
        false,
        {{"modes",
          "Enter an option to change [Invisible/Constant/Verify/Preset/Lock position] <done>:",
          ValueType::Keyword,
          {{"Invisible", "I"},
           {"Constant", "C"},
           {"Verify", "V"},
           {"Preset", "P"},
           {"Lock position", "L"}},
          true},
         {"tag", "Enter attribute tag name:", ValueType::Name, {}, false},
         {"prompt", "Enter attribute prompt:", ValueType::Text, {}, true},
         {"default", "Enter default attribute value:", ValueType::Text, {}, true},
         {"constant", "Enter attribute value:", ValueType::Text, {}, true},
         {"start",
          "Specify start point of text or [Justify/Style]:",
          ValueType::Point,
          {{"Justify", "J"}, {"Style", "S"}},
          false},
         {"justify",
          "Enter an option [Left/Center/Right/Align/Middle/Fit/TL/TC/TR/ML/MC/MR/BL/BC/BR]:",
          ValueType::Keyword, justify, false},
         {"style", "Enter style name or [?] <{default}>:", ValueType::Name, {}, true},
         {"left", "Specify start point of text:", ValueType::Point, {}, false},
         {"center", "Specify center point of text:", ValueType::Point, {}, false},
         {"right", "Specify right endpoint of text baseline:", ValueType::Point, {}, false},
         {"middle", "Specify middle point of text:", ValueType::Point, {}, false},
         {"top-left", "Specify top-left point of text:", ValueType::Point, {}, false},
         {"top-center", "Specify top-center point of text:", ValueType::Point, {}, false},
         {"top-right", "Specify top-right point of text:", ValueType::Point, {}, false},
         {"middle-left", "Specify middle-left point of text:", ValueType::Point, {}, false},
         {"middle-right", "Specify middle-right point of text:", ValueType::Point, {}, false},
         {"bottom-left", "Specify bottom-left point of text:", ValueType::Point, {}, false},
         {"bottom-center", "Specify bottom-center point of text:", ValueType::Point, {}, false},
         {"bottom-right", "Specify bottom-right point of text:", ValueType::Point, {}, false},
         {"first-endpoint",
          "Specify first endpoint of text baseline:",
          ValueType::Point,
          {},
          false},
         {"second-endpoint",
          "Specify second endpoint of text baseline:",
          ValueType::Point,
          {},
          false},
         {"height", "Specify height <{default}>:", ValueType::Distance, {}, true},
         {"rotation", "Specify rotation angle of text <{default}>:", ValueType::Angle, {}, true}}};
}

void AttdefCommand::start(CommandContext& context) {
    m_stage = Stage::Modes;
    m_flags = context.defaults.attributeFlags;
    m_definition = {};
    m_definition.text.height = context.defaults.textHeight;
    m_definition.text.rotation = context.defaults.textRotation;
    m_justify = "Left";
    useStyle(context);
    context.editor.beginTransaction("Attdef");
    reportModes(context);
}

void AttdefCommand::reportModes(CommandContext& context) const {
    context.message("Current attribute modes: Invisible=" + yesNo((m_flags & kInvisible) != 0) +
                    " Constant=" + yesNo((m_flags & kConstant) != 0) + " Verify=" +
                    yesNo((m_flags & kVerify) != 0) + " Preset=" + yesNo((m_flags & kPreset) != 0) +
                    " Lock position=" + yesNo((m_flags & kLockPosition) != 0));
}

void AttdefCommand::reportText(CommandContext& context) const {
    context.message("Current text style:  \"" + m_definition.text.style + "\"  Text height:  " +
                    formatLength(m_definition.text.height) + "  Justify:  " + m_justify);
}

void AttdefCommand::useStyle(const CommandContext& context) {
    const auto& styles = context.editor.document().textStyles();
    m_definition.text.style = styles.current();
    if (const auto* style = styles.find(styles.current())) {
        m_definition.text.widthFactor = style->widthFactor;
        m_definition.text.obliqueAngle = style->oblique;
        if (style->fixedHeight > 0.0) {
            m_definition.text.height = style->fixedHeight;
        }
    }
}

Status AttdefCommand::handle(CommandContext& context, const Input& input) {
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const auto* text = std::get_if<TextInput>(&input);
    const auto* point = std::get_if<PointInput>(&input);
    const auto* value = std::get_if<ValueInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Modes:
            if (confirm) {
                m_stage = Stage::Tag;
            } else if (keyword != nullptr) {
                int bit = 0;
                if (keyword->keyword == "Invisible") {
                    bit = kInvisible;
                }
                if (keyword->keyword == "Constant") {
                    bit = kConstant;
                }
                if (keyword->keyword == "Verify") {
                    bit = kVerify;
                }
                if (keyword->keyword == "Preset") {
                    bit = kPreset;
                }
                if (keyword->keyword == "Lock position") {
                    bit = kLockPosition;
                }
                m_flags ^= bit;
                reportModes(context);
            }
            break;
        case Stage::Tag:
            if (confirm) {
                context.message("Tag cannot be null.");
            } else if (text != nullptr) {
                if (!core::validAttributeTag(text->text)) {
                    context.message("Invalid tag name.");
                } else {
                    m_definition.tag = text->text;
                    m_stage = (m_flags & kConstant) != 0 ? Stage::Value : Stage::AttributePrompt;
                }
            }
            break;
        case Stage::AttributePrompt:
            if (confirm || text != nullptr) {
                m_definition.prompt = text == nullptr ? "" : text->text;
                m_stage = Stage::Value;
            }
            break;
        case Stage::Value:
            if (confirm || text != nullptr) {
                m_definition.text.value = text == nullptr ? "" : text->text;
                reportText(context);
                m_stage = Stage::Start;
            }
            break;
        case Stage::Start:
            if (point != nullptr) {
                m_first = point->point;
                m_definition.text.position = m_first;
                context.lastPoint = m_first;
                afterPoint(context);
            } else if (keyword != nullptr && keyword->keyword == "Justify") {
                m_stage = Stage::Justify;
            } else if (keyword != nullptr && keyword->keyword == "Style") {
                m_stage = Stage::Style;
            }
            break;
        case Stage::Justify:
            if (keyword != nullptr) {
                m_justify = keyword->keyword;
                const auto& j = m_justify;
                m_definition.text.horizontal = core::TextHorizontal::Left;
                m_definition.text.vertical = core::TextVertical::Baseline;
                if (j == "Center" || j == "TC" || j == "MC" || j == "BC") {
                    m_definition.text.horizontal = core::TextHorizontal::Center;
                }
                if (j == "Right" || j == "TR" || j == "MR" || j == "BR") {
                    m_definition.text.horizontal = core::TextHorizontal::Right;
                }
                if (j == "Align") {
                    m_definition.text.horizontal = core::TextHorizontal::Aligned;
                }
                if (j == "Middle") {
                    m_definition.text.horizontal = core::TextHorizontal::Middle;
                }
                if (j == "Fit") {
                    m_definition.text.horizontal = core::TextHorizontal::Fit;
                }
                if (j == "TL" || j == "TC" || j == "TR") {
                    m_definition.text.vertical = core::TextVertical::Top;
                }
                if (j == "ML" || j == "MC" || j == "MR" || j == "Middle") {
                    m_definition.text.vertical = core::TextVertical::Middle;
                }
                if (j == "BL" || j == "BC" || j == "BR") {
                    m_definition.text.vertical = core::TextVertical::Bottom;
                }
                m_stage = Stage::FirstPoint;
            }
            break;
        case Stage::Style:
            if (confirm) {
                reportText(context);
                m_stage = Stage::Start;
            } else if (text != nullptr) {
                const auto* style = context.editor.document().textStyles().find(text->text);
                if (style == nullptr) {
                    context.message("Cannot find text style \"" + text->text + "\".");
                } else {
                    edit::setCurrentTextStyle(context.editor, style->name);
                    useStyle(context);
                    reportText(context);
                    m_stage = Stage::Start;
                }
            }
            break;
        case Stage::FirstPoint:
            if (point != nullptr) {
                m_first = point->point;
                m_definition.text.position = m_first;
                m_definition.text.alignmentPoint = m_first;
                context.lastPoint = m_first;
                if (m_justify == "Align" || m_justify == "Fit") {
                    m_stage = Stage::SecondPoint;
                } else {
                    afterPoint(context);
                }
            }
            break;
        case Stage::SecondPoint:
            if (point != nullptr) {
                m_definition.text.alignmentPoint = point->point;
                context.lastPoint = point->point;
                if (m_justify == "Align") {
                    return finish(context);
                }
                m_stage = Stage::Height;
            }
            break;
        case Stage::Height:
            if (point != nullptr || value != nullptr || confirm) {
                double height = m_definition.text.height;
                if (point != nullptr) {
                    height = core::distance(m_first, point->point);
                }
                if (value != nullptr) {
                    height = value->value;
                }
                if (!(height > 0.0) || !std::isfinite(height)) {
                    context.message("Value must be positive and nonzero.");
                    break;
                }
                m_definition.text.height = height;
                if (m_justify == "Fit") {
                    return finish(context);
                }
                m_stage = Stage::Rotation;
            }
            break;
        case Stage::Rotation:
            if (point != nullptr || value != nullptr || confirm) {
                double rotation = m_definition.text.rotation;
                if (point != nullptr) {
                    const auto delta = point->point - m_first;
                    rotation = core::reducedDegreesToRadians(
                        core::radiansToDegrees(core::atan2(delta.y, delta.x)));
                }
                if (value != nullptr) {
                    rotation = value->value;
                }
                if (!std::isfinite(rotation)) {
                    break;
                }
                m_definition.text.rotation = rotation;
                return finish(context);
            }
            break;
    }
    return Status::Continue;
}

void AttdefCommand::afterPoint(const CommandContext& context) {
    const auto* style = context.editor.document().textStyles().find(m_definition.text.style);
    m_stage = style != nullptr && style->fixedHeight > 0.0 ? Stage::Rotation : Stage::Height;
}

Status AttdefCommand::finish(CommandContext& context) {
    m_definition.flags = {(m_flags & kInvisible) != 0, (m_flags & kConstant) != 0,
                          (m_flags & kVerify) != 0, (m_flags & kPreset) != 0};
    m_definition.lockPosition = (m_flags & kLockPosition) != 0;
    context.editor.add(m_definition);
    context.defaults.attributeFlags = m_flags;
    context.defaults.textHeight = m_definition.text.height;
    context.defaults.textRotation = m_definition.text.rotation;
    context.editor.commit();
    return Status::Finished;
}

Prompt AttdefCommand::prompt() const {
    switch (m_stage) {
        case Stage::Modes:
            return {
                "Enter an option to change [Invisible/Constant/Verify/Preset/Lock position] "
                "<done>:",
                {{"Invisible", "I"},
                 {"Constant", "C"},
                 {"Verify", "V"},
                 {"Preset", "P"},
                 {"Lock position", "L"}}};
        case Stage::Tag:
            return {"Enter attribute tag name:", {}, PromptKind::Text};
        case Stage::AttributePrompt:
            return {"Enter attribute prompt:", {}, PromptKind::Text};
        case Stage::Value:
            return {(m_flags & kConstant) != 0 ? "Enter attribute value:"
                                               : "Enter default attribute value:",
                    {},
                    PromptKind::Text};
        case Stage::Start:
            return {"Specify start point of text or [Justify/Style]:",
                    {{"Justify", "J"}, {"Style", "S"}}};
        case Stage::Justify:
            return {
                "Enter an option [Left/Center/Right/Align/Middle/Fit/TL/TC/TR/ML/MC/MR/BL/BC/BR]:",
                {{"Left", "L"},
                 {"Center", "C"},
                 {"Right", "R"},
                 {"Align", "A"},
                 {"Middle", "M"},
                 {"Fit", "F"},
                 {"TL", "TL"},
                 {"TC", "TC"},
                 {"TR", "TR"},
                 {"ML", "ML"},
                 {"MC", "MC"},
                 {"MR", "MR"},
                 {"BL", "BL"},
                 {"BC", "BC"},
                 {"BR", "BR"}}};
        case Stage::Style:
            return {
                "Enter style name or [?] <" + m_definition.text.style + ">:", {}, PromptKind::Text};
        case Stage::FirstPoint: {
            std::string name = m_justify;
            if (name == "Left") {
                return {"Specify start point of text:", {}};
            }
            if (name == "Center") {
                return {"Specify center point of text:", {}};
            }
            if (name == "Right") {
                return {"Specify right endpoint of text baseline:", {}};
            }
            if (name == "Middle" || name == "MC") {
                return {"Specify middle point of text:", {}};
            }
            if (name == "Align" || name == "Fit") {
                return {"Specify first endpoint of text baseline:", {}};
            }
            const std::string vertical = [&name] {
                if (name[0] == 'T') {
                    return "top";
                }
                return name[0] == 'M' ? "middle" : "bottom";
            }();
            const std::string horizontal = [&name] {
                if (name[1] == 'L') {
                    return "left";
                }
                return name[1] == 'C' ? "center" : "right";
            }();
            return {"Specify " + vertical + "-" + horizontal + " point of text:", {}};
        }
        case Stage::SecondPoint:
            return {"Specify second endpoint of text baseline:", {}};
        case Stage::Height:
            return {"Specify height <" + formatLength(m_definition.text.height) + ">:",
                    {},
                    PromptKind::Distance};
        case Stage::Rotation: {
            auto angle = formatAngle(m_definition.text.rotation);
            return {"Specify rotation angle of text <" + (angle == "360" ? "0" : angle) + ">:",
                    {},
                    PromptKind::Angle};
        }
    }
    return {};
}

std::optional<core::Vec2> AttdefCommand::basePoint() const {
    if (m_stage == Stage::SecondPoint || m_stage == Stage::Height || m_stage == Stage::Rotation) {
        return m_first;
    }
    return std::nullopt;
}

CommandDescription AflagsCommand::description() {
    return {"AFLAGS",
            {},
            "Sets the default attribute modes.",
            false,
            {{"value", "Enter new value for AFLAGS <{default}>:", ValueType::Integer, {}, true}}};
}

void AflagsCommand::start(CommandContext& context) {
    m_value = context.defaults.attributeFlags;
}

Status AflagsCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        auto value = detail::trim(text->text);
        if (value.starts_with('+')) {
            value.remove_prefix(1);
        }
        int number = 0;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || number < 0 ||
            number > 31) {
            context.message("Requires an integer between 0 and 31.");
            return Status::Continue;
        }
        context.defaults.attributeFlags = number;
        return Status::Finished;
    }
    return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
}

Prompt AflagsCommand::prompt() const {
    return {"Enter new value for AFLAGS <" + std::to_string(m_value) + ">:", {}, PromptKind::Text};
}

}  // namespace tg::cmd
