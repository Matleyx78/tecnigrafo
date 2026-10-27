// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "quick_select_dialog.hpp"

#include "property_widgets.hpp"

#include <QChar>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace tg::app {
namespace {

using Property = cmd::QuickSelectCriteria::Property;
using Operator = cmd::QuickSelectCriteria::Operator;

// The sign of the operator "not equal".
constexpr char16_t kNotEqualSign = u'\u2260';

// The pages of the value controls, in the order they are added.
enum class ValuePage : std::uint8_t { Color, Layer, Linetype, Lineweight, None };

struct TypeChoice {
    const char* dxf;
    const char* name;
};

// The handled DXF types (cmd::quickSelect), in the order of core::Entity.
constexpr std::array<TypeChoice, 5> kTypes{{{"LWPOLYLINE", QT_TR_NOOP("Polyline")},
                                            {"LINE", QT_TR_NOOP("Line")},
                                            {"CIRCLE", QT_TR_NOOP("Circle")},
                                            {"ARC", QT_TR_NOOP("Arc")},
                                            {"TEXT", QT_TR_NOOP("Text")}}};

int toInt(Property property) {
    return static_cast<int>(property);
}

int toInt(Operator op) {
    return static_cast<int>(op);
}

ValuePage pageOf(Property property) {
    switch (property) {
        case Property::Color:
            return ValuePage::Color;
        case Property::Layer:
            return ValuePage::Layer;
        case Property::Linetype:
            return ValuePage::Linetype;
        case Property::Lineweight:
            return ValuePage::Lineweight;
        case Property::None:
            break;
    }
    return ValuePage::None;
}

}  // namespace

QuickSelectDialog::QuickSelectDialog(const core::Document& document,
                                     std::vector<core::EntityId> current, QWidget* parent)
    : QDialog(parent),
      m_document(document),
      m_current(std::move(current)),
      m_where(new QComboBox(this)),
      m_type(new QComboBox(this)),
      m_property(new QComboBox(this)),
      m_operator(new QComboBox(this)),
      m_values(new QStackedWidget(this)),
      m_color(new ColorCombo(this)),
      m_layer(new LayerCombo(this)),
      m_linetype(new LinetypeCombo(this)),
      m_lineweight(new LineweightCombo(this)),
      m_include(new QRadioButton(tr("&Include in new selection set"), this)),
      m_exclude(new QRadioButton(tr("&Exclude from new selection set"), this)),
      m_colorValue(document.current().color),
      m_layerValue(document.current().layer),
      m_linetypeValue(document.current().linetype),
      m_lineweightValue(document.current().lineweight) {
    setObjectName(QStringLiteral("quickSelectDialog"));
    setWindowTitle(tr("Quick Select"));

    m_where->setObjectName(QStringLiteral("quickSelectWhere"));
    m_where->addItem(tr("Entire drawing"), false);
    if (!m_current.empty()) {
        m_where->addItem(tr("Current selection"), true);
        m_where->setCurrentIndex(1);
    }

    m_type->setObjectName(QStringLiteral("quickSelectType"));
    m_type->addItem(tr("Multiple"), QString());
    for (const TypeChoice& type : kTypes) {
        m_type->addItem(tr(type.name), QString::fromLatin1(type.dxf));
    }

    m_property->setObjectName(QStringLiteral("quickSelectProperty"));
    m_property->addItem(tr("Color"), toInt(Property::Color));
    m_property->addItem(tr("Layer"), toInt(Property::Layer));
    m_property->addItem(tr("Linetype"), toInt(Property::Linetype));
    m_property->addItem(tr("Lineweight"), toInt(Property::Lineweight));
    m_property->addItem(tr("(None)"), toInt(Property::None));

    m_operator->setObjectName(QStringLiteral("quickSelectOperator"));
    m_operator->addItem(tr("= Equals"), toInt(Operator::Equal));
    m_operator->addItem(tr("%1 Not Equal").arg(QChar(kNotEqualSign)), toInt(Operator::NotEqual));

    m_values->setObjectName(QStringLiteral("quickSelectValue"));
    m_color->setObjectName(QStringLiteral("quickSelectColor"));
    m_layer->setObjectName(QStringLiteral("quickSelectLayer"));
    m_linetype->setObjectName(QStringLiteral("quickSelectLinetype"));
    m_lineweight->setObjectName(QStringLiteral("quickSelectLineweight"));
    m_color->setValue(m_colorValue);
    m_layer->setDocument(&document);
    m_linetype->setDocument(&document);
    m_lineweight->setDocument(&document);
    // In the order of ValuePage.
    m_values->addWidget(m_color);
    m_values->addWidget(m_layer);
    m_values->addWidget(m_linetype);
    m_values->addWidget(m_lineweight);
    m_values->addWidget(new QWidget(m_values));

    m_include->setObjectName(QStringLiteral("quickSelectInclude"));
    m_exclude->setObjectName(QStringLiteral("quickSelectExclude"));
    m_include->setChecked(true);

    auto* form = new QFormLayout;
    form->addRow(tr("Apply to:"), m_where);
    form->addRow(tr("Object type:"), m_type);
    form->addRow(tr("Properties:"), m_property);
    form->addRow(tr("Operator:"), m_operator);
    form->addRow(tr("Value:"), m_values);

    auto* how = new QGroupBox(tr("How to apply:"), this);
    auto* howLayout = new QVBoxLayout(how);
    howLayout->addWidget(m_include);
    howLayout->addWidget(m_exclude);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName(QStringLiteral("quickSelectButtons"));

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(how);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_property, &QComboBox::activated, this, [this] { showValueControl(); });
    connect(m_color, &ColorCombo::valueChosen, this,
            [this](core::Color value) { m_colorValue = value; });
    connect(m_layer, &LayerCombo::valueChosen, this,
            [this](core::LayerId value) { m_layerValue = value; });
    connect(m_linetype, &LinetypeCombo::valueChosen, this,
            [this](core::LinetypeId value) { m_linetypeValue = value; });
    connect(m_lineweight, &LineweightCombo::valueChosen, this,
            [this](int value) { m_lineweightValue = value; });

    showValueControl();
}

