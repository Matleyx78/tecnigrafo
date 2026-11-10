// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "auto_hide.hpp"

#include "theme.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>

namespace tg::app {
namespace {

/// The buttons of the title bar: the 16 px icons of docs/ICONE.md.
constexpr int kTitleIconSize = 16;
/// Space around the name in the title bar, as the QDockWidget::title rule
/// of the style sheet gives the headers of Qt (style.cpp).
constexpr int kTitleMarginLeft = 6;
constexpr int kTitleMargin = 2;
/// Inside a tab: space at its ends and around its icon, and the icon size.
constexpr int kTabPadding = 6;
constexpr int kTabSpacing = 4;
constexpr int kTabIconSize = 16;
/// Space between two tabs of a bar.
constexpr int kTabGap = 2;

/// The four edges, in the order of AutoHide::m_bars.
constexpr std::array kEdges{Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea, Qt::TopDockWidgetArea,
                            Qt::BottomDockWidgetArea};

std::size_t edgeIndex(Qt::DockWidgetArea edge) {
    std::size_t index = 0;
    while (index + 1 < kEdges.size() && kEdges.at(index) != edge) {
        ++index;
    }
    assert(kEdges.at(index) == edge && "not an edge of the window");
    return index;
}

/// The name of an edge in the saved settings and in object names.
QString edgeName(Qt::DockWidgetArea edge) {
    switch (edge) {
        case Qt::LeftDockWidgetArea:
            return QStringLiteral("left");
        case Qt::RightDockWidgetArea:
            return QStringLiteral("right");
        case Qt::TopDockWidgetArea:
            return QStringLiteral("top");
        case Qt::BottomDockWidgetArea:
            return QStringLiteral("bottom");
        default:
            return {};
    }
}

/// The edge of `name`, or NoDockWidgetArea if it names none.
Qt::DockWidgetArea edgeFromName(const QString& name) {
    for (const Qt::DockWidgetArea edge : kEdges) {
        if (edgeName(edge) == name) {
            return edge;
        }
    }
    return Qt::NoDockWidgetArea;
}

/// Left and right panels keep their width, top and bottom their height.
bool keepsWidth(Qt::DockWidgetArea edge) {
    return edge == Qt::LeftDockWidgetArea || edge == Qt::RightDockWidgetArea;
}

}  // namespace

QStringList autoHideIconNames() {
    return {QStringLiteral("ui-pin"), QStringLiteral("ui-unpin")};
}

PanelTitleBar::PanelTitleBar(QDockWidget* panel, bool pinned, QWidget* parent)
    : QWidget(parent),
      m_panel(panel),
      m_title(new QLabel(this)),
      m_pin(new QToolButton(this)),
      m_close(new QToolButton(this)),
      m_pinned(pinned) {
    assert(panel != nullptr);
    setObjectName(QStringLiteral("autoHideTitleBar"));
    // The style sheet gives the bar the colours of the panel headers.
    setAttribute(Qt::WA_StyledBackground);
    m_title->setObjectName(QStringLiteral("autoHideTitle"));
    m_title->setTextFormat(Qt::PlainText);
    m_title->setText(panel->windowTitle());
    connect(panel, &QWidget::windowTitleChanged, m_title, &QLabel::setText);
    for (QToolButton* button : {m_pin, m_close}) {
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIconSize(QSize(kTitleIconSize, kTitleIconSize));
        button->setAutoRaise(true);
        // A click must not take the keyboard away from where it is.
        button->setFocusPolicy(Qt::NoFocus);
    }
    m_pin->setObjectName(QStringLiteral("autoHidePin"));
    m_pin->setToolTip(pinned ? tr("Auto-hide") : tr("Dock"));
    m_close->setObjectName(QStringLiteral("autoHideClose"));
    m_close->setToolTip(tr("Close"));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(kTitleMarginLeft, kTitleMargin, kTitleMargin, kTitleMargin);
    layout->setSpacing(kTitleMargin);
    layout->addWidget(m_title, 1);
    layout->addWidget(m_pin);
    layout->addWidget(m_close);
    connect(m_pin, &QToolButton::clicked, this, &PanelTitleBar::pinClicked);
    connect(m_close, &QToolButton::clicked, this, &PanelTitleBar::closeClicked);
    updateIcons();
}

void PanelTitleBar::setPinVisible(bool visible) {
    m_pin->setVisible(visible);
}

void PanelTitleBar::changeEvent(QEvent* event) {
    // applyTheme installs a new style and a new palette.
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        updateIcons();
    }
    QWidget::changeEvent(event);
}

