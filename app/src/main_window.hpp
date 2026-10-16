// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QMainWindow>

class QLabel;

namespace tg::app {

class CadView;
class CommandLine;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    [[nodiscard]] CadView* view() const { return m_view; }

private:
    // Connected with the function-pointer syntax of QObject::connect, so no
    // "slots" section is needed.
    void showCursorPosition(double worldX, double worldY);
    void showScale(double pixelsPerUnit);
    void showAbout();

    void createMenus();
    void createStatusBar();

    // Declared in dependency order: each member is built from the ones above.
    core::Document m_document;
    edit::Editor m_editor;
    cmd::CommandRegistry m_registry;
    cmd::CommandRunner m_runner;
    CadView* m_view{nullptr};
    CommandLine* m_commandLine{nullptr};
    QLabel* m_cursorLabel{nullptr};
    QLabel* m_scaleLabel{nullptr};
};

}  // namespace tg::app
