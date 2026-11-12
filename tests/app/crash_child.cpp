// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "log_file.hpp"

#include <QByteArray>
#include <QGuiApplication>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

#include <cstdio>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {

void writeTestMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    const QByteArray text = qFormatLogMessage(type, context, message).toUtf8();
    (void)std::fputs(text.constData(), stderr);
    (void)std::fputc('\n', stderr);
}

}  // namespace

int main(int argc, char* argv[]) {
    const QGuiApplication application(argc, argv);
    const QStringList arguments = QGuiApplication::arguments();
    if (arguments.size() < 2) {
        return 1;
    }
    // The default Qt diagnostic destination depends on the platform.
    (void)qInstallMessageHandler(writeTestMessage);
    const tg::app::LogStart log = tg::app::startLogFileIn(arguments.at(1));
    if (log.path.isEmpty()) {
        return 2;
    }
    QTextStream report(stdout);
    report << log.path << '\n' << log.previousPath << '\n';
    report << (log.previousCrashed ? 1 : 0) << Qt::endl;
    qWarning("message of the log test, %d", 42);

    if (arguments.size() == 3 && arguments.at(2) == QStringLiteral("--crash")) {
#ifdef Q_OS_WIN
        // Windows must terminate the child without a blocking error dialog.
        (void)SetErrorMode(SEM_NOGPFAULTERRORBOX);
#endif
        // A volatile read keeps the deliberate fault in optimised builds.
        volatile int* const volatile address = nullptr;
        *address = 1;
    }
    return 0;
}
