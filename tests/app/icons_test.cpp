// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"
#include "builtin_actions.hpp"
#include "layer_table_model.hpp"
#include "properties_panel.hpp"
#include "theme.hpp"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QSvgRenderer>
#include <QXmlStreamReader>
#include <gtest/gtest.h>

#include <utility>

namespace tg::app {
namespace {

// The rules of docs/ICONE.md.
constexpr int kGrid = 24;         // the icon's width and height, in px
constexpr int kMargin = 2;        // px of the grid left free of ink
constexpr int kScale = 4;         // the icons are drawn at 4x to measure the margin
constexpr int kMarginAlpha = 16;  // alpha (of 255) below which a pixel is not ink
constexpr int kFirstIcons = 12;   // the icons of 0.6-P18
constexpr int kSecondIcons = 25;  // the icons of 0.6-P19

bool isAllowedElement(const QString& name) {
    // The elements that Qt SVG draws predictably and the theme can tint.
    static const QStringList kAllowedElements{
        QStringLiteral("svg"),      QStringLiteral("g"),      QStringLiteral("path"),
        QStringLiteral("circle"),   QStringLiteral("line"),   QStringLiteral("rect"),
        QStringLiteral("polyline"), QStringLiteral("polygon")};
    return kAllowedElements.contains(name);
}

/// What an element of the icon does against docs/ICONE.md: one sentence per
/// problem.
void checkElement(const QXmlStreamReader& reader, bool isRoot, QStringList& problems) {
    const QString name = reader.name().toString();
    if (isRoot && name != QStringLiteral("svg")) {
        problems.append(QStringLiteral("the root element is <%1>, not <svg>").arg(name));
    }
    if (!isAllowedElement(name)) {
        problems.append(QStringLiteral("element <%1> is not allowed").arg(name));
    }
    const QXmlStreamAttributes attributes = reader.attributes();
    if (isRoot) {
        const QString size = QString::number(kGrid);
        if (attributes.value(QStringLiteral("width")).toString() != size ||
            attributes.value(QStringLiteral("height")).toString() != size ||
            attributes.value(QStringLiteral("viewBox")).toString() != QStringLiteral("0 0 24 24")) {
            problems.append(
                QStringLiteral("the grid must be width=24 height=24 viewBox=0 0 24 24"));
        }
    }
    for (const QXmlStreamAttribute& attribute : attributes) {
        const QString key = attribute.name().toString();
        const QString value = attribute.value().toString();
        if ((key == QStringLiteral("fill") || key == QStringLiteral("stroke")) &&
            value != QStringLiteral("none") && value != QStringLiteral("currentColor")) {
            problems.append(QStringLiteral("%1=\"%2\": only none or currentColor").arg(key, value));
        }
        if (key == QStringLiteral("fill") && value == QStringLiteral("currentColor") &&
            (name != QStringLiteral("circle") ||
             attributes.value(QStringLiteral("r")).toString() != QStringLiteral("1.5"))) {
            problems.append(QStringLiteral("only dots (circles with r=1.5) are filled"));
        }
        if (key == QStringLiteral("class") && value != QStringLiteral("accent")) {
            problems.append(QStringLiteral("class=\"%1\": only accent").arg(value));
        }
        if (key == QStringLiteral("style")) {
            problems.append(QStringLiteral("style attributes are not allowed"));
        }
        if (key == QStringLiteral("stroke-width") && value != QStringLiteral("1.5")) {
            problems.append(QStringLiteral("stroke-width=\"%1\": the stroke is 1.5").arg(value));
        }
        if ((key == QStringLiteral("stroke-linecap") || key == QStringLiteral("stroke-linejoin")) &&
            value != QStringLiteral("round")) {
            problems.append(QStringLiteral("%1=\"%2\": caps and joins are round").arg(key, value));
        }
    }
}

/// What is wrong with the file `data` as an icon; empty if it follows
/// docs/ICONE.md in everything that can be read from its text.
QStringList problemsOf(const QByteArray& data) {
    QStringList problems;
    QXmlStreamReader reader(data);
    bool hasLicense = false;
    bool sawRoot = false;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isComment()) {
            hasLicense =
                hasLicense ||
                reader.text().contains(QStringLiteral("SPDX-License-Identifier: GPL-3.0-or-later"));
        } else if (reader.isStartElement()) {
            checkElement(reader, !sawRoot, problems);
            sawRoot = true;
        }
    }
    if (reader.hasError()) {
        problems.append(QStringLiteral("not well-formed XML: %1").arg(reader.errorString()));
    }
    if (!hasLicense) {
        problems.append(QStringLiteral("no SPDX-License-Identifier comment"));
    }
    return problems;
}

