// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <QComboBox>
#include <QIcon>
#include <QPalette>

#include <vector>

namespace tg::app {

/// ACI 7 is deliberately shown as both black and white, on either theme.
[[nodiscard]] QIcon colorIcon(core::Color color, const QPalette& palette);

/// Programmatic setters never emit valueChosen; only user activation does.
/// The document is borrowed and never modified. Call setDocument to refresh it.
class ColorCombo final : public QComboBox {
    Q_OBJECT

public:
    explicit ColorCombo(QWidget* parent = nullptr);
    void setDocument(const core::Document* document);
    void setValue(core::Color value);
    void setVaries();

Q_SIGNALS:
    void valueChosen(tg::core::Color value);

private:
    void rebuild();
    void choose(int index);
    core::Color m_value{core::Color::byLayer()};
    std::vector<core::Color> m_recent;
};

class LayerCombo final : public QComboBox {
    Q_OBJECT

public:
    explicit LayerCombo(QWidget* parent = nullptr);
    void setDocument(const core::Document* document);
    void setValue(core::LayerId value);
    void setVaries();

Q_SIGNALS:
    void valueChosen(tg::core::LayerId value);

private:
    core::LayerId m_value;
};

class LinetypeCombo final : public QComboBox {
    Q_OBJECT

public:
    explicit LinetypeCombo(QWidget* parent = nullptr);
    void setDocument(const core::Document* document);
    void setValue(core::LinetypeId value);
    void setVaries();

Q_SIGNALS:
    void valueChosen(tg::core::LinetypeId value);

private:
    core::LinetypeId m_value;
};

class LineweightCombo final : public QComboBox {
    Q_OBJECT

public:
    explicit LineweightCombo(QWidget* parent = nullptr);
    void setDocument(const core::Document* document);
    void setValue(int value);
    void setVaries();

Q_SIGNALS:
    void valueChosen(int value);

private:
    int m_value{core::lineweight::kByLayer};
};

}  // namespace tg::app
