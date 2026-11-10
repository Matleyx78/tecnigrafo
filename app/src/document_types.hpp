// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>

namespace tg::cmd {
class CommandRegistry;
}  // namespace tg::cmd

namespace tg::io {
class FileAccess;
}  // namespace tg::io

namespace tg::app {

class DocumentSession;
class EnvironmentRegistry;

/// How a document is saved: on its own file, or as a file the user chose.
enum class SaveMode : std::uint8_t { Save, SaveAs };

/// Writes a line to the command line history.
using HistoryMessage = std::function<void(const QString&)>;

/// A type of document (docs/adr/0041, point 7). The window shows the file
/// dialogs and asks the questions; the type opens and saves the path chosen
/// (docs/tappe/0.7.5/P30b).
struct DocumentType {
    QString id;                ///< "drawing"
    QString name;              ///< "Drawing"
    QString icon;              ///< icon name, docs/adr/0040
    QStringList extensions;    ///< "cadx", "dxf": lower case, without the dot
    QStringList environments;  ///< the ones it can host, the first one by default
    /// A new document: a session whose DocumentSession::documentType() is
    /// `id`, with an empty history and no file. The window gives it its
    /// environment, its file access and the rest of its settings.
    std::function<std::unique_ptr<DocumentSession>()> create;

    /// Opens `path`, whose extension is one of `extensions`, as a new document
    /// of this type, in its default environment; nullptr if it cannot, after
    /// writing the reason to `message`.
    std::function<std::unique_ptr<DocumentSession>(const QString& path,
                                                   const HistoryMessage& message)>
        open;

    /// Saves `session`, a document of this type: with SaveMode::Save on its
    /// own file (`path` is its name), with SaveMode::SaveAs as `path`. On
    /// success the document has the file `path` and is not modified; false if
    /// it could not save, after writing the reason to `message`. The window
    /// has already confirmed replacing a file that exists, and `session` is
    /// the active one, with no command running.
    std::function<bool(DocumentSession& session, SaveMode mode, const QString& path,
                       const HistoryMessage& message)>
        save;
};

/// The types of document of the program, in the order they were added. It is
/// filled once, when the window is put together (docs/adr/0041, point 7):
/// no type is ever removed.
class DocumentTypeRegistry {
public:
    /// Preconditions: the id is not empty and not yet added; the type hosts
    /// at least one environment, has an extension, and can create, open and
    /// save a document; no other type has one of its extensions.
    void add(DocumentType type);

    /// nullptr if unknown; valid as long as the registry.
    [[nodiscard]] const DocumentType* find(const QString& id) const;

    /// The type with the extension `extension` ("cadx", in any case, without
    /// the dot), or nullptr if none has it.
    [[nodiscard]] const DocumentType* findByExtension(const QString& extension) const;

    /// Every type, in the order they were added.
    [[nodiscard]] QList<const DocumentType*> list() const;

private:
    // A deque keeps the types where they are when others are added, so that
    // the pointers returned by find() stay valid.
    std::deque<DocumentType> m_types;
};

/// The file access a type reads and writes with, asked each time it is used
/// (the window's changes: MainWindow::setFileAccess(), the script mode); it
/// may give nullptr, as an empty function does.
using FileAccessSource = std::function<io::FileAccess*()>;

/// The types and environments of the program: today the drawing in drawing2d
/// (docs/adr/0041, point 7). `commands` are the commands the runners of the
/// new documents read: borrowed, they must outlive the documents. The
/// drawing opens and saves its files with `files`, the file access the
/// window gives to every runner.
///
/// The drawing opens and saves as OPEN, QSAVE and SAVEAS do from the menu
/// (docs/tappe/0.7.5/P30b, point 2). `open` runs OPEN in the runner of the
/// new document, not shown yet: the lines typed (`Command: OPEN`, the name)
/// and its messages (errors of reading, entities not handled) go to
/// `message`, the reason of a failure last. `save` runs QSAVE
/// (SaveMode::Save) or SAVEAS (SaveMode::SaveAs) in the runner of the
/// document, answering the name and the question to replace: the lines typed
/// go to `message`, as the window writes them in the history, and the
/// messages of QSAVE and SAVEAS go where the runner of the active session
/// writes them, the same history. It succeeds only if the command wrote
/// `path` with `files`.
void registerBuiltinDocumentTypes(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                                  const cmd::CommandRegistry& commands,
                                  FileAccessSource files = {});

}  // namespace tg::app
