// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "property_widgets.hpp"

#include "color_dialog.hpp"

#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QVariant>

#include <algorithm>
#include <cmath>

namespace tg::app {
namespace {
constexpr int kPreviewWidth = 64;
constexpr int kPreviewHeight = 18;
constexpr std::size_t kMaxRecentColours = 8;

void clearVaries(QComboBox& combo) {
    if (combo.count() > 0 && !combo.itemData(0).isValid()) {
        combo.removeItem(0);
    }
}

void showVaries(QComboBox& combo) {
    const QSignalBlocker blocker(&combo);
    clearVaries(combo);
    combo.insertItem(0, QComboBox::tr("Varies"));
    combo.setCurrentIndex(0);
}

template <typename Value>
void selectValue(QComboBox& combo, Value value) {
    const QSignalBlocker blocker(&combo);
    clearVaries(combo);
    combo.setCurrentIndex(combo.findData(QVariant::fromValue(value)));
}

QString colorName(core::Color color) {
    if (color.kind() == core::Color::Kind::ByLayer) {
        return ColorCombo::tr("ByLayer");
    }
    if (color.kind() == core::Color::Kind::ByBlock) {
        return ColorCombo::tr("ByBlock");
    }
    if (color.kind() == core::Color::Kind::Index) {
        switch (color.index()) {
            case 1:
                return ColorCombo::tr("Red");
            case 2:
                return ColorCombo::tr("Yellow");
            case 3:
                return ColorCombo::tr("Green");
            case 4:
                return ColorCombo::tr("Cyan");
            case 5:
                return ColorCombo::tr("Blue");
            case 6:
                return ColorCombo::tr("Magenta");
            case 7:
                return ColorCombo::tr("White");
            default:
                return QString::number(color.index());
        }
    }
    return QString::fromStdString(core::formatColor(color));
}

QIcon lineIcon(const core::LinetypePattern& pattern, const QPalette& palette) {
    QPixmap pixmap(kPreviewWidth, kPreviewHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(palette.color(QPalette::Text));
    const int y = kPreviewHeight / 2;
    const double length = pattern.length();
    if (pattern.continuous() || length <= 0.0) {
        painter.drawLine(0, y, kPreviewWidth, y);
    } else {
        // Fit two periods in the sample, preserving the relative dash lengths.
        const double scale = static_cast<double>(kPreviewWidth) / (2.0 * length);
        double x = 0.0;
        for (int repeat = 0; repeat < 2; ++repeat) {
            for (const double element : pattern.elements) {
                const double next = x + std::abs(element) * scale;
                if (element > 0.0) {
                    painter.drawLine(QPointF(x, y), QPointF(next, y));
                } else if (element == 0.0) {
                    painter.drawPoint(QPointF(x, y));
                }
                x = next;
            }
        }
    }
    return QIcon(pixmap);
}

QIcon weightIcon(int weight, const QPalette& palette) {
    QPixmap pixmap(kPreviewWidth, kPreviewHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    // DXF hundredths of a millimetre, displayed at 96 logical pixels/inch.
    const double width = std::max(1.0, static_cast<double>(weight) * 96.0 / 2540.0);
    painter.setPen(QPen(palette.color(QPalette::Text), width));
    painter.drawLine(0, kPreviewHeight / 2, kPreviewWidth, kPreviewHeight / 2);
    return QIcon(pixmap);
}
}  // namespace

QIcon colorIcon(core::Color color, const QPalette& palette) {
    QPixmap pixmap(kPreviewWidth, kPreviewHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    if (color.kind() == core::Color::Kind::Index && color.index() == 7) {
        painter.fillRect(0, 0, kPreviewWidth / 2, kPreviewHeight, Qt::white);
        painter.fillRect(kPreviewWidth / 2, 0, kPreviewWidth / 2, kPreviewHeight, Qt::black);
    } else {
        QColor fill = palette.color(QPalette::Text);
        if (color.kind() == core::Color::Kind::Rgb) {
            const auto rgb = color.rgb();
            fill = QColor(rgb.r, rgb.g, rgb.b);
        } else if (color.kind() == core::Color::Kind::Index) {
            const auto rgb = core::aciToRgb(color.index());
            fill = QColor(rgb.r, rgb.g, rgb.b);
        }
        painter.fillRect(pixmap.rect(), fill);
    }
    painter.setPen(palette.color(QPalette::Text));
    painter.drawRect(0, 0, kPreviewWidth - 1, kPreviewHeight - 1);
    return QIcon(pixmap);
}

ColorCombo::ColorCombo(QWidget* parent) : QComboBox(parent) {
    rebuild();
    connect(this, &QComboBox::activated, this, &ColorCombo::choose);
}

void ColorCombo::rebuild() {
    const QSignalBlocker blocker(this);
    clear();
    const auto add = [this](core::Color color) {
        addItem(colorIcon(color, palette()), colorName(color), QVariant::fromValue(color));
    };
    add(core::Color::byLayer());
    add(core::Color::byBlock());
    for (int index = 1; index <= 7; ++index) {
        add(core::Color::fromIndex(index));
    }
    for (const auto color : m_recent) {
        add(color);
    }
    addItem(tr("Select Colour…"), QStringLiteral("selectColour"));
    selectValue(*this, m_value);
}

void ColorCombo::setDocument(const core::Document* document) {
    if (document != nullptr) {
        setValue(document->current().color);
    } else {
        setValue(core::Color::byLayer());
    }
}

void ColorCombo::setValue(core::Color value) {
    m_value = value;
    if (value.kind() == core::Color::Kind::Rgb ||
        (value.kind() == core::Color::Kind::Index && value.index() > 7)) {
        std::erase(m_recent, value);
        m_recent.insert(m_recent.begin(), value);
        if (m_recent.size() > kMaxRecentColours) {
            m_recent.pop_back();
        }
    }
    rebuild();
}

void ColorCombo::setVaries() {
    showVaries(*this);
}

void ColorCombo::choose(int index) {
    const auto chosenData = itemData(index);
    if (!chosenData.isValid()) {
        return;
    }
    if (chosenData.metaType() == QMetaType::fromType<core::Color>()) {
        // Color has no default constructor; the metatype was checked above.
        setValue(*static_cast<const core::Color*>(chosenData.constData()));
        Q_EMIT valueChosen(m_value);
        return;
    }
    const bool wasMixed = count() > 0 && !itemData(0).isValid();
    ColorDialog dialog(m_value, ColorDialog::Allow::All, this);
    if (dialog.exec() == QDialog::Accepted) {
        setValue(dialog.color());
        Q_EMIT valueChosen(m_value);
    } else if (wasMixed) {
        setVaries();
    } else {
        selectValue(*this, m_value);
    }
}

LayerCombo::LayerCombo(QWidget* parent) : QComboBox(parent) {
    connect(this, &QComboBox::activated, this, [this](int index) {
        if (itemData(index).isValid()) {
            setValue(itemData(index).value<core::LayerId>());
            Q_EMIT valueChosen(m_value);
        }
    });
}

void LayerCombo::setDocument(const core::Document* document) {
    const QSignalBlocker blocker(this);
    clear();
    if (document != nullptr) {
        for (const auto& layer : document->layers()) {
            const auto name = QString::fromStdString(layer.name);
            const auto on = layer.on ? tr("On") : tr("Off");
            const auto frozen = layer.frozen ? tr("Frozen") : tr("Thawed");
            const auto locked = layer.locked ? tr("Locked") : tr("Unlocked");
            const auto plot = layer.plot ? tr("Plot") : tr("No plot");
            const auto label = tr("%1 [%2, %3, %4, %5]").arg(name, on, frozen, locked, plot);
            addItem(colorIcon(layer.color, palette()), label, QVariant::fromValue(layer.id));
        }
        setValue(document->current().layer);
    } else {
        m_value = {};
    }
}

void LayerCombo::setValue(core::LayerId value) {
    m_value = value;
    selectValue(*this, value);
}

void LayerCombo::setVaries() {
    showVaries(*this);
}

LinetypeCombo::LinetypeCombo(QWidget* parent) : QComboBox(parent) {
    setIconSize(QSize(kPreviewWidth, kPreviewHeight));
    connect(this, &QComboBox::activated, this, [this](int index) {
        if (itemData(index).isValid()) {
            setValue(itemData(index).value<core::LinetypeId>());
            Q_EMIT valueChosen(m_value);
        }
    });
}

void LinetypeCombo::setDocument(const core::Document* document) {
    const QSignalBlocker blocker(this);
    clear();
    if (document != nullptr) {
        for (const auto& linetype : document->linetypes()) {
            addItem(lineIcon(linetype.pattern, palette()), QString::fromStdString(linetype.name),
                    QVariant::fromValue(linetype.id));
        }
        setValue(document->current().linetype);
    } else {
        m_value = {};
    }
}

void LinetypeCombo::setValue(core::LinetypeId value) {
    m_value = value;
    selectValue(*this, value);
}

void LinetypeCombo::setVaries() {
    showVaries(*this);
}

LineweightCombo::LineweightCombo(QWidget* parent) : QComboBox(parent) {
    setIconSize(QSize(kPreviewWidth, kPreviewHeight));
    setDocument(nullptr);
    connect(this, &QComboBox::activated, this, [this](int index) {
        if (itemData(index).isValid()) {
            setValue(itemData(index).toInt());
            Q_EMIT valueChosen(m_value);
        }
    });
}

void LineweightCombo::setDocument(const core::Document* document) {
    const QSignalBlocker blocker(this);
    clear();
    addItem(tr("ByLayer"), core::lineweight::kByLayer);
    addItem(tr("ByBlock"), core::lineweight::kByBlock);
    const int defaultWeight = document != nullptr ? document->lwdefault() : 25;
    addItem(weightIcon(defaultWeight, palette()), tr("Default"), core::lineweight::kDefault);
    for (const int weight : core::lineweight::kAllowed) {
        addItem(weightIcon(weight, palette()),
                tr("%1 mm").arg(static_cast<double>(weight) / 100.0, 0, 'f', 2), weight);
    }
    setValue(document != nullptr ? document->current().lineweight : core::lineweight::kByLayer);
}

void LineweightCombo::setValue(int value) {
    m_value = value;
    selectValue(*this, value);
}

void LineweightCombo::setVaries() {
    showVaries(*this);
}

}  // namespace tg::app
