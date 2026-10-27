// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "shortcut_map.hpp"

#include <QDialog>

class QLabel;
class QKeySequenceEdit;
class QLineEdit;
class QTableWidget;

namespace tg::app {

class ShortcutsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutsDialog(ActionRegistry& registry, QSettings& settings,
                             QWidget* parent = nullptr);
    void accept() override;
    void reject() override;

private:
    [[nodiscard]] QString selectedId() const;
    void refresh();
    void change(const QKeySequence& sequence);
    void exportFile();
    void importFile();

    ActionRegistry& m_registry;
    QSettings& m_settings;
    ShortcutMap m_map;
    QHash<QString, QList<QKeySequence>> m_original;
    QTableWidget* m_table;
    QLineEdit* m_filter;
    QKeySequenceEdit* m_edit;
    QLabel* m_warning;
};

}  // namespace tg::app
