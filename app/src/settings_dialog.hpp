// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "theme.hpp"
#include "ui_builder.hpp"

#include <QDialog>

class QComboBox;

namespace tg::app {

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(InterfaceKind interface, Theme theme, QWidget* parent = nullptr);
    [[nodiscard]] InterfaceKind interfaceKind() const;
    [[nodiscard]] Theme theme() const;

private:
    QComboBox* m_interface;
    QComboBox* m_theme;
};

}  // namespace tg::app
