// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "property_widgets.hpp"

#include "color_dialog.hpp"
#include "layer_table_model.hpp"
#include "theme.hpp"

#include <QEvent>
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
constexpr int kLayerIconSide = 16;
constexpr int kLayerIconStep = 18;
constexpr int kLayerIconWidth = 3 * kLayerIconStep + kLayerIconSide;

void paintColorSample(QPainter& painter, const QRect& rect, core::Color color,
                      const QPalette& palette) {
    if (color.kind() == core::Color::Kind::Index && color.index() == 7) {
        const int half = rect.width() / 2;
        painter.fillRect(rect.x(), rect.y(), half, rect.height(), Qt::white);
        painter.fillRect(rect.x() + half, rect.y(), rect.width() - half, rect.height(), Qt::black);
    } else {
        QColor fill = palette.color(QPalette::Text);
        if (color.kind() == core::Color::Kind::Rgb) {
            const auto rgb = color.rgb();
            fill = QColor(rgb.r, rgb.g, rgb.b);
        } else if (color.kind() == core::Color::Kind::Index) {
            const auto rgb = core::aciToRgb(color.index());
            fill = QColor(rgb.r, rgb.g, rgb.b);
        }
        painter.fillRect(rect, fill);
    }
    painter.setPen(palette.color(QPalette::Text));
    painter.drawRect(rect.adjusted(0, 0, -1, -1));
}