void PanelTitleBar::updateIcons() {
    const Theme theme = themeOf(palette());
    m_pin->setIcon(
        themedIcon(m_pinned ? QStringLiteral("ui-pin") : QStringLiteral("ui-unpin"), theme));
    // The cross that Style draws for the headers of a QDockWidget.
    m_close->setIcon(
        m_panel->style()->standardIcon(QStyle::SP_TitleBarCloseButton, nullptr, m_panel));
}

AutoHideTab::AutoHideTab(QDockWidget* panel, Qt::Orientation orientation, QWidget* parent)
    : QAbstractButton(parent), m_panel(panel), m_orientation(orientation) {
    assert(panel != nullptr);
    setObjectName(QStringLiteral("autoHideTab"));
    setFocusPolicy(Qt::NoFocus);
    // Repainted when the pointer comes and goes.
    setAttribute(Qt::WA_Hover);
}

void AutoHideTab::setActive(bool active) {
    m_active = active;
    update();
}

QSize AutoHideTab::sizeHint() const {
    const QFontMetrics metrics(font());
    const int length = kTabPadding + kTabIconSize + kTabSpacing +
                       metrics.horizontalAdvance(m_panel->windowTitle()) + kTabPadding;
    const int thickness = std::max(kTabIconSize, metrics.height()) + kTabPadding;
    return m_orientation == Qt::Horizontal ? QSize(length, thickness) : QSize(thickness, length);
}

QSize AutoHideTab::minimumSizeHint() const {
    return sizeHint();
}

void AutoHideTab::paintEvent(QPaintEvent* /*event*/) {
    const ThemeTokens& tokens = themeTokens(themeOf(palette()));
    QPainter painter(this);
    if (m_active || isDown() || underMouse()) {
        painter.fillRect(rect(), tokens.selection);
    }
    // The tab's own edge towards the drawing, so that neighbours stay apart.
    painter.setPen(tokens.border);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    const bool vertical = m_orientation == Qt::Vertical;
    // The icon stays upright; on the left and right edges the name is
    // written downwards, as on a turned book spine.
    const int across = vertical ? width() : height();
    const QRect iconRect =
        vertical ? QRect((across - kTabIconSize) / 2, kTabPadding, kTabIconSize, kTabIconSize)
                 : QRect(kTabPadding, (across - kTabIconSize) / 2, kTabIconSize, kTabIconSize);
    m_panel->windowIcon().paint(&painter, iconRect);
    if (vertical) {
        painter.translate(width(), 0);
        painter.rotate(90);
    }
    const int along = vertical ? height() : width();
    const int textStart = kTabPadding + kTabIconSize + kTabSpacing;
    painter.setPen(tokens.text);
    painter.drawText(QRect(textStart, 0, along - textStart - kTabPadding, across),
                     Qt::AlignLeft | Qt::AlignVCenter, m_panel->windowTitle());
}

AutoHideBar::AutoHideBar(Qt::DockWidgetArea edge, QWidget* parent)
    : QWidget(parent),
      m_edge(edge),
      m_layout(new QBoxLayout(keepsWidth(edge) ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight,
                              this)) {
    setObjectName(QStringLiteral("autoHideBar-%1").arg(edgeName(edge)));
    setAttribute(Qt::WA_StyledBackground);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(kTabGap);
    m_layout->addStretch(1);
    hide();
}

Qt::Orientation AutoHideBar::orientation() const {
    return keepsWidth(m_edge) ? Qt::Vertical : Qt::Horizontal;
}

