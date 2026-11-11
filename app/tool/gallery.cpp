// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "main_window.hpp"
#include "theme.hpp"

#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QKeyEvent>
#include <QLineEdit>
#include <QList>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPixmap>
#include <QResource>
#include <QSettings>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QWidget>

#include <SARibbon.h>
#include <iostream>

// Keep the font resource in static builds before the first window is created.
void initializeGalleryResources() {
    Q_INIT_RESOURCE(fonts);
}

namespace {

constexpr int kWindowWidth = 1600;
constexpr int kWindowHeight = 1000;
constexpr int kDockWidth = 360;

bool saveWidget(QWidget* widget, const QDir& output, const QString& name) {
    if (widget == nullptr || !widget->isVisible() || widget->size().isEmpty() ||
        !widget->grab().save(output.filePath(name), "PNG")) {
        std::cerr << "Cannot capture " << name.toStdString() << '\n';
        return false;
    }
    return true;
}

// Points QSettings at a new empty folder. Each window needs its own: a shown
// MainWindow saves its interface and dock layout, and the next window would
// restore them, so the second theme would inherit the first one's layout.
bool useEmptySettings(const QTemporaryDir& settings) {
    if (!settings.isValid()) {
        std::cerr << "Cannot create temporary settings folder\n";
        return false;
    }
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settings.path());
    return true;
}

// Fixes the split of the two docks. QMainWindow sizes them from size hints,
// and those depend on what the process did before (the second window of a
// run differs from the first), so captures would not line up between themes
// or between pull requests.
void pinDocks(tg::app::MainWindow& window) {
    auto* layers = window.findChild<QDockWidget*>(QStringLiteral("layerPanel"));
    auto* properties = window.findChild<QDockWidget*>(QStringLiteral("propertiesPanel"));
    if (layers == nullptr || properties == nullptr) {
        return;  // saveWidget reports the missing panel.
    }
    const QList<QDockWidget*> docks{layers, properties};
    window.resizeDocks(docks, {kDockWidth, kDockWidth}, Qt::Horizontal);
    const int half = (layers->height() + properties->height()) / 2;
    window.resizeDocks(docks, {half, half}, Qt::Vertical);
    QApplication::processEvents();
}

bool captureTheme(tg::app::Theme theme, const QString& suffix, const QDir& output) {
    const QTemporaryDir settings;
    if (!useEmptySettings(settings)) {
        return false;
    }
    // The ribbon's own frame, as the platform offscreen of the CI gives it.
    tg::app::MainWindow window(tg::app::MainWindow::InitialDrawing::Sample, nullptr, {},
                               tg::app::WindowFrame::Own);
    // Size before the first layout, so docks are split for the final size.
    window.resize(kWindowWidth, kWindowHeight);
    window.show();
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
    pinDocks(window);
    if (window.selection().size() != 1U || window.size() != QSize(kWindowWidth, kWindowHeight) ||
        window.devicePixelRatioF() != 1.0) {
        std::cerr << "Unexpected gallery geometry, scale or selection\n";
        return false;
    }
    const auto save = [&output, &suffix](QWidget* widget, const QString& stem) {
        return saveWidget(widget, output,
                          stem + QLatin1Char('-') + suffix + QStringLiteral(".png"));
    };
    if (!save(window.menuBar(), QStringLiteral("ribbon")) ||
        !save(window.findChild<QWidget*>(QStringLiteral("propertiesPanel")),
              QStringLiteral("proprieta")) ||
        !save(window.findChild<QWidget*>(QStringLiteral("layerPanel")), QStringLiteral("layer")) ||
        !save(window.statusBar(), QStringLiteral("stato"))) {
        return false;
    }
    // The whole window with a command in progress, as in the reference:
    // MOVE on the selection, typed in the command line (area 6). After the
    // panels, which MOVE leaves without a selection.
    auto* input = window.findChild<QLineEdit*>(QStringLiteral("input"));
    if (input == nullptr) {
        std::cerr << "Cannot find the command line\n";
        return false;
    }
    input->setText(QStringLiteral("MOVE"));
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(input, &enter);
    QApplication::processEvents();
    if (!save(&window, QStringLiteral("finestra"))) {
        return false;
    }
    window.setInterface(tg::app::InterfaceKind::Classic);
    QApplication::processEvents();
    pinDocks(window);
    return save(&window, QStringLiteral("finestra-classica"));
}

// The ribbon of a window that keeps the frame of the system, as under
// Wayland (docs/tappe/0.7.5/P40, point 5): the quick access bar at the left
// of File, which the tool checks before it saves the image.
bool captureSystemFrame(tg::app::Theme theme, const QString& suffix, const QDir& output) {
    const QTemporaryDir settings;
    if (!useEmptySettings(settings)) {
        return false;
    }
    tg::app::MainWindow window(tg::app::MainWindow::InitialDrawing::Sample, nullptr, {},
                               tg::app::WindowFrame::System);
    window.resize(kWindowWidth, kWindowHeight);
    window.show();
    window.setTheme(theme);
    window.setInterface(tg::app::InterfaceKind::Ribbon);
    // The ribbon lays its corner widget out again once the events in
    // progress are over (UiBuilder).
    for (int round = 0; round < 3; ++round) {
        QApplication::processEvents();
    }
    SARibbonBar* ribbon = window.ribbonBar();
    const QWidget* quick = ribbon != nullptr ? ribbon->cornerWidget(Qt::TopLeftCorner) : nullptr;
    const QWidget* file = ribbon != nullptr ? ribbon->applicationButton() : nullptr;
    if (window.windowButtonBar() != nullptr || quick == nullptr || file == nullptr ||
        !quick->isVisible() || quick->geometry().right() > file->geometry().left()) {
        std::cerr << "The quick access bar is not at the left of File\n";
        return false;
    }
    return saveWidget(ribbon, output,
                      QStringLiteral("ribbon-cornice-sistema-") + suffix + QStringLiteral(".png"));
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
    // Never the user's settings: each theme gets an empty folder (captureTheme).
    QSettings::setDefaultFormat(QSettings::IniFormat);
    initializeGalleryResources();
    const QDir output(arguments.at(1));
    if (!QDir().mkpath(output.absolutePath())) {
        std::cerr << "Cannot create gallery output folder\n";
        return 1;
    }
    return captureTheme(tg::app::Theme::Light, QStringLiteral("chiaro"), output) &&
                   captureTheme(tg::app::Theme::Dark, QStringLiteral("scuro"), output) &&
                   captureSystemFrame(tg::app::Theme::Light, QStringLiteral("chiaro"), output) &&
                   captureSystemFrame(tg::app::Theme::Dark, QStringLiteral("scuro"), output)
               ? 0
               : 1;
}
