// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/attribute_definition.hpp"
#include "tg/core/insert.hpp"

#include <QDialog>

#include <string>
#include <vector>

class QLineEdit;

namespace tg::app {

/// Edits the three textual fields of an ATTDEF in one window.
class AttributeDefinitionDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AttributeDefinitionDialog(const core::AttributeDefinition& definition,
                                       QWidget* parent = nullptr);

    [[nodiscard]] core::AttributeDefinition definition() const;

private:
    core::AttributeDefinition m_definition;
    QLineEdit* m_tag{nullptr};
    QLineEdit* m_prompt{nullptr};
    QLineEdit* m_value{nullptr};
};

/// Edits the values of the non-constant attributes owned by an INSERT.
class InsertAttributesDialog final : public QDialog {
    Q_OBJECT

public:
    explicit InsertAttributesDialog(const core::Insert& insert, QWidget* parent = nullptr);

    [[nodiscard]] std::vector<std::string> values() const;

private:
    std::vector<QLineEdit*> m_values;
};

}  // namespace tg::app
