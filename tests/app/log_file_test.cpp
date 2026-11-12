// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtGlobal>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

constexpr auto kCrashHeader = "fatal signal; the stack of the crash follows";
constexpr int kProcessTimeoutMs = 10000;

void runChild(QProcess& child, const QStringList& arguments) {
    child.start(QStringLiteral(TG_CRASH_CHILD), arguments);
    ASSERT_TRUE(child.waitForStarted(kProcessTimeoutMs)) << child.errorString().toStdString();
    ASSERT_TRUE(child.waitForFinished(kProcessTimeoutMs)) << child.errorString().toStdString();
}

QString readLog(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ADD_FAILURE() << file.errorString().toStdString();
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

QStringList readReport(QProcess& child) {
    return QString::fromUtf8(child.readAllStandardOutput())
        .replace(QStringLiteral("\r\n"), QStringLiteral("\n"))
        .split(QLatin1Char('\n'));
}

TEST(LogFileTest, TheLogNamesItselfAndKeepsTheMessagesOfTheSession) {
    const QTemporaryDir folder;
    ASSERT_TRUE(folder.isValid());
    const QString path = QDir(folder.path()).filePath(QStringLiteral("tecnigrafo.log"));
    QProcess child;
    ASSERT_NO_FATAL_FAILURE(runChild(child, {folder.path()}));
    ASSERT_EQ(child.exitStatus(), QProcess::NormalExit);
    ASSERT_EQ(child.exitCode(), 0) << child.readAllStandardError().constData();
    EXPECT_EQ(readReport(child), (QStringList{path, QString{}, QStringLiteral("0"), QString{}}));
    EXPECT_TRUE(child.readAllStandardError().contains("message of the log test, 42"));

    const QStringList lines = readLog(path).split(QLatin1Char('\n'));
    ASSERT_GE(lines.size(), 2);
    EXPECT_TRUE(lines.front().contains(QStringLiteral("Qt "))) << lines.front().toStdString();
    EXPECT_TRUE(lines.front().endsWith(QStringLiteral("tecnigrafo.log")))
        << lines.front().toStdString();
    EXPECT_TRUE(lines.at(1).endsWith(QStringLiteral("warning: message of the log test, 42")))
        << lines.at(1).toStdString();
}

TEST(LogFileTest, RotationDetectsOnlyTheCrashHeaderLine) {
    for (const bool crashed : {false, true}) {
        const QTemporaryDir folder;
        ASSERT_TRUE(folder.isValid());
        const QString path = QDir(folder.path()).filePath(QStringLiteral("tecnigrafo.log"));
        const QString previousPath = path + QStringLiteral(".old");
        const QByteArray marker = QByteArray(kCrashHeader) + '\n';
        const QByteArray contents = crashed ? marker : QByteArrayLiteral("warning: ") + marker;
        QFile oldLog(path);
        ASSERT_TRUE(oldLog.open(QIODevice::WriteOnly));
        ASSERT_EQ(oldLog.write(contents), contents.size());
        oldLog.close();

        QProcess child;
        ASSERT_NO_FATAL_FAILURE(runChild(child, {folder.path()}));
        ASSERT_EQ(child.exitStatus(), QProcess::NormalExit);
        ASSERT_EQ(child.exitCode(), 0) << child.readAllStandardError().constData();
        EXPECT_EQ(readReport(child),
                  (QStringList{path, previousPath, QString::number(crashed ? 1 : 0), QString{}}));
        EXPECT_EQ(readLog(previousPath), QString::fromUtf8(contents));
    }
}

TEST(LogFileTest, ACrashWritesAHeaderAndStackFrames) {
#if defined(TG_CRASH_TEST_UNDER_SANITIZER) || \
    (!defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(__GLIBC__))
    GTEST_SKIP() << "The crash handler is disabled on this build.";
#else
    const QTemporaryDir folder;
    ASSERT_TRUE(folder.isValid());
    const QString path = QDir(folder.path()).filePath(QStringLiteral("tecnigrafo.log"));
    QProcess child;
    ASSERT_NO_FATAL_FAILURE(runChild(child, {folder.path(), QStringLiteral("--crash")}));
    ASSERT_EQ(child.exitStatus(), QProcess::CrashExit);

    const QStringList lines = readLog(path).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const qsizetype header = lines.indexOf(QLatin1String(kCrashHeader));
    ASSERT_GE(header, 0);
#ifdef Q_OS_WIN
    const QRegularExpression exception(QStringLiteral("^exception 0x[cC]0000005 at 0x[0-9a-fA-F]+$"));
    ASSERT_LT(header + 1, lines.size());
    EXPECT_TRUE(exception.match(lines.at(header + 1)).hasMatch());
    const qsizetype firstFrame = header + 2;
    const QRegularExpression frame(QStringLiteral("^.+\\+0x[0-9a-fA-F]+$"));
#else
    const qsizetype firstFrame = header + 1;
    const QRegularExpression frame(QStringLiteral("0x[0-9a-fA-F]+"));
#endif
    ASSERT_LT(firstFrame, lines.size());
    EXPECT_LE(lines.size() - firstFrame, 64);
    for (qsizetype i = firstFrame; i < lines.size(); ++i) {
        EXPECT_TRUE(frame.match(lines.at(i)).hasMatch()) << lines.at(i).toStdString();
    }
#endif
}

}  // namespace
}  // namespace tg::app
