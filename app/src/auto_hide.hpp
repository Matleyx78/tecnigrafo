// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QAbstractButton>
#include <QDockWidget>
#include <QFrame>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include <array>
#include <vector>

class QBoxLayout;
class QEvent;
class QKeyEvent;
class QLabel;
class QMainWindow;
class QPaintEvent;
class QStackedWidget;
class QToolButton;

namespace tg::app {

/// The title bar of a docked panel (docs/tappe/0.7.5/P18, point 1): the
/// name, the pin (Auto-hide) and the cross. The same bar heads the panel in
/// the overlay, where the pin docks it again. A floating panel has no pin.
class PanelTitleBar final : public QWidget {
    Q_OBJECT

public:
    /// `panel` must outlive the bar; the bar shows its windowTitle().
    PanelTitleBar(QDockWidget* panel, bool pinned, QWidget* parent);

    [[nodiscard]] QToolButton* pinButton() const { return m_pin; }

    [[nodiscard]] QToolButton* closeButton() const { return m_close; }

    void setPinVisible(bool visible);

Q_SIGNALS:
    void pinClicked();
    void closeClicked();

protected:
    void changeEvent(QEvent* event) override;

private:
    /// The pin and the cross in the colours of the current theme.
    void updateIcons();

    QDockWidget* m_panel;
    QLabel* m_title{nullptr};
    QToolButton* m_pin{nullptr};
    QToolButton* m_close{nullptr};
    bool m_pinned;
};

/// The tab of a hidden panel on the edge where it was docked: its icon and
/// its name, written downwards on the left and right edges.
class AutoHideTab final : public QAbstractButton {
    Q_OBJECT

public:
    /// `panel` must outlive the tab.
    AutoHideTab(QDockWidget* panel, Qt::Orientation orientation, QWidget* parent);

    [[nodiscard]] QDockWidget* panel() const { return m_panel; }

    /// Marked while its panel is shown in the overlay.
    void setActive(bool active);

    [[nodiscard]] bool isActive() const { return m_active; }

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QDockWidget* m_panel;
    Qt::Orientation m_orientation;
    bool m_active{false};
};

/// The tabs of one edge, in the order their panels were hidden. Hidden when
/// it has none.
class AutoHideBar final : public QWidget {
    Q_OBJECT

public:
    AutoHideBar(Qt::DockWidgetArea edge, QWidget* parent);

    [[nodiscard]] Qt::DockWidgetArea edge() const { return m_edge; }

    [[nodiscard]] Qt::Orientation orientation() const;
    [[nodiscard]] QList<AutoHideTab*> tabs() const;

    void addTab(AutoHideTab* tab);
    /// Removes and deletes `tab`.
    void removeTab(AutoHideTab* tab);

private:
    Qt::DockWidgetArea m_edge;
    QBoxLayout* m_layout{nullptr};
};

/// The frame laid over the drawing area that shows a hidden panel, inside
/// the main window and never a window of its own: under Wayland a floating
/// window cannot be placed (docs/tappe/0.7.5/P18, point 3).
class AutoHideOverlay final : public QFrame {
    Q_OBJECT

public:
    explicit AutoHideOverlay(QWidget* parent);

    [[nodiscard]] QStackedWidget* pages() const { return m_pages; }

Q_SIGNALS:
    /// Esc that no widget of the panel used, such as a cell in editing.
    void escapePressed();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QStackedWidget* m_pages{nullptr};
};

/// Panels that hide themselves (docs/tappe/0.7.5/P18), built with Qt only
/// (docs/adr/0009, point 3; DECISIONI.md, 2026-11-07).
///
/// A hidden panel leaves its place: the QDockWidget is hidden where it was
/// docked, so showing it again puts it back there, and its content moves to
/// a page of the overlay. A tab on the same edge stands for it.
class AutoHide final : public QObject {
    Q_OBJECT

public:
    /// How long the pointer rests on a tab before the overlay opens.
    static constexpr int kShowDelayMs = 300;
    /// How long the pointer stays out of the overlay before it closes.
    static constexpr int kHideDelayMs = 500;

