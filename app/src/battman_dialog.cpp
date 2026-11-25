// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "battman_dialog.hpp"

#include "tg/core/document.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/edit/blocks.hpp"
#include "tg/edit/editor.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace tg::app {
namespace {

constexpr double kRadiansPerDegree = std::numbers::pi / 180.0;

struct DefinitionEntry {
    std::string tag;
    core::AttributeDefinition definition;
};

std::vector<DefinitionEntry> definitionsOf(const core::BlockDefinition& block) {
    std::vector<const core::EntityRecord*> records;
    records.reserve(block.entities.size());
    for (const core::EntityRecord& record : block.entities) {
        if (std::holds_alternative<core::AttributeDefinition>(record.entity)) {
            records.push_back(&record);
        }
    }
    std::ranges::sort(records, {}, &core::EntityRecord::order);
    std::vector<DefinitionEntry> result;
    result.reserve(records.size());
    for (const core::EntityRecord* record : records) {
        const auto& definition = std::get<core::AttributeDefinition>(record->entity);
        result.push_back({definition.tag, definition});
    }
    return result;
}

QString modesOf(const core::AttributeDefinition& definition) {
    QStringList modes;
    if (definition.flags.invisible) {
        modes.push_back(QObject::tr("Invisible"));
    }
    if (definition.flags.constant) {
        modes.push_back(QObject::tr("Constant"));
    }
    if (definition.flags.verify) {
        modes.push_back(QObject::tr("Verify"));
    }
    if (definition.flags.preset) {
        modes.push_back(QObject::tr("Preset"));
    }
    if (definition.lockPosition) {
        modes.push_back(QObject::tr("Lock position"));
    }
    return modes.join(QStringLiteral(", "));
}

class AttributeDefinitionDialog final : public QDialog {
public:
    explicit AttributeDefinitionDialog(core::AttributeDefinition definition, QWidget* parent)
        : QDialog(parent),
          m_definition(std::move(definition)),
          m_height(new QDoubleSpinBox(this)),
          m_rotation(new QDoubleSpinBox(this)) {
        setWindowTitle(tr("Edit Attribute Definition"));
        auto* form = new QFormLayout;
        m_tag = new QLineEdit(QString::fromStdString(m_definition.tag), this);
        m_tag->setObjectName(QStringLiteral("battmanEditTag"));
        m_prompt = new QLineEdit(QString::fromStdString(m_definition.prompt), this);
        m_prompt->setObjectName(QStringLiteral("battmanEditPrompt"));
        m_default = new QLineEdit(QString::fromStdString(m_definition.text.value), this);
        m_default->setObjectName(QStringLiteral("battmanEditDefault"));
        m_height->setObjectName(QStringLiteral("battmanEditHeight"));
        m_height->setRange(0.000001, 1.0e12);
        m_height->setDecimals(6);
        m_height->setValue(m_definition.text.height);
        m_rotation->setObjectName(QStringLiteral("battmanEditRotation"));
        m_rotation->setRange(-360.0, 360.0);
        m_rotation->setDecimals(6);
        m_rotation->setValue(m_definition.text.rotation / kRadiansPerDegree);
        m_style = new QLineEdit(QString::fromStdString(m_definition.text.style), this);
        m_style->setObjectName(QStringLiteral("battmanEditStyle"));
        form->addRow(tr("Tag:"), m_tag);
        form->addRow(tr("Prompt:"), m_prompt);
        form->addRow(tr("Default:"), m_default);

        auto addMode = [this, form](const QString& text, bool checked, QCheckBox*& field) {
            field = new QCheckBox(text, this);
            field->setChecked(checked);
            form->addRow(field);
        };
        addMode(tr("Invisible"), m_definition.flags.invisible, m_invisible);
        addMode(tr("Constant"), m_definition.flags.constant, m_constant);
        addMode(tr("Verify"), m_definition.flags.verify, m_verify);
        addMode(tr("Preset"), m_definition.flags.preset, m_preset);
        addMode(tr("Lock position"), m_definition.lockPosition, m_locked);
        form->addRow(tr("Height:"), m_height);
        form->addRow(tr("Rotation:"), m_rotation);
        form->addRow(tr("Style:"), m_style);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->setObjectName(QStringLiteral("battmanEditButtons"));
        auto* layout = new QVBoxLayout(this);
        layout->addLayout(form);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    [[nodiscard]] core::AttributeDefinition definition() const {
        core::AttributeDefinition result = m_definition;
        result.tag = m_tag->text().toStdString();
        result.prompt = m_prompt->text().toStdString();
        result.text.value = m_default->text().toStdString();
        result.flags = {m_invisible->isChecked(), m_constant->isChecked(), m_verify->isChecked(),
                        m_preset->isChecked()};
        result.lockPosition = m_locked->isChecked();
        result.text.height = m_height->value();
        result.text.rotation = m_rotation->value() * kRadiansPerDegree;
        result.text.style = m_style->text().toStdString();
        return result;
    }

private:
    core::AttributeDefinition m_definition;
    QLineEdit* m_tag{nullptr};
    QLineEdit* m_prompt{nullptr};
    QLineEdit* m_default{nullptr};
    QCheckBox* m_invisible{nullptr};
    QCheckBox* m_constant{nullptr};
    QCheckBox* m_verify{nullptr};
    QCheckBox* m_preset{nullptr};
    QCheckBox* m_locked{nullptr};
    QDoubleSpinBox* m_height;
    QDoubleSpinBox* m_rotation;
    QLineEdit* m_style{nullptr};
};

}  // namespace

BattmanDialog::BattmanDialog(edit::Editor& editor, const std::string& selectedBlock,
                             QWidget* parent)
    : QDialog(parent),
      m_editor(editor),
      m_blocks(new QComboBox(this)),
      m_table(new QTableWidget(this)),
      m_up(new QPushButton(tr("Up"), this)),
      m_down(new QPushButton(tr("Down"), this)),
      m_edit(new QPushButton(tr("Edit..."), this)),
      m_remove(new QPushButton(tr("Remove"), this)),
      m_synchronize(new QPushButton(tr("Synchronize"), this)),
      m_confirm([this](const QString& text) {
          return QMessageBox::question(this, tr("Block Attribute Manager"), text,
                                       QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                       QMessageBox::Yes);
      }) {
    setObjectName(QStringLiteral("battmanDialog"));
    setWindowTitle(tr("Block Attribute Manager"));
    m_blocks->setObjectName(QStringLiteral("battmanBlocks"));
    m_table->setObjectName(QStringLiteral("battmanDefinitions"));
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Tag"), tr("Prompt"), tr("Default"), tr("Modes")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_up->setObjectName(QStringLiteral("battmanUp"));
    m_down->setObjectName(QStringLiteral("battmanDown"));
    m_edit->setObjectName(QStringLiteral("battmanEdit"));
    m_remove->setObjectName(QStringLiteral("battmanRemove"));
    m_synchronize->setObjectName(QStringLiteral("battmanSynchronize"));

    auto* actions = new QHBoxLayout;
    actions->addWidget(m_up);
    actions->addWidget(m_down);
    actions->addWidget(m_edit);
    actions->addWidget(m_remove);
    actions->addWidget(m_synchronize);
    actions->addStretch();
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    buttons->setObjectName(QStringLiteral("battmanButtons"));
    m_apply = buttons->button(QDialogButtonBox::Apply);
    m_apply->setObjectName(QStringLiteral("battmanApply"));
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Block:"), this));
    layout->addWidget(m_blocks);
    layout->addWidget(m_table);
    layout->addLayout(actions);
    layout->addWidget(buttons);

    for (const core::BlockDefinition& block : editor.document().blocks().definitions()) {
        if (!definitionsOf(block).empty()) {
            m_blocks->addItem(QString::fromStdString(block.name));
        }
    }
    const int selected =
        m_blocks->findText(QString::fromStdString(selectedBlock), Qt::MatchFixedString);
    if (selected >= 0) {
        m_blocks->setCurrentIndex(selected);
    }
    connect(m_blocks, &QComboBox::currentIndexChanged, this, &BattmanDialog::refresh);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &BattmanDialog::updateButtons);
    connect(m_up, &QPushButton::clicked, this, &BattmanDialog::moveUp);
    connect(m_down, &QPushButton::clicked, this, &BattmanDialog::moveDown);
    connect(m_edit, &QPushButton::clicked, this, &BattmanDialog::editSelected);
    connect(m_remove, &QPushButton::clicked, this, &BattmanDialog::removeSelected);
    connect(m_synchronize, &QPushButton::clicked, this, &BattmanDialog::synchronize);
    connect(m_apply, &QPushButton::clicked, this, &BattmanDialog::synchronize);
    connect(buttons, &QDialogButtonBox::accepted, this, &BattmanDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &BattmanDialog::reject);
    refresh();
}

