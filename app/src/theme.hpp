// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/render/scene_renderer.hpp"

#include <QIcon>
#include <QPalette>
#include <QString>

#include <cstdint>

class QApplication;

namespace tg::app {

enum class Theme : std::uint8_t { Light, Dark };

/// Fusion with a platform-independent palette.
void applyTheme(QApplication& application, Theme theme);
[[nodiscard]] render::SceneStyle sceneStyle(Theme theme);
/// Resource name under :/tg/icons/, or an explicit resource/file path.
/// Unknown or unreadable images return an empty icon.
[[nodiscard]] QIcon themedIcon(const QString& name, const QPalette& palette);

}  // namespace tg::app
