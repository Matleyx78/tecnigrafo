// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "document_types.hpp"

#include "document_session.hpp"
#include "environments.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"

#include <QCoreApplication>

#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace tg::app {

namespace {

/// The commands of the drawing that save a file.
constexpr const char* kSaveCommand = "QSAVE";
constexpr const char* kSaveAsCommand = "SAVEAS";
constexpr const char* kOpenCommand = "OPEN";

/// `text` given to `runner` as the window gives a line typed in the command
/// line (MainWindow::submitFileText()): the prompt and the text, then the
/// text.
void typeIn(cmd::CommandRunner& runner, const QString& text, const HistoryMessage& message) {
    message(QStringLiteral("%1 %2").arg(QString::fromStdString(runner.promptText()), text));
    runner.submitText(text.toStdString());
}

/// Whether the command of `runner` asks to replace a file: the question of
/// SaveCommand, the only one with these two options.
bool asksToReplace(const cmd::CommandRunner& runner) {
    return runner.isActive() &&
           runner.prompt().keywords == std::vector<cmd::Keyword>{{"Yes", "Y"}, {"No", "N"}};
}

/// What OPEN does from the menu, in the runner of a new drawing without the
/// host of the window: OPEN then reads the file into that drawing, with its
/// messages, instead of adding a document (cmd::NewCommand).
std::unique_ptr<DocumentSession> openDrawing(const cmd::CommandRegistry& commands,
                                             const FileAccessSource& files, const QString& path,
                                             const HistoryMessage& message) {
    auto session = std::make_unique<DocumentSession>(commands, files ? files() : nullptr);
    cmd::CommandRunner& runner = session->runner();
    runner.setMessageSink(
        [&message](const std::string& text) { message(QString::fromStdString(text)); });
    // Only the messages of OPEN: the document is not shown yet, and with the
    // Start page they are what the page says (MainWindow::showStartMessage()).
    runner.submitText(kOpenCommand);
    if (runner.prompt().file == cmd::FilePrompt::Open) {
        runner.submitText(path.toStdString());
    }
    if (runner.isActive()) {
        runner.submit(cmd::CancelInput{});
    }
    // The window gives the runner its own sink, with the session.
    runner.setMessageSink({});
    const edit::DocumentFile& file = session->file();
    if (!file.hasFile() || file.name() != path.toStdString()) {
        return nullptr;
    }
    return session;
}

/// What QSAVE and SAVEAS do from the menu, with the name chosen in the
/// dialog and the replacement already confirmed (MainWindow::
/// handleFilePrompts() before docs/tappe/0.7.5/P30b).
bool saveDrawing(DocumentSession& session, SaveMode mode, const QString& path,
                 const HistoryMessage& message) {
    cmd::CommandRunner& runner = session.runner();
    assert(!runner.isActive() && "a document is saved with no command running");
    typeIn(runner, QString::fromLatin1(mode == SaveMode::Save ? kSaveCommand : kSaveAsCommand),
           message);
    if (runner.prompt().file == cmd::FilePrompt::Save) {
        typeIn(runner, path, message);
    }
    if (asksToReplace(runner)) {
        typeIn(runner, QStringLiteral("Yes"), message);
    }
    if (runner.isActive()) {
        // Nothing else is asked; never leave the document halfway.
        runner.submit(cmd::CancelInput{});
        return false;
    }
    const edit::DocumentFile& file = session.file();
    return file.hasFile() && file.name() == path.toStdString() && !session.editor().isModified();
}

/// Gives the names of the types and environments their own translation
/// context.
class BuiltinDocumentTypes {
    Q_DECLARE_TR_FUNCTIONS(BuiltinDocumentTypes)

public:
    static void add(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                    const cmd::CommandRegistry& commands, FileAccessSource files);
};

void BuiltinDocumentTypes::add(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                               const cmd::CommandRegistry& commands, FileAccessSource files) {
    const QString drawing2d = QStringLiteral("drawing2d");
    environments.add({drawing2d, tr("Drawing"), QStringLiteral("env-drawing2d"), {}});

    DocumentType drawing;
    drawing.id = QString::fromLatin1(DocumentSession::kDrawingType);
    drawing.name = tr("Drawing");
    drawing.icon = QStringLiteral("doc-drawing");
    drawing.extensions = {QStringLiteral("cadx"), QStringLiteral("dxf")};
    drawing.environments = {drawing2d};
    // No file access here: the window gives its own to every runner.
    drawing.create = [&commands] {
        return std::make_unique<DocumentSession>(commands, nullptr);
    };
    drawing.open = [&commands, source = std::move(files)](const QString& path,
                                                          const HistoryMessage& message) {
        return openDrawing(commands, source, path, message);
    };
    drawing.save = saveDrawing;
    types.add(std::move(drawing));
}

}  // namespace

void DocumentTypeRegistry::add(DocumentType type) {
    assert(!type.id.isEmpty() && "document type without id");
    assert(find(type.id) == nullptr && "document type id already registered");
    assert(!type.environments.isEmpty() && "a document type hosts at least one environment");
    assert(!type.extensions.isEmpty() && "a document type has its files");
    assert(type.create && type.open && type.save &&
           "a document type creates, opens and saves its documents");
#ifndef NDEBUG
    for (const QString& extension : type.extensions) {
        assert(findByExtension(extension) == nullptr && "extension of another document type");
    }
#endif
    m_types.push_back(std::move(type));
}

const DocumentType* DocumentTypeRegistry::find(const QString& id) const {
    for (const DocumentType& type : m_types) {
        if (type.id == id) {
            return &type;
        }
    }
    return nullptr;
}

const DocumentType* DocumentTypeRegistry::findByExtension(const QString& extension) const {
    for (const DocumentType& type : m_types) {
        if (type.extensions.contains(extension, Qt::CaseInsensitive)) {
            return &type;
        }
    }
    return nullptr;
}

QList<const DocumentType*> DocumentTypeRegistry::list() const {
    QList<const DocumentType*> result;
    result.reserve(static_cast<qsizetype>(m_types.size()));
    for (const DocumentType& type : m_types) {
        result.append(&type);
    }
    return result;
}

void registerBuiltinDocumentTypes(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                                  const cmd::CommandRegistry& commands, FileAccessSource files) {
    BuiltinDocumentTypes::add(types, environments, commands, std::move(files));
}

}  // namespace tg::app
