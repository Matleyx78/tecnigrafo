// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "theme.hpp"

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QHBoxLayout;
class QListWidget;
class QToolButton;

namespace tg::app {

/// The Start page (docs/adr/0041, point 9; docs/tappe/0.7.5/P29): a button
/// New for each type of document, Open… and the recent files. It is not a
/// document and has no session; MainWindow shows it in place of the drawing
/// and the command line while no document is active, and does what the
/// signals ask.
class StartPage final : public QWidget {
    Q_OBJECT

public:
    explicit StartPage(QWidget* parent = nullptr);

    /// Adds the button that creates a new document of `type`, named
    /// "startNew.<type>", with the text `name` and the icon `icon`
    /// (docs/ICONE.md). Today the only type is the drawing.
    void addDocumentType(const QString& type, const QString& name, const QString& icon);

    /// Shows `paths`, the most recent first.
    void setRecentFiles(const QStringList& paths);

    [[nodiscard]] QStringList recentFiles() const;

    void setTheme(Theme theme);

Q_SIGNALS:
    /// The user asked for a new document of `type`.
    void newRequested(const QString& type);
    /// The user asked to open a file.
    void openRequested();
    /// The user chose the recent file `path`.
    void recentRequested(const QString& path);

private:
    void updateIcons();

    struct TypeButton {
        QToolButton* button{nullptr};
        QString icon;
    };

    QHBoxLayout* m_newRow{nullptr};
    QList<TypeButton> m_typeButtons;
    QToolButton* m_openButton{nullptr};
    QListWidget* m_recent{nullptr};
    Theme m_theme{Theme::Light};
};

}  // namespace tg::app
