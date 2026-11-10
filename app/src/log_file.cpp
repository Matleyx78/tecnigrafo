// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "log_file.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QSysInfo>
#include <QtGlobal>

#include <cstdio>
#include <mutex>

// The stack of a crash: glibc and macOS have backtrace(). Not under a
// sanitizer, whose own report of the crash is the better one.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define TG_LOG_UNDER_SANITIZER
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define TG_LOG_UNDER_SANITIZER
#endif
#endif
#if (defined(__GLIBC__) || defined(__APPLE__)) && !defined(TG_LOG_UNDER_SANITIZER)
#define TG_LOG_CRASH_STACK
#include <array>
#include <csignal>
#include <cstddef>
#include <execinfo.h>
#include <string_view>
#include <unistd.h>
#endif

#ifdef TG_LOG_CRASH_STACK
namespace {

// The descriptor of the log, for the handler of the fatal signals, which
// can only reach state through a global; -1 before the log is open.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile std::sig_atomic_t g_logDescriptor = -1;

// The handler runs here: a stack overflow leaves no room on the stack of
// the program (SA_ONSTACK).
constexpr std::size_t kSignalStackSize = 64 * 1024;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<std::byte, kSignalStackSize> g_signalStack{};

constexpr std::size_t kCrashFrames = 64;

}  // namespace

extern "C" {
/// Writes the stack of the crash to the log, then lets the signal end the
/// program as it would have (SA_RESETHAND gave it back its default action).
/// A signal handler has C linkage (cert-msc54-cpp), which an unnamed
/// namespace cannot give: it stays static.
// NOLINTNEXTLINE(misc-use-anonymous-namespace)
static void tgOnFatalSignal(int signalNumber) {
    const int descriptor = g_logDescriptor;
    if (descriptor >= 0) {
        constexpr std::string_view kHeader{"fatal signal; the stack of the crash follows\n"};
        // write() is async-signal-safe; nothing is left to do if it fails.
        [[maybe_unused]] const auto written = write(descriptor, kHeader.data(), kHeader.size());
        std::array<void*, kCrashFrames> frames{};
        // backtrace() and backtrace_symbols_fd() are not in the POSIX list
        // of async-signal-safe functions, but glibc and macOS document them
        // for this use: they write straight to the descriptor, and the
        // program ends right after.
        // NOLINTNEXTLINE(bugprone-signal-handler,cert-sig30-c)
        const int count = backtrace(frames.data(), static_cast<int>(frames.size()));
        // NOLINTNEXTLINE(bugprone-signal-handler,cert-sig30-c)
        backtrace_symbols_fd(frames.data(), count, descriptor);
    }
    (void)raise(signalNumber);
}
}

namespace {

/// Calls tgOnFatalSignal() for the signals of a crash, on a stack of its own.
void installCrashHandler(std::FILE* stream) {
    g_logDescriptor = fileno(stream);
    stack_t alternate{};
    alternate.ss_sp = g_signalStack.data();
    alternate.ss_size = g_signalStack.size();
    if (sigaltstack(&alternate, nullptr) != 0) {
        return;
    }

    struct sigaction action {};

    // sa_handler is a member of a union in struct sigaction (POSIX).
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    action.sa_handler = tgOnFatalSignal;
    (void)sigemptyset(&action.sa_mask);
    // SA_RESETHAND is the sign bit on Linux: sa_flags is an int.
    action.sa_flags = static_cast<int>(static_cast<unsigned int>(SA_ONSTACK | SA_RESETHAND));
    for (const int signalNumber : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT}) {
        if (sigaction(signalNumber, &action, nullptr) != 0) {
            return;
        }
    }
}

}  // namespace
#endif

namespace tg::app {
namespace {

struct LogState {
    std::mutex mutex;
    std::FILE* stream{nullptr};
    QtMessageHandler previous{nullptr};
};

LogState& logState() {
    static LogState state;
    return state;
}

const char* kindName(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:
            return "debug";
        case QtInfoMsg:
            return "info";
        case QtWarningMsg:
            return "warning";
        case QtCriticalMsg:
            return "critical";
        case QtFatalMsg:
            return "fatal";
    }
    return "message";
}

void writeLine(std::FILE* stream, const QString& line) {
    const QByteArray bytes = line.toUtf8();
    (void)std::fputs(bytes.constData(), stream);
    (void)std::fputc('\n', stream);
    (void)std::fflush(stream);
}

void logMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    LogState& state = logState();
    QtMessageHandler previous = nullptr;
    {
        const std::lock_guard lock(state.mutex);
        if (state.stream != nullptr) {
            writeLine(state.stream,
                      QStringLiteral("%1 %2: %3")
                          .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                               QLatin1String(kindName(type)), message));
        }
        previous = state.previous;
    }
    // Where the message went before: the terminal.
    if (previous != nullptr) {
        previous(type, context, message);
    } else {
        const QByteArray text = qFormatLogMessage(type, context, message).toLocal8Bit();
        (void)std::fputs(text.constData(), stderr);
        (void)std::fputc('\n', stderr);
    }
}

}  // namespace

QString startLogFile() {
    const QString folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (folder.isEmpty() || !QDir().mkpath(folder)) {
        return {};
    }
    const QString path = QDir(folder).filePath(QStringLiteral("tecnigrafo.log"));
    const QString old = path + QStringLiteral(".old");
    // The log of the session before; a log that cannot be kept is lost.
    (void)QFile::remove(old);
    (void)QFile::rename(path, old);
    std::FILE* stream = std::fopen(QFile::encodeName(path).constData(), "w");
    if (stream == nullptr) {
        return {};
    }
    LogState& state = logState();
    {
        const std::lock_guard lock(state.mutex);
        state.stream = stream;
        writeLine(stream,
                  QStringLiteral("%1 Tecnigrafo %2, Qt %3, %4, platform %5; log %6")
                      .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                           QCoreApplication::applicationVersion(), QLatin1String(qVersion()),
                           QSysInfo::prettyProductName(), QGuiApplication::platformName(),
                           QDir::toNativeSeparators(path)));
    }
    const QtMessageHandler previous = qInstallMessageHandler(logMessage);
    {
        const std::lock_guard lock(state.mutex);
        state.previous = previous;
    }
#ifdef TG_LOG_CRASH_STACK
    installCrashHandler(stream);
#endif
    return path;
}

}  // namespace tg::app
