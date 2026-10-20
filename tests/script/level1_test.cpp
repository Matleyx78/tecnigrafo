// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/level1.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"
#include "tg/script/compare.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"
#include "tg/script/script.hpp"

#include <gtest/gtest.h>

#include <clocale>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <locale>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

const std::filesystem::path kData{TG_SCRIPT_TEST_DATA};

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    EXPECT_TRUE(file.good()) << path;
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}};
}

Script parsed(std::string_view text) {
    std::variant<Script, ParseError> result = parseScript(text);
    if (const auto* error = std::get_if<ParseError>(&result)) {
        ADD_FAILURE() << "line " << error->line << ": " << error->message;
        return {};
    }
    return std::get<Script>(std::move(result));
}

std::string reportOf(std::string_view text) {
    return runLevel1(parsed(text));
}

/// The section of `report` that starts with "[title]", up to the next one.
std::string section(const std::string& report, std::string_view title) {
    const std::string header = "[" + std::string{title} + "]\n";
    const std::size_t start = report.find(header);
    if (start == std::string::npos) {
        ADD_FAILURE() << "no section " << header << " in:\n" << report;
        return {};
    }
    const std::size_t end = report.find("\n[", start + header.size());
    return report.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

void expectContains(const std::string& text, std::string_view part) {
    EXPECT_NE(text.find(part), std::string::npos) << "missing:\n" << part << "\nin:\n" << text;
}

TEST(RunLevel1, PlineScriptGivesTheReportWrittenByHand) {
    const Script script = parsed(readFile(kData / "pline.tgs"));
    const std::string expected = readFile(kData / "pline-atteso.txt");
    const std::vector<Difference> differences =
        compareReports(expected, runLevel1(script), script.tolerances);
    for (const Difference& difference : differences) {
        ADD_FAILURE() << "line " << difference.line << "\n  expected: " << difference.expected
                      << "\n  obtained: " << difference.actual;
    }
}

TEST(RunLevel1, TwoRunsGiveTheSameReport) {
    const Script script = parsed(readFile(kData / "pline.tgs"));
    EXPECT_EQ(runLevel1(script), runLevel1(script));
}

/// Switches the C and C++ locales to the first of `names` the system has,
/// and back to the previous ones when it goes out of scope.
class LocaleSwitch {
public:
    explicit LocaleSwitch(std::initializer_list<const char*> names)
        : m_previous(std::locale::global(std::locale::classic())) {
        const char* current = std::setlocale(LC_ALL, nullptr);
        m_previousC = current != nullptr ? current : "C";
        for (const char* name : names) {
            try {
                std::locale::global(std::locale(name));
            } catch (const std::runtime_error&) {
                continue;
            }
            if (std::setlocale(LC_ALL, name) != nullptr) {
                m_chosen = name;
                return;
            }
        }
        restore();
    }

    LocaleSwitch(const LocaleSwitch&) = delete;
    LocaleSwitch& operator=(const LocaleSwitch&) = delete;
    LocaleSwitch(LocaleSwitch&&) = delete;
    LocaleSwitch& operator=(LocaleSwitch&&) = delete;

    ~LocaleSwitch() { restore(); }

    /// The locale chosen, empty if the system has none of them.
    [[nodiscard]] const std::string& chosen() const { return m_chosen; }

private:
    void restore() {
        std::locale::global(m_previous);
        static_cast<void>(std::setlocale(LC_ALL, m_previousC.c_str()));
    }

    std::locale m_previous;
    std::string m_previousC;
    std::string m_chosen;
};

std::vector<std::string> formatted(const std::vector<double>& values) {
    std::vector<std::string> texts;
    texts.reserve(values.size());
    for (const double value : values) {
        texts.push_back(formatNumber(value));
    }
    return texts;
}

TEST(RunLevel1, ReportIsTheSameWithALocaleThatWritesADecimalComma) {
    // Besides the PLINE script, numbers whose shortest text has 16 or 17
    // digits: with a decimal comma they would not read back, and the text
    // would change.
    const Script pline = parsed(readFile(kData / "pline.tgs"));
    const Script digits = parsed("PLINE\n0.3333333333333333,2.5\n@0.1,0.2\n123.456,-1e-7\n");
    const std::vector<double> values{0.1, 1.0 / 3.0, 2.5, 123.456, 0.1 + 0.2, 1e-7, 1e300};
    const std::string classicPline = runLevel1(pline);
    const std::string classicDigits = runLevel1(digits);
    const std::vector<std::string> classicValues = formatted(values);

    const LocaleSwitch locale{"de_DE.UTF-8", "de_DE.utf8",  "it_IT.UTF-8",
                              "it_IT.utf8",  "fr_FR.UTF-8", "fr_FR.utf8",
                              "de-DE",       "it-IT",       "German_Germany.1252"};
    if (locale.chosen().empty()) {
        GTEST_SKIP() << "no locale with a decimal comma on this system";
    }
    EXPECT_EQ(runLevel1(pline), classicPline) << "locale " << locale.chosen();
    EXPECT_EQ(runLevel1(digits), classicDigits) << "locale " << locale.chosen();
    EXPECT_EQ(formatted(values), classicValues) << "locale " << locale.chosen();
}

TEST(RunLevel1, EmptyScriptGivesTheFinalStateOnly) {
    EXPECT_EQ(reportOf(""),
              "tecnigrafo-report 1\n"
              "\n"
              "[final]\n"
              "entities:\n"
              "messages:\n"
              "prompt: Command:\n"
              "preview:\n"
              "selection: none\n"
              "rectangle: none\n"
              "undo:\n"
              "redo:\n"
              "snap: on\n"
              "ortho: off\n"
              "pointer: none\n"
              "active snap: none\n"
              "statistics:\n");
}

TEST(RunLevel1, SectionsAreNumberedAndMessagesAreNotRepeated) {
    const std::string report = reportOf("XYZ\n!report\n!report\nU\n");
    expectContains(section(report, "report 1"),
                   "messages:\n  Command: XYZ\n  Unknown command: XYZ.\nprompt:");
    expectContains(section(report, "report 2"), "messages:\nprompt:");
    expectContains(section(report, "final"),
                   "messages:\n  Command: U\n  Nothing to undo.\nprompt:");
}

TEST(RunLevel1, EnterRepeatsTheLastCommandAndEchoesThePromptAlone) {
    const std::string report = reportOf("PLINE\n0,0\n10,0\n\n\n");
    expectContains(section(report, "final"),
                   "messages:\n"
                   "  Command: PLINE\n"
                   "  Specify start point: 0,0\n"
                   "  Specify next point or [Arc/Undo]: 10,0\n"
                   "  Specify next point or [Arc/Undo]:\n"
                   "  Command:\n"
                   "prompt: Specify start point:\n");
}

TEST(RunLevel1, EscKeepsCompletedPolylineAndEndsTheCommand) {
    const std::string report = reportOf("PLINE\n0,0\n10,0\n!esc\n");
    const std::string final = section(report, "final");
    expectContains(final, "entities:\n  1 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n");
    expectContains(final, "  *Cancel*\nprompt: Command:\n");
    expectContains(final, "undo:\n  Polyline\nredo:\n");
}

TEST(RunLevel1, RightClickConfirms) {
    const std::string report = reportOf("PLINE\n0,0\n10,0\n!rightclick\n");
    expectContains(section(report, "final"),
                   "entities:\n  1 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n");
}

TEST(RunLevel1, ClicksBuildAWindowSelection) {
    const std::string report = reportOf(
        "PLINE\n0,0\n10,0\n\n"
        "PLINE\n0,20\n10,20\n\n"
        "PLINE\n100,0\n110,0\n\n"
        "!click -5,-5\n"
        "!move 50,30\n"
        "!report\n"
        "!click 50,30\n"
        "!report\n"
        "!click 5,20 remove\n");
    const std::string first = section(report, "report 1");
    expectContains(first, "selection: none\nrectangle: window -5,-5 50,30\n");
    expectContains(first, "prompt: Specify opposite corner:\n");
    expectContains(section(report, "report 2"), "selection: 1 2\nrectangle: none\n");
    expectContains(section(report, "final"), "selection: 1\n");
}

TEST(RunLevel1, ClickToTheLeftTracesACrossing) {
    const std::string report = reportOf("PLINE\n0,0\n10,0\n\n!click 5,5\n!move 3,-5\n");
    expectContains(section(report, "final"), "rectangle: crossing 5,5 3,-5\n");
}

TEST(RunLevel1, ViewSetsThePickBox) {
    // The pick box is 3 pixels: 3 units at scale 1, 30 at scale 10.
    const std::string drawn = "PLINE\n0,0\n100,0\n\n";
    expectContains(reportOf(drawn + "!click 50,20\n"), "selection: none\n");
    expectContains(reportOf(drawn + "!view 0,0 10 800x600\n!click 50,20\n"), "selection: 1\n");
}

TEST(RunLevel1, SnapDirectiveTurnsTheActiveSnapOff) {
    const std::string drawn = "PLINE\n0,0\n100,0\n\nPLINE\n!move 99,1\n";
    expectContains(reportOf(drawn), "pointer: 99,1\nactive snap: Endpoint 100,0\n");
    expectContains(reportOf(drawn + "!snap off\n"),
                   "snap: off\northo: off\npointer: 99,1\nactive snap: none\n");
}

TEST(RunLevel1, OrthoDirective) {
    const std::string report = reportOf("!snap off\n!ortho on\nPLINE\n0,0\n!move 10,3\n");
    expectContains(section(report, "final"),
                   "preview:\n  LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n");
    expectContains(section(report, "final"), "snap: off\northo: on\n");
}

TEST(RunLevel1, UndoAndRedoNamesInOrder) {
    const std::string report =
        reportOf("PLINE\n0,0\n10,0\n\nPLINE\n0,5\n10,5\n\n!click 5,0\nERASE\nU\n");
    const std::string final = section(report, "final");
    expectContains(final, "undo:\n  Polyline\n  Polyline\nredo:\n  Erase\n");
}

TEST(RunLevel1, EntitiesAreListedByIdentifier) {
    // Undo and redo of the first polyline move it to the end of the
    // document; the report still lists it first.
    const std::string report =
        reportOf("PLINE\n0,0\n10,0\n\nPLINE\n0,5\n10,5\n\n!click 5,0\nERASE\nU\n");
    expectContains(section(report, "final"),
                   "entities:\n"
                   "  1 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n"
                   "  2 LWPOLYLINE closed=no vertex=0,5 vertex=10,5\n"
                   "messages:");
}

TEST(WriteState, SnapModesOffAreListed) {
    core::Document document;
    edit::Editor editor(document);
    cmd::CommandRegistry registry;
    cmd::registerBuiltinCommands(registry);
    cmd::CommandRunner runner(registry, editor);
    cmd::DrawingAids aids;
    aids.snapModes.midpoint = false;
    aids.snapModes.center = false;
    runner.setDrawingAids(aids);
    std::string out;
    writeState({"final", editor, runner, {}, std::nullopt}, out);
    expectContains(out, "snap: on, modes off: midpoint center\n");
}

TEST(SnapModeNames, EveryModeOnce) {
    std::vector<std::string_view> names;
    for (const SnapModeName& mode : snapModeNames()) {
        names.push_back(mode.name);
    }
    EXPECT_EQ(names, (std::vector<std::string_view>{"endpoint", "midpoint", "center"}));
}

}  // namespace
}  // namespace tg::script
