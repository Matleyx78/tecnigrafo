// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "theme.hpp"

#include <QProxyStyle>
#include <QString>

namespace tg::app {

/// The style sheet of the interface, generated from the tokens
/// (docs/adr/0039, points 2 and 3): surfaces of ribbon, tabs, panels and
/// panel headers, fields, selected items, borderless buttons at rest.
/// Every colour in it is one of `tokens`.
[[nodiscard]] QString styleSheet(const ThemeTokens& tokens);

/// Fusion with what the style sheet does not control well (docs/adr/0039,
/// point 4): thin scroll bars without arrows, flat tabs, the combo boxes
/// (a field with a chevron), the buttons of the QDockWidget headers, and
/// rows of 22 px in the item views (point 3). Colours come from the tokens
/// it is built with; applyTheme installs a new Style when the theme
/// changes.
class Style final : public QProxyStyle {
    Q_OBJECT

public:
    /// `parent` owns the style until QApplication::setStyle takes it, as
    /// Qt does with the styles it creates.
    Style(const ThemeTokens& tokens, QObject* parent);

    [[nodiscard]] const ThemeTokens& tokens() const;

    [[nodiscard]] int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                                  const QWidget* widget = nullptr) const override;
    [[nodiscard]] QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                                         const QSize& size, const QWidget* widget) const override;
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget = nullptr) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
                            QPainter* painter, const QWidget* widget = nullptr) const override;
    [[nodiscard]] QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option,
                                       SubControl subControl,
                                       const QWidget* widget = nullptr) const override;
    [[nodiscard]] QIcon standardIcon(StandardPixmap icon, const QStyleOption* option = nullptr,
                                     const QWidget* widget = nullptr) const override;

private:
    ThemeTokens m_tokens;
};

/// The Style that applyTheme installed, or nullptr if there is none.
[[nodiscard]] const Style* installedStyle();

}  // namespace tg::app
