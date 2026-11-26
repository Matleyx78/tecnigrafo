// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "app_commands.hpp"

#include "attribute_dialogs.hpp"
#include "battman_dialog.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "layer_panel.hpp"
#include "main_window.hpp"
#include "properties_panel.hpp"
#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/core/insert.hpp"
#include "ui_builder.hpp"

#include <QPlainTextEdit>
#include <QString>
#include <QTimer>

#include <memory>
#include <optional>
#include <span>
#include <string>

namespace tg::app {
namespace {

const core::EntityRecord* attributeDefinitionForPrompt(const core::Document& document,
                                                       const std::string& prompt) {
    constexpr std::string_view kPrefix = "Enter attribute tag <";
    constexpr std::string_view kSuffix = ">:";
    if (!prompt.starts_with(kPrefix) || !prompt.ends_with(kSuffix)) {
        return nullptr;
    }
    const std::string tag =
        prompt.substr(kPrefix.size(), prompt.size() - kPrefix.size() - kSuffix.size());
    for (const core::EntityRecord& record : document.entities()) {
        const auto* definition = std::get_if<core::AttributeDefinition>(&record.entity);
        if (definition != nullptr && definition->tag == tag) {
            return &record;
        }
    }
    return nullptr;
}

const core::Insert* insertForPrompt(const core::Document& document, const std::string& prompt) {
    for (const core::EntityRecord& record : document.entities()) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr || insert->attributes.empty()) {
            continue;
        }
        const core::Attribute& first = insert->attributes.front();
        if (prompt == first.tag + " <" + first.text.value + ">:") {
            return insert;
        }
    }
    return nullptr;
}

void answerAttributeRequest(MainWindow& window) {
    DocumentSession* session = window.activeSession();
    const auto* history =
        window.commandLine()->findChild<QPlainTextEdit*>(QStringLiteral("history"));
    // ScriptSession removes the history limit before sending any input. Its
    // TEXTEDIT requests must remain on the command line (PT21, point 3).
    const bool script = history != nullptr && history->maximumBlockCount() == 0;
    if (session == nullptr || script || window.property("answeringAttributeRequest").toBool()) {
        return;
    }
    cmd::CommandRunner& runner = session->runner();
    const std::string prompt = runner.promptText();
    const core::Document& document = session->document();
    const core::EntityRecord* definitionRecord = attributeDefinitionForPrompt(document, prompt);
    const core::Insert* insert =
        definitionRecord == nullptr ? insertForPrompt(document, prompt) : nullptr;
    if (definitionRecord == nullptr && insert == nullptr) {
        return;
    }

    window.setProperty("answeringAttributeRequest", true);
    if (definitionRecord != nullptr) {
        AttributeDefinitionDialog dialog(
            std::get<core::AttributeDefinition>(definitionRecord->entity), &window);
        if (dialog.exec() == QDialog::Accepted) {
            const core::AttributeDefinition edited = dialog.definition();
            runner.submit(cmd::TextInput{edited.tag});
            runner.submit(cmd::TextInput{edited.prompt});
            runner.submit(cmd::TextInput{edited.text.value});
        } else {
            runner.submit(cmd::CancelInput{});
        }
    } else {
        InsertAttributesDialog dialog(*insert, &window);
        if (dialog.exec() == QDialog::Accepted) {
            for (const std::string& value : dialog.values()) {
                runner.submit(cmd::TextInput{value});
            }
        } else {
            runner.submit(cmd::CancelInput{});
        }
    }
    window.setProperty("answeringAttributeRequest", false);
    window.inputHandled();
}

class InterfaceCommand final : public cmd::Command {
public:
    InterfaceCommand(MainWindow& window, InterfaceKind kind) : m_window(window), m_kind(kind) {}

    void start(cmd::CommandContext& /*context*/) override { m_window.setInterface(m_kind); }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
    InterfaceKind m_kind;
};

/// LAYER shows the layers panel and gives it the keyboard: no message and no
/// question (docs/tappe/0.6/P22).
class LayerPanelCommand final : public cmd::Command {
public:
    explicit LayerPanelCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        if (auto* panel = m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"))) {
            panel->showAndFocus();
        }
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

/// PROPERTIES shows the properties panel and gives it the keyboard: no
/// message and no question (docs/tappe/0.6/P24).
class PropertiesPanelCommand final : public cmd::Command {
public:
    explicit PropertiesPanelCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        if (auto* panel = m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
            panel->showAndFocus();
        }
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

/// QSELECT opens the window of the quick selection, with no question in the
/// command line. Starting it cleared the selection (docs/adr/0008): the
/// window gets the one the panel saw before, to search within it.
class QuickSelectCommand final : public cmd::Command {
public:
    explicit QuickSelectCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        if (auto* panel = m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
            panel->showQuickSelect(panel->idleSelection());
        }
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

class BattmanCommand final : public cmd::Command {
public:
    explicit BattmanCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        DocumentSession* session = m_window.activeSession();
        if (session == nullptr || session->editor().inTransaction()) {
            return;
        }
        std::string selectedBlock;
        if (auto* panel = m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
            for (const core::EntityId id : panel->idleSelection()) {
                const core::EntityRecord* record = session->document().find(id);
                if (record != nullptr) {
                    if (const auto* insert = std::get_if<core::Insert>(&record->entity)) {
                        selectedBlock = insert->block;
                        break;
                    }
                }
            }
        }
        auto* dialog = new BattmanDialog(session->editor(), selectedBlock, &m_window);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

}  // namespace

void registerAppCommands(cmd::CommandRegistry& registry, MainWindow& window) {
    const auto request = [&window] {
        QTimer::singleShot(0, &window, [&window] { answerAttributeRequest(window); });
    };
    QObject::connect(window.commandLine(), &CommandLine::inputHandled, &window, request);
    QObject::connect(window.view(), &CadView::inputHandled, &window, request);
    registry.add({"RIBBON", {}, "Shows the ribbon interface.", false, {}}, [&window] {
        return std::make_unique<InterfaceCommand>(window, InterfaceKind::Ribbon);
    });
    const cmd::CommandDescription close{
        "RIBBONCLOSE", {}, "Shows the classic menus and toolbars.", false, {}};
    registry.add(close, [&window] {
        return std::make_unique<InterfaceCommand>(window, InterfaceKind::Classic);
    });
    registry.add({"LAYER", {"LA"}, "Shows the layers panel.", false, {}},
                 [&window] { return std::make_unique<LayerPanelCommand>(window); });
    const cmd::CommandDescription properties{
        "PROPERTIES", {"PR", "CH", "MO", "PROPS"}, "Shows the properties panel.", false, {}};
    registry.add(properties,
                 [&window] { return std::make_unique<PropertiesPanelCommand>(window); });
    const cmd::CommandDescription quickSelect{
        "QSELECT", {}, "Selects objects by type and property in a window.", false, {}};
    registry.add(quickSelect, [&window] { return std::make_unique<QuickSelectCommand>(window); });
    registry.add({"BATTMAN", {}, "Manages block attribute definitions in a window.", false, {}},
                 [&window] { return std::make_unique<BattmanCommand>(window); });
}

}  // namespace tg::app
