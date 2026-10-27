// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Level 2 of the control mode (docs/adr/0011): the script session of the
// main window, `tecnigrafo --script` and the test tool confronta_immagini.

#include "script_session.hpp"

#include "main_window.hpp"
#include "tg/script/compare.hpp"
#include "tg/script/level1.hpp"
#include "tg/script/script.hpp"
#include "theme.hpp"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QSize>
#include <QString>
#include <QStringList>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::app {
namespace {

using script::compareReports;
using script::Difference;
using script::Tolerances;

/// Output folder of one test, empty at the start.
QString outputFolder(const char* name) {
    const QDir folder(QStringLiteral(TG_APP_TEST_OUTPUT));
    const QString path = folder.filePath(QString::fromLatin1(name));
    QDir(path).removeRecursively();
    folder.mkpath(QString::fromLatin1(name));
    return path;
}

std::optional<std::string> readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const QByteArray bytes = file.readAll();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

bool writeFile(const QString& path, std::string_view text) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) &&
           file.write(text.data(), static_cast<qint64>(text.size())) ==
               static_cast<qint64>(text.size());
}

std::string testData(const char* name) {
    return readFile(QDir(QStringLiteral(TG_SCRIPT_TEST_DATA)).filePath(QString::fromLatin1(name)))
        .value_or(std::string{});
}

script::Script parse(std::string_view text) {
    std::variant<script::Script, script::ParseError> parsed = script::parseScript(text);
    if (const auto* error = std::get_if<script::ParseError>(&parsed)) {
        ADD_FAILURE() << "line " << error->line << ": " << error->message;
        return {};
    }
    return std::get<script::Script>(parsed);
}

/// Runs `text` at level 2 in a new main window and returns the report.
std::string runLevel2(std::string_view text, const QString& output) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    ScriptSession session(window, output);
    const bool ran = session.run(parse(text));
    EXPECT_TRUE(ran) << session.error().toStdString();
    return session.report();
}

/// The lines of the section `[title]` of a report, without its title.
std::string section(const std::string& report, const std::string& title) {
    const std::string heading = "[" + title + "]\n";
    const std::size_t start = report.find(heading);
    if (start == std::string::npos) {
        return {};
    }
    const std::size_t end = report.find("\n[", start + heading.size());
    return report.substr(start + heading.size(), end == std::string::npos
                                                     ? std::string::npos
                                                     : end - start - heading.size());
}

std::string describe(const std::vector<Difference>& differences) {
    std::string text;
    for (const Difference& difference : differences) {
        text += "line " + std::to_string(difference.line) + "\n  expected: " + difference.expected +
                "\n  obtained: " + difference.actual + "\n";
    }
    return text;
}

struct ProcessResult {
    int exitCode{-1};
    std::string output;
};

ProcessResult runProcess(const QString& program, const QStringList& arguments) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, arguments);
    constexpr int kTimeoutMilliseconds = 120000;
    if (!process.waitForFinished(kTimeoutMilliseconds) ||
        process.exitStatus() != QProcess::NormalExit) {
        return {-1, process.readAll().toStdString()};
    }
    return {process.exitCode(), process.readAll().toStdString()};
}

// ---- the session ------------------------------------------------------------

TEST(ScriptSessionTest, PlineScriptGivesTheReportOfLevel1) {
    const std::string text = testData("pline.tgs");
    ASSERT_FALSE(text.empty());
    const std::string level1 = script::runLevel1(parse(text));
    const std::string level2 = runLevel2(text, outputFolder("pline"));

    const std::vector<Difference> differences = compareReports(level1, level2, Tolerances{});
    EXPECT_TRUE(differences.empty()) << describe(differences) << "\nlevel 2 report:\n" << level2;
    // The report written by hand for level 1 holds at level 2 too.
    const std::vector<Difference> fromExpected =
        compareReports(testData("pline-atteso.txt"), level2, Tolerances{});
    EXPECT_TRUE(fromExpected.empty()) << describe(fromExpected);
}

TEST(ScriptSessionTest, KeyF8SwitchesOrthoAndWritesItsMessage) {
    const std::string report =
        runLevel2("!level 2\n!key F8\n!report\n!key F8\n", outputFolder("key-f8"));
    const std::string first = section(report, "report 1");
    EXPECT_NE(first.find("messages:\n  <Ortho on>\nprompt: Command:\n"), std::string::npos)
        << first;
    EXPECT_NE(first.find("\northo: on\n"), std::string::npos) << first;
    const std::string last = section(report, "final");
    EXPECT_NE(last.find("messages:\n  <Ortho off>\nprompt: Command:\n"), std::string::npos) << last;
    EXPECT_NE(last.find("\northo: off\n"), std::string::npos) << last;
}

