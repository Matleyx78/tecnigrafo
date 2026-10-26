// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/layer_commands.hpp"

#include "../name_list.hpp"
#include "../text.hpp"
#include "tg/edit/layer_rules.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

namespace tg::cmd {
namespace {

std::string currentName(const CommandContext& context) {
    const auto& document = context.editor.document();
    return document.layer(document.current().layer)->name;
}

bool setCurrent(CommandContext& context, std::string_view name) {
    const auto& document = context.editor.document();
    const auto* layer = document.findLayer(name);
    if (layer == nullptr) {
        context.message("Layer \"" + std::string(name) + "\" not found.");
        return false;
    }
    if (layer->frozen) {
        context.message("Cannot set current to a frozen layer.");
        return false;
    }
    auto current = document.current();
    current.layer = layer->id;
    context.editor.setCurrent(current);
    return true;
}

bool validName(CommandContext& context, std::string_view name) {
    if (core::isValidTableName(name)) {
        return true;
    }
    context.message("Invalid layer name.");
    return false;
}

void alreadyExists(CommandContext& context, std::string_view name) {
    context.message("Layer \"" + std::string(name) + "\" already exists.");
}

void createLayer(CommandContext& context, std::string name) {
    core::Layer layer;
    layer.name = std::move(name);
    layer.linetype = context.editor.document().continuousLinetype();
    context.editor.addLayer(std::move(layer));
}

Prompt textPrompt(std::string text, std::vector<Keyword> keywords = {}) {
    return {std::move(text), std::move(keywords), PromptKind::Text};
}

bool isOption(std::string_view text, const Keyword& keyword) {
    return detail::equalsIgnoringCase(text, keyword.name) ||
           detail::equalsIgnoringCase(text, keyword.shortcut);
}

}  // namespace

void LayerCommand::start(CommandContext& context) {
    context.editor.beginTransaction("Layer");
    m_currentName = currentName(context);
    context.message("Current layer:  \"" + m_currentName + "\"");
}

void LayerCommand::cancel(CommandContext& context) {
    context.editor.commit();
}

Status LayerCommand::handle(CommandContext& context, const Input& input) {
    const auto* typed = std::get_if<TextInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    if (typed == nullptr && keyword == nullptr && !std::holds_alternative<ConfirmInput>(input)) {
        return Status::Continue;
    }
    std::string_view text;
    if (typed != nullptr) {
        text = detail::trim(typed->text);
    } else if (keyword != nullptr) {
        text = keyword->keyword;
    }
    if (m_stage == Stage::Option) {
        if (text.empty()) {
            context.editor.commit();
            return Status::Finished;
        }
        chooseOption(context, text);
    } else if (m_stage == Stage::List) {
        list(context, text.empty() ? "*" : text);
        m_stage = Stage::Option;
    } else if (m_stage == Stage::Names) {
        applyNames(context, text.empty() ? m_currentName : text);
    } else if (m_stage == Stage::ConfirmOff) {
        if (detail::equalsIgnoringCase(text, "Yes") || detail::equalsIgnoringCase(text, "Y")) {
            auto layer = *context.editor.document().layer(m_offId);
            layer.on = false;
            context.editor.changeLayer(layer);
        } else if (!text.empty() && !detail::equalsIgnoringCase(text, "No") &&
                   !detail::equalsIgnoringCase(text, "N")) {
            context.message("Invalid option keyword.");
            return Status::Continue;
        }
        m_stage = Stage::Option;
    } else if (m_stage == Stage::Make || m_stage == Stage::Set || m_stage == Stage::New ||
               m_stage == Stage::RenameOld || m_stage == Stage::RenameNew) {
        handleName(context, typed != nullptr ? std::string_view(typed->text) : text);
    } else {
        handleValue(context, text);
    }
    m_currentName = currentName(context);
    return Status::Continue;
}

void LayerCommand::chooseOption(CommandContext& context, std::string_view text) {
    const auto options = description().prompts.front().keywords;
    const auto found = std::ranges::find_if(
        options, [text](const Keyword& option) { return isOption(text, option); });
    if (found == options.end()) {
        context.message("Invalid option keyword.");
        return;
    }
    constexpr std::array<Stage, 15> kStages{
        Stage::List,
        Stage::Make,
        Stage::Set,
        Stage::New,
        Stage::RenameOld,
        Stage::Names,
        Stage::Names,
        Stage::Color,
        Stage::Linetype,
        Stage::Lineweight,
        Stage::Plot,
        Stage::Names,
        Stage::Names,
        Stage::Names,
        Stage::Names,
    };
    constexpr std::array<Operation, 15> kOperations{
        Operation::On,
        Operation::On,
        Operation::On,
        Operation::On,
        Operation::On,
        Operation::On,
        Operation::Off,
        Operation::Color,
        Operation::Linetype,
        Operation::Lineweight,
        Operation::Plot,
        Operation::Freeze,
        Operation::Thaw,
        Operation::Lock,
        Operation::Unlock,
    };
    const auto index = static_cast<std::size_t>(found - options.begin());
    m_stage = kStages.at(index);
    m_operation = kOperations.at(index);
}

void LayerCommand::handleName(CommandContext& context, std::string_view text) {
    if (text.empty()) {
        if (m_stage == Stage::Make) {
            setCurrent(context, m_currentName);
        }
        m_stage = Stage::Option;
        return;
    }
    if (m_stage == Stage::New) {
        const auto names = detail::splitNameList(text);
        for (const auto& name : names) {
            if (!validName(context, name)) {
                return;
            }
        }
        for (const auto& name : names) {
            if (context.editor.document().findLayer(name) != nullptr) {
                alreadyExists(context, name);
            } else {
                createLayer(context, name);
            }
        }
        m_stage = Stage::Option;
        return;
    }
    if (!validName(context, text)) {
        return;
    }
    const auto& document = context.editor.document();
    if (m_stage == Stage::Make) {
        if (document.findLayer(text) == nullptr) {
            createLayer(context, std::string(text));
        }
        setCurrent(context, text);
    } else if (m_stage == Stage::Set) {
        setCurrent(context, text);
    } else if (m_stage == Stage::RenameOld) {
        const auto* layer = document.findLayer(text);
        if (layer == nullptr) {
            context.message("Layer \"" + std::string(text) + "\" not found.");
        } else if (layer->id == document.layerZero()) {
            context.message("Cannot rename layer \"0\".");
        } else {
            m_renameId = layer->id;
            m_oldName = layer->name;
            m_stage = Stage::RenameNew;
            return;
        }
    } else {
        if (edit::checkRename(document, m_renameId, text) == edit::LayerProblem::NameInUse) {
            alreadyExists(context, text);
        } else {
            if (edit::namedInOtherUnhandledData(document, m_renameId)) {
                context.message("Layer \"" + m_oldName +
                                "\" is also named in data Tecnigrafo does not handle; that "
                                "data keeps the old name.");
            }
            context.editor.renameLayer(m_renameId, std::string(text));
        }
    }
    m_stage = Stage::Option;
}

void LayerCommand::handleValue(CommandContext& context, std::string_view text) {
    if (m_stage == Stage::Color &&
        (detail::equalsIgnoringCase(text, "Truecolor") || detail::equalsIgnoringCase(text, "T"))) {
        m_stage = Stage::Rgb;
        return;
    }
    if (m_stage == Stage::Color || m_stage == Stage::Rgb) {
        const auto color = core::parseColor(text);
        const auto required =
            m_stage == Stage::Rgb ? core::Color::Kind::Rgb : core::Color::Kind::Index;
        if (!color || color->kind() != required) {
            if (m_stage == Stage::Rgb) {
                context.message("Requires red, green, blue values from 0 to 255.");
            } else {
                context.message("Requires a color index from 1 to 255.");
            }
            return;
        }
        m_color = *color;
    } else if (m_stage == Stage::Linetype) {
        const auto& document = context.editor.document();
        const auto name = text.empty() ? std::string_view("Continuous") : text;
        const auto* linetype = document.findLinetype(name);
        if (linetype == nullptr || linetype->id == document.byLayerLinetype() ||
            linetype->id == document.byBlockLinetype()) {
            context.message("Linetype \"" + std::string(name) + "\" is not loaded.");
            m_stage = Stage::Option;
            return;
        }
        m_linetype = linetype->id;
        m_linetypeName = linetype->name;
    } else if (m_stage == Stage::Lineweight) {
        const auto weight = core::parseLineweight(text);
        if (!weight || (*weight < 0 && *weight != core::lineweight::kDefault)) {
            context.message("Requires a valid lineweight.");
            return;
        }
        m_lineweight = *weight;
    } else {
        if (text.empty() || detail::equalsIgnoringCase(text, "Plot") ||
            detail::equalsIgnoringCase(text, "P")) {
            m_plot = true;
        } else if (detail::equalsIgnoringCase(text, "No plot") ||
                   detail::equalsIgnoringCase(text, "N")) {
            m_plot = false;
        } else {
            context.message("Invalid option keyword.");
            return;
        }
    }
    m_stage = Stage::Names;
}

void LayerCommand::applyNames(CommandContext& context, std::string_view text) {
    const auto& document = context.editor.document();
    const auto ids = detail::matchingLayers(document, text);
    m_stage = Stage::Option;
    if (ids.empty()) {
        context.message("No matching layer names found.");
        return;
    }
    for (const auto id : ids) {
        auto layer = *document.layer(id);
        switch (m_operation) {
            case Operation::On:
                layer.on = true;
                break;
            case Operation::Off:
                if (id == document.current().layer) {
                    m_offId = id;
                    m_stage = Stage::ConfirmOff;
                    continue;
                }
                layer.on = false;
                break;
            case Operation::Freeze:
                if (edit::checkFreeze(document, id) == edit::LayerProblem::Current) {
                    context.message("Cannot freeze layer \"" + layer.name +
                                    "\".  It is the CURRENT layer.");
                    continue;
                }
                layer.frozen = true;
                break;
            case Operation::Thaw:
                layer.frozen = false;
                break;
            case Operation::Lock:
                layer.locked = true;
                break;
            case Operation::Unlock:
                layer.locked = false;
                break;
            case Operation::Color:
                layer.color = m_color;
                break;
            case Operation::Linetype:
                layer.linetype = m_linetype;
                break;
            case Operation::Lineweight:
                layer.lineweight = m_lineweight;
                break;
            case Operation::Plot:
                layer.plot = m_plot;
                break;
        }
        context.editor.changeLayer(layer);
    }
}

void LayerCommand::list(CommandContext& context, std::string_view text) const {
    const auto& document = context.editor.document();
    const auto ids = detail::matchingLayers(document, text);
    if (ids.empty()) {
        context.message("No matching layer names found.");
    }
    for (const auto id : ids) {
        const auto& layer = *document.layer(id);
        std::string message = layer.name + "  " + (layer.on ? "on" : "off");
        message += layer.frozen ? " frozen" : " thawed";
        message += layer.locked ? " locked" : " unlocked";
        message += " " + core::formatColor(layer.color);
        message += " \"" + document.linetype(layer.linetype)->name + "\" ";
        message += core::formatLineweight(layer.lineweight);
        context.message(std::move(message));
    }
    context.message("Current layer: \"" + m_currentName + "\"");
}

Prompt LayerCommand::prompt() const {
    switch (m_stage) {
        case Stage::Option:
            return textPrompt("Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/"
                              "Plot/Freeze/Thaw/LOck/Unlock]:",
                              description().prompts.front().keywords);
        case Stage::List:
            return textPrompt("Enter layer name(s) to list <*>:");
        case Stage::Make:
            return textPrompt(
                "Enter name for new layer (becomes the current layer) <" + m_currentName + ">:");
        case Stage::Set:
            return textPrompt("Enter layer name to make current:");
        case Stage::New:
            return textPrompt("Enter name list for new layer(s):");
        case Stage::RenameOld:
            return textPrompt("Enter old layer name:");
        case Stage::RenameNew:
            return textPrompt("Enter new layer name:");
        case Stage::Color:
            return textPrompt("New color [Truecolor]:", {{"Truecolor", "T"}});
        case Stage::Rgb:
            return textPrompt("Red, Green, Blue:");
        case Stage::Linetype:
            return textPrompt("Enter a loaded linetype name <Continuous>:");
        case Stage::Lineweight:
            return textPrompt("Enter lineweight (0.0mm - 2.11mm):");
        case Stage::Plot:
            return textPrompt("Enter a plotting preference [Plot/No plot] <Plot>:",
                              {{"Plot", "P"}, {"No plot", "N"}});
        case Stage::ConfirmOff:
            return textPrompt(
                "Really want layer " + m_currentName + " (the CURRENT layer) off? [Yes/No] <No>:",
                {{"Yes", "Y"}, {"No", "N"}});
        case Stage::Names:
            break;
    }
    switch (m_operation) {
        case Operation::On:
            return textPrompt("Enter name list of layer(s) to turn on:");
        case Operation::Off:
            return textPrompt("Enter name list of layer(s) to turn off:");
        case Operation::Freeze:
            return textPrompt("Enter name list of layer(s) to freeze:");
        case Operation::Thaw:
            return textPrompt("Enter name list of layer(s) to thaw:");
        case Operation::Lock:
            return textPrompt("Enter name list of layer(s) to lock:");
        case Operation::Unlock:
            return textPrompt("Enter name list of layer(s) to unlock:");
        case Operation::Color:
            return textPrompt("Enter name list of layer(s) for color " +
                              core::formatColor(m_color) + " <" + m_currentName + ">:");
        case Operation::Linetype:
            return textPrompt("Enter name list of layer(s) for linetype \"" + m_linetypeName +
                              "\" <" + m_currentName + ">:");
        case Operation::Lineweight: {
            const auto weight = core::formatLineweight(m_lineweight);
            return textPrompt("Enter name list of layer(s) for lineweight " + weight +
                              (m_lineweight >= 0 ? "mm" : "") + " <" + m_currentName + ">:");
        }
        case Operation::Plot:
            return textPrompt("Enter name list of layer(s) for this plot preference <" +
                              m_currentName + ">:");
    }
    return {};
}

void ClayerCommand::start(CommandContext& context) {
    m_currentName = currentName(context);
}

Status ClayerCommand::handle(CommandContext& context, const Input& input) {
    if (std::holds_alternative<ConfirmInput>(input)) {
        return Status::Finished;
    }
    const auto* text = std::get_if<TextInput>(&input);
    if (text == nullptr) {
        return Status::Continue;
    }
    if (!validName(context, text->text)) {
        return Status::Continue;
    }
    context.editor.beginTransaction("CLAYER");
    if (!setCurrent(context, text->text)) {
        context.editor.rollback();
        return Status::Continue;
    }
    context.editor.commit();
    return Status::Finished;
}

Prompt ClayerCommand::prompt() const {
    return {"Enter new value for CLAYER <\"" + m_currentName + "\">:", {}, PromptKind::Text};
}

}  // namespace tg::cmd
