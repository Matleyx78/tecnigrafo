// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"
#include "auto_hide.hpp"
#include "builtin_actions.hpp"
#include "document_tabs.hpp"
#include "layer_table_model.hpp"
#include "properties_panel.hpp"
#include "theme.hpp"

#include <QAction>
#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QSvgRenderer>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <utility>

namespace tg::app {
namespace {

// SVG rules and declared geometry are checked by scripts/icone.py;
// these tests also check the grids and margins as Qt draws them.
constexpr int kScale = 4;         // draw at 4x to measure the margin
constexpr int kMarginAlpha = 16;  // alpha (of 255) below which a pixel is not ink
constexpr int kFirstIcons = 12;   // the icons of 0.6-P18

/// The grid of a role icon from its name (16 or 32), or 0 for an unsupported name.
int roleGrid(const QString& fileName) {
    if (fileName.endsWith(QStringLiteral("-16.svg"))) {
        return 16;
    }
    if (fileName.endsWith(QStringLiteral("-32.svg"))) {
        return 32;
    }
    return 0;
}

/// The px left free of ink on a role grid (docs/ICONE.md).
int roleMargin(int grid) {
    return grid == 16 ? 1 : 2;
}

/// The icon name of a file: without .svg and without the grid suffix.
QString iconName(const QString& fileName) {
    const QString name = fileName.chopped(4);
    return roleGrid(fileName) == 0 ? name : name.chopped(3);
}

/// The icon drawn at kScale times its size on a transparent background.
QImage drawAtScale(QSvgRenderer& renderer, int grid) {
    const int side = grid * kScale;
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

InkCount countInk(const QImage& image, int marginPx) {
    InkCount count;
    const int margin = marginPx * kScale;
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

/// Whether the icon has both grids, except panel-only 16 px icons.
bool hasIconFile(const QString& icon) {
    return QFile::exists(resourcePath(icon + QStringLiteral("-16"))) &&
           (icon == QStringLiteral("layer-check") || icon == QStringLiteral("ui-pin") ||
            icon == QStringLiteral("ui-unpin") ||
            QFile::exists(resourcePath(icon + QStringLiteral("-32"))));
}

/// Writes `svg` as `name` in `directory`.
void writeSvg(const QTemporaryDir& directory, const QString& name, const QByteArray& svg) {
    QFile file(directory.filePath(name));
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(svg), svg.size());
}

/// A square icon of `grid` px whose whole area is filled in the role `role`.
QByteArray filledIcon(int grid, const char* role) {
    return QStringLiteral(R"(<svg xmlns="http://www.w3.org/2000/svg" width="%1" height="%1" )"
                          R"(viewBox="0 0 %1 %1"><rect class="%2" x="0" y="0" width="%1" )"
                          R"(height="%1" fill="currentColor"/></svg>)")
        .arg(grid)
        .arg(QString::fromLatin1(role))
        .toUtf8();
}

/// The colour at the centre of the icon drawn at `size` px.
QColor centreColour(const QIcon& icon, int size) {
    const QImage image = icon.pixmap(size, size).toImage();
    return image.pixelColor(image.width() / 2, image.height() / 2);
}

/// The actions that have no icon: the separators (no text), the recent files
/// (the submenu and the run-time list) and Quit (docs/tappe/0.6/P19).
bool needsNoIcon(const ActionSpec& spec) {
    return spec.text.isEmpty() || spec.id == QStringLiteral("file.quit") ||
           spec.id == QStringLiteral("file.recent") ||
           spec.id.startsWith(QStringLiteral("file.recent."));
}

TEST(IconsTest, RoleIconsAreValidSvgOnTheirGrid) {
    // The rules are checked by scripts/icone.py; here, what Qt draws.
    const QDir directory(QStringLiteral(TG_ICONS_DIR));
    int roleIcons = 0;
    for (const QString& name :
         directory.entryList({QStringLiteral("*.svg")}, QDir::Files, QDir::Name)) {
        const int grid = roleGrid(name);
        SCOPED_TRACE(name.toStdString());
        ASSERT_NE(grid, 0) << "only -16.svg and -32.svg icons are supported";
        EXPECT_TRUE(hasIconFile(iconName(name))) << "missing companion grid";
        ++roleIcons;
        QSvgRenderer renderer(directory.filePath(name));
        ASSERT_TRUE(renderer.isValid());
        EXPECT_EQ(renderer.defaultSize(), QSize(grid, grid));
        EXPECT_EQ(renderer.viewBoxF(), QRectF(0, 0, grid, grid));
        const InkCount ink = countInk(drawAtScale(renderer, grid), roleMargin(grid));
        EXPECT_GT(ink.total, 0) << "nothing is drawn";
        EXPECT_EQ(ink.inMargin, 0) << "ink in the margin";
        EXPECT_TRUE(QFile::exists(resourcePath(name.chopped(4)))) << "not in the resources";
    }
    // The migrated catalog has 62 pairs plus the panel-only check mark.
    EXPECT_GE(roleIcons, 2 * 62 + 1);
}

TEST(IconsTest, ResourceIconsAreReusedPerResolvedFilesAndTheme) {
    const QString name = QStringLiteral("draw-circle-cr");
    const QIcon light = themedIcon(name, Theme::Light);
    const QIcon dark = themedIcon(name, Theme::Dark);
    ASSERT_FALSE(light.isNull());
    ASSERT_FALSE(dark.isNull());
    EXPECT_EQ(themedIcon(name, Theme::Light).cacheKey(), light.cacheKey());
    EXPECT_EQ(themedIcon(name, Theme::Dark).cacheKey(), dark.cacheKey());
    EXPECT_NE(light.cacheKey(), dark.cacheKey());
    // The explicit resource base resolves to the same pair of files.
    EXPECT_EQ(themedIcon(QStringLiteral(":/tg/icons/draw-circle-cr"), Theme::Light).cacheKey(),
              light.cacheKey());
    // One explicit grid is a different pair and must not reuse the two-grid icon.
    const QIcon single = themedIcon(resourcePath(name + QStringLiteral("-16")), Theme::Light);
    ASSERT_FALSE(single.isNull());
    EXPECT_NE(single.cacheKey(), light.cacheKey());
    EXPECT_EQ(themedIcon(resourcePath(name + QStringLiteral("-16")), Theme::Light).cacheKey(),
              single.cacheKey());
}

TEST(IconsTest, MissingIconsStayEmptyOnRepeatedCalls) {
    const QString missing = QStringLiteral("missing-icon-for-cache-test");
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        for (const QString& name : {missing, resourcePath(missing)}) {
            EXPECT_TRUE(themedIcon(name, theme).isNull());
            EXPECT_TRUE(themedIcon(name, theme).isNull());
        }
    }
}

TEST(IconsTest, DiskIconsAreReadAgainAfterTheirFileChanges) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString fileName = QStringLiteral("changing.svg");
    const QString path = directory.filePath(fileName);
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        writeSvg(directory, fileName, filledIcon(32, "geo"));
        const QIcon first = themedIcon(path, theme);
        ASSERT_FALSE(first.isNull());
        const QImage before = first.pixmap(32, 32).toImage();
        EXPECT_EQ(before.pixelColor(16, 16), themeTokens(theme).iconGeo);
        // Keep the square and add a point in the centre, at the very same path.
        writeSvg(directory, fileName,
                 R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">)"
                 R"(<rect class="geo" width="32" height="32" fill="currentColor"/>)"
                 R"(<circle class="pt" cx="16" cy="16" r="6" fill="currentColor"/></svg>)");
        const QIcon second = themedIcon(path, theme);
        ASSERT_FALSE(second.isNull());
        EXPECT_NE(second.cacheKey(), first.cacheKey());
        const QImage after = second.pixmap(32, 32).toImage();
        EXPECT_NE(after, before);
        EXPECT_EQ(after.pixelColor(16, 16), themeTokens(theme).iconPt);
        EXPECT_EQ(after.pixelColor(2, 2), themeTokens(theme).iconGeo);
    }
}

