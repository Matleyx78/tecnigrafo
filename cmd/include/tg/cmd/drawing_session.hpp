// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/document_file.hpp"
#include "tg/edit/editor.hpp"
#include "tg/io/file_access.hpp"

namespace tg::cmd {

/// One open drawing (docs/adr/0041, point 2): the document, its history, its
/// file and its runner, built in this order and destroyed in reverse.
///
/// The session never moves: the editor holds the document, the runner holds
/// the editor and the file, and callers hold the session by reference.
class DrawingSession {
public:
    /// `registry` and `files` are borrowed, not owned: both must outlive the
    /// session, since its runner reads the registry at every command and
    /// hands `files` to the file commands. `files` may be nullptr: then the
    /// file commands are disabled, as with CommandRunner::setFileAccess().
    /// The runner of the session works on the session's own DocumentFile.
    DrawingSession(const CommandRegistry& registry, io::FileAccess* files,
                   core::Document document = core::Document::newDrawing());

    DrawingSession(const DrawingSession&) = delete;
    DrawingSession& operator=(const DrawingSession&) = delete;
    DrawingSession(DrawingSession&&) = delete;
    DrawingSession& operator=(DrawingSession&&) = delete;
    ~DrawingSession() = default;

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    [[nodiscard]] edit::DocumentFile& file() { return m_file; }

    [[nodiscard]] const edit::DocumentFile& file() const { return m_file; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const CommandRunner& runner() const { return m_runner; }

private:
    // Declared in dependency order: each member is built from the ones above.
    core::Document m_document;
    edit::Editor m_editor;
    edit::DocumentFile m_file;
    CommandRunner m_runner;
};

}  // namespace tg::cmd
