// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/attedit_command.hpp"

#include "../name_list.hpp"
#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/color.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/text.hpp"

#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr std::string_view kInvalidKeyword = "Invalid option keyword.";
constexpr std::string_view kLocked = "Attribute position is locked.";

/// The text of an answer, as typed; an option recognised by the runner by its
/// name; Enter as an empty text.
std::optional<std::string> answer(const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        return text->text;
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        return keyword->keyword;
    }
    if (std::holds_alternative<ConfirmInput>(input)) {
        return std::string{};
    }
    return std::nullopt;
}

bool option(std::string_view text, std::string_view name, std::string_view shortcut) {
    return detail::equalsIgnoringCase(text, name) || detail::equalsIgnoringCase(text, shortcut);
}

/// Up to 4 decimals without trailing zeros: "3.5", "0".
std::string shortNumber(double value) {
    std::string text = formatLength(value);
    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') {
            text.pop_back();
        }
        if (text.back() == '.') {
            text.pop_back();
        }
    }
    return text == "-0" ? "0" : text;
}

/// As COLOR shows its default: ByLayer and ByBlock in capitals.
std::string colorLabel(const core::Color& color) {
    std::string label = core::formatColor(color);
    if (color.kind() == core::Color::Kind::ByLayer || color.kind() == core::Color::Kind::ByBlock) {
        for (char& ch : label) {
            ch = detail::toUpperAscii(ch);
        }
    }
    return label;
}

bool onLockedLayer(const core::Document& document, const core::EntityRecord& record) {
    const core::Layer* layer = document.layer(record.properties.layer);
    return layer != nullptr && layer->locked;
}

/// True when the attribute `target` of the drawing is drawn: the rule of
/// core::pickAttribute(), which admits only that attribute and measures
/// with an unbounded tolerance.
bool isDrawn(const core::Document& document, const core::Attribute& target) {
    return core::pickAttribute(
               document, target.text.position, std::numeric_limits<double>::infinity(),
               [&target](const core::Insert& /*insert*/, const core::Attribute& attribute) {
                   return &attribute == &target;
               })
        .has_value();
}

/// The point the text is placed by: the one Position moves.
core::Vec2 insertionOf(const core::Text& text) {
    return core::textFrame(text).insertion;
}

/// `value` with every occurrence of `from`, left to right, replaced by `to`.
std::string replaceAll(std::string value, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return value;
    }
    std::size_t position = value.find(from);
    while (position != std::string::npos) {
        value.replace(position, from.size(), to);
        position = value.find(from, position + to.size());
    }
    return value;
}

const std::vector<Keyword>& yesNo() {
    static const std::vector<Keyword> kKeywords{{"Yes", "Y"}, {"No", "N"}};
    return kKeywords;
}

const std::vector<Keyword>& optionKeywords() {
    static const std::vector<Keyword> kKeywords{{"Value", "V"}, {"Position", "P"}, {"Height", "H"},
                                                {"Angle", "A"}, {"Style", "S"},    {"Layer", "L"},
                                                {"Color", "C"}, {"Next", "N"}};
    return kKeywords;
}

const std::vector<Keyword>& selectKeywords() {
    static const std::vector<Keyword> kKeywords{{"Window", "W"}, {"Crossing", "C"}};
    return kKeywords;
}

const std::vector<Keyword>& modificationKeywords() {
    static const std::vector<Keyword> kKeywords{{"Change", "C"}, {"Replace", "R"}};
    return kKeywords;
}

const std::vector<Keyword>& colorKeywords() {
    static const std::vector<Keyword> kKeywords{{"Truecolor", "T"}, {"Colorbook", "CO"}};
    return kKeywords;
}

}  // namespace

