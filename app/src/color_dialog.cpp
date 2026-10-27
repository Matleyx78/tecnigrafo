// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "color_dialog.hpp"

#include "property_widgets.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace tg::app {

ColorDialog::ColorDialog(core::Color initial, Allow allow, QWidget* parent)
    : QDialog(parent),
      m_color(initial),
      m_tabs(new QTabWidget(this)),
      m_index(new QSpinBox(this)),
      m_preview(new QLabel(this)),
      m_nearest(new QLabel(this)) {
    setWindowTitle(tr("Select Colour"));
    const auto kind = initial.kind();
    const bool inherited = kind == core::Color::Kind::ByLayer || kind == core::Color::Kind::ByBlock;
    if (allow == Allow::IndexAndRgb && inherited) {
        m_color = core::Color::fromIndex(7);
    }
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_tabs);
    auto* indexed = new QWidget(m_tabs);
    auto* grid = new QGridLayout(indexed);
    // The 24 hues are columns; each column has five brightness pairs.
    for (int index = 1; index <= 255; ++index) {
        auto* button = new QPushButton(indexed);
        button->setObjectName(QStringLiteral("aci%1").arg(index));
        button->setAccessibleName(tr("Colour index %1").arg(index));
        button->setToolTip(QString::number(index));
        button->setIcon(colorIcon(core::Color::fromIndex(index), palette()));
        button->setFixedSize(22, 22);
        const int row = index >= 10 && index <= 249 ? (index - 10) % 10 : 10;
        int column = index - 250 + 9;
        if (index >= 10 && index <= 249) {
            column = (index - 10) / 10;
        } else if (index < 10) {
            column = index - 1;
        }
        grid->addWidget(button, row, column);
        connect(button, &QPushButton::clicked, this, [this, index] {
            m_index->setValue(index);
            m_color = core::Color::fromIndex(index);
        });
    }
    m_index->setObjectName(QStringLiteral("colourIndex"));
    m_index->setRange(1, 255);
    m_index->setPrefix(tr("Index: "));
    m_index->setValue(m_color.kind() == core::Color::Kind::Index ? m_color.index() : 7);
    grid->addWidget(m_index, 11, 0, 1, 12);
    connect(m_index, &QSpinBox::valueChanged, this,
            [this](int index) { m_color = core::Color::fromIndex(index); });
    if (allow == Allow::All) {
        auto* byLayer = new QPushButton(tr("ByLayer"), indexed);
        auto* byBlock = new QPushButton(tr("ByBlock"), indexed);
        byLayer->setObjectName(QStringLiteral("byLayer"));
        byBlock->setObjectName(QStringLiteral("byBlock"));
        grid->addWidget(byLayer, 12, 0, 1, 12);
        grid->addWidget(byBlock, 12, 12, 1, 12);
        connect(byLayer, &QPushButton::clicked, this, [this] { m_color = core::Color::byLayer(); });
        connect(byBlock, &QPushButton::clicked, this, [this] { m_color = core::Color::byBlock(); });
    }
    m_tabs->addTab(indexed, tr("Index"));
    auto* rgbPage = new QWidget(m_tabs);
    auto* form = new QFormLayout(rgbPage);
    auto rgb = core::aciToRgb(7);
    if (m_color.kind() == core::Color::Kind::Rgb) {
        rgb = m_color.rgb();
    } else if (m_color.kind() == core::Color::Kind::Index) {
        rgb = core::aciToRgb(m_color.index());
    }
    const std::array<int, 3> channels{rgb.r, rgb.g, rgb.b};
    const std::array<QString, 3> names{tr("Red"), tr("Green"), tr("Blue")};
    for (std::size_t channel = 0; channel < m_rgb.size(); ++channel) {
        auto* spin = new QSpinBox(rgbPage);
        m_rgb.at(channel) = spin;
        spin->setObjectName(QStringLiteral("rgb%1").arg(channel));
        spin->setRange(0, 255);
        spin->setValue(channels.at(channel));
        form->addRow(names.at(channel), spin);
        connect(spin, &QSpinBox::valueChanged, this, [this] { updateRgb(); });
    }
    m_preview->setObjectName(QStringLiteral("colourPreview"));
    m_nearest->setObjectName(QStringLiteral("nearestAci"));
    form->addRow(tr("Preview"), m_preview);
    form->addRow(tr("Nearest index"), m_nearest);
    m_tabs->addTab(rgbPage, tr("RGB"));
    updateRgb();
    m_color = initial;
    if (allow == Allow::IndexAndRgb && inherited) {
        m_color = core::Color::fromIndex(7);
    }
    m_tabs->setCurrentIndex(initial.kind() == core::Color::Kind::Rgb ? 1 : 0);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int tab) {
        if (tab == 1) {
            updateRgb();
        } else {
            m_color = core::Color::fromIndex(m_index->value());
        }
    });
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

core::Color ColorDialog::color() const {
    return m_color;
}

void ColorDialog::updateRgb() {
    const core::Rgb rgb{static_cast<std::uint8_t>(m_rgb.at(0)->value()),
                       static_cast<std::uint8_t>(m_rgb.at(1)->value()),
                       static_cast<std::uint8_t>(m_rgb.at(2)->value())};
    m_color = core::Color::fromRgb(rgb);
    m_preview->setPixmap(colorIcon(m_color, palette()).pixmap(64, 24));
    m_nearest->setText(QString::number(core::nearestAci(rgb)));
}

}  // namespace tg::app
