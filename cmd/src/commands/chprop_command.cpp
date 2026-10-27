// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/chprop_command.hpp"

#include "../text.hpp"
#include "tg/cmd/coordinate_input.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/color.hpp"
#include "tg/core/lineweight.hpp"

#include <cmath>
#include <optional>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

// What the question shows between < > when the selected entities differ.
constexpr std::string_view kVaries = "varies";

// The text of an answer: what was typed, or the option the runner recognised;
// Enter is an empty text.
std::optional<std::string_view> answer(const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        return detail::trim(text->text);
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        return keyword->keyword;
    }
    if (std::holds_alternative<ConfirmInput>(input)) {
        return std::string_view{};
    }
    return std::nullopt;
}

bool option(std::string_view text, std::string_view name, std::string_view shortcut) {
    return detail::equalsIgnoringCase(text, name) || detail::equalsIgnoringCase(text, shortcut);
}

// As COLOR shows its default: ByLayer and ByBlock in capitals.
std::string colorLabel(const core::Color& color) {
    std::string label = core::formatColor(color);
    if (color.kind() == core::Color::Kind::ByLayer || color.kind() == core::Color::Kind::ByBlock) {
        for (char& ch : label) {
            ch = detail::toUpperAscii(ch);
        }
    }
    return label;
}

// As LWEIGHT shows its value: the three special values by name.
std::string weightLabel(int value) {
    if (value == core::lineweight::kByLayer) {
        return "ByLayer";
    }
    if (value == core::lineweight::kByBlock) {
        return "ByBlock";
    }
    if (value == core::lineweight::kDefault) {
        return "Default";
    }
    return core::formatLineweight(value);
}

// The value of `field` that every entity of `ids` has, or std::nullopt if
// they differ. An entity the document lacks is not counted, so with none
// left the result is std::nullopt as well.
template <typename Value>
std::optional<Value> sharedValue(const core::Document& document,
                                 std::span<const core::EntityId> ids,
                                 Value core::EntityProperties::*field) {
    std::optional<Value> shared;
    for (const core::EntityId id : ids) {
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr) {
            continue;
        }
        const Value& value = record->properties.*field;
        if (!shared) {
            shared = value;
        } else if (*shared != value) {
            return std::nullopt;
        }
    }
    return shared;
}

// The text between < > for `field`: the shared value as `format` writes it,
// or "varies".
template <typename Value, typename Format>
std::string sharedLabel(const core::Document& document, std::span<const core::EntityId> ids,
                        Value core::EntityProperties::*field, Format format) {
    const std::optional<Value> shared = sharedValue(document, ids, field);
    return shared ? format(*shared) : std::string{kVaries};
}

// Gives every entity of `ids` the properties that `change` makes of its
// current ones. An entity that already has them, or is gone, is left alone.
template <typename Change>
void changeAll(CommandContext& context, std::span<const core::EntityId> ids, Change change) {
    for (const core::EntityId id : ids) {
        const core::EntityRecord* record = context.editor.document().find(id);
        if (record == nullptr) {
            continue;
        }
        core::EntityProperties properties = record->properties;  // a copy: setProperties moves it
        change(properties);
        context.editor.setProperties(id, properties);
    }
}

}  // namespace

void ChpropCommand::start(CommandContext& context) {
    context.editor.beginTransaction("CHPROP");
}

void ChpropCommand::cancel(CommandContext& context) {
    // Esc keeps the changes already made (docs/adr/0015).
    context.editor.commit();
}

Status ChpropCommand::handle(CommandContext& context, const Input& input) {
    if (m_stage == Stage::Selection) {
        if (const auto* selection = std::get_if<SelectionInput>(&input)) {
            m_ids = selection->ids;
            m_stage = Stage::Property;
        }
        return Status::Continue;
    }
    const std::optional<std::string_view> text = answer(input);
    if (!text) {
        return Status::Continue;
    }
    switch (m_stage) {
        case Stage::Selection:
            break;  // handled above
        case Stage::Property:
            return chooseProperty(context, *text);
        case Stage::Color:
        case Stage::TrueColor:
            setColor(context, *text);
            break;
        case Stage::Layer:
            setLayer(context, *text);
            break;
        case Stage::Linetype:
            setLinetype(context, *text);
            break;
        case Stage::LinetypeScale:
            setLinetypeScale(context, *text);
            break;
        case Stage::Lineweight:
            setLineweight(context, *text);
            break;
    }
    return Status::Continue;
}

Prompt ChpropCommand::prompt() const {
    switch (m_stage) {
        case Stage::Selection:
            break;
        case Stage::Property:
            return {"Enter property to change [Color/LAyer/LType/ltScale/LWeight]:",
                    {{"Color", "C"},
                     {"LAyer", "LA"},
                     {"LType", "LT"},
                     {"ltScale", "LTS"},
                     {"LWeight", "LW"}},
                    PromptKind::Text};
        case Stage::Color:
            return {"New color [Truecolor] <" + m_shared + ">:",
                    {{"Truecolor", "T"}},
                    PromptKind::Text};
        case Stage::TrueColor:
            return {"Red, Green, Blue:", {}, PromptKind::Text};
        case Stage::Layer:
            return {"Enter new layer name <" + m_shared + ">:", {}, PromptKind::Text};
        case Stage::Linetype:
            return {"Enter new linetype name <" + m_shared + ">:", {}, PromptKind::Text};
        case Stage::LinetypeScale:
            return {"Specify new linetype scale <" + m_shared + ">:", {}, PromptKind::Text};
        case Stage::Lineweight:
            return {"Enter new lineweight <" + m_shared + ">:", {}, PromptKind::Text};
    }
    // Before the selection arrives the runner only needs a question that is
    // not empty: it is its own "Select objects:" that the user sees.
    return {"Select objects:", {}};
}

