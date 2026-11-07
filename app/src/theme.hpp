// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/render/scene_renderer.hpp"

#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QString>

#include <cstdint>

class QApplication;

namespace tg::app {

enum class Theme : std::uint8_t { Light, Dark };

/// The colours of the interface, one set per theme (docs/adr/0039, point 2).
/// No other file of app writes a colour of the interface: the palette, the
/// style sheet, the proxy style and the icons take them from here.
struct ThemeTokens {
    QColor chrome, ribbon, group, tabs, panel, field, canvas;  ///< surfaces
    QColor border, text, textSecondary, accent, selection;
    QColor iconGeo, iconPt, iconCost, iconEsist;  ///< docs/adr/0040, point 1
};

[[nodiscard]] const ThemeTokens& themeTokens(Theme theme);

/// Installs the proxy style over Fusion (style.hpp), a palette built from
/// themeTokens(theme) and the style sheet generated from the same tokens.
/// The result is the same on every platform: nothing comes from the OS.
void applyTheme(QApplication& application, Theme theme);
/// The colours of the drawing: only the background follows the tokens
/// (ThemeTokens::canvas); entities, grid and markers keep their own colours.
[[nodiscard]] render::SceneStyle sceneStyle(Theme theme);
/// Resource name under :/tg/icons/, or an explicit resource/file path.
/// Unknown or unreadable images return an empty icon.
[[nodiscard]] QIcon themedIcon(const QString& name, const QPalette& palette);

}  // namespace tg::app
