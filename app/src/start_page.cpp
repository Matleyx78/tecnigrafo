// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "start_page.hpp"

#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>

namespace tg::app {

namespace {

/// The buttons of the page are large: the 32 px grid of docs/adr/0040,
/// point 3 (docs/ICONE.md, "Due griglie": the start page).
constexpr int kButtonIconSize = 32;

/// The space around and between the parts of the page.
constexpr int kMargin = 24;
constexpr int kSpacing = 12;

/// The width of the column of the page, so that it does not stretch over a
/// wide window.
constexpr int kColumnWidth = 560;

/// How much larger than the text of the window the title is.
constexpr double kTitleScale = 1.6;

/// The role of the items of the recent files that holds the whole path.
constexpr int kPathRole = Qt::UserRole;

QLabel* heading(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    return label;
}

QToolButton* pageButton(const QString& name, const QString& text, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setObjectName(name);
    button->setText(text);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIconSize(QSize(kButtonIconSize, kButtonIconSize));
    button->setAutoRaise(true);
    return button;
}

}  // namespace

StartPage::StartPage(QWidget* parent) : QWidget(parent), m_newRow(new QHBoxLayout) {
    setObjectName(QStringLiteral("startPage"));
    setAutoFillBackground(true);

    auto* column = new QWidget;
    column->setMaximumWidth(kColumnWidth);
    auto* content = new QVBoxLayout(column);
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(kSpacing);

    auto* title = new QLabel(tr("Start"), column);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * kTitleScale);
    title->setFont(titleFont);
    content->addWidget(title);

    // New, with a button for each type, then Open… on the same row.
    content->addWidget(heading(tr("New"), column));
    m_newRow->setSpacing(kSpacing);
    m_openButton = pageButton(QStringLiteral("startOpen"), tr("Open…"), column);
    m_openButton->setToolTip(tr("Opens a drawing."));
    connect(m_openButton, &QToolButton::clicked, this, &StartPage::openRequested);
    m_newRow->addWidget(m_openButton);
    m_newRow->addStretch(1);
    content->addLayout(m_newRow);

    content->addWidget(heading(tr("Recent Files"), column));
    m_recent = new QListWidget(column);
    m_recent->setObjectName(QStringLiteral("startRecentFiles"));
    m_recent->setFrameShape(QFrame::NoFrame);
    m_recent->setSelectionMode(QAbstractItemView::SingleSelection);
    // One click opens, as the links of a start page do.
    connect(m_recent, &QListWidget::itemClicked, this, [this](const QListWidgetItem* item) {
        Q_EMIT recentRequested(item->data(kPathRole).toString());
    });
    content->addWidget(m_recent, 1);

    // In a scroll area: the page asks the window for no more room than the
    // drawing and the command line it replaces.
    auto* sheet = new QWidget;
    auto* row = new QHBoxLayout(sheet);
    row->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    row->addWidget(column, 1);
    row->addStretch(1);
    auto* scroll = new QScrollArea(this);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(sheet);
    auto* page = new QVBoxLayout(this);
    page->setContentsMargins(0, 0, 0, 0);
    page->addWidget(scroll);

    updateIcons();
}

void StartPage::addDocumentType(const QString& type, const QString& name, const QString& icon) {
    QToolButton* button =
        pageButton(QStringLiteral("startNew.%1").arg(type), name, m_openButton->parentWidget());
    button->setToolTip(tr("Creates a new %1.").arg(name.toLower()));
    connect(button, &QToolButton::clicked, this, [this, type] { Q_EMIT newRequested(type); });
    // The types in the order they are added, before Open….
    m_newRow->insertWidget(static_cast<int>(m_typeButtons.size()), button);
    m_typeButtons.append({button, icon});
    updateIcons();
}

void StartPage::setRecentFiles(const QStringList& paths) {
    if (paths == recentFiles()) {
        return;
    }
    m_recent->clear();
    for (const QString& path : paths) {
        auto* item = new QListWidgetItem(QFileInfo(path).fileName(), m_recent);
        item->setData(kPathRole, path);
        item->setToolTip(path);
    }
    updateIcons();
}

QStringList StartPage::recentFiles() const {
    QStringList paths;
    for (int row = 0; row < m_recent->count(); ++row) {
        paths.append(m_recent->item(row)->data(kPathRole).toString());
    }
    return paths;
}

void StartPage::setTheme(Theme theme) {
    m_theme = theme;
    updateIcons();
}

void StartPage::updateIcons() {
    for (const TypeButton& type : m_typeButtons) {
        type.button->setIcon(themedIcon(type.icon, m_theme));
    }
    m_openButton->setIcon(themedIcon(QStringLiteral("file-open"), m_theme));
    const QIcon file = themedIcon(QStringLiteral("doc-drawing"), m_theme);
    for (int row = 0; row < m_recent->count(); ++row) {
        m_recent->item(row)->setIcon(file);
    }
}

}  // namespace tg::app
