// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "document_session.hpp"

#include <utility>

namespace tg::app {

DocumentSession::DocumentSession(const cmd::CommandRegistry& registry, io::FileAccess* files,
                                 core::Document document, QString documentType)
    : m_documentType(std::move(documentType)), m_drawing(registry, files, std::move(document)) {}

}  // namespace tg::app
