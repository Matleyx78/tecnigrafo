// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/attribute_definition.hpp"

#include <QDialog>
#include <QMessageBox>

#include <functional>
#include <string>
#include <utility>

class QComboBox;
class QPushButton;
class QTableWidget;

namespace tg::edit {
class Editor;
}

namespace tg::app {

/// The BATTMAN window. Definition changes are committed immediately as
/// separate "Battman" undo steps; Apply and OK synchronize existing inserts.
class BattmanDialog final : public QDialog {
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(BattmanDialog)

    using Confirm = std::function<QMessageBox::StandardButton(const QString&)>;

    explicit BattmanDialog(edit::Editor& editor, const std::string& selectedBlock = {},
                           QWidget* parent = nullptr);
    ~BattmanDialog() override = default;

    void setConfirm(Confirm confirm) { m_confirm = std::move(confirm); }

    [[nodiscard]] std::string blockName() const;

    [[nodiscard]] bool hasUnsynchronizedChanges() const { return m_unsynchronized; }

    void refresh();
    void moveUp();
    void moveDown();
    void editSelected();
    void removeSelected();
    void synchronize();
    void accept() override;
    void reject() override;

private:
    [[nodiscard]] core::AttributeDefinition* selectedDefinition(
        core::AttributeDefinition& storage) const;
    [[nodiscard]] bool applyDefinition(const core::AttributeDefinition& definition);
    [[nodiscard]] bool confirmClose();
    void transact(const std::function<bool()>& operation);
    void updateButtons();

    edit::Editor& m_editor;
    QComboBox* m_blocks;
    QTableWidget* m_table;
    QPushButton* m_up;
    QPushButton* m_down;
    QPushButton* m_edit;
    QPushButton* m_remove;
    QPushButton* m_synchronize;
    QPushButton* m_apply{nullptr};
    bool m_unsynchronized{false};
    bool m_closing{false};
    Confirm m_confirm;
};

}  // namespace tg::app