CommandDescription AtteditCommand::description() {
    return {
        "-ATTEDIT",
        {"ATE", "ATTEDIT"},
        "Edits the attributes of block references one at a time or globally.",
        false,
        {{"one-at-a-time", "Edit attributes one at a time? [Yes/No] <Y>:", ValueType::Keyword,
          yesNo(), true},
         {"visible-only", "Edit only attributes visible on screen? [Yes/No] <Y>:",
          ValueType::Keyword, yesNo(), true},
         {"block-specification", "Enter block name specification <*>:", ValueType::Text, {}, true},
         {"tag-specification", "Enter attribute tag specification <*>:", ValueType::Text, {}, true},
         {"value-specification",
          "Enter attribute value specification <*>:",
          ValueType::Text,
          {},
          true},
         {"select-attributes", "Select Attributes:", ValueType::Object, selectKeywords(), true},
         {"first-corner", "Specify first corner:", ValueType::Point, {}, false},
         {"opposite-corner", "Specify opposite corner:", ValueType::Point, {}, false},
         {"option", "Enter an option [Value/Position/Height/Angle/Style/Layer/Color/Next] <N>:",
          ValueType::Keyword, optionKeywords(), true},
         {"value-modification", "Enter type of value modification [Change/Replace] <R>:",
          ValueType::Keyword, modificationKeywords(), true},
         {"string-to-change", "Enter string to change:", ValueType::Text, {}, true},
         {"new-string", "Enter new string:", ValueType::Text, {}, true},
         {"new-value", "Enter new attribute value:", ValueType::Text, {}, true},
         {"insertion-point",
          "Specify new text insertion point <no change>:",
          ValueType::Point,
          {},
          true},
         {"height", "Specify new height <{default}>:", ValueType::Distance, {}, true},
         {"rotation", "Specify new rotation angle <{default}>:", ValueType::Angle, {}, true},
         {"style", "Enter new text style <{default}>:", ValueType::Name, {}, true},
         {"layer", "Enter new layer name <{default}>:", ValueType::Name, {}, true},
         {"color", "Enter new color [Truecolor/Colorbook] <{default}>:", ValueType::Color,
          colorKeywords(), true},
         {"rgb", "Red, Green, Blue:", ValueType::Color, {}, false}}};
}

void AtteditCommand::start(CommandContext& context) {
    context.editor.beginTransaction("Attedit");
}

void AtteditCommand::cancel(CommandContext& context) {
    // Esc keeps the changes already made (docs/adr/0015).
    if (context.editor.inTransaction()) {
        context.editor.commit();
    }
}