TEST(IconsTest, EveryRoleHasTheColourOfItsTokenInBothThemes) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    // Squares of 6 px, one per case, in a 32 px icon drawn at 32 px; the
    // line is 4 px wide. The inner group is geo: its square without a class
    // is geo, its square of class pt is pt, though the group sets the fill.
    writeSvg(
        directory, QStringLiteral("roles.svg"),
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">)"
        R"(<g fill="currentColor">)"
        R"(<rect class="geo" x="1" y="1" width="6" height="6"/>)"
        R"(<rect class="pt" x="17" y="1" width="6" height="6"/>)"
        R"(<rect class="cost" x="1" y="9" width="6" height="6"/>)"
        R"(<line class="esist" x1="17" y1="12" x2="23" y2="12" stroke="currentColor" )"
        R"(stroke-width="4"/>)"
        R"(<rect x="1" y="17" width="6" height="6"/>)"
        R"(<rect class="accent" x="17" y="17" width="6" height="6"/>)"
        R"(<g class="geo"><rect x="1" y="25" width="6" height="6"/>)"
        R"(<rect class="pt" x="17" y="25" width="6" height="6"/></g>)"
        R"(</g></svg>)");
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        SCOPED_TRACE(theme == Theme::Dark ? "dark" : "light");
        const ThemeTokens& tokens = themeTokens(theme);
        const QIcon icon = themedIcon(directory.filePath(QStringLiteral("roles.svg")), theme);
        ASSERT_FALSE(icon.isNull());
        const QImage image = icon.pixmap(32, 32).toImage();
        ASSERT_EQ(image.size(), QSize(32, 32));
        EXPECT_EQ(image.pixelColor(4, 4), tokens.iconGeo);
        EXPECT_EQ(image.pixelColor(20, 4), tokens.iconPt);
        EXPECT_EQ(image.pixelColor(4, 12), tokens.iconCost);
        EXPECT_EQ(image.pixelColor(20, 12), tokens.iconEsist);
        EXPECT_EQ(image.pixelColor(4, 20), tokens.iconEsist) << "no class is esist";
        EXPECT_EQ(image.pixelColor(20, 20), tokens.iconPt) << "accent is pt";
        EXPECT_EQ(image.pixelColor(4, 28), tokens.iconGeo) << "the role of the group";
        EXPECT_EQ(image.pixelColor(20, 28), tokens.iconPt) << "its own role";
        EXPECT_EQ(image.pixelColor(12, 4).alpha(), 0);
    }
}

