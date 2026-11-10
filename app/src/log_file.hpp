// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QString>

namespace tg::app {

/// The log of the program: what Qt and the program report (qDebug(),
/// qWarning(), qCritical(), qFatal()), each line with its time and kind,
/// and, on Linux and macOS, the stack of a crash. It is written to
/// tecnigrafo.log in the folder of the program's data
/// (QStandardPaths::AppLocalDataLocation: on Linux ~/.local/share/
/// Tecnigrafo/Tecnigrafo), which the first line names; the log of the
/// session before is kept as tecnigrafo.log.old. Messages also go where they
/// went before (the terminal). Call it once, after QApplication has its
/// name; it returns the path of the log, empty if it cannot be written.
QString startLogFile();

}  // namespace tg::app