Status AtteditCommand::handle(CommandContext& context, const Input& input) {
    switch (m_stage) {
        case Stage::Select:
            if (const auto* object = std::get_if<ObjectInput>(&input)) {
                return pick(context, object->point);
            }
            if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
                m_crossing = keyword->keyword == "Crossing";
                m_stage = Stage::FirstCorner;
            } else if (std::holds_alternative<ConfirmInput>(input)) {
                return chosen(context, {});
            }
            return Status::Continue;
        case Stage::FirstCorner:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                m_firstCorner = point->point;
                m_stage = Stage::OppositeCorner;
            }
            return Status::Continue;
        case Stage::OppositeCorner:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                return selectInRectangle(context, point->point);
            }
            return Status::Continue;
        case Stage::Position:
            if (const auto* point = std::get_if<PointInput>(&input)) {
                const core::Vec2 target = point->point;
                changeCurrent(context, [target](core::Attribute& attribute) {
                    const core::Vec2 shift = target - insertionOf(attribute.text);
                    attribute.text.position = attribute.text.position + shift;
                    attribute.text.alignmentPoint = attribute.text.alignmentPoint + shift;
                });
            } else if (!std::holds_alternative<ConfirmInput>(input)) {
                return Status::Continue;
            }
            m_stage = Stage::Option;
            return Status::Continue;
        case Stage::Height:
        case Stage::Angle: {
            std::optional<double> value;
            if (const auto* number = std::get_if<ValueInput>(&input)) {
                value = number->value;
            } else if (const auto* point = std::get_if<PointInput>(&input);
                       point != nullptr && m_basePoint) {
                const core::Vec2 offset = point->point - *m_basePoint;
                value = m_stage == Stage::Height ? std::hypot(offset.x, offset.y)
                                                 : std::atan2(offset.y, offset.x);
            } else if (!std::holds_alternative<ConfirmInput>(input)) {
                return Status::Continue;
            }
            if (value && m_stage == Stage::Height) {
                if (!std::isfinite(*value) || *value <= 0.0) {
                    context.message("Value must be positive and nonzero.");
                    return Status::Continue;
                }
                const double height = *value;
                changeCurrent(context, [height](core::Attribute& attribute) {
                    attribute.text.height = height;
                });
            } else if (value && std::isfinite(*value)) {
                const double rotation = *value;
                changeCurrent(context, [rotation](core::Attribute& attribute) {
                    attribute.text.rotation = rotation;
                });
            }
            m_stage = Stage::Option;
            return Status::Continue;
        }
        default:
            break;
    }

    const std::optional<std::string> typed = answer(input);
    if (!typed) {
        return Status::Continue;
    }
    const std::string_view text = detail::trim(*typed);
    switch (m_stage) {
        case Stage::OneAtATime:
            if (text.empty() || option(text, "Yes", "Y")) {
                m_oneAtATime = true;
                m_stage = Stage::BlockSpecification;
            } else if (option(text, "No", "N")) {
                m_oneAtATime = false;
                context.message("Performing global editing of attribute values.");
                m_stage = Stage::Visible;
            } else {
                context.message(std::string{kInvalidKeyword});
            }
            break;
        case Stage::Visible:
            if (text.empty() || option(text, "Yes", "Y")) {
                m_visibleOnly = true;
                m_stage = Stage::BlockSpecification;
            } else if (option(text, "No", "N")) {
                m_visibleOnly = false;
                context.message("Drawing must be regenerated afterwards.");
                m_stage = Stage::BlockSpecification;
            } else {
                context.message(std::string{kInvalidKeyword});
            }
            break;
        case Stage::BlockSpecification:
            m_blockSpecification = text.empty() ? "*" : std::string{text};
            m_stage = Stage::TagSpecification;
            break;
        case Stage::TagSpecification:
            m_tagSpecification = text.empty() ? "*" : std::string{text};
            m_stage = Stage::ValueSpecification;
            break;
        case Stage::ValueSpecification:
            m_valueSpecification = text.empty() ? "*" : std::string{text};
            return afterFilters(context);
        case Stage::Option:
            return chooseOption(context, text);
        case Stage::ValueModification:
            if (text.empty() || option(text, "Replace", "R")) {
                m_stage = Stage::NewValue;
            } else if (option(text, "Change", "C")) {
                m_stage = Stage::StringToChange;
            } else {
                context.message(std::string{kInvalidKeyword});
            }
            break;
        case Stage::StringToChange:
        case Stage::GlobalStringToChange:
            m_stringToChange = *typed;
            m_stage = m_stage == Stage::StringToChange ? Stage::NewString : Stage::GlobalNewString;
            break;
        case Stage::NewString: {
            const std::string& from = m_stringToChange;
            const std::string& to = *typed;
            changeCurrent(context, [&from, &to](core::Attribute& attribute) {
                std::string& value = attribute.text.value;
                if (const std::size_t found = from.empty() ? std::string::npos : value.find(from);
                    found != std::string::npos) {
                    value.replace(found, from.size(), to);
                }
            });
            m_stage = Stage::Option;
            break;
        }
        case Stage::GlobalNewString:
            for (const Chosen& chosenAttribute : m_attributes) {
                const core::EntityRecord* record =
                    context.editor.document().find(chosenAttribute.insert);
                const auto* insert =
                    record != nullptr ? std::get_if<core::Insert>(&record->entity) : nullptr;
                if (insert == nullptr || chosenAttribute.index >= insert->attributes.size()) {
                    continue;
                }
                core::Insert changed = *insert;
                std::string& value = changed.attributes[chosenAttribute.index].text.value;
                value = replaceAll(value, m_stringToChange, *typed);
                context.editor.replace(chosenAttribute.insert, std::move(changed));
            }
            return finish(context);
        case Stage::NewValue: {
            const std::string& value = *typed;
            changeCurrent(context,
                          [&value](core::Attribute& attribute) { attribute.text.value = value; });
            m_stage = Stage::Option;
            break;
        }
        case Stage::Style:
            if (!text.empty()) {
                const core::TextStyle* style = context.editor.document().textStyles().find(text);
                if (style == nullptr) {
                    context.message("Cannot find text style \"" + std::string{text} + "\".");
                    break;
                }
                const std::string name = style->name;
                changeCurrent(context,
                              [&name](core::Attribute& attribute) { attribute.text.style = name; });
            }
            m_stage = Stage::Option;
            break;
        case Stage::Layer:
            if (!text.empty()) {
                const core::Layer* layer = context.editor.document().findLayer(text);
                if (layer == nullptr) {
                    context.message("Layer \"" + std::string{text} + "\" not found.");
                    break;
                }
                const core::LayerId id = layer->id;
                changeCurrent(
                    context, [id](core::Attribute& attribute) { attribute.properties.layer = id; });
            }
            m_stage = Stage::Option;
            break;
        case Stage::Color:
        case Stage::TrueColor: {
            if (text.empty()) {
                m_stage = Stage::Option;
                break;
            }
            if (m_stage == Stage::Color && option(text, "Truecolor", "T")) {
                m_stage = Stage::TrueColor;
                break;
            }
            const std::optional<core::Color> color = core::parseColor(text);
            if (!color ||
                (m_stage == Stage::TrueColor && color->kind() != core::Color::Kind::Rgb)) {
                context.message("Invalid color.");
                break;
            }
            const core::Color value = *color;
            changeCurrent(context, [value](core::Attribute& attribute) {
                attribute.properties.color = value;
            });
            m_stage = Stage::Option;
            break;
        }
        default:
            break;
    }
    return Status::Continue;
}