/// The icon drawn at kScale times its size on a transparent background.
QImage drawAtScale(QSvgRenderer& renderer) {
    const int side = kGrid * kScale;
    QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(0, 0, side, side));
    painter.end();
    return image;
}

/// The pixels of `image` that carry ink and, among them, those in the margin.
struct InkCount {
    int total{0};
    int inMargin{0};
};

InkCount countInk(const QImage& image) {
    InkCount count;
    const int margin = kMargin * kScale;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) <= kMarginAlpha) {
                continue;
            }
            ++count.total;
            const bool inside = x >= margin && x < image.width() - margin && y >= margin &&
                                y < image.height() - margin;
            if (!inside) {
                ++count.inMargin;
            }
        }
    }
    return count;
}

QString resourcePath(const QString& icon) {
    return QStringLiteral(":/tg/icons/%1.svg").arg(icon);
}

/// The actions that have no icon: the separators (no text), the recent files
/// (the submenu and the run-time list) and Quit (docs/tappe/0.6/P19).
bool needsNoIcon(const ActionSpec& spec) {
    return spec.text.isEmpty() || spec.id == QStringLiteral("file.quit") ||
           spec.id == QStringLiteral("file.recent") ||
           spec.id.startsWith(QStringLiteral("file.recent."));
}

TEST(IconsTest, EveryFileIsAValidSvgOnTheGrid) {
    const QDir directory(QStringLiteral(TG_ICONS_DIR));
    const QStringList names =
        directory.entryList({QStringLiteral("*.svg")}, QDir::Files, QDir::Name);
    EXPECT_GE(names.size(), kFirstIcons + kSecondIcons);
    for (const QString& name : names) {
        SCOPED_TRACE(name.toStdString());
        QFile file(directory.filePath(name));
        ASSERT_TRUE(file.open(QIODevice::ReadOnly));
        const QByteArray data = file.readAll();

        const QStringList problems = problemsOf(data);
        EXPECT_TRUE(problems.isEmpty()) << problems.join(QStringLiteral("; ")).toStdString();

        QSvgRenderer renderer(data);
        ASSERT_TRUE(renderer.isValid());
        EXPECT_EQ(renderer.defaultSize(), QSize(kGrid, kGrid));
        EXPECT_EQ(renderer.viewBoxF(), QRectF(0, 0, kGrid, kGrid));
        const InkCount ink = countInk(drawAtScale(renderer));
        EXPECT_GT(ink.total, 0) << "nothing is drawn";
        EXPECT_EQ(ink.inMargin, 0) << "ink in the 2 px margin";

        EXPECT_TRUE(QFile::exists(resourcePath(name.chopped(4)))) << "not in the resources";
    }
}

TEST(IconsTest, EveryActionHasAnIconWithItsFile) {
    ActionRegistry registry;
    addBuiltinActions(registry);
    int withIcon = 0;
    for (const QAction* action : registry.actions()) {
        const ActionSpec* spec = registry.spec(action->objectName());
        ASSERT_NE(spec, nullptr);
        if (needsNoIcon(*spec)) {
            continue;
        }
        ++withIcon;
        SCOPED_TRACE(spec->id.toStdString());
        ASSERT_FALSE(spec->icon.isEmpty()) << "the action has no icon";
        EXPECT_TRUE(QFile::exists(resourcePath(spec->icon)))
            << "no resources/icons/" << spec->icon.toStdString() << ".svg";
        EXPECT_FALSE(themedIcon(spec->icon, QApplication::palette()).isNull());
    }
    EXPECT_GE(withIcon, kFirstIcons);
}

