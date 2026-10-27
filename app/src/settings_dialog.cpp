// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "settings_dialog.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace tg::app {

SettingsDialog::SettingsDialog(InterfaceKind interface, Theme theme, QWidget* parent)
    : QDialog(parent), m_interface(new QComboBox(this)), m_theme(new QComboBox(this)) {
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
    form->addRow(tr("Theme"), m_theme);
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

}  // namespace tg::app
