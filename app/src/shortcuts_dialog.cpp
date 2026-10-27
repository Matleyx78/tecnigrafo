// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "shortcuts_dialog.hpp"

#include <QAction>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

namespace tg::app {

ShortcutsDialog::ShortcutsDialog(ActionRegistry& registry, QSettings& settings, QWidget* parent)
    : QDialog(parent), m_registry(registry), m_settings(settings), m_map(registry),
      m_table(new QTableWidget(this)), m_filter(new QLineEdit(this)),
      m_edit(new QKeySequenceEdit(this)), m_warning(new QLabel(this)) {
    setObjectName(QStringLiteral("shortcutsDialog"));
    setWindowTitle(tr("Keyboard Shortcuts"));
    resize(800, 500);
    // Start from the current session, including changes not saved yet.
    for (const QAction* action : registry.actions()) {
        m_original.insert(action->objectName(), action->shortcuts());
    }
    QString current;
    for (const QAction* action : registry.actions()) {
        const ActionSpec* spec = registry.spec(action->objectName());
        if (!spec->text.isEmpty() && action->shortcut() != spec->shortcut) {
            current += spec->id + QLatin1Char('\t') +
                       action->shortcut().toString(QKeySequence::PortableText) + QLatin1Char('\n');
        }
    }
    static_cast<void>(m_map.importText(current));
    m_filter->setObjectName(QStringLiteral("shortcutFilter"));
    m_filter->setPlaceholderText(tr("Search actions"));
    m_table->setObjectName(QStringLiteral("shortcutTable"));
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(
        {tr("Action"), tr("Description"), tr("Shortcut"), tr("Default")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_edit->setObjectName(QStringLiteral("shortcutEdit"));
    m_warning->setObjectName(QStringLiteral("shortcutWarning"));
    m_warning->setWordWrap(true);
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_filter);
    layout->addWidget(m_table);
    layout->addWidget(m_edit);
    layout->addWidget(m_warning);
    auto* operations = new QHBoxLayout;
    const auto button = [this, operations](const QString& text, const QString& name, auto callback) {
        auto* result = new QPushButton(text, this);
        result->setObjectName(name);
        operations->addWidget(result);
        connect(result, &QPushButton::clicked, this, callback);
    };
    button(tr("Remove"), QStringLiteral("removeShortcut"), [this] { change({}); });
    button(tr("Reset"), QStringLiteral("resetShortcut"), [this] {
        const ActionSpec* spec = m_registry.spec(selectedId());
        if (spec != nullptr) {
            change(spec->shortcut);
        }
    });
    button(tr("Reset All"), QStringLiteral("resetAllShortcuts"), [this] {
        m_map.resetAll();
        m_map.apply(m_registry);
        m_warning->clear();
        refresh();
    });
    button(tr("Export…"), QStringLiteral("exportShortcuts"), [this] { exportFile(); });
    button(tr("Import…"), QStringLiteral("importShortcuts"), [this] { importFile(); });
    layout->addLayout(operations);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ShortcutsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &ShortcutsDialog::reject);
    layout->addWidget(buttons);
    connect(m_filter, &QLineEdit::textChanged, this, [this] { refresh(); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] {
        const QSignalBlocker guard(m_edit);
        m_edit->setEnabled(!selectedId().isEmpty());
        m_edit->setKeySequence(m_map.shortcut(selectedId()));
    });
    connect(m_edit, &QKeySequenceEdit::editingFinished, this,
            [this] { change(m_edit->keySequence()); });
    refresh();
}

QString ShortcutsDialog::selectedId() const {
    const auto* item = m_table->item(m_table->currentRow(), 0);
    return item != nullptr ? item->data(Qt::UserRole).toString() : QString{};
}

void ShortcutsDialog::refresh() {
    const QString selected = selectedId();
    const QSignalBlocker guard(m_table);
    m_table->setRowCount(0);
    for (const QAction* action : m_registry.actions()) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        if (spec->text.isEmpty() ||
            !(spec->text + spec->description + spec->id)
                 .contains(m_filter->text(), Qt::CaseInsensitive)) {
            continue;
        }
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto* item = new QTableWidgetItem(spec->text);
        item->setData(Qt::UserRole, spec->id);
        m_table->setItem(row, 0, item);
        m_table->setItem(row, 1, new QTableWidgetItem(spec->description));
        m_table->setItem(row, 2, new QTableWidgetItem(m_map.shortcut(spec->id).toString()));
        m_table->setItem(row, 3, new QTableWidgetItem(spec->shortcut.toString()));
        if (spec->id == selected) {
            m_table->selectRow(row);
        }
    }
    const QSignalBlocker editGuard(m_edit);
    m_edit->setEnabled(!selectedId().isEmpty());
    m_edit->setKeySequence(m_map.shortcut(selectedId()));
}

void ShortcutsDialog::change(const QKeySequence& sequence) {
    const QString id = selectedId();
    const QString other = m_map.conflict(id, sequence);
    if (!other.isEmpty()) {
        m_warning->setText(tr("Shortcut already used by %1.").arg(m_registry.spec(other)->text));
        return;
    }
    if (m_map.set(id, sequence)) {
        m_map.apply(m_registry);
        m_warning->clear();
        refresh();
    }
}

void ShortcutsDialog::accept() {
    // Finish a pending edit even when OK is clicked before its timeout.
    if (!selectedId().isEmpty()) {
        change(m_edit->keySequence());
    }
    if (!m_warning->text().isEmpty()) {
        return;
    }
    m_map.save(m_settings);
    m_settings.sync();
    QDialog::accept();
}

void ShortcutsDialog::reject() {
    for (QAction* action : m_registry.actions()) {
        if (m_original.contains(action->objectName())) {
            action->setShortcuts(m_original.value(action->objectName()));
        }
    }
    QDialog::reject();
}

void ShortcutsDialog::exportFile() {
    const QString path = QFileDialog::getSaveFileName(this, tr("Export Shortcuts"));
    if (path.isEmpty()) {
        return;
    }
    QSaveFile file(path);
    const QByteArray data = m_map.exportText().toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        m_warning->setText(tr("Could not export shortcuts: %1").arg(file.errorString()));
    }
}

void ShortcutsDialog::importFile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Import Shortcuts"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_warning->setText(tr("Could not import shortcuts: %1").arg(file.errorString()));
        return;
    }
    const QStringList errors = m_map.importText(QString::fromUtf8(file.readAll()));
    m_warning->setText(errors.join(QLatin1Char('\n')));
    if (errors.isEmpty()) {
        m_map.apply(m_registry);
        refresh();
    }
}

}  // namespace tg::app