Status AtteditCommand::afterFilters(CommandContext& context) {
    if (m_oneAtATime || m_visibleOnly) {
        m_stage = Stage::Select;
        return Status::Continue;
    }
    // Globally, without the selection: every attribute that passes the
    // filters, the invisible ones too.
    std::vector<Chosen> attributes;
    const core::Document& document = context.editor.document();
    for (const core::EntityRecord& record : document.entities()) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr || !core::supported(*insert) || onLockedLayer(document, record)) {
            continue;
        }
        for (std::size_t index = 0; index < insert->attributes.size(); ++index) {
            if (passesFilters(*insert, insert->attributes[index])) {
                attributes.push_back({record.id, index});
            }
        }
    }
    return chosen(context, std::move(attributes));
}

Status AtteditCommand::pick(CommandContext& context, const core::Vec2& point) {
    const core::Document& document = context.editor.document();
    // The filters go into `accept`, so that they act before the nearest
    // attribute is chosen (docs/tappe/0.8.1/PT19-attedit.md, point 1). A
    // click inside the box of a text is at distance 0.
    const std::optional<core::AttributePick> picked = core::pickAttribute(
        document, point, 0.0, [this](const core::Insert& insert, const core::Attribute& attribute) {
            return passesFilters(insert, attribute);
        });
    std::vector<Chosen> attributes;
    if (picked) {
        const core::EntityRecord* record = document.find(picked->insert);
        if (record != nullptr && !onLockedLayer(document, *record)) {
            attributes.push_back({picked->insert, picked->index});
        }
    }
    return chosen(context, std::move(attributes));
}

Status AtteditCommand::selectInRectangle(CommandContext& context, const core::Vec2& corner) {
    const core::Document& document = context.editor.document();
    const core::BBox rectangle{m_firstCorner, corner};
    std::vector<Chosen> attributes;
    for (const core::EntityRecord& record : document.entities()) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr || !core::supported(*insert) || onLockedLayer(document, record)) {
            continue;
        }
        for (std::size_t index = 0; index < insert->attributes.size(); ++index) {
            const core::Attribute& attribute = insert->attributes[index];
            if (!passesFilters(*insert, attribute)) {
                continue;
            }
            const core::Entity text{attribute.text};
            const bool selected =
                m_crossing ? core::isInside(text, rectangle) || core::intersects(text, rectangle)
                           : core::isInside(text, rectangle);
            if (selected && isDrawn(document, attribute)) {
                attributes.push_back({record.id, index});
            }
        }
    }
    context.message(std::to_string(attributes.size()) + " found");
    return chosen(context, std::move(attributes));
}