    /// `drawingArea` (the drawing and the command line) goes inside frame(),
    /// between the bars; the overlay covers `drawing`, a descendant of it.
    /// The docks belong to `window`, which owns this object.
    AutoHide(QMainWindow& window, QWidget* drawingArea, QWidget* drawing);

    /// The central widget of the window: the bars around the drawing area.
    [[nodiscard]] QWidget* frame() const { return m_frame; }

    [[nodiscard]] AutoHideOverlay* overlay() const { return m_overlay; }

    [[nodiscard]] AutoHideBar* bar(Qt::DockWidgetArea edge) const;
    [[nodiscard]] AutoHideTab* tab(const QDockWidget* panel) const;

    /// Gives a docked panel its title bar; it must belong to the window.
    void addPanel(QDockWidget* panel);

    /// Hides a docked panel (true) or docks it again where it was (false).
    /// Returns false, changing nothing, for a floating or undocked panel,
    /// or to hide while disabled.
    bool setAutoHidden(QDockWidget* panel, bool hidden);
    [[nodiscard]] bool isAutoHidden(const QDockWidget* panel) const;

    /// Shows a hidden panel in the overlay, closing the one shown; with
    /// `focus` the keyboard goes to it, as QDockWidget::show() asked.
    void open(QDockWidget* panel, bool focus);
    void close();
    /// The panel in the overlay, or nullptr.
    [[nodiscard]] QDockWidget* openPanel() const;

    /// The script mode docks every panel again and hides nothing more.
    void setEnabled(bool enabled);

    /// Gives every hidden panel its content back, without showing it and
    /// without signals. The window calls it before it is destroyed: a panel
    /// is deleted after the overlay, and its destructor still reaches its
    /// content.
    void releaseContents();

    /// The hidden panels in the order they were hidden, as
    /// "<objectName>:<edge>:<extent>", edge being left, right, top or bottom.
    [[nodiscard]] QStringList saveState() const;
    /// Hides the panels of `state` as they were; a panel added later is
    /// hidden by addPanel().
    void restoreState(const QStringList& state);

Q_SIGNALS:
    /// A panel was hidden or docked again: the layout to save changed.
    void changed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Hidden {
        QPointer<QDockWidget> panel;
        Qt::DockWidgetArea edge{Qt::RightDockWidgetArea};
        /// Width (left, right) or height (top, bottom) it had docked.
        int extent{0};
        QWidget* content{nullptr};
        QWidget* page{nullptr};
        AutoHideTab* tab{nullptr};
    };

    struct Pending {
        QString name;
        Qt::DockWidgetArea edge{Qt::RightDockWidgetArea};
        int extent{0};
    };

    bool hidePanel(QDockWidget* panel, Qt::DockWidgetArea edge, int extent);
    [[nodiscard]] const Hidden* find(const QDockWidget* panel) const;
    void placeOverlay();
    [[nodiscard]] bool focusInOverlay() const;
    /// Closes the overlay unless the pointer or the keyboard is in it.
    void closeIfLeft();
    /// After a hide or close, a panel shown again by QDockWidget::show().
    void reopen(QDockWidget* panel);

    QMainWindow& m_window;
    QWidget* m_drawing;
    QWidget* m_frame{nullptr};
    AutoHideOverlay* m_overlay{nullptr};
    std::array<AutoHideBar*, 4> m_bars{};
    std::vector<Hidden> m_hidden;
    std::vector<Pending> m_pending;
    /// Every panel given a title bar, hidden or not.
    std::vector<QPointer<QDockWidget>> m_panels;
    QPointer<QDockWidget> m_open;
    QPointer<AutoHideTab> m_hoverTab;
    QTimer m_showTimer;
    QTimer m_closeTimer;
    bool m_pointerInside{false};
    bool m_enabled{true};
};

}  // namespace tg::app
