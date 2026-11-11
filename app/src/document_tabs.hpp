// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "theme.hpp"

#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

class QMenu;
class QPaintEvent;
class QTabBar;
class QToolButton;

namespace tg::app {

/// The icons of the row of the tabs and of the Start page (docs/ICONE.md):
/// no action names them.
[[nodiscard]] QStringList documentTabsIconNames();

/// The row of the document tabs under the ribbon (docs/adr/0039, point 1,
/// area 4; docs/tappe/0.7.5/P29): the button of the menu of the open
/// documents, the Start tab, one tab for each open document and the `+`
/// button. It is there with both interfaces.
///
/// The row only shows and asks: MainWindow owns the sessions, gives the row
/// the documents with setDocuments() after every change, and does what the
/// signals ask. Tab 0 of tabBar() is the Start page; tab i + 1 is the
/// document i of setDocuments().
class DocumentTabs final : public QWidget {
    Q_OBJECT

public:
    /// What a tab shows of an open document.
    struct Document {
        QString name;  ///< the name of its file, without the folder
        QString path;  ///< the whole name of its file, for the tooltip
        QString icon;  ///< the icon of its type ("doc-drawing", docs/ICONE.md)
        bool modified{false};
    };

    explicit DocumentTabs(QWidget* parent = nullptr);

    /// Shows `documents`, in this order, with `active` the document shown,
    /// or -1 for the Start page. Emits nothing.
    void setDocuments(const QList<Document>& documents, int active);

    /// The document shown, or -1 for the Start page.
    [[nodiscard]] int activeDocument() const;

    /// The text of the tab of a document: its name, with `*` when modified.
    [[nodiscard]] static QString tabText(const Document& document);

    void setTheme(Theme theme);

    [[nodiscard]] QTabBar* tabBar() const { return m_tabs; }

    [[nodiscard]] QToolButton* newButton() const { return m_newButton; }

    [[nodiscard]] QToolButton* menuButton() const { return m_menuButton; }

    /// The close button of the tab of the document `index`.
    [[nodiscard]] QWidget* closeButton(int index) const;

Q_SIGNALS:
    /// The user chose the Start tab.
    void startRequested();
    /// The user chose the tab of the document `index`.
    void documentRequested(int index);
    /// The user clicked the cross of the tab of the document `index`.
    void closeRequested(int index);
    /// The user clicked `+`.
    void newRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void choose(int tab);
    void populateMenu();
    /// Takes away the close button of the Start tab.
    void dropStartCloseButton();
    void updateIcons();

    QToolButton* m_menuButton{nullptr};
    QMenu* m_menu{nullptr};
    QTabBar* m_tabs{nullptr};
    QToolButton* m_newButton{nullptr};
    QList<Document> m_documents;
    Theme m_theme{Theme::Light};
};

}  // namespace tg::app