TEST(IconsTest, SmallSizesUseTheSixteenGridAndLargeOnesTheThirtyTwo) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    // Each file fills the icon in its own role, which tells them apart.
    writeSvg(directory, QStringLiteral("both-16.svg"), filledIcon(16, "geo"));
    writeSvg(directory, QStringLiteral("both-32.svg"), filledIcon(32, "pt"));
    writeSvg(directory, QStringLiteral("only32-32.svg"), filledIcon(32, "pt"));
    writeSvg(directory, QStringLiteral("only16-16.svg"), filledIcon(16, "geo"));
    // Unsuffixed legacy files are never a fallback, even without role files.
    writeSvg(directory, QStringLiteral("old.svg"), filledIcon(24, "cost"));
    writeSvg(directory, QStringLiteral("both.svg"), filledIcon(24, "cost"));
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        SCOPED_TRACE(theme == Theme::Dark ? "dark" : "light");
        const ThemeTokens& tokens = themeTokens(theme);
        const QIcon both = themedIcon(directory.filePath(QStringLiteral("both")), theme);
        EXPECT_EQ(centreColour(both, 16), tokens.iconGeo);
        EXPECT_EQ(centreColour(both, 24), tokens.iconGeo);
        EXPECT_EQ(centreColour(both, 32), tokens.iconPt);
        EXPECT_EQ(centreColour(both, 48), tokens.iconPt);
        const QIcon only32 = themedIcon(directory.filePath(QStringLiteral("only32")), theme);
        EXPECT_EQ(centreColour(only32, 16), tokens.iconPt);
        EXPECT_EQ(centreColour(only32, 32), tokens.iconPt);
        const QIcon only16 = themedIcon(directory.filePath(QStringLiteral("only16")), theme);
        EXPECT_EQ(centreColour(only16, 16), tokens.iconGeo);
        EXPECT_EQ(centreColour(only16, 32), tokens.iconGeo);
        const QIcon old = themedIcon(directory.filePath(QStringLiteral("old")), theme);
        EXPECT_TRUE(old.isNull());
        // The example of docs/ICONE.md, from the resources: at 32 px the
        // centre of the circle is its point, of class pt.
        const QIcon example = themedIcon(QStringLiteral("draw-circle-cr"), theme);
        ASSERT_FALSE(example.isNull());
        EXPECT_EQ(centreColour(example, 32), tokens.iconPt);
    }
    EXPECT_TRUE(themedIcon(directory.filePath(QStringLiteral("none")), Theme::Light).isNull());
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
        EXPECT_TRUE(hasIconFile(spec->icon))
            << "no resources/icons/" << spec->icon.toStdString() << ".svg";
        for (const Theme theme : {Theme::Light, Theme::Dark}) {
            EXPECT_FALSE(themedIcon(spec->icon, theme).isNull());
        }
    }
    EXPECT_GE(withIcon, kFirstIcons);
}

