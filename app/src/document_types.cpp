// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "document_types.hpp"

#include "document_session.hpp"
#include "environments.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/io/file_access.hpp"

#include <QCoreApplication>

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
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
/// messages, instead of adding a document (cmd::NewCommand). The lines typed
/// go to `message` as the window wrote them in the history before
/// docs/tappe/0.7.5/P30b (MainWindow::submitFileText()), then the messages
/// of OPEN: the last one is the reason of a failure.
std::unique_ptr<DocumentSession> openDrawing(const cmd::CommandRegistry& commands,
                                             const FileAccessSource& files, const QString& path,
                                             const HistoryMessage& message) {
    auto session = std::make_unique<DocumentSession>(commands, files ? files() : nullptr);
    cmd::CommandRunner& runner = session->runner();
    runner.setMessageSink(
        [&message](const std::string& text) { message(QString::fromStdString(text)); });
    typeIn(runner, QString::fromLatin1(kOpenCommand), message);
    if (runner.prompt().file == cmd::FilePrompt::Open) {
        typeIn(runner, path, message);
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

/// The file access of a save: `files`, and whether the file `name` was
/// written. QSAVE and SAVEAS end the same way when they write and when they
/// fail, and a drawing already saved on its own file has the same name and
/// state after a failed QSAVE: only the write says what happened.
class SaveWatch final : public io::FileAccess {
public:
    SaveWatch(io::FileAccess& files, std::string name) : m_files(files), m_name(std::move(name)) {}

    [[nodiscard]] io::Result<std::string> read(std::string_view name) override {
        return m_files.read(name);
    }

    [[nodiscard]] std::optional<io::Error> write(std::string_view name,
                                                 std::string_view bytes) override {
        auto error = m_files.write(name, bytes);
        if (!error && name == m_name) {
            m_written = true;
        }
        return error;
    }

    [[nodiscard]] bool exists(std::string_view name) const override { return m_files.exists(name); }

    [[nodiscard]] bool written() const { return m_written; }

private:
    io::FileAccess& m_files;
    std::string m_name;
    bool m_written{false};
};

/// What QSAVE and SAVEAS do from the menu, with the name chosen in the
/// dialog and the replacement already confirmed (MainWindow::
/// handleFilePrompts() before docs/tappe/0.7.5/P30b). The runner of
/// `session` reads and writes with `files()`, as the window gives it to every
/// runner: while the command runs it writes through a SaveWatch instead, so
/// that true means that `path` was written.
bool saveDrawing(const FileAccessSource& files, DocumentSession& session, SaveMode mode,
                 const QString& path, const HistoryMessage& message) {
    cmd::CommandRunner& runner = session.runner();
    assert(!runner.isActive() && "a document is saved with no command running");
    io::FileAccess* const access = files ? files() : nullptr;
    std::optional<SaveWatch> watch;
    if (access != nullptr) {
        runner.setFileAccess(&watch.emplace(*access, path.toStdString()));
    }
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
    }
    runner.setFileAccess(access);
    const edit::DocumentFile& file = session.file();
    return watch.has_value() && watch->written() && file.hasFile() &&
           file.name() == path.toStdString() && !session.editor().isModified();
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
    drawing.open = [&commands, files](const QString& path, const HistoryMessage& message) {
        return openDrawing(commands, files, path, message);
    };
    drawing.save = [files = std::move(files)](DocumentSession& session, SaveMode mode,
                                              const QString& path, const HistoryMessage& message) {
        return saveDrawing(files, session, mode, path, message);
    };
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
