// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/color.hpp"

#include <QDialog>

#include <array>
#include <cstdint>

class QLabel;
class QSpinBox;
class QTabWidget;

namespace tg::app {

class ColorDialog final : public QDialog {
    Q_OBJECT
public:
    enum class Allow : std::uint8_t { All, IndexAndRgb };
    ColorDialog(core::Color initial, Allow allow, QWidget* parent = nullptr);
    [[nodiscard]] core::Color color() const;

private:
    void updateRgb();
    core::Color m_color;
    QTabWidget* m_tabs;
    QSpinBox* m_index;
    std::array<QSpinBox*, 3> m_rgb{};
    QLabel* m_preview;
    QLabel* m_nearest;
};

}  // namespace tg::app
