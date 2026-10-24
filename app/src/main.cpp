// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "main_window.hpp"
#include "script_session.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QPixmap>
#include <QTimer>
#include <QtGlobal>

#include <iostream>

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
    // The control mode at level 2 (docs/adr/0011, docs/verifica/script.md).
    const QCommandLineOption scriptOption(
        QStringLiteral("script"),
        QStringLiteral("Run the control mode script <file> on an empty drawing, write the report "
                       "and the screenshots, and exit."),
        QStringLiteral("file"));
    const QCommandLineOption outputOption(
        QStringLiteral("output"),
        QStringLiteral("Folder for the report and the screenshots of --script (default: the "
                       "current folder)."),
        QStringLiteral("folder"), QStringLiteral("."));
    const QCommandLineOption sourcesOption(
        QStringLiteral("sources"), QStringLiteral("Folder of pinned source files for --script."),
        QStringLiteral("folder"));
    parser.addOption(sourcesOption);
    parser.addOption(scriptOption);
    parser.addOption(outputOption);
    parser.process(application);

    if (parser.isSet(scriptOption)) {
        if (parser.isSet(screenshotOption)) {
            std::cerr << "--script and --screenshot cannot be used together\n";
            return 1;
        }
        return tg::app::runScriptFile(parser.value(scriptOption), parser.value(outputOption));
    }
    if (parser.isSet(sourcesOption)) {
        std::cerr << "--sources needs --script\n";
        return 1;
    }
    if (parser.isSet(outputOption)) {
        std::cerr << "--output needs --script\n";
        return 1;
    }

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
