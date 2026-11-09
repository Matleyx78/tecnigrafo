// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/edit/document_file.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {

class DrawingSession;

/// Who keeps the open documents, for NEW and OPEN (docs/adr/0041, point 2).
///
/// NEW and OPEN call it from inside the runner of the active document: the
/// host never destroys that runner, nor any session it holds, while a
/// command of it runs. A session left by activate() keeps its runner as it
/// is, with its command halfway, if any.
class DocumentHost {
public:
    DocumentHost() = default;
    DocumentHost(const DocumentHost&) = delete;
    DocumentHost& operator=(const DocumentHost&) = delete;
    DocumentHost(DocumentHost&&) = delete;
    DocumentHost& operator=(DocumentHost&&) = delete;
    virtual ~DocumentHost() = default;

    /// Adds `document`, with its file, after the others and makes it active.
    virtual DrawingSession& addDocument(core::Document document, edit::DocumentFile file) = 0;

    /// The open document whose file is `name` (the same text as
    /// DocumentFile::name()), or nullptr.
    [[nodiscard]] virtual DrawingSession* findOpen(std::string_view name) = 0;

    /// Makes `session`, one of the open documents, active.
    virtual void activate(DrawingSession& session) = 0;

    /// The names of the documents not saved yet ("Drawing<N>.cadx"), for the
    /// next untitled name.
    [[nodiscard]] virtual std::vector<std::string> openNames() const = 0;
};

}  // namespace tg::cmd