QList<AutoHideTab*> AutoHideBar::tabs() const {
    QList<AutoHideTab*> result;
    for (int index = 0; index < m_layout->count(); ++index) {
        if (auto* tab = qobject_cast<AutoHideTab*>(m_layout->itemAt(index)->widget());
            tab != nullptr) {
            result.append(tab);
        }
    }
    return result;
}

void AutoHideBar::addTab(AutoHideTab* tab) {
    // Before the stretch: the tabs start at the top or on the left.
    m_layout->insertWidget(m_layout->count() - 1, tab);
    show();
}

void AutoHideBar::removeTab(AutoHideTab* tab) {
    m_layout->removeWidget(tab);
    tab->hide();
    // The tab may be the sender of the signal that removes it.
    tab->deleteLater();
    setVisible(!tabs().isEmpty());
}

AutoHideOverlay::AutoHideOverlay(QWidget* parent)
    : QFrame(parent), m_pages(new QStackedWidget(this)), m_handle(new QWidget(this)) {
    setObjectName(QStringLiteral("autoHideOverlay"));
    setAutoFillBackground(true);
    m_pages->setObjectName(QStringLiteral("autoHidePages"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    layout->addWidget(m_pages);
    // Outside the layout: AutoHide::placeOverlay() puts it on the edge
    // towards the drawing area, over the border and the pages. It draws
    // nothing; the cursor shows it.
    m_handle->setObjectName(QStringLiteral("autoHideHandle"));
}

void AutoHideOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        emit escapePressed();
        return;
    }
    QFrame::keyPressEvent(event);
}

AutoHide::AutoHide(QMainWindow& window, QWidget* drawingArea, QWidget* drawing)
    : QObject(&window),
      m_window(window),
      m_drawing(drawing),
      m_frame(new QWidget(&window)),
      m_overlay(new AutoHideOverlay(m_frame)) {
    assert(drawingArea != nullptr && drawing != nullptr && drawingArea->isAncestorOf(drawing));
    m_frame->setObjectName(QStringLiteral("autoHideFrame"));
    for (std::size_t index = 0; index < kEdges.size(); ++index) {
        m_bars.at(index) = new AutoHideBar(kEdges.at(index), m_frame);
    }
    auto* grid = new QGridLayout(m_frame);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);
    grid->addWidget(bar(Qt::TopDockWidgetArea), 0, 0, 1, 3);
    grid->addWidget(bar(Qt::LeftDockWidgetArea), 1, 0);
    grid->addWidget(drawingArea, 1, 1);
    grid->addWidget(bar(Qt::RightDockWidgetArea), 1, 2);
    grid->addWidget(bar(Qt::BottomDockWidgetArea), 2, 0, 1, 3);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(1, 1);

    m_overlay->hide();
    m_overlay->installEventFilter(this);
    m_overlay->handle()->installEventFilter(this);
    connect(m_overlay, &AutoHideOverlay::escapePressed, this, &AutoHide::close);
    // The overlay follows the drawing when the window changes size.
    m_drawing->installEventFilter(this);

    m_showTimer.setSingleShot(true);
    m_showTimer.setInterval(kShowDelayMs);
    connect(&m_showTimer, &QTimer::timeout, this, [this] {
        if (m_hoverTab != nullptr) {
            open(m_hoverTab->panel(), false);
        }
    });
    m_closeTimer.setSingleShot(true);
    m_closeTimer.setInterval(kHideDelayMs);
    connect(&m_closeTimer, &QTimer::timeout, this, &AutoHide::closeIfLeft);
    auto* application = qobject_cast<QApplication*>(QCoreApplication::instance());
    assert(application != nullptr);
    connect(application, &QApplication::focusChanged, this, [this](QWidget* /*old*/, QWidget* now) {
        const bool inside = now != nullptr && (now == m_overlay || m_overlay->isAncestorOf(now));
        if (m_open != nullptr && !inside && !m_pointerInside && !m_resizing) {
            m_closeTimer.start();
        }
    });
}