void ChpropCommand::ask(Stage stage, std::string shared) {
    m_stage = stage;
    m_shared = std::move(shared);
}

Status ChpropCommand::chooseProperty(CommandContext& context, std::string_view text) {
    if (text.empty()) {
        context.editor.commit();
        return Status::Finished;
    }
    const core::Document& document = context.editor.document();
    if (option(text, "Color", "C")) {
        ask(Stage::Color, sharedLabel(document, m_ids, &core::EntityProperties::color, colorLabel));
    } else if (option(text, "LAyer", "LA")) {
        ask(Stage::Layer, sharedLabel(document, m_ids, &core::EntityProperties::layer,
                                      [&document](core::LayerId id) {
                                          const core::Layer* layer = document.layer(id);
                                          return layer != nullptr ? layer->name : std::string{};
                                      }));
    } else if (option(text, "LType", "LT")) {
        ask(Stage::Linetype, sharedLabel(document, m_ids, &core::EntityProperties::linetype,
                                         [&document](core::LinetypeId id) {
                                             const core::Linetype* type = document.linetype(id);
                                             return type != nullptr ? type->name : std::string{};
                                         }));
    } else if (option(text, "ltScale", "LTS")) {
        ask(Stage::LinetypeScale,
            sharedLabel(document, m_ids, &core::EntityProperties::linetypeScale, formatLength));
    } else if (option(text, "LWeight", "LW")) {
        ask(Stage::Lineweight,
            sharedLabel(document, m_ids, &core::EntityProperties::lineweight, weightLabel));
    } else {
        context.message("Invalid option keyword.");
    }
    return Status::Continue;
}

void ChpropCommand::setColor(CommandContext& context, std::string_view text) {
    if (text.empty()) {
        m_stage = Stage::Property;  // leave the colors as they are
        return;
    }
    if (m_stage == Stage::Color && option(text, "Truecolor", "T")) {
        m_stage = Stage::TrueColor;
        return;
    }
    const std::optional<core::Color> color = core::parseColor(text);
    if (!color || (m_stage == Stage::TrueColor && color->kind() != core::Color::Kind::Rgb)) {
        context.message("Invalid color.");
        return;
    }
    changeAll(context, m_ids,
              [&color](core::EntityProperties& properties) { properties.color = *color; });
    m_stage = Stage::Property;
}

void ChpropCommand::setLayer(CommandContext& context, std::string_view name) {
    if (name.empty()) {
        m_stage = Stage::Property;
        return;
    }
    // A frozen layer is accepted, as in AutoCAD: the entities disappear.
    const core::Layer* layer = context.editor.document().findLayer(name);
    if (layer == nullptr) {
        context.message("Layer \"" + std::string(name) + "\" not found.");
        return;
    }
    const core::LayerId id = layer->id;  // a copy: the table changes under the pointer
    changeAll(context, m_ids, [id](core::EntityProperties& properties) { properties.layer = id; });
    m_stage = Stage::Property;
}

void ChpropCommand::setLinetype(CommandContext& context, std::string_view name) {
    if (name.empty()) {
        m_stage = Stage::Property;
        return;
    }
    const core::Linetype* type = context.editor.document().findLinetype(name);
    if (type == nullptr) {
        context.message("Linetype \"" + std::string(name) + "\" is not loaded.");
        return;
    }
    const core::LinetypeId id = type->id;
    changeAll(context, m_ids,
              [id](core::EntityProperties& properties) { properties.linetype = id; });
    m_stage = Stage::Property;
}

void ChpropCommand::setLinetypeScale(CommandContext& context, std::string_view text) {
    if (text.empty()) {
        m_stage = Stage::Property;
        return;
    }
    const std::optional<PointSpec> number = parseCoordinate(text);
    if (!number || number->kind != PointSpec::Kind::Distance || !std::isfinite(number->a) ||
        number->a <= 0.0) {
        context.message("Requires a positive number.");
        return;
    }
    const double scale = number->a;
    changeAll(context, m_ids,
              [scale](core::EntityProperties& properties) { properties.linetypeScale = scale; });
    m_stage = Stage::Property;
}

void ChpropCommand::setLineweight(CommandContext& context, std::string_view text) {
    if (text.empty()) {
        m_stage = Stage::Property;
        return;
    }
    const std::optional<int> weight = core::parseLineweight(text);
    if (!weight) {
        context.message("Invalid lineweight.");
        return;
    }
    const int value = *weight;
    changeAll(context, m_ids,
              [value](core::EntityProperties& properties) { properties.lineweight = value; });
    m_stage = Stage::Property;
}

}  // namespace tg::cmd
