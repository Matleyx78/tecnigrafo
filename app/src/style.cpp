// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "style.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QTabBar>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <utility>

namespace tg::app {
namespace {

/// docs/adr/0039, point 3: rows of 20-22 px in panels and grids.
constexpr int kRowHeight = 22;
/// Thin scroll bars, as in the reference of docs/adr/0039 (point 4).
constexpr int kScrollBarExtent = 10;
/// Space around the rounded handle inside the scroll bar.
constexpr int kScrollBarHandleInset = 2;
/// A handle shorter than this is hard to grab with the pointer.
constexpr int kScrollBarMinHandle = 24;
/// Half width and height of the chevron of the combo boxes, in pixels.
constexpr qreal kArrowHalfWidth = 3.5;
constexpr qreal kArrowHalfHeight = 2.0;
/// Pen of the chevrons and of the dock header buttons: the 16 px icon
/// stroke of docs/adr/0040, point 3.
constexpr qreal kStrokeWidth = 1.25;
/// The 16 px icon grid of docs/adr/0040, point 3, for which kStrokeWidth
/// is given.
constexpr qreal kIconGrid = 16.0;
/// Sizes of the dock header buttons, including high-DPI variants.
constexpr std::array kDockIconSizes{10, 12, 16, 20, 24, 32};

/// Space left above and below the separators and the handles of the
/// toolbars, which are not as tall as the bar (area 3).
constexpr int kToolBarLineInset = 4;
/// The dots of the handle of a toolbar, and their pitch.
constexpr int kHandleDot = 2;
constexpr int kHandleDotStep = 4;

// Combo boxes are drawn by Style, not by the style sheet: with a styled
// drop-down the style sheet draws no arrow at all unless it is given an
// image, and without one it draws Windows' raised button.
constexpr qreal kFieldRadius = 2.0;

// The colours of the style sheet are written as {token} and replaced
// below, so that every colour comes from ThemeTokens.
//
// SARibbon gives the names of the groups a font of 80% of the height of
// their band (SARibbonPanel::PrivateData::resetTitleLabelFont); the style
// sheet keeps them at the 11 px of docs/adr/0039 (point 1, area 3).
//
// The tabs of the documents and those of the ribbon have rules of their own
// (docs/tappe/0.7.5/P40, point 2): SARibbonTabBar derives from QTabBar, and
// the padding and the top border of the tabs of the documents made the tabs
// of the ribbon taller than their row and than File. A tab of the ribbon is
// as tall as its bar (SARibbonTabBar::tabSizeHint), with the text in the
// middle.
constexpr std::string_view kStyleSheetTemplate{R"(
QMainWindow::separator { background: {border}; width: 1px; height: 1px; }
QMenuBar { background: {chrome}; color: {text}; }
QMenuBar::item:selected { background: {selection}; }
QMenu { background: {field}; color: {text}; border: 1px solid {border}; }
QMenu::separator { height: 1px; background: {border}; margin: 3px 6px; }
QToolTip { background: {field}; color: {text}; border: 1px solid {border}; padding: 2px 4px; }
QToolBar { background: {ribbon}; border: none; border-bottom: 1px solid {border}; spacing: 2px; padding: 2px; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }
QToolButton:hover { background: {selection}; border-color: {border}; }
QToolButton:pressed, QToolButton:checked { background: {selection}; border-color: {accent}; }
QToolBar QToolButton:checked { border-color: transparent; }
QToolBar QToolButton[popupMode="1"] { padding-right: 12px; }
QToolBar QToolButton::menu-button { background: transparent; border: none; width: 10px; }
SARibbonBar { background: {ribbon}; color: {text}; border-bottom: 1px solid {border}; }
SARibbonButtonGroupWidget, QToolBar#quickAccessBar { background: transparent; border: none; }
SARibbonSeparatorWidget { color: {border}; }
SARibbonTabBar { background: {chrome}; }
SARibbonTabBar::tab { background: transparent; color: {text}; border: none; border-top: 2px solid transparent; margin: 0px; padding: 0px 12px; }
SARibbonTabBar::tab:hover { background: {ribbon}; }
SARibbonTabBar::tab:selected { background: {ribbon}; border-top-color: {accent}; }
SARibbonPanelLabel { background: {group}; color: {textSecondary}; font-size: 11px; }
QTabWidget::pane { background: {panel}; border: 1px solid {border}; top: -1px; }
QWidget#documentTabs { background: {tabs}; border-top: 1px solid {border}; }
QFrame#documentsSeparator { background: {border}; }
QTabBar#documentTabBar { background: {tabs}; }
QTabBar#documentTabBar::tab { background: {tabs}; color: {text}; border: 1px solid transparent; border-top: 2px solid transparent; border-bottom: none; padding: 4px 12px; }
QTabBar#documentTabBar::tab:hover:!selected { background: {ribbon}; }
QTabBar#documentTabBar::tab:selected { background: {canvas}; color: {text}; border-left-color: {border}; border-right-color: {border}; border-top: 2px solid {accent}; }
QDockWidget { color: {text}; }
QDockWidget::title { background: {ribbon}; border-bottom: 1px solid {border}; padding: 4px 6px; text-align: left; }
tg--app--PanelTitleBar { background: {ribbon}; border-bottom: 1px solid {border}; }
tg--app--AutoHideBar { background: {chrome}; }
tg--app--AutoHideOverlay { background: {panel}; border: 1px solid {border}; }
QLineEdit, QAbstractSpinBox { background: {field}; color: {text}; border: 1px solid {border}; border-radius: 2px; padding: 1px 4px; selection-background-color: {selection}; selection-color: {text}; }
QLineEdit:focus, QAbstractSpinBox:focus { border-color: {accent}; }
QComboBox QLineEdit { background: transparent; border: none; padding: 0px; }
QWidget#commandLine { background: {panel}; border: 1px solid {border}; border-radius: 4px; }
QWidget#commandLine QPlainTextEdit#history { background: transparent; color: {textSecondary}; border: none; }
QFrame#commandInput { background: {field}; border: 1px solid {border}; border-radius: 3px; }
QFrame#commandInput QLineEdit { background: transparent; border: none; padding: 0px; }
QLabel#promptMark { color: {accent}; }
QWidget#dynamicInputBox { background: {panel}; border: 1px solid {border}; border-radius: 4px; }
QWidget#dynamicInputBox QLabel { background: transparent; }
QStatusBar { background: {chrome}; border-top: 1px solid {border}; }
QStatusBar QToolButton:checked { border-color: transparent; }
QStatusBar QFrame#statusSeparator { background: {border}; }
QStatusBar QToolButton#environmentIndicator::menu-indicator { image: none; width: 0px; }
QDockWidget QScrollArea, QDockWidget QScrollArea > QWidget > QWidget { background: {panel}; }
QDockWidget QAbstractItemView { background: {panel}; border: none; }
QAbstractItemView { background: {field}; alternate-background-color: {group}; color: {text}; border: 1px solid {border}; selection-background-color: {selection}; selection-color: {text}; }
QComboBox QAbstractItemView { outline: none; }
QTableView#layerTable { border: none; }
QTableView#layerTable::item { border-bottom: 1px solid {border}; }
QTableView#layerTable QComboBox { background: transparent; border: none; }
QTableView#layerTable QComboBox::drop-down { width: 0px; border: none; }
QTableView#layerTable QComboBox::down-arrow { image: none; }
QHeaderView::section { background: {group}; color: {textSecondary}; border: none; border-right: 1px solid {border}; border-bottom: 1px solid {border}; padding: 2px 6px; }
)"};

bool isDockHeader(const QWidget* widget) {
    // The buttons of a dock header are children of the QDockWidget.
    return widget != nullptr &&
           (qobject_cast<const QDockWidget*>(widget) != nullptr ||
            qobject_cast<const QDockWidget*>(widget->parentWidget()) != nullptr);
}

QIcon drawnIcon(const QColor& color, void (*draw)(QPainter&, const QRectF&)) {
    QIcon icon;
    for (const int size : kDockIconSizes) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color, kStrokeWidth * size / kIconGrid, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        // A quarter of the side is left free around the glyph, as Fusion does.
        const qreal margin = size / 4.0;
        draw(painter, QRectF(margin, margin, size - 2 * margin, size - 2 * margin));
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

void drawCross(QPainter& painter, const QRectF& box) {
    painter.drawLine(box.topLeft(), box.bottomRight());
    painter.drawLine(box.topRight(), box.bottomLeft());
}

void drawFloat(QPainter& painter, const QRectF& box) {
    // Two overlapping windows: the floating state of the panel.
    const qreal side = box.width() * 0.7;
    painter.drawRect(QRectF(box.right() - side, box.top(), side, side));
    painter.drawRect(QRectF(box.left(), box.bottom() - side, side, side));
}

}  // namespace

QString styleSheet(const ThemeTokens& tokens) {
    const std::array<std::pair<QString, const QColor*>, 12> colours{{
        {QStringLiteral("{chrome}"), &tokens.chrome},
        {QStringLiteral("{ribbon}"), &tokens.ribbon},
        {QStringLiteral("{group}"), &tokens.group},
        {QStringLiteral("{tabs}"), &tokens.tabs},
        {QStringLiteral("{panel}"), &tokens.panel},
        {QStringLiteral("{field}"), &tokens.field},
        {QStringLiteral("{canvas}"), &tokens.canvas},
        {QStringLiteral("{border}"), &tokens.border},
        {QStringLiteral("{textSecondary}"), &tokens.textSecondary},
        {QStringLiteral("{text}"), &tokens.text},
        {QStringLiteral("{accent}"), &tokens.accent},
        {QStringLiteral("{selection}"), &tokens.selection},
    }};
    QString sheet = QString::fromLatin1(kStyleSheetTemplate);
    for (const auto& [name, colour] : colours) {
        sheet.replace(name, colour->name(QColor::HexRgb));
    }
    assert(!sheet.contains(QRegularExpression(QStringLiteral(R"(\{[A-Za-z]+\})"))) &&
           "a token of the style sheet has no colour");
    return sheet;
}

const Style* installedStyle() {
    QStyle* style = QApplication::style();
    if (style == nullptr) {
        return nullptr;
    }
    if (const auto* own = qobject_cast<const Style*>(style)) {
        return own;
    }
    // With a style sheet, QApplication wraps the installed style in its own
    // style sheet style, which becomes the parent of the installed one.
    return style->findChild<Style*>();
}

Style::Style(const ThemeTokens& tokens, QObject* parent)
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion"))), m_tokens(tokens) {
    setParent(parent);
}

const ThemeTokens& Style::tokens() const {
    return m_tokens;
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption* option,
                       const QWidget* widget) const {
    switch (metric) {
        case PM_ScrollBarExtent:
            return kScrollBarExtent;
        case PM_ScrollBarSliderMin:
            return kScrollBarMinHandle;
        case PM_HeaderDefaultSectionSizeVertical:
            // Table rows are the sections of the vertical header.
            return kRowHeight;
        case PM_TabBarTabShiftHorizontal:
        case PM_TabBarTabShiftVertical:
            // The current tab is marked by its colours, not moved.
            return 0;
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QSize Style::sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& size,
                              const QWidget* widget) const {
    QSize result = QProxyStyle::sizeFromContents(type, option, size, widget);
    if (type == CT_ItemViewItem) {
        result.setHeight(std::max(result.height(), kRowHeight));
    }
    return result;
}

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                          const QWidget* widget) const {
    if (element == PE_IndicatorArrowDown && qobject_cast<const QComboBox*>(widget) != nullptr) {
        // A thin chevron instead of Fusion's filled triangle.
        const QPointF centre = QRectF(option->rect).center();
        const bool enabled = (option->state & State_Enabled) != 0;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(enabled ? m_tokens.text : m_tokens.textSecondary, kStrokeWidth,
                             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath chevron(centre + QPointF(-kArrowHalfWidth, -kArrowHalfHeight));
        chevron.lineTo(centre + QPointF(0.0, kArrowHalfHeight));
        chevron.lineTo(centre + QPointF(kArrowHalfWidth, -kArrowHalfHeight));
        painter->drawPath(chevron);
        painter->restore();
        return;
    }
    if (element == PE_IndicatorTabClose) {
        // A thin grey cross, lighter under the mouse, as in the reference
        // of docs/adr/0039 (area 4), instead of the platform's icon.
        const bool hover = (option->state & State_MouseOver) != 0;
        const QRectF box = QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5);
        const qreal side = std::min(box.width(), box.height());
        constexpr qreal kCrossArm = 4.0;
        const qreal arm = std::min(side / 4.0, kCrossArm);
        const QPointF centre = box.center();
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (hover) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_tokens.selection);
            painter->drawRoundedRect(
                QRectF(centre.x() - side / 2, centre.y() - side / 2, side, side), 2.0, 2.0);
        }
        painter->setPen(QPen(hover ? m_tokens.text : m_tokens.textSecondary, kStrokeWidth,
                             Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(centre + QPointF(-arm, -arm), centre + QPointF(arm, arm));
        painter->drawLine(centre + QPointF(arm, -arm), centre + QPointF(-arm, arm));
        painter->restore();
        return;
    }
    if (element == PE_IndicatorToolBarSeparator) {
        // A thin line, not as tall as the bar, as between the groups of the
        // ribbon (area 3), instead of Fusion's light relief.
        const QRect rect = option->rect;
        const bool across = (option->state & State_Horizontal) != 0;
        painter->save();
        painter->setPen(m_tokens.border);
        if (across) {
            const int x = rect.center().x();
            painter->drawLine(x, rect.top() + kToolBarLineInset, x,
                              rect.bottom() - kToolBarLineInset);
        } else {
            const int y = rect.center().y();
            painter->drawLine(rect.left() + kToolBarLineInset, y, rect.right() - kToolBarLineInset,
                              y);
        }
        painter->restore();
        return;
    }
    if (element == PE_IndicatorToolBarHandle) {
        // A column of small dots in the colour of the borders: the handle
        // stays, so that the toolbars can be moved, without standing out.
        const QRect rect = option->rect;
        const bool across = (option->state & State_Horizontal) != 0;
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_tokens.border);
        if (across) {
            const int x = rect.center().x() - (kHandleDot / 2);
            for (int y = rect.top() + kToolBarLineInset;
                 y + kHandleDot <= rect.bottom() - kToolBarLineInset; y += kHandleDotStep) {
                painter->drawRect(x, y, kHandleDot, kHandleDot);
            }
        } else {
            const int y = rect.center().y() - (kHandleDot / 2);
            for (int x = rect.left() + kToolBarLineInset;
                 x + kHandleDot <= rect.right() - kToolBarLineInset; x += kHandleDotStep) {
                painter->drawRect(x, y, kHandleDot, kHandleDot);
            }
        }
        painter->restore();
        return;
    }
    if (element == PE_FrameTabBarBase) {
        // One border line under the tabs, without Fusion's relief.
        if (const auto* base = qstyleoption_cast<const QStyleOptionTabBarBase*>(option)) {
            const QRect rect = base->rect;
            painter->save();
            painter->setPen(m_tokens.border);
            switch (base->shape) {
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                    painter->drawLine(rect.topLeft(), rect.topRight());
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                    painter->drawLine(rect.topRight(), rect.bottomRight());
                    break;
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    painter->drawLine(rect.topLeft(), rect.bottomLeft());
                    break;
                default:
                    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
                    break;
            }
            painter->restore();
            return;
        }
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                               QPainter* painter, const QWidget* widget) const {
    if (const auto* combo = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        control == CC_ComboBox && combo != nullptr) {
        // A field with a chevron, as the style sheet draws line edits; the
        // text is drawn by the combo box itself (CE_ComboBoxLabel).
        const bool active = (combo->state & (State_HasFocus | State_On)) != 0;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(active ? m_tokens.accent : m_tokens.border);
        painter->setBrush(m_tokens.field);
        painter->drawRoundedRect(QRectF(combo->rect).adjusted(0.5, 0.5, -0.5, -0.5), kFieldRadius,
                                 kFieldRadius);
        painter->restore();
        if ((combo->subControls & SC_ComboBoxArrow) != 0) {
            QStyleOptionComboBox arrow(*combo);
            arrow.rect = subControlRect(CC_ComboBox, combo, SC_ComboBoxArrow, widget);
            drawPrimitive(PE_IndicatorArrowDown, &arrow, painter, widget);
        }
        return;
    }
    const auto* bar = qstyleoption_cast<const QStyleOptionSlider*>(option);
    if (control != CC_ScrollBar || bar == nullptr) {
        QProxyStyle::drawComplexControl(control, option, painter, widget);
        return;
    }
    painter->save();
    // Scroll bars stand mostly beside fields and item views.
    painter->fillRect(bar->rect, m_tokens.field);
    const QRect handle = subControlRect(control, bar, SC_ScrollBarSlider, widget);
    if (handle.isValid() && bar->maximum > bar->minimum) {
        const bool hot = (bar->state & State_Sunken) != 0 ||
                         ((bar->state & State_MouseOver) != 0 &&
                          (bar->activeSubControls & SC_ScrollBarSlider) != 0);
        const QRectF shape =
            QRectF(handle).adjusted(kScrollBarHandleInset, kScrollBarHandleInset,
                                    -kScrollBarHandleInset, -kScrollBarHandleInset);
        const qreal radius = std::min(shape.width(), shape.height()) / 2.0;
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(hot ? m_tokens.textSecondary : m_tokens.border);
        painter->drawRoundedRect(shape, radius, radius);
    }
    painter->restore();
}

QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex* option,
                            SubControl subControl, const QWidget* widget) const {
    const auto* bar = qstyleoption_cast<const QStyleOptionSlider*>(option);
    if (control != CC_ScrollBar || bar == nullptr) {
        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }
    // No arrow buttons: the groove is the whole bar.
    const QRect groove = bar->rect;
    const bool horizontal = bar->orientation == Qt::Horizontal;
    const int length = horizontal ? groove.width() : groove.height();
    const std::int64_t range = static_cast<std::int64_t>(bar->maximum) - bar->minimum;
    int handleLength = length;
    if (range > 0) {
        const std::int64_t page = std::max(bar->pageStep, 0);
        handleLength = static_cast<int>(page * length / (range + page));
        handleLength = std::clamp(handleLength, std::min(kScrollBarMinHandle, length), length);
    }
    const int start = sliderPositionFromValue(bar->minimum, bar->maximum, bar->sliderPosition,
                                              length - handleLength, bar->upsideDown);
    // Before the handle, the handle, after the handle, along the bar.
    int from = 0;
    int to = length;
    switch (subControl) {
        case SC_ScrollBarGroove:
            break;
        case SC_ScrollBarSlider:
            from = start;
            to = start + handleLength;
            break;
        case SC_ScrollBarSubPage:
            to = start;
            break;
        case SC_ScrollBarAddPage:
            from = start + handleLength;
            break;
        default:
            return {};
    }
    const QRect result = horizontal
                             ? QRect(groove.left() + from, groove.top(), to - from, groove.height())
                             : QRect(groove.left(), groove.top() + from, groove.width(), to - from);
    return visualRect(bar->direction, groove, result);
}

QIcon Style::standardIcon(StandardPixmap icon, const QStyleOption* option,
                          const QWidget* widget) const {
    if (isDockHeader(widget)) {
        switch (icon) {
            case SP_TitleBarCloseButton:
            case SP_DockWidgetCloseButton:
                return drawnIcon(m_tokens.text, drawCross);
            case SP_TitleBarNormalButton:
                return drawnIcon(m_tokens.text, drawFloat);
            default:
                break;
        }
    }
    return QProxyStyle::standardIcon(icon, option, widget);
}

}  // namespace tg::app