AutoHide::~AutoHide() {
    // The window is being destroyed: nothing is shown, focused or saved.
    if (QCoreApplication* application = QCoreApplication::instance(); application != nullptr) {
        application->removeEventFilter(this);
    }
    for (const Hidden& entry : m_hidden) {
        if (entry.content != nullptr) {
            entry.panel->setWidget(entry.content);
        }
    }
}

AutoHideBar* AutoHide::bar(Qt::DockWidgetArea edge) const {
    return m_bars.at(edgeIndex(edge));
}

AutoHideTab* AutoHide::tab(const QDockWidget* panel) const {
    const Hidden* hidden = find(panel);
    return hidden == nullptr ? nullptr : hidden->tab;
}

void AutoHide::addPanel(QDockWidget* panel) {
    assert(panel != nullptr);
    auto* title = new PanelTitleBar(panel, true, panel);
    title->setPinVisible(m_enabled && !panel->isFloating());
    panel->setTitleBarWidget(title);
    // The title bar lives as long as its panel, and the panel as long as
    // the window.
    connect(panel, &QDockWidget::topLevelChanged, this,
            [this, title](bool floating) { title->setPinVisible(m_enabled && !floating); });
    connect(title, &PanelTitleBar::pinClicked, this, [this, panel] { setAutoHidden(panel, true); });
    connect(title, &PanelTitleBar::closeClicked, panel, &QWidget::close);
    panel->installEventFilter(this);
    m_panels.emplace_back(panel);

    // A panel added after the layout was restored (MainWindow::addPanel).
    const auto pending =
        std::find_if(m_pending.begin(), m_pending.end(),
                     [panel](const Pending& item) { return item.name == panel->objectName(); });
    if (pending != m_pending.end()) {
        const Pending item = *pending;
        m_pending.erase(pending);
        if (m_enabled && !panel->isFloating()) {
            hidePanel(panel, item.edge, item.extent);
        }
    }
}

bool AutoHide::setAutoHidden(QDockWidget* panel, bool hidden) {
    assert(panel != nullptr);
    if (hidden) {
        if (isAutoHidden(panel)) {
            return true;
        }
        if (!m_enabled || panel->isFloating()) {
            return false;
        }
        const Qt::DockWidgetArea edge = m_window.dockWidgetArea(panel);
        if (edge == Qt::NoDockWidgetArea) {
            return false;
        }
        return hidePanel(panel, edge, keepsWidth(edge) ? panel->width() : panel->height());
    }
    const auto found = std::find_if(m_hidden.begin(), m_hidden.end(),
                                    [panel](const Hidden& item) { return item.panel == panel; });
    if (found == m_hidden.end()) {
        return false;
    }
    if (m_open == panel) {
        close();
    }
    const Hidden entry = *found;
    m_hidden.erase(found);
    if (m_hoverTab == entry.tab) {
        m_showTimer.stop();
        m_hoverTab = nullptr;
    }
    bar(entry.edge)->removeTab(entry.tab);
    if (entry.content != nullptr) {
        // setWidget() takes the content back from the page and shows it.
        panel->setWidget(entry.content);
    }
    m_overlay->pages()->removeWidget(entry.page);
    // The pin of the page's header may be the sender of this call.
    entry.page->deleteLater();
    // The panel was hidden in its place, which showing it gives back.
    panel->show();
    if (entry.extent > 0) {
        m_window.resizeDocks({panel}, {entry.extent},
                             keepsWidth(entry.edge) ? Qt::Horizontal : Qt::Vertical);
    }
    emit changed();
    return true;
}

bool AutoHide::isAutoHidden(const QDockWidget* panel) const {
    return find(panel) != nullptr;
}