TEST(ScriptSessionTest, AidDirectivesWriteNothingAndKeysToggleFromThem) {
    // !ortho on sets the switch without a message; F8 then turns it off.
    const std::string report =
        runLevel2("!level 2\n!ortho on\n!snap off\n!report\n!key F8\n", outputFolder("aids"));
    const std::string first = section(report, "report 1");
    EXPECT_NE(first.find("messages:\nprompt: Command:\n"), std::string::npos) << first;
    EXPECT_NE(first.find("\nsnap: off\northo: on\n"), std::string::npos) << first;
    const std::string last = section(report, "final");
    EXPECT_NE(last.find("messages:\n  <Ortho off>\n"), std::string::npos) << last;
    EXPECT_NE(last.find("\nsnap: off\northo: off\n"), std::string::npos) << last;
}

TEST(ScriptSessionTest, TypedLineReachesTheCommandLineAndIsEchoed) {
    const std::string report = runLevel2("PLINE\n0,0\n@10,0\n\n", outputFolder("typed"));
    const std::string last = section(report, "final");
    EXPECT_NE(last.find("entities:\n  257 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n"),
              std::string::npos)
        << last;
    EXPECT_NE(last.find("  Command: PLINE\n  Specify start point: 0,0\n"), std::string::npos)
        << last;
}

TEST(ScriptSessionTest, ScreenshotHasTheSizeOfTheView) {
    const QString output = outputFolder("screenshot");
    const std::string report = runLevel2(
        "!level 2\n!screenshot default\n!view 5,5 0.25 320x200\n!screenshot small\n", output);
    EXPECT_FALSE(report.empty());
    const QImage standard(QDir(output).filePath(QStringLiteral("default.png")));
    EXPECT_EQ(standard.size(), QSize(800, 600));
    const QImage small(QDir(output).filePath(QStringLiteral("small.png")));
    EXPECT_EQ(small.size(), QSize(320, 200));
}

TEST(ScriptSessionTest, ScreenshotUsesTheLightTheme) {
    // A horizontal polyline at y = 10: on screen, row 50 - 10 = 40.
    const QString output = outputFolder("light-theme");
    const std::string report = runLevel2(
        "!level 2\n!view 0,0 1 200x100\nPLINE\n-50,10\n50,10\n\n!screenshot tema\n", output);
    EXPECT_FALSE(report.empty());
    const QImage image(QDir(output).filePath(QStringLiteral("tema.png")));
    ASSERT_EQ(image.size(), QSize(200, 100));

    const render::Color colour = sceneStyle(Theme::Light).background;
    const QColor background(colour.r, colour.g, colour.b);
    ASSERT_EQ(background, QColor(Qt::white));
    int white = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            white += image.pixelColor(x, y) == background ? 1 : 0;
        }
    }
    // The grid takes a few rows and columns; the rest is background.
    EXPECT_GT(white, image.width() * image.height() * 3 / 4);
    // The polyline is dark, wherever antialiasing puts it around row 40.
    int darkest = 255;
    for (int y = 37; y <= 43; ++y) {
        darkest = std::min(darkest, qGray(image.pixel(100, y)));
    }
    EXPECT_LT(darkest, 128);
}

TEST(ScriptSessionTest, ClickSnapsToTheVertexItIsDrawnAt) {
    // At 0.5 units per pixel the endpoint 100,0 is 2 pixels from 101,0:
    // the second polyline starts on it, through a real click in the view.
    const std::string report =
        runLevel2("!view 0,0 0.5 800x600\nPLINE\n0,0\n100,0\n\nPLINE\n!click 101,0\n@0,10\n\n",
                  outputFolder("click-snap"));
    EXPECT_NE(
        section(report, "final").find("  258 LWPOLYLINE closed=no vertex=100,0 vertex=100,10\n"),
        std::string::npos)
        << report;
}

TEST(ScriptSessionTest, UnknownKeyStopsTheScript) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    ScriptSession session(window, outputFolder("unknown-key"));
    EXPECT_FALSE(session.run(parse("!level 2\n!key NoSuchKey\n")));
    EXPECT_FALSE(session.error().isEmpty());
}

