// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "document_tabs.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QSignalBlocker>
#include <QSize>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>

namespace tg::app {

namespace {

/// The icons of the row: the 16 px grid of docs/adr/0040, point 3.
constexpr int kTabIconSize = 16;

// The icons of the row; the type of a document gives the icon of its tab.
constexpr const char* kMenuIcon = "ui-tab-menu";
constexpr const char* kNewIcon = "ui-tab-new";
constexpr const char* kDrawingIcon = "doc-drawing";

/// The tab of the Start page, always the first.
constexpr int kStartTab = 0;

/// The side of a tab where the style puts its close button.
QTabBar::ButtonPosition closeSide(const QTabBar& bar) {
    return static_cast<QTabBar::ButtonPosition>(
        bar.style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, &bar));
}

QToolButton* rowButton(QWidget* parent, const QString& name, const QString& tip) {
    auto* button = new QToolButton(parent);
    button->setObjectName(name);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIconSize(QSize(kTabIconSize, kTabIconSize));
    button->setAutoRaise(true);
    button->setToolTip(tip);
    // A click must not take the keyboard away from the drawing and the
    // command line.
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

}  // namespace

QStringList documentTabsIconNames() {
    return {QString::fromLatin1(kMenuIcon), QString::fromLatin1(kNewIcon),
            QString::fromLatin1(kDrawingIcon)};
}

DocumentTabs::DocumentTabs(QWidget* parent)
    : QWidget(parent),
      m_menuButton(rowButton(this, QStringLiteral("documentsMenuButton"), tr("Open documents"))),
      m_menu(new QMenu(this)),
      m_tabs(new QTabBar(this)),
      m_newButton(rowButton(this, QStringLiteral("newTabButton"), tr("New drawing"))) {
    setObjectName(QStringLiteral("documentTabs"));

    m_menuButton->setPopupMode(QToolButton::InstantPopup);
    m_menu->setObjectName(QStringLiteral("documentsMenu"));
    m_menuButton->setMenu(m_menu);
    connect(m_menu, &QMenu::aboutToShow, this, &DocumentTabs::populateMenu);

    m_tabs->setObjectName(QStringLiteral("documentTabBar"));
    m_tabs->setDocumentMode(true);
    m_tabs->setExpanding(false);
    m_tabs->setMovable(false);
    m_tabs->setUsesScrollButtons(true);
    m_tabs->setElideMode(Qt::ElideRight);
    m_tabs->setIconSize(QSize(kTabIconSize, kTabIconSize));
    m_tabs->setFocusPolicy(Qt::NoFocus);
    m_tabs->setTabsClosable(true);
    m_tabs->addTab(tr("Start"));
    m_tabs->setTabToolTip(kStartTab, tr("Start page"));
    dropStartCloseButton();
    connect(m_tabs, &QTabBar::currentChanged, this, &DocumentTabs::choose);
    connect(m_tabs, &QTabBar::tabCloseRequested, this, [this](int tab) {
        if (tab > kStartTab) {
            Q_EMIT closeRequested(tab - 1);
        }
    });

    connect(m_newButton, &QToolButton::clicked, this, &DocumentTabs::newRequested);

    // The `+` follows the last tab; the row is as wide as the drawing.
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_menuButton);
    layout->addWidget(m_tabs);
    layout->addWidget(m_newButton);
    layout->addStretch(1);

    updateIcons();
}

void DocumentTabs::setDocuments(const QList<Document>& documents, int active) {
    const QSignalBlocker blocker(m_tabs);
    const int wanted = static_cast<int>(documents.size()) + 1;
    while (m_tabs->count() > wanted) {
        m_tabs->removeTab(m_tabs->count() - 1);
    }
    while (m_tabs->count() < wanted) {
        m_tabs->addTab(QString{});
    }
    for (int i = 0; i < documents.size(); ++i) {
        const Document& document = documents[i];
        const int tab = i + 1;
        const QString text = tabText(document);
        // Setting the same text again would lay the bar out again.
        if (m_tabs->tabText(tab) != text) {
            m_tabs->setTabText(tab, text);
        }
        if (m_tabs->tabToolTip(tab) != document.path) {
            m_tabs->setTabToolTip(tab, document.path);
        }
        if (i >= m_documents.size() || m_documents[i].icon != document.icon ||
            m_tabs->tabIcon(tab).isNull()) {
            m_tabs->setTabIcon(tab, themedIcon(document.icon, m_theme));
        }
    }
    m_documents = documents;
    const int current = active >= 0 && active < documents.size() ? active + 1 : kStartTab;
    m_tabs->setCurrentIndex(current);
}

int DocumentTabs::activeDocument() const {
    return m_tabs->currentIndex() - 1;
}

QString DocumentTabs::tabText(const Document& document) {
    return document.modified ? document.name + QLatin1Char('*') : document.name;
}

void DocumentTabs::setTheme(Theme theme) {
    m_theme = theme;
    updateIcons();
}

QWidget* DocumentTabs::closeButton(int index) const {
    return m_tabs->tabButton(index + 1, closeSide(*m_tabs));
}

void DocumentTabs::choose(int tab) {
    if (tab == kStartTab) {
        Q_EMIT startRequested();
    } else if (tab > kStartTab) {
        Q_EMIT documentRequested(tab - 1);
    }
}

void DocumentTabs::populateMenu() {
    m_menu->clear();
    for (int tab = 0; tab < m_tabs->count(); ++tab) {
        QAction* item = m_menu->addAction(m_tabs->tabIcon(tab), m_tabs->tabText(tab));
        item->setCheckable(true);
        item->setChecked(tab == m_tabs->currentIndex());
        // Through the bar, as a click on the tab: a tab already shown asks
        // nothing.
        connect(item, &QAction::triggered, this, [this, tab] { m_tabs->setCurrentIndex(tab); });
    }
}

void DocumentTabs::dropStartCloseButton() {
    const QTabBar::ButtonPosition side = closeSide(*m_tabs);
    if (QWidget* button = m_tabs->tabButton(kStartTab, side)) {
        m_tabs->setTabButton(kStartTab, side, nullptr);
        button->deleteLater();
    }
}

void DocumentTabs::updateIcons() {
    m_menuButton->setIcon(themedIcon(QString::fromLatin1(kMenuIcon), m_theme));
    m_newButton->setIcon(themedIcon(QString::fromLatin1(kNewIcon), m_theme));
    for (int i = 0; i < m_documents.size(); ++i) {
        m_tabs->setTabIcon(i + 1, themedIcon(m_documents[i].icon, m_theme));
    }
}

}  // namespace tg::app
