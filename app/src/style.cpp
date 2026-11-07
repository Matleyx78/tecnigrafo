// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "style.hpp"

#include <QComboBox>
#include <QDockWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>
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

// The colours of the style sheet are written as {token} and replaced
// below, so that every colour comes from ThemeTokens.
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
SARibbonBar { background: {ribbon}; color: {text}; border-bottom: 1px solid {border}; }
SARibbonTabBar { background: {chrome}; }
SARibbonTabBar::tab:selected { background: {ribbon}; }
SARibbonPanelLabel { background: {group}; color: {textSecondary}; }
QTabWidget::pane { background: {panel}; border: 1px solid {border}; top: -1px; }
QTabBar::tab { background: {tabs}; color: {textSecondary}; border: 1px solid transparent; border-top: 2px solid transparent; padding: 4px 12px; }
QTabBar::tab:hover { color: {text}; }
QTabBar::tab:selected { background: {panel}; color: {text}; border-color: {border}; border-bottom-color: {panel}; border-top: 2px solid {accent}; }
QDockWidget { color: {text}; }
QDockWidget::title { background: {ribbon}; border-bottom: 1px solid {border}; padding: 4px 6px; text-align: left; }
QLineEdit, QAbstractSpinBox, QComboBox { background: {field}; color: {text}; border: 1px solid {border}; border-radius: 2px; padding: 1px 4px; selection-background-color: {selection}; selection-color: {text}; }
QLineEdit:focus, QAbstractSpinBox:focus, QComboBox:focus, QComboBox:on { border-color: {accent}; }
QComboBox::drop-down { border: none; width: 16px; }
QAbstractItemView { background: {field}; alternate-background-color: {group}; color: {text}; border: 1px solid {border}; selection-background-color: {selection}; selection-color: {text}; }
QComboBox QAbstractItemView { outline: none; }
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
    const std::array<std::pair<QString, const QColor*>, 11> colours{{
        {QStringLiteral("{chrome}"), &tokens.chrome},
        {QStringLiteral("{ribbon}"), &tokens.ribbon},
        {QStringLiteral("{group}"), &tokens.group},
        {QStringLiteral("{tabs}"), &tokens.tabs},
        {QStringLiteral("{panel}"), &tokens.panel},
        {QStringLiteral("{field}"), &tokens.field},
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
