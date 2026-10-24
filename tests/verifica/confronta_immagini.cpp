// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// confronta_immagini: the test tool of level 2 of the control mode
// (docs/adr/0011, points 6 and 8; docs/verifica/casi.md). A development
// tool, never installed with the program.
//
//   confronta_immagini images <expected.png> <obtained.png>
//                      [--channel <0-255>] [--pixels <percent>]
//                      [--difference <file.png>]
//   confronta_immagini case <case folder> --program <tecnigrafo>
//                      --output <folder> [--pending]
//
// "images" compares two images: a pixel differs if one channel differs by
// more than --channel (default 8), and the images differ if more than
// --pixels percent of the pixels differ (default 0.1), or if their sizes
// differ. --difference writes the pixels that differ in red.
//
// "case" runs a case at level 2: `tecnigrafo --script` writes the report
// and the screenshots in the output folder; the report is compared with
// atteso.txt, and every atteso-<name>.png of the case with <name>.png, with
// the tolerances of the script. --pending reverses the outcome, as for
// tecnigrafo-script (tests/verifica/in-attesa.txt).
//
// Exit codes, as for tecnigrafo-script: 0 passed (or equal), 1 not passed,
// 2 invalid case or arguments.

#include "tg/script/compare.hpp"
#include "tg/script/script.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {

namespace script = tg::script;

constexpr int kExitPassed = 0;
constexpr int kExitFailed = 1;
constexpr int kExitInvalid = 2;

constexpr const char* kUsage =
    "usage:\n"
    "  confronta_immagini images <expected.png> <obtained.png> [--channel <0-255>]\n"
    "                     [--pixels <percent>] [--difference <file.png>]\n"
    "  confronta_immagini case <case folder> --program <tecnigrafo> --output <folder>\n"
    "                     [--pending]\n"
    "\n"
    "Exit codes: 0 passed, 1 not passed, 2 invalid case or arguments.\n"
    "See docs/verifica/casi.md.\n";

// Files of a case and of the level 2 output (docs/verifica/casi.md).
constexpr const char* kScriptFile = "script.tgs";
constexpr const char* kExpectedFile = "atteso.txt";
constexpr const char* kObtainedReport = "rapporto.txt";
constexpr const char* kExpectedImagePrefix = "atteso-";
constexpr const char* kDifferenceImagePrefix = "differenza-";
constexpr std::string_view kPendingList = "tests/verifica/in-attesa.txt";

// A run of the program longer than this is a failure: a script that hangs
// must not block the CI.
constexpr int kProgramTimeoutMilliseconds = 120000;

// Differences of the report printed for one case, as tecnigrafo-script.
constexpr std::size_t kMaxPrintedDifferences = 20;

constexpr int kMaxChannel = 255;
constexpr double kMaxPercent = 100.0;

/// The outcome of comparing two images.
struct ImageComparison {
    bool sameSize{false};
    std::int64_t differing{0};  ///< pixels with a channel beyond the tolerance
    std::int64_t total{0};
    QImage difference;  ///< the expected image, pale, with the differing pixels red
};

int channelDistance(int expected, int obtained) {
    return expected > obtained ? expected - obtained : obtained - expected;
}

ImageComparison compareImages(const QImage& expectedImage, const QImage& obtainedImage,
                              int channel) {
    ImageComparison result;
    if (expectedImage.size() != obtainedImage.size()) {
        return result;
    }
    result.sameSize = true;
    const QImage expected = expectedImage.convertToFormat(QImage::Format_ARGB32);
    const QImage obtained = obtainedImage.convertToFormat(QImage::Format_ARGB32);
    result.difference = expected;
    result.total = static_cast<std::int64_t>(expected.width()) * expected.height();
    for (int y = 0; y < expected.height(); ++y) {
        for (int x = 0; x < expected.width(); ++x) {
            const QRgb a = expected.pixel(x, y);
            const QRgb b = obtained.pixel(x, y);
            const bool differs = channelDistance(qRed(a), qRed(b)) > channel ||
                                 channelDistance(qGreen(a), qGreen(b)) > channel ||
                                 channelDistance(qBlue(a), qBlue(b)) > channel ||
                                 channelDistance(qAlpha(a), qAlpha(b)) > channel;
            if (differs) {
                ++result.differing;
                result.difference.setPixel(x, y, qRgb(kMaxChannel, 0, 0));
            } else {
                // A quarter of the expected colour over white: the drawing
                // stays readable around the red pixels.
                const int grey = qGray(a);
                const int pale = kMaxChannel - ((kMaxChannel - grey) / 4);
                result.difference.setPixel(x, y, qRgb(pale, pale, pale));
            }
        }
    }
    return result;
}

