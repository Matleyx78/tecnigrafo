// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "main_window.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QPixmap>
#include <QTimer>
#include <QtGlobal>

int main(int argc, char* argv[]) {
    const QApplication application(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Tecnigrafo"));
    QApplication::setOrganizationName(QStringLiteral("Tecnigrafo"));
    QApplication::setApplicationVersion(QStringLiteral(TG_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Tecnigrafo - free 2D/3D CAD"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Render the window to <file> (PNG) and exit. Used by CI smoke tests."),
        QStringLiteral("file"));
    parser.addOption(screenshotOption);
    parser.process(application);

    tg::app::MainWindow window;
    window.show();

    if (parser.isSet(screenshotOption)) {
        const QString path = parser.value(screenshotOption);
        QTimer::singleShot(0, &window, [&window, path]() {
            const bool saved = window.grab().save(path, "PNG");
            if (!saved) {
                qWarning("Cannot write screenshot to %s", qPrintable(path));
            }
            QApplication::exit(saved ? 0 : 1);
        });
    }

    return QApplication::exec();
}
