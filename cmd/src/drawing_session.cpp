// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/drawing_session.hpp"

#include <utility>

namespace tg::cmd {

DrawingSession::DrawingSession(const CommandRegistry& registry, io::FileAccess* files,
                               core::Document document)
    : m_document(std::move(document)), m_editor(m_document), m_runner(registry, m_editor) {
    m_runner.setFileAccess(files);
    m_runner.setDocumentFile(&m_file);
}

}  // namespace tg::cmd