TEST(IconsTest, DrawingEditingAndFileActionsUseTheirOwnIcons) {
    ActionRegistry registry;
    addBuiltinActions(registry);
    // The icons of 0.6-P18: named like the action, with dashes for dots;
    // CIRCLE and ARC show the icon of their first variant (0.7.5-P19).
    const QList<std::pair<QString, QString>> expected{
        {QStringLiteral("draw.line"), QStringLiteral("draw-line")},
        {QStringLiteral("draw.pline"), QStringLiteral("draw-pline")},
        {QStringLiteral("draw.circle"), QStringLiteral("draw-circle-cr")},
        {QStringLiteral("draw.arc"), QStringLiteral("draw-arc-3p")},
        {QStringLiteral("draw.text"), QStringLiteral("draw-text")},
        {QStringLiteral("edit.erase"), QStringLiteral("edit-erase")},
        {QStringLiteral("edit.undo"), QStringLiteral("edit-undo")},
        {QStringLiteral("edit.redo"), QStringLiteral("edit-redo")},
        {QStringLiteral("file.new"), QStringLiteral("file-new")},
        {QStringLiteral("file.open"), QStringLiteral("file-open")},
        {QStringLiteral("file.save"), QStringLiteral("file-save")},
        {QStringLiteral("file.saveas"), QStringLiteral("file-saveas")}};
    EXPECT_EQ(expected.size(), kFirstIcons);
    for (const auto& [id, icon] : expected) {
        SCOPED_TRACE(id.toStdString());
        const ActionSpec* spec = registry.spec(id);
        ASSERT_NE(spec, nullptr);
        EXPECT_EQ(spec->icon, icon);
    }
}

TEST(IconsTest, DrawingCommandsAndTheirVariantsHaveRoleIconsOnBothGrids) {
    ActionRegistry registry;
    addBuiltinActions(registry);
    // docs/tappe/0.7.5/P19: the drawing commands and the variants of CIRCLE
    // and ARC, each with its own icon on the 16 and 32 px grids, and no
    // file of the old 24 px grid left.
    const QList<std::pair<QString, QString>> expected{
        {QStringLiteral("draw.line"), QStringLiteral("draw-line")},
        {QStringLiteral("draw.pline"), QStringLiteral("draw-pline")},
        {QStringLiteral("draw.text"), QStringLiteral("draw-text")},
        {QStringLiteral("draw.circle.center-radius"), QStringLiteral("draw-circle-cr")},
        {QStringLiteral("draw.circle.center-diameter"), QStringLiteral("draw-circle-cd")},
        {QStringLiteral("draw.circle.2-point"), QStringLiteral("draw-circle-2p")},
        {QStringLiteral("draw.circle.3-point"), QStringLiteral("draw-circle-3p")},
        {QStringLiteral("draw.circle.tan-tan-radius"), QStringLiteral("draw-circle-ttr")},
        {QStringLiteral("draw.arc.3-point"), QStringLiteral("draw-arc-3p")},
        {QStringLiteral("draw.arc.start-center-end"), QStringLiteral("draw-arc-sce")}};
    for (const auto& [id, icon] : expected) {
        SCOPED_TRACE(id.toStdString());
        const ActionSpec* spec = registry.spec(id);
        ASSERT_NE(spec, nullptr);
        EXPECT_EQ(spec->icon, icon);
        EXPECT_TRUE(QFile::exists(resourcePath(icon + QStringLiteral("-16"))));
        EXPECT_TRUE(QFile::exists(resourcePath(icon + QStringLiteral("-32"))));
        EXPECT_FALSE(QFile::exists(resourcePath(icon)));
    }
    EXPECT_EQ(registry.variants(QStringLiteral("draw.circle")).size(), 5);
    EXPECT_EQ(registry.variants(QStringLiteral("draw.arc")).size(), 2);
    for (const QString& old : {QStringLiteral("draw-circle"), QStringLiteral("draw-arc")}) {
        EXPECT_FALSE(QFile::exists(resourcePath(old))) << old.toStdString();
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
    // The pins of the panels that hide themselves (0.7.5-P18).
    used.append(autoHideIconNames());
    // The row of the tabs and the Start page (0.7.5-P29).
    used.append(documentTabsIconNames());

    const QDir directory(QStringLiteral(TG_ICONS_DIR));
    const QStringList names =
        directory.entryList({QStringLiteral("*.svg")}, QDir::Files, QDir::Name);
    for (const QString& name : names) {
        const QString icon = iconName(name);
        SCOPED_TRACE(icon.toStdString());
        EXPECT_TRUE(used.contains(icon)) << "neither an action nor a panel uses this icon";
    }
    // The icons the panels name are all drawn.
    for (const QString& icon : propertiesPanelIconNames()) {
        SCOPED_TRACE(icon.toStdString());
        EXPECT_TRUE(hasIconFile(icon)) << "icon not in the resources";
    }
}

}  // namespace
}  // namespace tg::app