QIcon layerIcon(const core::Layer& layer, const QPalette& palette) {
    QIcon icon;
    // Build both display scales so the ribbon stays sharp at 100% and 200%.
    for (const int scale : {1, 2}) {
        QPixmap pixmap(kLayerIconWidth * scale, kLayerIconSide * scale);
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        int x = 0;
        for (const LayerState state : {LayerState::On, LayerState::Frozen, LayerState::Locked}) {
            const QString name = layerStateIconName(state, layerStateValue(layer, state));
            const QIcon stateIcon = themedIcon(name, themeOf(palette));
            stateIcon.paint(&painter, QRect(x, 0, kLayerIconSide, kLayerIconSide));
            x += kLayerIconStep;
        }
        // Draw on the target device: no rectangular raster is scaled into a square.
        const QRect sample(x, 0, kLayerIconSide, kLayerIconSide);
        paintColorSample(painter, sample, layer.color, palette);
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

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
}  // namespace

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

QIcon linetypeIcon(const core::LinetypePattern& pattern, const QPalette& palette) {
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
    return {pixmap};
}

QIcon lineweightIcon(int lineweight, const QPalette& palette) {
    QPixmap pixmap(kPreviewWidth, kPreviewHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    // DXF hundredths of a millimetre, displayed at 96 logical pixels/inch.
    const double width = std::max(1.0, static_cast<double>(lineweight) * 96.0 / 2540.0);
    painter.setPen(QPen(palette.color(QPalette::Text), width));
    painter.drawLine(0, kPreviewHeight / 2, kPreviewWidth, kPreviewHeight / 2);
    return {pixmap};
}

QString lineweightName(int lineweight) {
    switch (lineweight) {
        case core::lineweight::kByLayer:
            return LineweightCombo::tr("ByLayer");
        case core::lineweight::kByBlock:
            return LineweightCombo::tr("ByBlock");
        case core::lineweight::kDefault:
            return LineweightCombo::tr("Default");
        default:
            return LineweightCombo::tr("%1 mm").arg(static_cast<double>(lineweight) / 100.0, 0, 'f',
                                                    2);
    }
}

QIcon colorIcon(core::Color color, const QPalette& palette) {
    QPixmap pixmap(kPreviewWidth, kPreviewHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    paintColorSample(painter, pixmap.rect(), color, palette);
    return {pixmap};
}

QIcon colorSampleIcon(core::Color color, const QPalette& palette) {
    constexpr int kSampleSide = 12;
    constexpr int kSampleMargin = (kLayerIconSide - kSampleSide) / 2;
    QIcon icon;
    for (const int scale : {1, 2}) {
        QPixmap pixmap(kLayerIconSide * scale, kLayerIconSide * scale);
        pixmap.setDevicePixelRatio(scale);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        paintColorSample(painter, QRect(kSampleMargin, kSampleMargin, kSampleSide, kSampleSide),
                         color, palette);
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

ColorCombo::ColorCombo(QWidget* parent) : QComboBox(parent) {
    rebuild();
    connect(this, &QComboBox::activated, this, &ColorCombo::choose);
}

void ColorCombo::rebuild() {
    const QSignalBlocker blocker(this);
    clear();
    const auto add = [this](core::Color color) {
        addItem(colorSampleIcon(color, palette()), colorName(color), QVariant::fromValue(color));
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
        // The newest colour goes first. Not insert(begin()): GCC 13 at -O3
        // warns of a false null dereference in it (-Wnull-dereference).
        std::erase(m_recent, value);
        if (m_recent.size() >= kMaxRecentColours) {
            m_recent.pop_back();
        }
        m_recent.push_back(value);
        std::rotate(m_recent.rbegin(), m_recent.rbegin() + 1, m_recent.rend());
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
    setIconSize(QSize(kLayerIconWidth, kLayerIconSide));
    connect(this, &QComboBox::currentIndexChanged, this, [this] { updateDescription(); });
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
    m_layers.clear();
    if (document != nullptr) {
        m_layers.assign(document->layers().begin(), document->layers().end());
        for (const auto& layer : m_layers) {
            const auto name = QString::fromStdString(layer.name);
            const auto on = layer.on ? tr("On") : tr("Off");
            const auto frozen = layer.frozen ? tr("Frozen") : tr("Thawed");
            const auto locked = layer.locked ? tr("Locked") : tr("Unlocked");
            const auto plot = layer.plot ? tr("Plot") : tr("No plot");
            const auto label = tr("%1 [%2, %3, %4, %5]").arg(name, on, frozen, locked, plot);
            addItem(layerIcon(layer, palette()), name, QVariant::fromValue(layer.id));
            setItemData(count() - 1, label, Qt::ToolTipRole);
            setItemData(count() - 1, label, Qt::AccessibleTextRole);
        }
        setValue(document->current().layer);
    } else {
        m_value = {};
    }
    updateDescription();
}

void LayerCombo::setValue(core::LayerId value) {
    m_value = value;
    selectValue(*this, value);
    updateDescription();
}

void LayerCombo::setVaries() {
    showVaries(*this);
    updateDescription();
}

void LayerCombo::updateDescription() {
    const QString description = currentData(Qt::AccessibleTextRole).toString();
    setToolTip(description);
    setAccessibleName(description.isEmpty() ? currentText() : description);
}

void LayerCombo::changeEvent(QEvent* event) {
    QComboBox::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        for (const auto& layer : m_layers) {
            const int index = findData(QVariant::fromValue(layer.id));
            if (index >= 0) {
                setItemIcon(index, layerIcon(layer, palette()));
            }
        }
    }
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
            addItem(linetypeIcon(linetype.pattern, palette()),
                    QString::fromStdString(linetype.name), QVariant::fromValue(linetype.id));
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
    addItem(lineweightName(core::lineweight::kByLayer), core::lineweight::kByLayer);
    addItem(lineweightName(core::lineweight::kByBlock), core::lineweight::kByBlock);
    const int defaultWeight =
        document != nullptr ? document->lwdefault() : core::Document::kDefaultLwdefault;
    addItem(lineweightIcon(defaultWeight, palette()), lineweightName(core::lineweight::kDefault),
            core::lineweight::kDefault);
    for (const int weight : core::lineweight::kAllowed) {
        addItem(lineweightIcon(weight, palette()), lineweightName(weight), weight);
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