TEST(ScriptSessionTest, RunScriptFileWritesTheReport) {
    const QString output = outputFolder("run-file");
    const QString script = QDir(output).filePath(QStringLiteral("script.tgs"));
    ASSERT_TRUE(writeFile(script, "PL\n0,0\n5,5\n\n"));
    EXPECT_EQ(runScriptFile(script, output), 0);
    const std::string report =
        readFile(QDir(output).filePath(QString::fromLatin1(kScriptReportFile))).value_or("");
    EXPECT_EQ(report.rfind("tecnigrafo-report 1\n", 0), 0U) << report;
    EXPECT_NE(report.find("  257 LWPOLYLINE closed=no vertex=0,0 vertex=5,5\n"), std::string::npos)
        << report;

    const QString broken = QDir(output).filePath(QStringLiteral("broken.tgs"));
    ASSERT_TRUE(writeFile(broken, "!nosuchdirective\n"));
    EXPECT_EQ(runScriptFile(broken, output), 1);
    EXPECT_EQ(runScriptFile(QDir(output).filePath(QStringLiteral("missing.tgs")), output), 1);
}

// ---- confronta_immagini -------------------------------------------------------

constexpr int kImageSide = 100;  // 10000 pixels: 0.1% is 10 pixels

QImage whiteImage(QSize size = QSize(kImageSide, kImageSide)) {
    QImage image(size, QImage::Format_ARGB32);
    image.fill(Qt::white);
    return image;
}

/// `count` pixels of the first row turned from white to `colour`.
QImage withPixels(int count, QColor colour) {
    QImage image = whiteImage();
    for (int x = 0; x < count; ++x) {
        image.setPixelColor(x, 0, colour);
    }
    return image;
}

class ConfrontaImmaginiTest : public testing::Test {
protected:
    void SetUp() override {
        m_folder = outputFolder("confronta-immagini");
        ASSERT_TRUE(whiteImage().save(path("expected.png")));
    }

    [[nodiscard]] QString path(const char* name) const {
        return QDir(m_folder).filePath(QString::fromLatin1(name));
    }

    /// Exit code of `confronta_immagini images expected.png <image>`.
    int compareWith(const QImage& image, const QStringList& options = {}) {
        if (!image.save(path("obtained.png"))) {
            ADD_FAILURE() << "cannot write the obtained image";
            return -1;
        }
        const ProcessResult result = runProcess(
            QStringLiteral(TG_CONFRONTA_IMMAGINI),
            QStringList{QStringLiteral("images"), path("expected.png"), path("obtained.png")} +
                options);
        m_output = result.output;
        return result.exitCode;
    }

    /// What the last compareWith() printed.
    [[nodiscard]] const std::string& output() const { return m_output; }

private:
    QString m_folder;
    std::string m_output;
};

TEST_F(ConfrontaImmaginiTest, EqualImagesMatch) {
    EXPECT_EQ(compareWith(whiteImage()), 0) << output();
}

TEST_F(ConfrontaImmaginiTest, DifferencesWithinTheThresholdsMatch) {
    // Every pixel 8 levels darker: not a different pixel.
    EXPECT_EQ(compareWith(withPixels(kImageSide, QColor(247, 247, 247))), 0) << output();
    // 10 different pixels out of 10000: exactly 0.1%.
    EXPECT_EQ(compareWith(withPixels(10, Qt::black)), 0) << output();
}

TEST_F(ConfrontaImmaginiTest, DifferencesBeyondTheThresholdsDoNotMatch) {
    // 11 different pixels: 0.11%.
    EXPECT_EQ(compareWith(withPixels(11, Qt::black)), 1) << output();
    // 9 levels in a channel make a pixel different.
    QImage image = whiteImage();
    image.fill(QColor(255, 246, 255));
    EXPECT_EQ(compareWith(image), 1) << output();
    // The thresholds of !tolerance, given as options.
    EXPECT_EQ(compareWith(image, {QStringLiteral("--channel"), QStringLiteral("9")}), 0)
        << output();
    EXPECT_EQ(
        compareWith(withPixels(11, Qt::black), {QStringLiteral("--pixels"), QStringLiteral("0.2")}),
        0)
        << output();
}

TEST_F(ConfrontaImmaginiTest, DifferentSizesDoNotMatch) {
    EXPECT_EQ(compareWith(whiteImage(QSize(kImageSide, kImageSide + 1))), 1) << output();
    EXPECT_NE(output().find("size 100x101, expected 100x100"), std::string::npos) << output();
}

