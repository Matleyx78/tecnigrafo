// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <deque>
#include <functional>
#include <memory>

namespace tg::cmd {
class CommandRegistry;
}  // namespace tg::cmd

namespace tg::app {

class DocumentSession;
class EnvironmentRegistry;

/// A type of document (docs/adr/0041, point 7).
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
};

/// The types of document of the program, in the order they were added. It is
/// filled once, when the window is put together (docs/adr/0041, point 7):
/// no type is ever removed.
class DocumentTypeRegistry {
public:
    /// Preconditions: the id is not empty and not yet added; the type hosts
    /// at least one environment and can create a document; no other type
    /// has one of its extensions.
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

/// The types and environments of the program: today the drawing in drawing2d
/// (docs/adr/0041, point 7). `commands` are the commands the runners of the
/// new documents read: borrowed, they must outlive the documents.
void registerBuiltinDocumentTypes(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                                  const cmd::CommandRegistry& commands);

}  // namespace tg::app