Status AtteditCommand::chosen(CommandContext& context, std::vector<Chosen> attributes) {
    context.message(std::to_string(attributes.size()) + " attributes selected.");
    m_attributes = std::move(attributes);
    m_current = 0;
    if (m_attributes.empty()) {
        return finish(context);
    }
    if (m_oneAtATime) {
        m_highlighted = {m_attributes.front().insert};
        m_stage = Stage::Option;
    } else {
        m_stage = Stage::GlobalStringToChange;
    }
    return Status::Continue;
}

Status AtteditCommand::nextAttribute(CommandContext& context) {
    ++m_current;
    if (m_current >= m_attributes.size()) {
        return finish(context);
    }
    m_highlighted = {m_attributes[m_current].insert};
    return Status::Continue;
}

Status AtteditCommand::chooseOption(CommandContext& context, std::string_view text) {
    const core::Document& document = context.editor.document();
    const core::Attribute* attribute = current(document);
    if (attribute == nullptr) {
        return nextAttribute(context);  // the insert has gone
    }
    if (text.empty() || option(text, "Next", "N")) {
        return nextAttribute(context);
    }
    const bool position = option(text, "Position", "P");
    const bool height = option(text, "Height", "H");
    const bool angle = option(text, "Angle", "A");
    if ((position || height || angle) && attribute->lockPosition) {
        context.message(std::string{kLocked});
        return Status::Continue;
    }
    m_basePoint = insertionOf(attribute->text);
    if (option(text, "Value", "V")) {
        m_stage = Stage::ValueModification;
    } else if (position) {
        m_stage = Stage::Position;
    } else if (height) {
        m_default = shortNumber(attribute->text.height);
        m_stage = Stage::Height;
    } else if (angle) {
        m_default = formatAngle(attribute->text.rotation);
        m_stage = Stage::Angle;
    } else if (option(text, "Style", "S")) {
        const core::TextStyle* style = document.textStyles().find(attribute->text.style);
        m_default = style != nullptr ? style->name : attribute->text.style;
        m_stage = Stage::Style;
    } else if (option(text, "Layer", "L")) {
        const core::Layer* layer = document.layer(attribute->properties.layer);
        m_default = layer != nullptr ? layer->name : std::string{};
        m_stage = Stage::Layer;
    } else if (option(text, "Color", "C")) {
        m_default = colorLabel(attribute->properties.color);
        m_stage = Stage::Color;
    } else {
        context.message(std::string{kInvalidKeyword});
    }
    return Status::Continue;
}

Status AtteditCommand::finish(CommandContext& context) {
    // The whole command is one undo step, also when it has changed nothing
    // (docs/adr/0056, point 4): the editor keeps no empty step, so layer 0,
    // which every drawing has, even one without entities, changes its plot
    // state and back, which leaves the drawing as it was.
    edit::Editor& editor = context.editor;
    if (editor.savepoint().changes == 0) {
        const core::Layer* zero = editor.document().layer(editor.document().layerZero());
        if (zero != nullptr) {
            const core::Layer original = *zero;
            core::Layer other = original;
            other.plot = !original.plot;
            if (editor.tables().changeLayer(other)) {
                editor.tables().changeLayer(original);
            }
        }
    }
    editor.commit();
    m_highlighted.clear();
    m_stage = Stage::OneAtATime;
    return Status::Finished;
}

bool AtteditCommand::passesFilters(const core::Insert& insert,
                                   const core::Attribute& attribute) const {
    return detail::matchesNameList(m_blockSpecification, insert.block) &&
           detail::matchesNameList(m_tagSpecification, attribute.tag) &&
           detail::matchesNameList(m_valueSpecification, attribute.text.value);
}

const core::Attribute* AtteditCommand::current(const core::Document& document) const {
    if (m_current >= m_attributes.size()) {
        return nullptr;
    }
    const Chosen& chosenAttribute = m_attributes[m_current];
    const core::EntityRecord* record = document.find(chosenAttribute.insert);
    const auto* insert = record != nullptr ? std::get_if<core::Insert>(&record->entity) : nullptr;
    if (insert == nullptr || chosenAttribute.index >= insert->attributes.size()) {
        return nullptr;
    }
    return &insert->attributes[chosenAttribute.index];
}

