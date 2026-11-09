// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_protocol.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/drawing_session.hpp"
#include "tg/cmd/dynamic_input.hpp"
#include "tg/core/document.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/document_file.hpp"
#include "tg/edit/editor.hpp"
#include "tg/io/file_access.hpp"

#include <QString>
#include <QStringList>

#include <optional>
#include <utility>

namespace tg::app {

/// One open document in the window (docs/adr/0041, point 2): the drawing
/// session, with document, history, file and runner, and what the interface
/// keeps of it while another session is active: the view, the text typed and
/// not yet sent, the responses of the action that started its command.
///
/// MainWindow owns its sessions and never moves them: widgets and callbacks
/// may hold a session by pointer while it is active, and let it go before it
/// is destroyed (docs/tappe/0.7.5/P28, "Durata dei riferimenti").
class DocumentSession {
public:
    /// The identifier of the drawing in the registry of the types
    /// (DocumentTypeRegistry, docs/adr/0041, point 7).
    static constexpr const char* kDrawingType = "drawing";

    /// Where the view of the session looked when it was left: the centre and
    /// the scale of render::Viewport, in pixels per drawing unit.
    struct ViewState {
        core::Vec2 center;
        double scale{1.0};
    };

    /// What the user typed and has not yet handed to the runner (decision B2
    /// of docs/DECISIONI.md): the text of the command line and the fields of
    /// the dynamic input, with the active field. It belongs to the session:
    /// kept when the session is left, given back when it is active again,
    /// never handed to another session.
    struct PendingInput {
        QString commandLine;
        /// The fields of the dynamic input as they were left; std::nullopt
        /// when nothing was saved, and the fields start from the runner.
        std::optional<cmd::DynamicInput> dynamicInput;
    };

    /// The responses of the action that started the running command, still
    /// to give, and the prompt where the first, kWaitForUser, waits for the
    /// user's answer (ActionSpec::responses, docs/tappe/0.7.5/P19). They
    /// belong to the command of this session's runner.
    struct ActionResponses {
        QStringList responses;
        std::optional<cmd::Prompt> waitingAt;
    };

    /// `registry` and `files` are borrowed: both must outlive the session
    /// (cmd::DrawingSession). `files` may be nullptr. `documentType` is the
    /// id of the session's type in DocumentTypeRegistry.
    DocumentSession(const cmd::CommandRegistry& registry, io::FileAccess* files,
                    core::Document document = core::Document::newDrawing(),
                    QString documentType = QString::fromLatin1(kDrawingType));

    DocumentSession(const DocumentSession&) = delete;
    DocumentSession& operator=(const DocumentSession&) = delete;
    DocumentSession(DocumentSession&&) = delete;
    DocumentSession& operator=(DocumentSession&&) = delete;
    ~DocumentSession() = default;

    /// The id of the type of the document in DocumentTypeRegistry.
    [[nodiscard]] const QString& documentType() const { return m_documentType; }

    /// The environment the document is edited in (docs/adr/0041, point 1;
    /// decision B8 of docs/DECISIONI.md): it belongs to the session, so it
    /// comes back with it. MainWindow gives a new session the default
    /// environment of its type, and gives the runner the environment of the
    /// active session (cmd::CommandRunner::setEnvironment()).
    [[nodiscard]] const QString& environment() const { return m_environment; }

    void setEnvironment(QString environment) { m_environment = std::move(environment); }

    [[nodiscard]] cmd::DrawingSession& drawing() { return m_drawing; }

    [[nodiscard]] const cmd::DrawingSession& drawing() const { return m_drawing; }

    [[nodiscard]] const core::Document& document() const { return m_drawing.document(); }

    [[nodiscard]] edit::Editor& editor() { return m_drawing.editor(); }

    [[nodiscard]] const edit::Editor& editor() const { return m_drawing.editor(); }

    [[nodiscard]] edit::DocumentFile& file() { return m_drawing.file(); }

    [[nodiscard]] const edit::DocumentFile& file() const { return m_drawing.file(); }

    [[nodiscard]] cmd::CommandRunner& runner() { return m_drawing.runner(); }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_drawing.runner(); }

    /// The view saved when the session was last left; std::nullopt before
    /// the first time, when the view fits the drawing.
    [[nodiscard]] const std::optional<ViewState>& viewState() const { return m_viewState; }

    void setViewState(const ViewState& state) { m_viewState = state; }

    [[nodiscard]] PendingInput& pendingInput() { return m_pendingInput; }

    [[nodiscard]] const PendingInput& pendingInput() const { return m_pendingInput; }

    [[nodiscard]] ActionResponses& actionResponses() { return m_actionResponses; }

private:
    QString m_documentType;
    QString m_environment;
    cmd::DrawingSession m_drawing;
    std::optional<ViewState> m_viewState;
    PendingInput m_pendingInput;
    ActionResponses m_actionResponses;
};

}  // namespace tg::app
