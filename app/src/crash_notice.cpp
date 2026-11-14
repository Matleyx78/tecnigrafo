// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "crash_notice.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>

namespace tg::app {

void showCrashNotice(QWidget& parent, const QString& previousPath) {
    QMessageBox notice(&parent);
    notice.setWindowTitle(QCoreApplication::applicationName());
    notice.setIcon(QMessageBox::Warning);
    notice.setTextFormat(Qt::PlainText);
    notice.setText(QCoreApplication::translate(
        "CrashNotice",
        "Tecnigrafo closed unexpectedly last time. The log of that session, with the stack of "
        "the crash, is in:"));
    notice.setInformativeText(QDir::toNativeSeparators(previousPath));
    const QString openFolderText = QCoreApplication::translate("CrashNotice", "Open folder");
    const QPushButton* openFolder = notice.addButton(openFolderText, QMessageBox::ActionRole);
    notice.setStandardButtons(QMessageBox::Ok);
    notice.setDefaultButton(QMessageBox::Ok);
    (void)notice.exec();
    if (notice.clickedButton() == openFolder) {
        const QString folder = QFileInfo(previousPath).absolutePath();
        (void)QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    }
}

}  // namespace tg::app
