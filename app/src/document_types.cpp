// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "document_types.hpp"

#include "document_session.hpp"
#include "environments.hpp"
#include "tg/cmd/command_registry.hpp"

#include <QCoreApplication>

#include <cassert>
#include <utility>

namespace tg::app {

namespace {

/// Gives the names of the types and environments their own translation
/// context.
class BuiltinDocumentTypes {
    Q_DECLARE_TR_FUNCTIONS(BuiltinDocumentTypes)

public:
    static void add(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                    const cmd::CommandRegistry& commands);
};

void BuiltinDocumentTypes::add(DocumentTypeRegistry& types, EnvironmentRegistry& environments,
                               const cmd::CommandRegistry& commands) {
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
    types.add(std::move(drawing));
}

}  // namespace

void DocumentTypeRegistry::add(DocumentType type) {
    assert(!type.id.isEmpty() && "document type without id");
    assert(find(type.id) == nullptr && "document type id already registered");
    assert(!type.environments.isEmpty() && "a document type hosts at least one environment");
    assert(type.create && "a document type creates its documents");
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
                                  const cmd::CommandRegistry& commands) {
    BuiltinDocumentTypes::add(types, environments, commands);
}

}  // namespace tg::app
