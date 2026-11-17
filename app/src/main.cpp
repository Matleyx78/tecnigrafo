// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "assistant_bridge.hpp"
#include "cad_view.hpp"
#include "crash_notice.hpp"
#include "log_file.hpp"
#include "main_window.hpp"
#include "script_session.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QPixmap>
#include <QTimer>
#include <QtGlobal>

#include <iostream>
#include <string_view>

namespace {

/// The option of the bridge of the assistant (docs/adr/0051, point 1).
constexpr std::string_view kBridgeOption = "--mcp";

void nameApplication() {
    QCoreApplication::setApplicationName(QStringLiteral("Tecnigrafo"));
    QCoreApplication::setOrganizationName(QStringLiteral("Tecnigrafo"));
    QCoreApplication::setApplicationVersion(QStringLiteral(TG_VERSION));
}

}  // namespace

int main(int argc, char* argv[]) {
    // The bridge shows no window: it needs no QApplication, which on Linux
    // would ask for a display that an assistant may not have.
    if (argc == 2 && argv[1] == kBridgeOption) {
        const QCoreApplication bridge(argc, argv);
        nameApplication();
        return tg::app::runAssistantBridge();
    }
    const QApplication application(argc, argv);
    nameApplication();

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
    // Read above, before the QApplication: here only for --help, and for an
    // error if other options come with it.
    const QCommandLineOption bridgeOption(
        QStringLiteral("mcp"),
        QStringLiteral("Run the bridge of the assistant: MCP on stdin and stdout, passed on to "
                       "the assistant channel of the running program, until stdin ends. It "
                       "takes no other option."));
    parser.addOption(bridgeOption);
    parser.process(application);

    if (parser.isSet(bridgeOption)) {
        std::cerr << "--mcp takes no other option\n";
        return 1;
    }

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

    // The log of an interactive session: what Qt and the program report,
    // and the stack of a crash (log_file.hpp). Scripts and screenshots of
    // the CI run without it.
    const bool screenshot = parser.isSet(screenshotOption);
    const tg::app::LogStart log = screenshot ? tg::app::LogStart{} : tg::app::startLogFile();

    // The program opens on the Start page, without documents, as AutoCAD
    // does (docs/DECISIONI.md, 2026-11-07).
    tg::app::MainWindow window(tg::app::MainWindow::InitialDrawing::None);
    window.show();

    if (screenshot) {
        const QString path = parser.value(screenshotOption);
        QTimer::singleShot(0, &window, [&window, path]() {
            const bool saved = window.grab().save(path, "PNG");
            if (!saved) {
                qWarning("Cannot write screenshot to %s", qPrintable(path));
            }
            QApplication::exit(saved ? 0 : 1);
        });
    } else if (log.previousCrashed) {
        QTimer::singleShot(0, &window, [&window, path = log.previousPath]() {
            tg::app::showCrashNotice(window, path);
        });
    }

    return QApplication::exec();
}