bool AutoHide::hidePanel(QDockWidget* panel, Qt::DockWidgetArea edge, int extent) {
    if (isAutoHidden(panel)) {
        return true;
    }
    close();
    Hidden entry;
    entry.panel = panel;
    entry.edge = edge;
    entry.extent = extent;
    if (entry.extent <= 0) {
        const QSize hint = panel->sizeHint();
        entry.extent = keepsWidth(edge) ? hint.width() : hint.height();
    }
    entry.content = panel->widget();
    // The panel keeps its place in the layout while hidden.
    panel->hide();

    auto* page = new QWidget(m_overlay->pages());
    page->setObjectName(QStringLiteral("autoHidePage"));
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* header = new PanelTitleBar(panel, false, page);
    layout->addWidget(header);
    if (entry.content != nullptr) {
        panel->setWidget(nullptr);
        layout->addWidget(entry.content, 1);
        entry.content->show();
    } else {
        layout->addStretch(1);
    }
    m_overlay->pages()->addWidget(page);
    entry.page = page;
    connect(header, &PanelTitleBar::pinClicked, this,
            [this, panel] { setAutoHidden(panel, false); });
    connect(header, &PanelTitleBar::closeClicked, this, [this, panel] {
        setAutoHidden(panel, false);
        panel->close();
    });

    AutoHideBar* edgeBar = bar(edge);
    auto* tab = new AutoHideTab(panel, edgeBar->orientation(), edgeBar);
    tab->setToolTip(panel->windowTitle());
    tab->installEventFilter(this);
    // A click opens the panel; it never closes it, as the pointer may have
    // opened it already by resting on the tab.
    connect(tab, &QAbstractButton::clicked, this, [this, tab] { open(tab->panel(), false); });
    edgeBar->addTab(tab);
    entry.tab = tab;
    m_hidden.push_back(entry);
    emit changed();
    return true;
}

const AutoHide::Hidden* AutoHide::find(const QDockWidget* panel) const {
    const auto found = std::find_if(m_hidden.begin(), m_hidden.end(),
                                    [panel](const Hidden& item) { return item.panel == panel; });
    return found == m_hidden.end() ? nullptr : &*found;
}

AutoHide::Hidden* AutoHide::find(const QDockWidget* panel) {
    const auto found = std::find_if(m_hidden.begin(), m_hidden.end(),
                                    [panel](const Hidden& item) { return item.panel == panel; });
    return found == m_hidden.end() ? nullptr : &*found;
}

void AutoHide::open(QDockWidget* panel, bool focus) {
    const Hidden* hidden = find(panel);
    if (hidden == nullptr) {
        return;
    }
    if (m_open != panel) {
        close();
        m_open = panel;
        m_overlay->pages()->setCurrentWidget(hidden->page);
        hidden->tab->setActive(true);
        placeOverlay();
        m_overlay->show();
        m_overlay->raise();
        m_pointerInside = m_hoverTab == hidden->tab;
        // Esc closes the overlay wherever the keyboard is.
        QCoreApplication::instance()->installEventFilter(this);
    }
    if (focus && hidden->content != nullptr) {
        QWidget* target = hidden->content->focusWidget();
        if (target == nullptr) {
            target = hidden->content;
        }
        target->setFocus(Qt::OtherFocusReason);
    }
}

void AutoHide::close() {
    if (m_open == nullptr) {
        return;
    }
    const bool hadFocus = focusInOverlay();
    if (const Hidden* hidden = find(m_open); hidden != nullptr) {
        hidden->tab->setActive(false);
    }
    m_open = nullptr;
    m_closeTimer.stop();
    m_pointerInside = false;
    m_resizing = false;
    QCoreApplication::instance()->removeEventFilter(this);
    m_overlay->hide();
    if (hadFocus) {
        m_drawing->setFocus(Qt::OtherFocusReason);
    }
}

QDockWidget* AutoHide::openPanel() const {
    return m_open;
}

void AutoHide::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled) {
        m_pending.clear();
        while (!m_hidden.empty()) {
            setAutoHidden(m_hidden.back().panel, false);
        }
    }
    for (const QDockWidget* panel : m_panels) {
        if (auto* title = qobject_cast<PanelTitleBar*>(panel->titleBarWidget()); title != nullptr) {
            title->setPinVisible(enabled && !panel->isFloating());
        }
    }
}

QStringList AutoHide::saveState() const {
    QStringList state;
    for (const Hidden& entry : m_hidden) {
        state.append(QStringLiteral("%1:%2:%3")
                         .arg(entry.panel->objectName(), edgeName(entry.edge))
                         .arg(entry.extent));
    }
    return state;
}

