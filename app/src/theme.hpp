// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/render/scene_renderer.hpp"

#include <QColor>
#include <QFont>
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

    friend bool operator==(const ThemeTokens&, const ThemeTokens&) = default;
};

[[nodiscard]] const ThemeTokens& themeTokens(Theme theme);

/// The font of the interface (docs/adr/0039, point 1, and
/// docs/tappe/fuori-tappa/F6-stato.md): Segoe UI, else Noto Sans, else
/// DejaVu Sans, else the platform's sans serif, at 9 points, the 12 px of the
/// reference at 100%. It does not follow the desktop's font, which on GNOME
/// is Cantarell at 11 points, so that the window looks the same everywhere.
[[nodiscard]] QFont interfaceFont();
/// The fixed-pitch font of the command line, at the size of interfaceFont():
/// Cascadia Mono, else DejaVu Sans Mono, else the platform's monospace.
[[nodiscard]] QFont fixedFont();

/// Installs interfaceFont(), the proxy style over Fusion (style.hpp), a palette built from
/// themeTokens(theme) and the style sheet generated from the same tokens.
/// The result is the same on every platform: nothing comes from the OS.
void applyTheme(QApplication& application, Theme theme);
/// The colours of the drawing: only the background follows the tokens
/// (ThemeTokens::canvas); entities, grid and markers keep their own colours.
[[nodiscard]] render::SceneStyle sceneStyle(Theme theme);
/// The theme that `palette` belongs to: Dark when its window is darker than
/// its text. For widgets that only know their palette (panels, models).
[[nodiscard]] Theme themeOf(const QPalette& palette);

/// The icon `name` (docs/adr/0040): the -16 and -32 files,
/// each role coloured by the tokens of `theme`.
///
/// `name` is a resource name under :/tg/icons/ or a path without extension,
/// and then <name>-16.svg draws the sizes up to 24 px and <name>-32.svg the
/// larger ones, either one standing in for the other when it is missing.
/// No unsuffixed fallback is searched; a path ending in .svg is that file only.
/// Every element whose fill or stroke is currentColor gets the colour of its
/// role: its class geo, pt, cost or esist, else that of its nearest group
/// with one, else esist; the old class accent is pt (docs/ICONE.md, "Come si
/// colorano"). Unknown or unreadable images return an empty icon.
/// Resource icons are calculated once per theme and reused.
[[nodiscard]] QIcon themedIcon(const QString& name, Theme theme);

/// The logo of the program, the same in every theme: the simple drawing of
/// resources/logo at 16 and 32 px, where the full one could not be read, and
/// the full one from 48 px up.
[[nodiscard]] QIcon programIcon();

}  // namespace tg::app