/// True if the images are equal within the tolerances: same size, and at
/// most `pixels` percent of the pixels differ.
bool imagesMatch(const ImageComparison& comparison, double pixels) {
    if (!comparison.sameSize) {
        return false;
    }
    if (comparison.total == 0) {
        return true;
    }
    const double percent =
        100.0 * static_cast<double>(comparison.differing) / static_cast<double>(comparison.total);
    return percent <= pixels;
}

std::string sizeText(const QImage& image) {
    return std::to_string(image.width()) + "x" + std::to_string(image.height());
}

/// One line that says how two images differ, or are equal.
std::string describe(const ImageComparison& comparison, const QImage& expected,
                     const QImage& obtained) {
    if (!comparison.sameSize) {
        return "size " + sizeText(obtained) + ", expected " + sizeText(expected);
    }
    const double percent = comparison.total == 0
                               ? 0.0
                               : 100.0 * static_cast<double>(comparison.differing) /
                                     static_cast<double>(comparison.total);
    return std::to_string(comparison.differing) + " of " + std::to_string(comparison.total) +
           " pixels differ (" + QString::number(percent, 'f', 4).toStdString() + "%)";
}

std::optional<QImage> loadImage(const QString& path) {
    QImage image;
    if (!image.load(path)) {
        return std::nullopt;
    }
    return image;
}

std::optional<std::string> readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const QByteArray bytes = file.readAll();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

int usageError() {
    std::cerr << kUsage;
    return kExitInvalid;
}

// ---- images ----------------------------------------------------------------

int compareImageFiles(const QStringList& arguments) {
    std::vector<QString> files;
    script::Tolerances tolerances;
    std::optional<QString> differencePath;
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        const QString& argument = arguments[i];
        const bool hasValue = i + 1 < arguments.size();
        bool ok = false;
        if (argument == QLatin1String("--channel") && hasValue) {
            tolerances.channel = arguments[++i].toInt(&ok);
            if (!ok || tolerances.channel < 0 || tolerances.channel > kMaxChannel) {
                return usageError();
            }
        } else if (argument == QLatin1String("--pixels") && hasValue) {
            tolerances.pixels = arguments[++i].toDouble(&ok);
            if (!ok || tolerances.pixels < 0.0 || tolerances.pixels > kMaxPercent) {
                return usageError();
            }
        } else if (argument == QLatin1String("--difference") && hasValue && !differencePath) {
            differencePath = arguments[++i];
        } else if (!argument.startsWith(QLatin1String("--")) && files.size() < 2) {
            files.push_back(argument);
        } else {
            return usageError();
        }
    }
    if (files.size() != 2) {
        return usageError();
    }
    const std::optional<QImage> expected = loadImage(files[0]);
    const std::optional<QImage> obtained = loadImage(files[1]);
    if (!expected || !obtained) {
        std::cerr << "cannot read the image " << (expected ? files[1] : files[0]).toStdString()
                  << "\n";
        return kExitInvalid;
    }
    const ImageComparison comparison = compareImages(*expected, *obtained, tolerances.channel);
    const bool match = imagesMatch(comparison, tolerances.pixels);
    std::cout << (match ? "equal: " : "DIFFERENT: ") << describe(comparison, *expected, *obtained)
              << "\n";
    if (!match && differencePath && comparison.sameSize) {
        if (!comparison.difference.save(*differencePath, "PNG")) {
            std::cerr << "cannot write " << differencePath->toStdString() << "\n";
            return kExitInvalid;
        }
    }
    return match ? kExitPassed : kExitFailed;
}

// ---- case ------------------------------------------------------------------

enum class Verdict : std::uint8_t { Passed, Failed, Invalid };

struct Outcome {
    Verdict verdict{Verdict::Passed};
    std::string message;  ///< why a case is invalid, or why it was not run
    std::vector<script::Difference> differences;
    std::vector<std::string> images;  ///< one line per image that differs
};

Outcome invalid(std::string message) {
    Outcome outcome;
    outcome.verdict = Verdict::Invalid;
    outcome.message = std::move(message);
    return outcome;
}

Outcome failed(std::string message) {
    Outcome outcome;
    outcome.verdict = Verdict::Failed;
    outcome.message = std::move(message);
    return outcome;
}