void AutoHide::restoreState(const QStringList& state) {
    if (!m_enabled) {
        return;
    }
    for (const QString& item : state) {
        const QStringList parts = item.split(QLatin1Char(':'));
        constexpr qsizetype kParts = 3;
        if (parts.size() != kParts) {
            continue;
        }
        const Qt::DockWidgetArea edge = edgeFromName(parts.at(1));
        bool number = false;
        const int extent = parts.at(2).toInt(&number);
        if (parts.at(0).isEmpty() || edge == Qt::NoDockWidgetArea || !number) {
            continue;
        }
        QDockWidget* panel = nullptr;
        for (QDockWidget* candidate : m_panels) {
            if (candidate->objectName() == parts.at(0)) {
                panel = candidate;
                break;
            }
        }
        if (panel == nullptr) {
            m_pending.push_back({parts.at(0), edge, extent});
        } else if (!panel->isFloating()) {
            hidePanel(panel, edge, extent);
        }
    }
}

bool AutoHide::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_overlay->handle()) {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            case QEvent::MouseButtonRelease:
                if (const auto* mouse = dynamic_cast<const QMouseEvent*>(event);
                    mouse != nullptr && dragHandle(*mouse)) {
                    return true;
                }
                break;
            default:
                break;
        }
        return QObject::eventFilter(watched, event);
    }
    auto* hoveredTab = qobject_cast<AutoHideTab*>(watched);
    const AutoHideTab* openTab = m_open == nullptr ? nullptr : tab(m_open);
    switch (event->type()) {
        case QEvent::Enter:
            if (watched == m_overlay || (openTab != nullptr && watched == openTab)) {
                m_pointerInside = true;
                m_closeTimer.stop();
            }
            if (hoveredTab != nullptr && hoveredTab->panel() != m_open) {
                m_hoverTab = hoveredTab;
                m_showTimer.start();
            }
            break;
        case QEvent::Leave:
            if (hoveredTab != nullptr && hoveredTab == m_hoverTab) {
                m_showTimer.stop();
                m_hoverTab = nullptr;
            }
            // While the handle is dragged the pointer counts as inside.
            if (!m_resizing &&
                (watched == m_overlay || (openTab != nullptr && watched == openTab))) {
                m_pointerInside = false;
                m_closeTimer.start();
            }
            break;
        case QEvent::KeyPress:
            if (m_open != nullptr && !focusInOverlay()) {
                if (const auto* key = dynamic_cast<const QKeyEvent*>(event);
                    key != nullptr && key->key() == Qt::Key_Escape) {
                    close();
                    return true;
                }
            }
            break;
        case QEvent::Show:
            // QDockWidget::show() of a hidden panel (its action, the
            // command that opens it) shows it in the overlay instead.
            if (auto* panel = qobject_cast<QDockWidget*>(watched);
                panel != nullptr && isAutoHidden(panel)) {
                QTimer::singleShot(0, this, [this, panel] { reopen(panel); });
            }
            break;
        case QEvent::Resize:
        case QEvent::Move:
            if (watched == m_drawing && m_open != nullptr) {
                placeOverlay();
            }
            break;
        case QEvent::WindowTitleChange:
        case QEvent::WindowIconChange:
            if (auto* panel = qobject_cast<QDockWidget*>(watched); panel != nullptr) {
                if (AutoHideTab* panelTab = tab(panel); panelTab != nullptr) {
                    panelTab->setToolTip(panel->windowTitle());
                    panelTab->updateGeometry();
                    panelTab->update();
                }
            }
            break;
        default:
            break;
    }
    return QObject::eventFilter(watched, event);
}

