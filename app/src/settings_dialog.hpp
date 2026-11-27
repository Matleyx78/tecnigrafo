// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/render/scene_renderer.hpp"
#include "theme.hpp"
#include "ui_builder.hpp"

#include <QDialog>

class QComboBox;

namespace tg::app {

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(InterfaceKind interface, Theme theme, bool snapMagnet, render::AxesStyle axes,
                   QWidget* parent = nullptr);
    [[nodiscard]] InterfaceKind interfaceKind() const;
    [[nodiscard]] Theme theme() const;
    [[nodiscard]] bool snapMagnet() const;
    [[nodiscard]] render::AxesStyle axesStyle() const;

private:
    QComboBox* m_interface;
    QComboBox* m_theme;
    QComboBox* m_snapMagnet;
    QComboBox* m_axes;
};

}  // namespace tg::app
