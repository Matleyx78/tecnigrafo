// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/quick_select.hpp"
#include "tg/core/document.hpp"

#include <QDialog>

#include <vector>

class QComboBox;
class QRadioButton;
class QStackedWidget;

namespace tg::app {

class ColorCombo;
class LayerCombo;
class LinetypeCombo;
class LineweightCombo;

/// The window of QSELECT (docs/tappe/0.6/P24): where to search, the type of
/// object, the property, the operator (= or ≠), the value with the control
/// of that property, and whether the matches are included or excluded.
///
/// It only gathers the criteria: whoever opens it calls cmd::quickSelect on
/// OK. The document is borrowed and read only; it must outlive the window.
class QuickSelectDialog final : public QDialog {
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(QuickSelectDialog)

    /// `current` is the selection the user may search within; the choice is
    /// offered, and taken by default, only if it is not empty.
    QuickSelectDialog(const core::Document& document, std::vector<core::EntityId> current,
                      QWidget* parent = nullptr);
    ~QuickSelectDialog() override = default;

    /// The criteria as the window shows them now.
    [[nodiscard]] cmd::QuickSelectCriteria criteria() const;

    /// True if the search is within the current selection.
    [[nodiscard]] bool searchesSelection() const;

    /// What cmd::quickSelect searches: the current selection, or nothing for
    /// the whole drawing.
    [[nodiscard]] std::vector<core::EntityId> within() const;

    /// What OK selects: cmd::quickSelect with within() and criteria().
    [[nodiscard]] std::vector<core::EntityId> result() const;

private:
    void showValueControl();

    const core::Document& m_document;
    std::vector<core::EntityId> m_current;
    QComboBox* m_where;
    QComboBox* m_type;
    QComboBox* m_property;
    QComboBox* m_operator;
    QStackedWidget* m_values;
    ColorCombo* m_color;
    LayerCombo* m_layer;
    LinetypeCombo* m_linetype;
    LineweightCombo* m_lineweight;
    QRadioButton* m_include;
    QRadioButton* m_exclude;

    // The values chosen in the controls: their setters emit nothing, so the
    // window keeps what the user chose (0.6-P21).
    core::Color m_colorValue;
    core::LayerId m_layerValue;
    core::LinetypeId m_linetypeValue;
    int m_lineweightValue;
};

}  // namespace tg::app
