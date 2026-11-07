// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "main_window.hpp"
#include "theme.hpp"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPixmap>
#include <QResource>
#include <QSettings>
#include <QStatusBar>
#include <QTemporaryDir>

#include <iostream>

// Keep the font resource in static builds before the first window is created.
void initializeGalleryResources() {
    Q_INIT_RESOURCE(fonts);
}

namespace {

constexpr int kWindowWidth = 1600;
constexpr int kWindowHeight = 1000;

bool saveWidget(QWidget* widget, const QDir& output, const QString& name) {
    if (widget == nullptr || !widget->isVisible() || widget->size().isEmpty() ||
        !widget->grab().save(output.filePath(name), "PNG")) {
        std::cerr << "Cannot capture " << name.toStdString() << '\n';
        return false;
    }
    return true;
}

bool captureTheme(tg::app::Theme theme, const QString& suffix, const QDir& output) {
    tg::app::MainWindow window;
    window.show();
    window.resize(kWindowWidth, kWindowHeight);
    window.setTheme(theme);
    window.setInterface(tg::app::InterfaceKind::Ribbon);
    QApplication::processEvents();
    window.view()->zoomExtents();

    // Pick the midpoint of the sample plate's bottom edge through the same
    // input path as a user, so the properties panel and selection stay in sync.
    auto* view = window.view();
    const auto pixel = view->viewport().worldToScreen({60.0, 0.0});
    const QPointF local(pixel.x, pixel.y);
    const QPointF global = view->mapToGlobal(local);
    QMouseEvent press(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QApplication::sendEvent(view, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, global, Qt::LeftButton, Qt::NoButton,
                        Qt::NoModifier);
    QApplication::sendEvent(view, &release);
    QApplication::processEvents();
    if (window.selection().size() != 1U || window.size() != QSize(kWindowWidth, kWindowHeight) ||
        window.devicePixelRatioF() != 1.0) {
        std::cerr << "Unexpected gallery geometry, scale or selection\n";
        return false;
    }

    const auto save = [&output, &suffix](QWidget* widget, const QString& stem) {
        return saveWidget(widget, output,
                          stem + QLatin1Char('-') + suffix + QStringLiteral(".png"));
    };
    if (!save(&window, QStringLiteral("finestra")) ||
        !save(window.menuBar(), QStringLiteral("ribbon")) ||
        !save(window.findChild<QWidget*>(QStringLiteral("propertiesPanel")),
              QStringLiteral("proprieta")) ||
        !save(window.findChild<QWidget*>(QStringLiteral("layerPanel")), QStringLiteral("layer")) ||
        !save(window.statusBar(), QStringLiteral("stato"))) {
        return false;
    }
    window.setInterface(tg::app::InterfaceKind::Classic);
    QApplication::processEvents();
    return save(&window, QStringLiteral("finestra-classica"));
}

}  // namespace

int main(int argc, char* argv[]) {
    // Set before QApplication: inherited desktop scaling must not change PNG sizes.
    qputenv("QT_SCALE_FACTOR", "1");
    qputenv("QT_SCREEN_SCALE_FACTORS", "1");
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
    const QApplication application(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Tecnigrafo"));
    QApplication::setApplicationName(QStringLiteral("Tecnigrafo-galleria"));
    const auto arguments = QApplication::arguments();
    if (arguments.size() != 2) {
        std::cerr << "Usage: tecnigrafo-galleria <folder>\n";
        return 1;
    }
    const QTemporaryDir settings;
    if (!settings.isValid()) {
        std::cerr << "Cannot create temporary settings folder\n";
        return 1;
    }
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settings.path());
    initializeGalleryResources();
    const QString fontPath = QStringLiteral(":/tg/fonts/LiberationSans-Regular.ttf");
    const int fontId = QFontDatabase::addApplicationFont(fontPath);
    const auto families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        std::cerr << "Cannot load the application's bundled font\n";
        return 1;
    }
    QFont font(families.first());
    font.setPixelSize(13);
    QApplication::setFont(font);
    const QDir output(arguments.at(1));
    if (!QDir().mkpath(output.absolutePath())) {
        std::cerr << "Cannot create gallery output folder\n";
        return 1;
    }
    return captureTheme(tg::app::Theme::Light, QStringLiteral("chiaro"), output) &&
                   captureTheme(tg::app::Theme::Dark, QStringLiteral("scuro"), output)
               ? 0
               : 1;
}