TEST_F(ConfrontaImmaginiTest, DifferenceImageMarksThePixels) {
    const QString difference = path("difference.png");
    EXPECT_EQ(compareWith(withPixels(20, Qt::black), {QStringLiteral("--difference"), difference}),
              1);
    const QImage marked(difference);
    ASSERT_EQ(marked.size(), QSize(kImageSide, kImageSide));
    EXPECT_EQ(marked.pixelColor(0, 0), QColor(Qt::red));
    EXPECT_NE(marked.pixelColor(50, 50), QColor(Qt::red));
}

TEST_F(ConfrontaImmaginiTest, MissingImageIsInvalid) {
    const ProcessResult result =
        runProcess(QStringLiteral(TG_CONFRONTA_IMMAGINI),
                   {QStringLiteral("images"), path("expected.png"), path("no-such-image.png")});
    EXPECT_EQ(result.exitCode, 2) << result.output;
}

// ---- a case at level 2 --------------------------------------------------------

/// A case folder with `script` and `expected` as atteso.txt.
QString makeCase(const char* name, std::string_view script, std::string_view expected) {
    const QString folder = outputFolder(name);
    EXPECT_TRUE(writeFile(QDir(folder).filePath(QStringLiteral("script.tgs")), script));
    EXPECT_TRUE(writeFile(QDir(folder).filePath(QStringLiteral("atteso.txt")), expected));
    return folder;
}

ProcessResult checkCase(const QString& folder, bool pending = false) {
    QStringList arguments{
        QStringLiteral("case"),      folder,
        QStringLiteral("--program"), QStringLiteral(TG_TECNIGRAFO),
        QStringLiteral("--output"),  QDir(folder).filePath(QStringLiteral("ottenuto"))};
    if (pending) {
        arguments << QStringLiteral("--pending");
    }
    return runProcess(QStringLiteral(TG_CONFRONTA_IMMAGINI), arguments);
}

TEST(ConfrontaImmaginiCaseTest, CaseRunsTheProgramAndComparesTheReport) {
    const std::string script = testData("pline.tgs");
    const std::string expected = testData("pline-atteso.txt");
    const QString passing = makeCase("case-passing", script, expected);
    ProcessResult result = checkCase(passing);
    EXPECT_EQ(result.exitCode, 0) << result.output;
    EXPECT_EQ(checkCase(passing, true).exitCode, 1);

    std::string wrong = expected;
    const std::size_t vertex = wrong.find("vertex=100,0");
    ASSERT_NE(vertex, std::string::npos);
    wrong.replace(vertex, std::string_view{"vertex=100,0"}.size(), "vertex=100.5,0");
    const QString failing = makeCase("case-failing", script, wrong);
    result = checkCase(failing);
    EXPECT_EQ(result.exitCode, 1) << result.output;
    EXPECT_NE(result.output.find("expected: "), std::string::npos) << result.output;
    EXPECT_EQ(checkCase(failing, true).exitCode, 0);
}

TEST(ConfrontaImmaginiCaseTest, CaseComparesTheScreenshots) {
    const std::string script = "!level 2\n!view 0,0 1 64x48\n!screenshot vista\n";
    const QString folder = makeCase("case-image", script, "");
    // Any report differs from an empty atteso.txt; only the image counts.
    ASSERT_TRUE(
        whiteImage(QSize(64, 48)).save(QDir(folder).filePath(QStringLiteral("atteso-vista.png"))));
    const ProcessResult result = checkCase(folder);
    EXPECT_EQ(result.exitCode, 1) << result.output;
    // The empty view is the grid and the axes on the white background: more
    // than 0.1% of the pixels are not white.
    EXPECT_NE(result.output.find("image vista: "), std::string::npos) << result.output;
    EXPECT_TRUE(
        QFile::exists(QDir(folder).filePath(QStringLiteral("ottenuto/differenza-vista.png"))));
}

TEST(ConfrontaImmaginiCaseTest, InvalidScriptIsInvalid) {
    const QString folder = makeCase("case-invalid", "!nosuchdirective\n", "");
    EXPECT_EQ(checkCase(folder).exitCode, 2);
    EXPECT_EQ(checkCase(folder, true).exitCode, 2);
}

}  // namespace
}  // namespace tg::app