TEST(IconsTest, DrawingEditingAndFileActionsUseTheirOwnIcons) {
    ActionRegistry registry;
    addBuiltinActions(registry);
    // The icons of 0.6-P18: named like the action, with dashes for dots.
    const QStringList ids{
        QStringLiteral("draw.line"), QStringLiteral("draw.pline"), QStringLiteral("draw.circle"),
        QStringLiteral("draw.arc"),  QStringLiteral("draw.text"),  QStringLiteral("edit.erase"),
        QStringLiteral("edit.undo"), QStringLiteral("edit.redo"),  QStringLiteral("file.new"),
        QStringLiteral("file.open"), QStringLiteral("file.save"),  QStringLiteral("file.saveas")};
    EXPECT_EQ(ids.size(), kFirstIcons);
    for (const QString& id : ids) {
        SCOPED_TRACE(id.toStdString());
        const ActionSpec* spec = registry.spec(id);
        ASSERT_NE(spec, nullptr);
        QString expected = id;
        expected.replace(QLatin1Char('.'), QLatin1Char('-'));
        EXPECT_EQ(spec->icon, expected);
    }
}

TEST(IconsTest, ViewAidsInterfaceAndHelpActionsUseTheirOwnIcons) {
    ActionRegistry registry;
    addBuiltinActions(registry);
    // The icons of 0.6-P19 that actions already use. Named like the action
    // with dashes for dots, except the switch between ribbon and classic
    // menus, which shows the ribbon.
    const QList<std::pair<QString, QString>> expected{
        {QStringLiteral("view.zoom-extents"), QStringLiteral("view-zoom-extents")},
        {QStringLiteral("aids.osnap"), QStringLiteral("aids-osnap")},
        {QStringLiteral("aids.ortho"), QStringLiteral("aids-ortho")},
        {QStringLiteral("aids.snap.endpoint"), QStringLiteral("aids-snap-endpoint")},
        {QStringLiteral("aids.snap.midpoint"), QStringLiteral("aids-snap-midpoint")},
        {QStringLiteral("aids.snap.center"), QStringLiteral("aids-snap-center")},
        {QStringLiteral("aids.snap.insertion"), QStringLiteral("aids-snap-insertion")},
        {QStringLiteral("aids.lwdisplay"), QStringLiteral("aids-lwdisplay")},
        {QStringLiteral("ui.interface"), QStringLiteral("ui-ribbon")},
        {QStringLiteral("ui.theme"), QStringLiteral("ui-theme")},
        {QStringLiteral("tools.settings"), QStringLiteral("tools-settings")},
        {QStringLiteral("help.about"), QStringLiteral("help-about")},
        // The properties panel and QSELECT (0.6-P24).
        {QStringLiteral("properties.panel"), QStringLiteral("properties-panel")},
        {QStringLiteral("edit.quick-select"), QStringLiteral("quick-select")}};
    for (const auto& [id, icon] : expected) {
        SCOPED_TRACE(id.toStdString());
        const ActionSpec* spec = registry.spec(id);
        ASSERT_NE(spec, nullptr);
        EXPECT_EQ(spec->icon, icon);
    }
}

TEST(IconsTest, EveryIconFileHasAnActionOrAPanel) {
    ActionRegistry registry;
    addBuiltinActions(registry);
    QStringList used;
    for (const QAction* action : registry.actions()) {
        const ActionSpec* spec = registry.spec(action->objectName());
        ASSERT_NE(spec, nullptr);
        if (!spec->icon.isEmpty()) {
            used.append(spec->icon);
        }
    }
    // The states and the mark of the current layer are drawn by the layers
    // panel (0.6-P22), the icons of colour, linetype and lineweight by the
    // properties panel and its controls (0.6-P24): no action names them.
    used.append(layerPanelIconNames());
    used.append(propertiesPanelIconNames());

    const QDir directory(QStringLiteral(TG_ICONS_DIR));
    const QStringList names =
        directory.entryList({QStringLiteral("*.svg")}, QDir::Files, QDir::Name);
    for (const QString& name : names) {
        const QString icon = name.chopped(4);
        SCOPED_TRACE(icon.toStdString());
        EXPECT_TRUE(used.contains(icon)) << "neither an action nor a panel uses this icon";
    }
    // The icons the panels name are all drawn.
    for (const QString& icon : propertiesPanelIconNames()) {
        SCOPED_TRACE(icon.toStdString());
        EXPECT_TRUE(QFile::exists(resourcePath(icon))) << "icon not in the resources";
    }
}

}  // namespace
}  // namespace tg::app