template <typename Change>
void AtteditCommand::changeCurrent(CommandContext& context, Change change) {
    if (m_current >= m_attributes.size()) {
        return;
    }
    const Chosen& chosenAttribute = m_attributes[m_current];
    const core::EntityRecord* record = context.editor.document().find(chosenAttribute.insert);
    const auto* insert = record != nullptr ? std::get_if<core::Insert>(&record->entity) : nullptr;
    if (insert == nullptr || chosenAttribute.index >= insert->attributes.size()) {
        return;
    }
    core::Insert changed = *insert;
    change(changed.attributes[chosenAttribute.index]);
    context.editor.replace(chosenAttribute.insert, std::move(changed));
}

void AtteditCommand::pointerMoved(const core::Vec2& /*point*/) {}

Prompt AtteditCommand::prompt() const {
    switch (m_stage) {
        case Stage::OneAtATime:
            return {"Edit attributes one at a time? [Yes/No] <Y>:", yesNo(), PromptKind::Text};
        case Stage::Visible:
            return {"Edit only attributes visible on screen? [Yes/No] <Y>:", yesNo(),
                    PromptKind::Text};
        case Stage::BlockSpecification:
            return {"Enter block name specification <*>:", {}, PromptKind::Text};
        case Stage::TagSpecification:
            return {"Enter attribute tag specification <*>:", {}, PromptKind::Text};
        case Stage::ValueSpecification:
            return {"Enter attribute value specification <*>:", {}, PromptKind::Text};
        case Stage::Select:
            return {"Select Attributes:", selectKeywords(), PromptKind::Object};
        case Stage::FirstCorner:
            return {"Specify first corner:", {}, PromptKind::Point};
        case Stage::OppositeCorner:
            return {"Specify opposite corner:", {}, PromptKind::Point};
        case Stage::Option:
            return {"Enter an option [Value/Position/Height/Angle/Style/Layer/Color/Next] <N>:",
                    optionKeywords(), PromptKind::Text};
        case Stage::ValueModification:
            return {"Enter type of value modification [Change/Replace] <R>:",
                    modificationKeywords(), PromptKind::Text};
        case Stage::StringToChange:
        case Stage::GlobalStringToChange:
            return {"Enter string to change:", {}, PromptKind::Text};
        case Stage::NewString:
        case Stage::GlobalNewString:
            return {"Enter new string:", {}, PromptKind::Text};
        case Stage::NewValue:
            return {"Enter new attribute value:", {}, PromptKind::Text};
        case Stage::Position:
            return {"Specify new text insertion point <no change>:", {}, PromptKind::Point};
        case Stage::Height:
            return {"Specify new height <" + m_default + ">:", {}, PromptKind::Distance};
        case Stage::Angle:
            return {"Specify new rotation angle <" + m_default + ">:", {}, PromptKind::Angle};
        case Stage::Style:
            return {"Enter new text style <" + m_default + ">:", {}, PromptKind::Text};
        case Stage::Layer:
            return {"Enter new layer name <" + m_default + ">:", {}, PromptKind::Text};
        case Stage::Color:
            return {"Enter new color [Truecolor/Colorbook] <" + m_default + ">:", colorKeywords(),
                    PromptKind::Text};
        case Stage::TrueColor:
            return {"Red, Green, Blue:", {}, PromptKind::Text};
    }
    return {"Select Attributes:", selectKeywords(), PromptKind::Object};
}

std::span<const core::Entity> AtteditCommand::preview() const {
    return {};
}

std::optional<core::Vec2> AtteditCommand::basePoint() const {
    switch (m_stage) {
        case Stage::Position:
        case Stage::Height:
        case Stage::Angle:
            return m_basePoint;
        default:
            return std::nullopt;
    }
}

std::span<const core::EntityId> AtteditCommand::highlighted() const {
    return m_highlighted;
}

}  // namespace tg::cmd
