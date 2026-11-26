// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "settings_dialog.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace tg::app {

SettingsDialog::SettingsDialog(InterfaceKind interface, Theme theme, bool snapMagnet,
                               render::AxesStyle axes, QWidget* parent)
    : QDialog(parent),
      m_interface(new QComboBox(this)),
      m_theme(new QComboBox(this)),
      m_snapMagnet(new QComboBox(this)),
      m_axes(new QComboBox(this)) {
    setObjectName(QStringLiteral("settingsDialog"));
    setWindowTitle(tr("Settings"));
    m_interface->setObjectName(QStringLiteral("interfaceCombo"));
    m_interface->addItems({tr("Ribbon"), tr("Classic")});
    m_interface->setCurrentIndex(interface == InterfaceKind::Ribbon ? 0 : 1);
    m_theme->setObjectName(QStringLiteral("themeCombo"));
    m_theme->addItems({tr("Light"), tr("Dark")});
    m_theme->setCurrentIndex(theme == Theme::Light ? 0 : 1);
    auto* form = new QFormLayout;
    form->addRow(tr("Interface"), m_interface);
    m_snapMagnet->setObjectName(QStringLiteral("snapMagnetCombo"));
    m_snapMagnet->addItems({tr("On"), tr("Off")});
    m_snapMagnet->setCurrentIndex(snapMagnet ? 0 : 1);
    m_axes->setObjectName(QStringLiteral("axesCombo"));
    m_axes->addItems({tr("UCS icon"), tr("Lines"), tr("None")});
    m_axes->setCurrentIndex(static_cast<int>(axes));
    form->addRow(tr("Theme"), m_theme);
    form->addRow(tr("Object snap magnet"), m_snapMagnet);
    form->addRow(tr("Axes"), m_axes);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

InterfaceKind SettingsDialog::interfaceKind() const {
    return m_interface->currentIndex() == 0 ? InterfaceKind::Ribbon : InterfaceKind::Classic;
}

Theme SettingsDialog::theme() const {
    return m_theme->currentIndex() == 0 ? Theme::Light : Theme::Dark;
}

bool SettingsDialog::snapMagnet() const {
    return m_snapMagnet->currentIndex() == 0;
}

render::AxesStyle SettingsDialog::axesStyle() const {
    return static_cast<render::AxesStyle>(m_axes->currentIndex());
}

}  // namespace tg::app