std::string BattmanDialog::blockName() const {
    return m_blocks->currentText().toStdString();
}

void BattmanDialog::refresh() {
    const core::BlockDefinition* block = m_editor.document().blocks().find(blockName());
    const std::vector<DefinitionEntry> definitions =
        block != nullptr ? definitionsOf(*block) : std::vector<DefinitionEntry>{};
    const int oldRow = m_table->currentRow();
    m_table->setRowCount(static_cast<int>(definitions.size()));
    for (int row = 0; row < static_cast<int>(definitions.size()); ++row) {
        const core::AttributeDefinition& definition =
            definitions[static_cast<std::size_t>(row)].definition;
        const std::array<QString, 4> cells{
            QString::fromStdString(definition.tag), QString::fromStdString(definition.prompt),
            QString::fromStdString(definition.text.value), modesOf(definition)};
        for (int column = 0; column < static_cast<int>(cells.size()); ++column) {
            m_table->setItem(row, column,
                             new QTableWidgetItem(cells.at(static_cast<std::size_t>(column))));
        }
    }
    if (!definitions.empty()) {
        m_table->selectRow(std::clamp(oldRow, 0, static_cast<int>(definitions.size()) - 1));
    }
    updateButtons();
}

core::AttributeDefinition* BattmanDialog::selectedDefinition(
    core::AttributeDefinition& storage) const {
    const core::BlockDefinition* block = m_editor.document().blocks().find(blockName());
    if (block == nullptr || m_table->currentRow() < 0) {
        return nullptr;
    }
    const auto definitions = definitionsOf(*block);
    const auto row = static_cast<std::size_t>(m_table->currentRow());
    if (row >= definitions.size()) {
        return nullptr;
    }
    storage = definitions[row].definition;
    return &storage;
}

