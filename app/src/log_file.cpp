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
#include <memory>
#include <mutex>
#include <string_view>

// The sanitizer's own crash report is the better one.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define TG_LOG_UNDER_SANITIZER
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define TG_LOG_UNDER_SANITIZER
#endif
#endif
#if defined(Q_OS_WIN) && !defined(TG_LOG_UNDER_SANITIZER)
#define TG_LOG_CRASH_STACK
#include <algorithm>
#include <array>
#include <cstdint>
#include <io.h>
#include <qt_windows.h>
#elif (defined(__GLIBC__) || defined(__APPLE__)) && !defined(TG_LOG_UNDER_SANITIZER)
#define TG_LOG_CRASH_STACK
#include <array>
#include <csignal>
#include <cstddef>
#include <execinfo.h>
#include <unistd.h>
#endif

namespace {

constexpr std::string_view kCrashHeader{"fatal signal; the stack of the crash follows\n"};

#ifdef TG_LOG_CRASH_STACK
constexpr std::size_t kCrashFrames = 64;
#endif

}  // namespace

#if defined(TG_LOG_CRASH_STACK) && defined(Q_OS_WIN)
namespace {

// The exception filter can only get to the open log through process-wide state.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HANDLE crashLog = INVALID_HANDLE_VALUE;

void writeCrashText(std::string_view text) {
    DWORD written = 0;
    (void)WriteFile(crashLog, text.data(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void writeCrashFrame(const void* address) {
    HMODULE module = nullptr;
    // The path may have an extended-length prefix. Only the basename is logged.
    constexpr int kMaxWindowsPath = 32768;
    static std::array<wchar_t, kMaxWindowsPath> path{};
    constexpr int kModuleNameBytes = MAX_PATH * 4;
    std::array<char, kModuleNameBytes + 32> line{};
    int nameLength = 0;
    constexpr DWORD kModuleFlags =
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
    if (GetModuleHandleExW(kModuleFlags, reinterpret_cast<LPCWSTR>(address), &module) != 0) {
        const DWORD length =
            GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length > 0 && length < path.size()) {
            std::wstring_view name(path.data(), length);
            const auto slash = name.find_last_of(L"\\/");
            if (slash != std::wstring_view::npos) {
                name.remove_prefix(slash + 1);
            }
            const auto units = static_cast<int>(name.size());
            nameLength = WideCharToMultiByte(CP_UTF8, 0, name.data(), units, line.data(),
                                            kModuleNameBytes, nullptr, nullptr);
        }
    }
    if (nameLength == 0) {
        constexpr std::string_view kUnknown{"unknown"};
        std::copy(kUnknown.begin(), kUnknown.end(), line.begin());
        nameLength = static_cast<int>(kUnknown.size());
    }
    const auto offset =
        reinterpret_cast<std::uintptr_t>(address) - reinterpret_cast<std::uintptr_t>(module);
    const auto nameSize = static_cast<std::size_t>(nameLength);
    const auto suffixCapacity = line.size() - nameSize;
    const int size = std::snprintf(line.data() + nameSize, suffixCapacity, "+0x%llx\n",
                                   static_cast<unsigned long long>(offset));
    if (size > 0 && static_cast<std::size_t>(size) < suffixCapacity) {
        writeCrashText(std::string_view(line.data(), nameSize + static_cast<std::size_t>(size)));
    }
}

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* exception) {
    // If more than one thread crashes, only the first writes to the log.
    static LONG handling = 0;
    if (InterlockedCompareExchange(&handling, 1, 0) != 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    writeCrashText(kCrashHeader);
    const EXCEPTION_RECORD& record = *exception->ExceptionRecord;
    std::array<char, 64> description{};
    const int size = std::snprintf(
        description.data(), description.size(), "exception 0x%lx at 0x%llx\n", record.ExceptionCode,
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(record.ExceptionAddress)));
    if (size > 0 && static_cast<std::size_t>(size) < description.size()) {
        writeCrashText(std::string_view(description.data(), static_cast<std::size_t>(size)));
    }
    std::array<void*, kCrashFrames> frames{};
    const USHORT count =
        CaptureStackBackTrace(1, static_cast<ULONG>(frames.size()), frames.data(), nullptr);
    for (std::size_t i = 0; i < count; ++i) {
        writeCrashFrame(frames.at(i));
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void installCrashHandler(int descriptor) {
    if (descriptor >= 0) {
        crashLog = reinterpret_cast<HANDLE>(_get_osfhandle(descriptor));
        if (crashLog != INVALID_HANDLE_VALUE) {
            (void)SetUnhandledExceptionFilter(onUnhandledException);
        }
    }
}

}  // namespace
#elif defined(TG_LOG_CRASH_STACK)
namespace {

// The descriptor of the log, for the handler of the fatal signals, which
// can only reach state through a global; -1 before the log is open.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
volatile std::sig_atomic_t logDescriptor = -1;

// The handler runs here: a stack overflow leaves no room on the stack of
// the program (SA_ONSTACK).
constexpr std::size_t kSignalStackSize = std::size_t{64} * 1024;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<std::byte, kSignalStackSize> signalStack{};

}  // namespace

extern "C" {
/// Writes the stack of the crash to the log, then lets the signal end the
/// program as it would have (SA_RESETHAND gave it back its default action).
/// A signal handler has C linkage (cert-msc54-cpp), which an unnamed
/// namespace cannot give: it stays static.
// NOLINTNEXTLINE(misc-use-anonymous-namespace)
static void tgOnFatalSignal(int signalNumber) {
    const int descriptor = logDescriptor;
    if (descriptor >= 0) {
        // write() is async-signal-safe; nothing is left to do if it fails.
        [[maybe_unused]] const auto written =
            write(descriptor, kCrashHeader.data(), kCrashHeader.size());
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

/// Calls tgOnFatalSignal() for the signals of a crash, on a stack of its own;
/// it writes to `descriptor`, that of the log.
void installCrashHandler(int descriptor) {
    logDescriptor = descriptor;
    stack_t alternate{};
    alternate.ss_sp = signalStack.data();
    alternate.ss_size = signalStack.size();
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
    // Open until the program ends, unbuffered: what is written is in the
    // file, before a crash too.
    QFile* file{nullptr};
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

void writeLine(QFile& file, const QString& line) {
    (void)file.write(line.toUtf8() + '\n');
}

bool containsCrashHeader(QFile& previous) {
    if (!previous.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    while (!previous.atEnd()) {
        const QByteArray line = previous.readLine();
        if (line.isEmpty()) {
            break;
        }
        std::string_view text(line.constData(), static_cast<std::size_t>(line.size()));
        if (text.ends_with('\n')) {
            text.remove_suffix(1);
        }
        if (text == kCrashHeader.substr(0, kCrashHeader.size() - 1)) {
            return true;
        }
    }
    return false;
}

void logMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    LogState& state = logState();
    QtMessageHandler previous = nullptr;
    {
        const std::lock_guard lock(state.mutex);
        if (state.file != nullptr) {
            writeLine(*state.file,
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

LogStart startLogFile() {
    return startLogFileIn(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
}

LogStart startLogFileIn(const QString& folder) {
    if (folder.isEmpty() || !QDir().mkpath(folder)) {
        return {};
    }
    const QString logPath = QDir(folder).filePath(QStringLiteral("tecnigrafo.log"));
    const QString keptPath = logPath + QStringLiteral(".old");
    // The log of the session before; a log that cannot be kept is lost.
    (void)QFile::remove(keptPath);
    (void)QFile::rename(logPath, keptPath);
    LogStart result;
    QFile previousLog(keptPath);
    if (previousLog.exists()) {
        result.previousPath = keptPath;
        result.previousCrashed = containsCrashHeader(previousLog);
    }
    auto file = std::make_unique<QFile>(logPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text |
                    QIODevice::Unbuffered)) {
        return result;
    }
    LogState& state = logState();
    {
        const std::lock_guard lock(state.mutex);
        // The log stays open as long as the program runs.
        state.file = file.release();
        writeLine(*state.file,
                  QStringLiteral("%1 Tecnigrafo %2, Qt %3, %4, platform %5; log %6")
                      .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs),
                           QCoreApplication::applicationVersion(), QLatin1String(qVersion()),
                           QSysInfo::prettyProductName(), QGuiApplication::platformName(),
                           QDir::toNativeSeparators(logPath)));
    }
    const QtMessageHandler previous = qInstallMessageHandler(logMessage);
    {
        const std::lock_guard lock(state.mutex);
        state.previous = previous;
    }
#ifdef TG_LOG_CRASH_STACK
    installCrashHandler(state.file->handle());
#endif
    result.path = logPath;
    return result;
}

}  // namespace tg::app