void QuickSelectDialog::showValueControl() {
    const ValuePage page = pageOf(static_cast<Property>(m_property->currentData().toInt()));
    m_values->setCurrentIndex(static_cast<int>(page));
    m_operator->setEnabled(page != ValuePage::None);
    m_values->setEnabled(page != ValuePage::None);
}

bool QuickSelectDialog::searchesSelection() const {
    return m_where->currentData().toBool();
}

std::vector<core::EntityId> QuickSelectDialog::within() const {
    return searchesSelection() ? m_current : std::vector<core::EntityId>{};
}

cmd::QuickSelectCriteria QuickSelectDialog::criteria() const {
    cmd::QuickSelectCriteria chosen;
    const QString type = m_type->currentData().toString();
    if (!type.isEmpty()) {
        chosen.type = type.toStdString();
    }
    chosen.property = static_cast<Property>(m_property->currentData().toInt());
    chosen.op = static_cast<Operator>(m_operator->currentData().toInt());
    chosen.exclude = m_exclude->isChecked();
    switch (chosen.property) {
        case Property::Color:
            chosen.value = core::formatColor(m_colorValue);
            break;
        case Property::Layer: {
            const core::Layer* layer = m_document.layer(m_layerValue);
            chosen.value = layer != nullptr ? layer->name : std::string("0");
            break;
        }
        case Property::Linetype: {
            const core::Linetype* linetype = m_document.linetype(m_linetypeValue);
            chosen.value = linetype != nullptr ? linetype->name : std::string("ByLayer");
            break;
        }
        case Property::Lineweight:
            chosen.value = core::formatLineweight(m_lineweightValue);
            break;
        case Property::None:
            chosen.op = Operator::Equal;
            break;
    }
    return chosen;
}

std::vector<core::EntityId> QuickSelectDialog::result() const {
    return cmd::quickSelect(m_document, within(), criteria());
}

}  // namespace tg::app