void BattmanDialog::transact(const std::function<bool()>& operation) {
    if (m_editor.inTransaction()) {
        return;
    }
    m_editor.beginTransaction("Battman");
    if (operation()) {
        m_editor.commit();
        m_unsynchronized = true;
    } else {
        m_editor.rollback();
    }
    refresh();
}

void BattmanDialog::moveUp() {
    const int row = m_table->currentRow();
    if (row <= 0) {
        return;
    }
    const core::BlockDefinition* block = m_editor.document().blocks().find(blockName());
    if (block == nullptr) {
        return;
    }
    auto definitions = definitionsOf(*block);
    std::swap(definitions[static_cast<std::size_t>(row - 1)],
              definitions[static_cast<std::size_t>(row)]);
    std::vector<std::string> tags;
    tags.reserve(definitions.size());
    for (const DefinitionEntry& definition : definitions) {
        tags.push_back(definition.tag);
    }
    transact([this, tags = std::move(tags)] {
        return edit::reorderAttributeDefinitions(m_editor, blockName(), tags);
    });
    m_table->selectRow(row - 1);
}

void BattmanDialog::moveDown() {
    const int row = m_table->currentRow();
    if (row < 0 || row + 1 >= m_table->rowCount()) {
        return;
    }
    const core::BlockDefinition* block = m_editor.document().blocks().find(blockName());
    if (block == nullptr) {
        return;
    }
    auto definitions = definitionsOf(*block);
    const auto index = static_cast<std::size_t>(row);
    std::swap(definitions[index], definitions[index + 1]);
    std::vector<std::string> tags;
    tags.reserve(definitions.size());
    for (const DefinitionEntry& definition : definitions) {
        tags.push_back(definition.tag);
    }
    transact([this, tags = std::move(tags)] {
        return edit::reorderAttributeDefinitions(m_editor, blockName(), tags);
    });
    m_table->selectRow(row + 1);
}

bool BattmanDialog::applyDefinition(const core::AttributeDefinition& definition) {
    core::AttributeDefinition old;
    if (selectedDefinition(old) == nullptr || !core::supported(definition)) {
        return false;
    }
    bool changed = false;
    transact([this, &old, &definition, &changed] {
        changed = edit::replaceAttributeDefinition(m_editor, blockName(), old.tag, definition);
        return changed;
    });
    return changed;
}

void BattmanDialog::editSelected() {
    core::AttributeDefinition definition;
    if (selectedDefinition(definition) == nullptr) {
        return;
    }
    AttributeDefinitionDialog dialog(definition, this);
    if (dialog.exec() == QDialog::Accepted) {
        static_cast<void>(applyDefinition(dialog.definition()));
    }
}

void BattmanDialog::removeSelected() {
    core::AttributeDefinition definition;
    if (selectedDefinition(definition) == nullptr) {
        return;
    }
    transact([this, tag = definition.tag] {
        return edit::removeAttributeDefinition(m_editor, blockName(), tag);
    });
}

void BattmanDialog::synchronize() {
    if (m_editor.inTransaction() || m_blocks->currentIndex() < 0) {
        return;
    }
    m_editor.beginTransaction("Battman");
    static_cast<void>(edit::synchronizeInserts(m_editor, blockName()));
    m_editor.commit();
    m_unsynchronized = false;
    updateButtons();
}

bool BattmanDialog::confirmClose() {
    if (!m_unsynchronized) {
        return true;
    }
    const QMessageBox::StandardButton answer =
        m_confirm(tr("Attribute definitions changed. Synchronize existing inserts?"));
    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Yes) {
        synchronize();
    }
    return true;
}

void BattmanDialog::accept() {
    synchronize();
    m_closing = true;
    QDialog::accept();
}

void BattmanDialog::reject() {
    if (!m_closing && !confirmClose()) {
        return;
    }
    m_closing = true;
    QDialog::reject();
}

void BattmanDialog::updateButtons() {
    const int row = m_table->currentRow();
    m_up->setEnabled(row > 0);
    m_down->setEnabled(row >= 0 && row + 1 < m_table->rowCount());
    m_edit->setEnabled(row >= 0);
    m_remove->setEnabled(row >= 0);
    m_synchronize->setEnabled(m_blocks->currentIndex() >= 0);
    m_apply->setEnabled(m_blocks->currentIndex() >= 0);
}

}  // namespace tg::app
