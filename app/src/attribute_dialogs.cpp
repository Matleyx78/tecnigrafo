// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "attribute_dialogs.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

namespace tg::app {
namespace {

QDialogButtonBox* buttons(QDialog& dialog) {
    auto* result = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(result, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(result, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    return result;
}

QLineEdit* field(QFormLayout& layout, const QString& label, const std::string& value,
                 const QString& name) {
    auto* edit = new QLineEdit(QString::fromStdString(value));
    edit->setObjectName(name);
    layout.addRow(label, edit);
    return edit;
}

}  // namespace

AttributeDefinitionDialog::AttributeDefinitionDialog(const core::AttributeDefinition& definition,
                                                     QWidget* parent)
    : QDialog(parent), m_definition(definition) {
    setObjectName(QStringLiteral("attributeDefinitionDialog"));
    setWindowTitle(tr("Edit Attribute Definition"));
    auto* layout = new QFormLayout(this);
    m_tag = field(*layout, tr("Tag"), definition.tag, QStringLiteral("attributeTag"));
    m_prompt = field(*layout, tr("Prompt"), definition.prompt, QStringLiteral("attributePrompt"));
    m_value =
        field(*layout, tr("Default"), definition.text.value, QStringLiteral("attributeDefault"));
    QDialogButtonBox* controls = buttons(*this);
    QPushButton* ok = controls->button(QDialogButtonBox::Ok);
    const auto validate = [this, ok] {
        ok->setEnabled(core::validAttributeTag(m_tag->text().toStdString()));
    };
    connect(m_tag, &QLineEdit::textChanged, this, validate);
    validate();
    layout->addRow(controls);
    m_tag->setFocus();
}

core::AttributeDefinition AttributeDefinitionDialog::definition() const {
    core::AttributeDefinition result = m_definition;
    result.tag = m_tag->text().toStdString();
    result.prompt = m_prompt->text().toStdString();
    result.text.value = m_value->text().toStdString();
    return result;
}

InsertAttributesDialog::InsertAttributesDialog(const core::Insert& insert, QWidget* parent)
    : QDialog(parent) {
    setObjectName(QStringLiteral("insertAttributesDialog"));
    setWindowTitle(tr("Edit Attributes"));
    auto* layout = new QFormLayout(this);
    m_values.reserve(insert.attributes.size());
    for (const core::Attribute& attribute : insert.attributes) {
        m_values.push_back(field(*layout, QString::fromStdString(attribute.tag),
                                 attribute.text.value,
                                 QStringLiteral("attributeValue%1").arg(m_values.size())));
    }
    layout->addRow(buttons(*this));
    if (!m_values.empty()) {
        m_values.front()->setFocus();
        m_values.front()->selectAll();
    }
}

std::vector<std::string> InsertAttributesDialog::values() const {
    std::vector<std::string> result;
    result.reserve(m_values.size());
    for (const QLineEdit* value : m_values) {
        result.push_back(value->text().toStdString());
    }
    return result;
}

}  // namespace tg::app