/// Runs `program --script <script> --output <output>`; an empty string if
/// it ran to the end with exit code 0, otherwise what went wrong.
std::string runProgram(const QString& program, const QString& scriptPath, const QString& output,
                       const std::optional<QString>& sources) {
    QProcess process;
    // The program's own errors (a script it cannot perform) reach the log.
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    QStringList arguments{QStringLiteral("--script"), scriptPath, QStringLiteral("--output"),
                          output};
    if (sources) {
        arguments << QStringLiteral("--sources") << *sources;
    }
    process.start(program, arguments);
    if (!process.waitForStarted()) {
        return "cannot start " + program.toStdString();
    }
    if (!process.waitForFinished(kProgramTimeoutMilliseconds)) {
        process.kill();
        process.waitForFinished();
        return "tecnigrafo --script did not end within " +
               std::to_string(kProgramTimeoutMilliseconds / 1000) + " seconds";
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        return "tecnigrafo --script crashed";
    }
    if (process.exitCode() != 0) {
        return "tecnigrafo --script exited with code " + std::to_string(process.exitCode());
    }
    return {};
}

/// Compares every atteso-<name>.png of the case with <name>.png of the
/// output; writes differenza-<name>.png for those that differ.
std::vector<std::string> compareCaseImages(const QDir& folder, const QDir& output,
                                           const script::Tolerances& tolerances) {
    std::vector<std::string> lines;
    const QString prefix = QString::fromLatin1(kExpectedImagePrefix);
    const QStringList expectedFiles =
        folder.entryList({prefix + QStringLiteral("*.png")}, QDir::Files, QDir::Name);
    for (const QString& file : expectedFiles) {
        const QString name = QFileInfo(file).completeBaseName().mid(prefix.size());
        const std::string label = "image " + name.toStdString() + ": ";
        const std::optional<QImage> expected = loadImage(folder.filePath(file));
        const std::optional<QImage> obtained =
            loadImage(output.filePath(name + QStringLiteral(".png")));
        if (!expected) {
            lines.push_back(label + "cannot read " + file.toStdString());
            continue;
        }
        if (!obtained) {
            lines.push_back(label + "not written: the script has no !screenshot " +
                            name.toStdString());
            continue;
        }
        const ImageComparison comparison = compareImages(*expected, *obtained, tolerances.channel);
        if (imagesMatch(comparison, tolerances.pixels)) {
            continue;
        }
        std::string line = label + describe(comparison, *expected, *obtained);
        if (comparison.sameSize) {
            const QString differencePath = output.filePath(
                QString::fromLatin1(kDifferenceImagePrefix) + name + QStringLiteral(".png"));
            if (comparison.difference.save(differencePath, "PNG")) {
                line += ", see " + differencePath.toStdString();
            }
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

Outcome checkCase(const QString& folderPath, const QString& program, const QString& outputPath,
                  const std::optional<QString>& sources) {
    const QDir folder(folderPath);
    const QDir output(outputPath);
    if (!folder.exists()) {
        return invalid("not a folder");
    }
    const std::optional<std::string> text = readFile(folder.filePath(kScriptFile));
    const std::optional<std::string> expected = readFile(folder.filePath(kExpectedFile));
    if (!text) {
        return invalid(std::string{"cannot read "} + kScriptFile);
    }
    std::variant<script::Script, script::ParseError> parsed = script::parseScript(*text);
    if (const auto* error = std::get_if<script::ParseError>(&parsed)) {
        if (error->notImplemented) {
            return failed(std::string{"not implemented yet: "} + kScriptFile + ", line " +
                          std::to_string(error->line) + ": " + error->message);
        }
        return invalid(std::string{kScriptFile} + ", line " + std::to_string(error->line) + ": " +
                       error->message);
    }
    if (!expected) {
        return invalid(std::string{"cannot read "} + kExpectedFile);
    }
    if (!output.mkpath(QStringLiteral("."))) {
        return invalid("cannot create " + outputPath.toStdString());
    }
    // The files of an earlier run must not pass for this run's. Only those:
    // the output folder may hold anything else.
    QFile::remove(output.filePath(kObtainedReport));
    const QString prefix = QString::fromLatin1(kExpectedImagePrefix);
    for (const QString& file :
         folder.entryList({prefix + QStringLiteral("*.png")}, QDir::Files, QDir::Name)) {
        const QString name = QFileInfo(file).completeBaseName().mid(prefix.size());
        QFile::remove(output.filePath(name + QStringLiteral(".png")));
        QFile::remove(output.filePath(QString::fromLatin1(kDifferenceImagePrefix) + name +
                                      QStringLiteral(".png")));
    }

    const std::string problem =
        runProgram(program, folder.filePath(kScriptFile), output.absolutePath(), sources);
    if (!problem.empty()) {
        return failed(problem);
    }
    const std::optional<std::string> report = readFile(output.filePath(kObtainedReport));
    if (!report) {
        return failed(std::string{"tecnigrafo --script wrote no "} + kObtainedReport);
    }
    const script::Script& parsedScript = std::get<script::Script>(parsed);
    Outcome outcome;
    outcome.differences = script::compareReports(*expected, *report, parsedScript.tolerances);
    outcome.images = compareCaseImages(folder, output, parsedScript.tolerances);
    outcome.verdict =
        outcome.differences.empty() && outcome.images.empty() ? Verdict::Passed : Verdict::Failed;
    return outcome;
}

void printDifferences(const Outcome& outcome) {
    std::size_t printed = 0;
    for (const script::Difference& difference : outcome.differences) {
        if (printed == kMaxPrintedDifferences) {
            std::cout << "  ... and " << (outcome.differences.size() - printed)
                      << " more differences\n";
            break;
        }
        std::cout << "  line " << difference.line << "\n"
                  << "    expected: " << difference.expected << "\n"
                  << "    obtained: " << difference.actual << "\n";
        ++printed;
    }
    for (const std::string& line : outcome.images) {
        std::cout << "  " << line << "\n";
    }
}

int conclude(const Outcome& outcome, const QString& folder, bool pending) {
    const std::string name = QDir::fromNativeSeparators(folder).toStdString() + " (level 2)";
    switch (outcome.verdict) {
        case Verdict::Invalid:
            std::cout << "INVALID: " << name << ": " << outcome.message << "\n";
            return kExitInvalid;
        case Verdict::Passed:
            if (pending) {
                std::cout << "FAILED: " << name << " passes now: remove it from " << kPendingList
                          << "\n";
                return kExitFailed;
            }
            std::cout << "passed: " << name << "\n";
            return kExitPassed;
        case Verdict::Failed:
            break;
    }
    std::cout << (pending ? "pending: " : "FAILED: ") << name;
    if (outcome.message.empty()) {
        std::cout << ": " << outcome.differences.size() << " differences in the report, "
                  << outcome.images.size() << " images differ\n";
    } else {
        std::cout << ": " << outcome.message << "\n";
    }
    printDifferences(outcome);
    if (pending) {
        std::cout << "warning: not passed yet, as expected while it is listed in " << kPendingList
                  << "\n";
        return kExitPassed;
    }
    return kExitFailed;
}

int checkCaseCommand(const QStringList& arguments) {
    std::optional<QString> folder;
    std::optional<QString> program;
    std::optional<QString> output;
    std::optional<QString> sources;
    bool pending = false;
    for (qsizetype i = 0; i < arguments.size(); ++i) {
        const QString& argument = arguments[i];
        const bool hasValue = i + 1 < arguments.size();
        if (argument == QLatin1String("--program") && hasValue && !program) {
            program = arguments[++i];
        } else if (argument == QLatin1String("--output") && hasValue && !output) {
            output = arguments[++i];
        } else if (argument == QLatin1String("--sources") && hasValue && !sources) {
            sources = arguments[++i];
        } else if (argument == QLatin1String("--pending") && !pending) {
            pending = true;
        } else if (!argument.startsWith(QLatin1String("--")) && !folder) {
            folder = argument;
        } else {
            return usageError();
        }
    }
    if (!folder || !program || !output) {
        return usageError();
    }
    return conclude(checkCase(*folder, *program, *output, sources), *folder, pending);
}

}  // namespace

int main(int argc, char* argv[]) {
    // QProcess needs an application object; QImage only needs QtGui.
    const QCoreApplication application(argc, argv);
    QStringList arguments = QCoreApplication::arguments();
    arguments.removeFirst();
    if (arguments.isEmpty()) {
        return usageError();
    }
    const QString command = arguments.takeFirst();
    if (command == QLatin1String("--help") || command == QLatin1String("-h")) {
        std::cout << kUsage;
        return kExitPassed;
    }
    if (command == QLatin1String("images")) {
        return compareImageFiles(arguments);
    }
    if (command == QLatin1String("case")) {
        return checkCaseCommand(arguments);
    }
    return usageError();
}
