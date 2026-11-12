// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "log_file.hpp"

#include <QGuiApplication>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

#include <cstdint>
#include <cstdio>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

int main(int argc, char* argv[]) {
    const QGuiApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    if (arguments.size() < 2) {
        return 1;
    }
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
        const volatile std::uintptr_t address = 0;
        // This child deliberately faults at the null address to exercise the crash handler.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        *reinterpret_cast<volatile int*>(address) = 1;
    }
    return 0;
}