void AutoHide::placeOverlay() {
    const Hidden* hidden = find(m_open);
    if (hidden == nullptr) {
        return;
    }
    const QRect area(m_drawing->mapTo(m_frame, QPoint(0, 0)), m_drawing->size());
    QRect rect = area;
    switch (hidden->edge) {
        case Qt::LeftDockWidgetArea:
            rect.setWidth(std::min(hidden->extent, area.width()));
            break;
        case Qt::RightDockWidgetArea:
            rect.setLeft(area.right() + 1 - std::min(hidden->extent, area.width()));
            break;
        case Qt::TopDockWidgetArea:
            rect.setHeight(std::min(hidden->extent, area.height()));
            break;
        case Qt::BottomDockWidgetArea:
            rect.setTop(area.bottom() + 1 - std::min(hidden->extent, area.height()));
            break;
        default:
            break;
    }
    m_overlay->setGeometry(rect);
    // The handle on the edge towards the drawing area.
    QWidget* handle = m_overlay->handle();
    switch (hidden->edge) {
        case Qt::LeftDockWidgetArea:
            handle->setGeometry(rect.width() - kHandleThickness, 0, kHandleThickness,
                                rect.height());
            break;
        case Qt::RightDockWidgetArea:
            handle->setGeometry(0, 0, kHandleThickness, rect.height());
            break;
        case Qt::TopDockWidgetArea:
            handle->setGeometry(0, rect.height() - kHandleThickness, rect.width(),
                                kHandleThickness);
            break;
        case Qt::BottomDockWidgetArea:
            handle->setGeometry(0, 0, rect.width(), kHandleThickness);
            break;
        default:
            break;
    }
    handle->setCursor(keepsWidth(hidden->edge) ? Qt::SizeHorCursor : Qt::SizeVerCursor);
    handle->raise();
    handle->show();
}

bool AutoHide::dragHandle(const QMouseEvent& event) {
    const QEvent::Type type = event.type();
    Hidden* hidden = find(m_open);
    if (hidden == nullptr) {
        return false;
    }
    const QPoint global = event.globalPosition().toPoint();
    if (type == QEvent::MouseButtonPress) {
        if (event.button() != Qt::LeftButton) {
            return false;
        }
        m_resizing = true;
        m_resizeStart = global;
        m_resizeExtent = hidden->extent;
        m_pointerInside = true;
        m_closeTimer.stop();
        return true;
    }
    if (!m_resizing) {
        return false;
    }
    if (type == QEvent::MouseMove) {
        // The overlay grows towards the drawing area: leftwards from the
        // right edge, rightwards from the left one, and so on.
        const QPoint moved = global - m_resizeStart;
        int delta = 0;
        int area = 0;
        switch (hidden->edge) {
            case Qt::LeftDockWidgetArea:
                delta = moved.x();
                area = m_drawing->width();
                break;
            case Qt::RightDockWidgetArea:
                delta = -moved.x();
                area = m_drawing->width();
                break;
            case Qt::TopDockWidgetArea:
                delta = moved.y();
                area = m_drawing->height();
                break;
            case Qt::BottomDockWidgetArea:
                delta = -moved.y();
                area = m_drawing->height();
                break;
            default:
                break;
        }
        const int largest = std::max(kMinimumExtent, area - kMinimumExtent);
        hidden->extent = std::clamp(m_resizeExtent + delta, kMinimumExtent, largest);
        placeOverlay();
        return true;
    }
    // The release: the new extent is the one to save.
    m_resizing = false;
    m_pointerInside = m_overlay->rect().contains(m_overlay->mapFromGlobal(global));
    if (!m_pointerInside && !focusInOverlay()) {
        m_closeTimer.start();
    }
    emit changed();
    return true;
}

bool AutoHide::focusInOverlay() const {
    const QWidget* focus = QApplication::focusWidget();
    return focus != nullptr && (focus == m_overlay || m_overlay->isAncestorOf(focus));
}

void AutoHide::closeIfLeft() {
    if (m_open == nullptr || m_pointerInside || m_resizing || focusInOverlay()) {
        return;
    }
    // A list or a window opened from the panel, such as a combo box's list
    // or the colour window, belongs to it.
    if (QApplication::activePopupWidget() != nullptr ||
        QApplication::activeModalWidget() != nullptr) {
        return;
    }
    close();
}

void AutoHide::reopen(QDockWidget* panel) {
    if (!isAutoHidden(panel) || !panel->isVisible()) {
        return;
    }
    panel->hide();
    open(panel, true);
}

}  // namespace tg::app
