// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Files in the control mode (docs/verifica/script.md and domande.md,
// "File"; docs/adr/0021, point 4): CaseFiles, the definitions and questions
// about files, --sources and the drawing fields of the report.

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/core/line.hpp"
#include "tg/edit/editor.hpp"
#include "tg/io/io.hpp"
#include "tg/script/case_check.hpp"
#include "tg/script/case_files.hpp"
#include "tg/script/level1.hpp"
#include "tg/script/questions.hpp"
#include "tg/script/report.hpp"
#include "tg/script/script.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

namespace fs = std::filesystem;

const fs::path kRoot = fs::path{TG_SCRIPT_TEST_OUTPUT} / "files";

// A DXF R2000 written by hand: a LINE with handle 2F from (0,0) to (10,5),
// a HATCH (not handled) with handle 30, units 6 (metres).
constexpr std::string_view kDxf =
    "  0\nSECTION\n  2\nHEADER\n"
    "  9\n$ACADVER\n  1\nAC1015\n"
    "  9\n$HANDSEED\n  5\n40\n"
    "  9\n$INSUNITS\n 70\n6\n"
    "  0\nENDSEC\n"
    "  0\nSECTION\n  2\nENTITIES\n"
    "  0\nLINE\n  5\n2F\n100\nAcDbEntity\n  8\n0\n100\nAcDbLine\n"
    " 10\n0.0\n 20\n0.0\n 30\n0.0\n 11\n10.0\n 21\n5.0\n 31\n0.0\n"
    "  0\nHATCH\n  5\n30\n100\nAcDbEntity\n  8\n0\n"
    "  0\nENDSEC\n  0\nEOF\n";

// Not a DXF: a group code must be an integer.
constexpr std::string_view kBrokenDxf = "  0\nSECTION\nabc\nHEADER\n";

std::string readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}};
}

void writeFile(const fs::path& path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

/// An empty folder kRoot/<name>.
fs::path emptyFolder(const std::string& name) {
    const fs::path folder = kRoot / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

/// A folder of public files, as CMake downloads them, with "esempio".
fs::path sourcesFolder() {
    const fs::path folder = emptyFolder("sorgenti");
    writeFile(folder / "esempio", kDxf);
    return folder;
}

std::variant<std::vector<Answer>, ParseError> ask(std::string_view text, io::FileAccess* files) {
    return answerQuestions(text, files);
}

ParseError failure(std::string_view text, io::FileAccess* files) {
    std::variant<std::vector<Answer>, ParseError> result = ask(text, files);
    if (!std::holds_alternative<ParseError>(result)) {
        ADD_FAILURE() << "expected an error for:\n" << text;
        return {};
    }
    return std::get<ParseError>(std::move(result));
}

void expectContains(std::string_view text, std::string_view part) {
    EXPECT_NE(text.find(part), std::string_view::npos) << "missing: " << part << "\nin: " << text;
}

// ---------------------------------------------------------------------------
// CaseFiles

TEST(CaseFiles, ReadsAndWritesRelativeNamesInsideTheOutputFolder) {
    const fs::path output = emptyFolder("output");
    fs::create_directories(output / "sub");
    CaseFiles files{output, std::nullopt};
    EXPECT_FALSE(files.exists("plan.dxf"));
    EXPECT_FALSE(files.write("plan.dxf", "bytes").has_value());
    EXPECT_FALSE(files.write("sub/piano è.cadx", "more").has_value());
    EXPECT_TRUE(files.exists("plan.dxf"));
    EXPECT_EQ(readFile(output / "plan.dxf"), "bytes");
    const io::Result<std::string> read = files.read("sub/piano è.cadx");
    ASSERT_TRUE(read.ok());
    EXPECT_EQ(read.value(), "more");
}

TEST(CaseFiles, ErrorsNameTheFileAsWrittenNeverTheFolders) {
    CaseFiles files{emptyFolder("names"), std::nullopt};
    const io::Result<std::string> read = files.read("missing.dxf");
    ASSERT_FALSE(read.ok());
    EXPECT_EQ(read.error().kind, io::Error::Kind::File);
    EXPECT_EQ(read.error().message, "\"missing.dxf\": does not exist");
}

TEST(CaseFiles, RejectsEveryNameThatCouldLeaveTheFolder) {
    const fs::path output = emptyFolder("rejected");
    CaseFiles files{output, sourcesFolder()};
    const std::vector<std::string> names{
        "",       "/etc/passwd", "C:plan.dxf",       "c:/plan.dxf",    "../plan.dxf",
        "a/../b", "..",          "a\\b.dxf",         "a\tb.dxf",       std::string{"a\x7f"},
        "@other", "@sources/",   "@sources/Esempio", "@sources/a.dxf", "@sources/../x"};
    for (const std::string& name : names) {
        const io::Result<std::string> read = files.read(name);
        ASSERT_FALSE(read.ok()) << name;
        EXPECT_EQ(read.error().message, kInvalidFileName) << name;
        const std::optional<io::Error> write = files.write(name, "x");
        ASSERT_TRUE(write.has_value()) << name;
        EXPECT_EQ(write.value_or(io::Error{}).message, kInvalidFileName) << name;
        EXPECT_FALSE(files.exists(name)) << name;
    }
    EXPECT_TRUE(fs::is_empty(output));
}

TEST(CaseFiles, SourcesAreReadOnly) {
    const fs::path sources = sourcesFolder();
    CaseFiles files{emptyFolder("sources"), sources};
    EXPECT_TRUE(files.exists("@sources/esempio"));
    EXPECT_FALSE(files.exists("@sources/assente"));
    const io::Result<std::string> read = files.read("@sources/esempio");
    ASSERT_TRUE(read.ok());
    EXPECT_EQ(read.value(), kDxf);
    const std::optional<io::Error> write = files.write("@sources/esempio", "x");
    ASSERT_TRUE(write.has_value());
    EXPECT_EQ(write.value_or(io::Error{}).message,
              "\"@sources/esempio\": the files of @sources/ are read only");
    EXPECT_EQ(readFile(sources / "esempio"), kDxf);
}

TEST(CaseFiles, SourcesWithoutTheirFolderSayWhy) {
    CaseFiles files{emptyFolder("no-sources"), std::nullopt};
    const io::Result<std::string> read = files.read("@sources/esempio");
    ASSERT_FALSE(read.ok());
    expectContains(read.error().message, "--sources");
    EXPECT_FALSE(files.exists("@sources/esempio"));
}

// ---------------------------------------------------------------------------
// Definitions and questions about files

TEST(FileQuestions, DefinitionsReadThroughTheFileAccess) {
    const fs::path folder = emptyFolder("definitions");
    writeFile(folder / "plan.dxf", kDxf);
    CaseFiles files{folder, sourcesFolder()};
    // Definitions use io's real readers, including the native container.
    for (const std::string_view text : {"D = DXF source=esempio\n", "D = DXF file=plan.dxf\n"}) {
        const auto result = ask(text, &files);
        ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result)) << text;
    }
    const auto document = io::readDxf(kDxf);
    ASSERT_TRUE(document.ok());
    const auto cadx = io::writeCadx(document.value().document);
    ASSERT_TRUE(cadx.ok());
    writeFile(folder / "plan.cadx", cadx.value());
    const auto native = ask("C = CADX file=plan.cadx\n", &files);
    EXPECT_TRUE(std::holds_alternative<std::vector<Answer>>(native));

    const ParseError wrongFormat = failure("C = CADX file=plan.dxf\n", &files);
    EXPECT_FALSE(wrongFormat.notImplemented);
    EXPECT_EQ(wrongFormat.line, 1);
    expectContains(wrongFormat.message, "expected a ZIP archive");
}

TEST(FileQuestions, DefinitionErrors) {
    const fs::path folder = emptyFolder("definition-errors");
    writeFile(folder / "broken.dxf", kBrokenDxf);
    CaseFiles files{folder, std::nullopt};
    CaseFiles withSources{folder, sourcesFolder()};

    // source= without --sources.
    ParseError error = failure("D = DXF source=esempio\n", &files);
    EXPECT_FALSE(error.notImplemented);
    expectContains(error.message, "--sources");
    // A name that does not exist, a name that is not valid.
    error = failure("D = DXF file=missing.dxf\n", &files);
    EXPECT_FALSE(error.notImplemented);
    expectContains(error.message, "cannot read missing.dxf: \"missing.dxf\": does not exist");
    expectContains(failure("D = DXF source=assente\n", &withSources).message, "does not exist");
    expectContains(failure("D = DXF file=../x.dxf\n", &files).message, kInvalidFileName);
    // A file that cannot be read makes the question file invalid.
    error = failure("D = DXF file=broken.dxf\n", &files);
    EXPECT_FALSE(error.notImplemented);
    expectContains(error.message, "cannot read broken.dxf: line ");
    // Wrong forms, and no file access at all.
    expectContains(failure("D = DXF plan.dxf\n", &files).message, "wrong arguments");
    expectContains(failure("D = DXF file=a.dxf file=b.dxf\n", &files).message, "wrong arguments");
    expectContains(failure("D = CADX name=a.cadx\n", &files).message, "wrong arguments");
    expectContains(failure("D = DXF file=plan.dxf\n", nullptr).message, "cannot read files");
    // A name defined twice, whatever it names.
    error = failure("D = LINE start=0,0 end=1,0\nD = DXF file=broken.dxf\n", &files);
    EXPECT_EQ(error.line, 2);
    expectContains(error.message, "already defined");
}

TEST(FileQuestions, QuestionsAboutADrawingNotReadAreErrors) {
    CaseFiles files{emptyFolder("not-read"), std::nullopt};
    for (const std::string_view question :
         {"entity D 2F", "count D", "unhandled D", "version D", "units D", "warnings D",
          "roundtrip D", "roundtrip D as=cadx"}) {
        const std::string text = "? " + std::string{question} + " -> none\n";
        expectContains(failure(text, &files).message, "no drawing named 'D' is read above");
    }
    expectContains(failure("? count -> none\n", &files).message, "wrong arguments");
    expectContains(failure("? entity D -> none\n", &files).message, "wrong arguments");
    expectContains(failure("? roundtrip -> identical\n", &files).message, "wrong arguments");
    // "D:2F" without a drawing D is an ordinary name, not defined.
    expectContains(failure("? bbox D:2F -> empty\n", &files).message, "no entity named 'D:2F'");
}

TEST(FileQuestions, OpenError) {
    const fs::path folder = emptyFolder("open-error");
    writeFile(folder / "broken.dxf", kBrokenDxf);
    writeFile(folder / "broken.cadx", kBrokenDxf);
    CaseFiles files{folder, sourcesFolder()};
    std::variant<std::vector<Answer>, ParseError> result =
        ask("? open-error file=broken.dxf -> \"line 3: x\"\n"
            "? open-error file=broken.cadx -> none\n"
            "? open-error source=esempio -> none\n",
            &files);
    ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result));
    const std::vector<Answer>& answers = std::get<std::vector<Answer>>(result);
    ASSERT_EQ(answers.size(), 3U);
    // The message of the reader, with its line, between quotes.
    EXPECT_TRUE(answers[0].actual.starts_with("\"line 3: ")) << answers[0].actual;
    EXPECT_TRUE(answers[0].actual.ends_with("\"")) << answers[0].actual;
    EXPECT_NE(answers[1].actual, "none");  // not an archive
    EXPECT_FALSE(answers[2].actual.empty());

    expectContains(failure("? open-error file=missing.dxf -> none\n", &files).message,
                   "does not exist");
    expectContains(failure("? open-error file=plan.dwg -> none\n", &files).message,
                   ".dxf and .cadx");
    expectContains(failure("? open-error plan.dxf -> none\n", &files).message, "wrong arguments");
    CaseFiles withoutSources{folder, std::nullopt};
    expectContains(failure("? open-error source=esempio -> none\n", &withoutSources).message,
                   "--sources");
}

// The questions about a drawing read from a file, once io reads it (P10).
// Before, the definition is "not implemented yet" and the test is skipped.
// Only what P10 answers: the LINE of kDxf is handled from P11 (below).
TEST(FileQuestions, QuestionsAboutADrawing) {
    const fs::path folder = emptyFolder("drawing");
    writeFile(folder / "plan.dxf", kDxf);
    CaseFiles files{folder, std::nullopt};
    const std::string text =
        "D = DXF file=plan.dxf\n"
        "? entity D 30 -> unhandled HATCH\n"
        "? entity D 99 -> none\n"
        "? version D -> AC1015\n"
        "? units D -> 6\n";
    std::variant<std::vector<Answer>, ParseError> result = ask(text, &files);
    if (const auto* error = std::get_if<ParseError>(&result)) {
        ASSERT_TRUE(error->notImplemented) << error->message;
        GTEST_SKIP() << "io does not read DXF drawings yet: " << error->message;
    }
    for (const Answer& answer : std::get<std::vector<Answer>>(result)) {
        EXPECT_TRUE(answer.matches) << "line " << answer.line << "\n  expected: " << answer.expected
                                    << "\n  obtained: " << answer.actual;
    }
    // An unhandled entity in a question about an object: not passed, not an
    // invalid case, because a later package may handle its type
    // (docs/verifica/casi.md, exit codes).
    result = ask("D = DXF file=plan.dxf\n? bbox D:30 -> empty\n", &files);
    ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result));
    const std::vector<Answer>& answers = std::get<std::vector<Answer>>(result);
    ASSERT_EQ(answers.size(), 1U);
    EXPECT_FALSE(answers[0].matches);
    EXPECT_EQ(answers[0].actual, "error: D:30 is an unhandled HATCH entity");
    // A handle without an entity, a drawing read twice, a wrong as=.
    expectContains(failure("D = DXF file=plan.dxf\n? bbox D:99 -> empty\n", &files).message,
                   "D has no entity with handle '99'");
    EXPECT_EQ(failure("D = DXF file=plan.dxf\nD = DXF file=plan.dxf\n", &files).line, 2);
    expectContains(
        failure("D = DXF file=plan.dxf\n? roundtrip D as=svg -> identical\n", &files).message,
        "wrong arguments");
}

// The questions about a handled entity of a drawing read from a file: io
// handles LINE since P11.
TEST(FileQuestions, QuestionsAboutAHandledEntity) {
    const fs::path folder = emptyFolder("handled");
    writeFile(folder / "plan.dxf", kDxf);
    CaseFiles files{folder, std::nullopt};
    std::variant<std::vector<Answer>, ParseError> probe =
        ask("D = DXF file=plan.dxf\n? entity D 2F -> unhandled LINE\n", &files);
    if (const auto* error = std::get_if<ParseError>(&probe)) {
        ASSERT_TRUE(error->notImplemented) << error->message;
        GTEST_SKIP() << "io does not read DXF drawings yet: " << error->message;
    }
    EXPECT_FALSE(std::get<std::vector<Answer>>(probe).at(0).matches);
    const std::string text =
        "D = DXF file=plan.dxf\n"
        "? entity D 2F -> LINE start=0,0 end=10,5\n"
        "? entity D 2f -> LINE start=0,0 end=10,5\n"
        "? count D -> LINE 1\n"
        "? unhandled D -> HATCH 1\n"
        "? bbox D:2F -> 0,0 10,5\n"
        "? length D:2f -> 11.180339887498949\n";
    std::variant<std::vector<Answer>, ParseError> result = ask(text, &files);
    ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result));
    for (const Answer& answer : std::get<std::vector<Answer>>(result)) {
        EXPECT_TRUE(answer.matches) << "line " << answer.line << "\n  expected: " << answer.expected
                                    << "\n  obtained: " << answer.actual;
    }
}

// ---------------------------------------------------------------------------
// tecnigrafo-script with files

struct Outcome {
    int code{0};
    std::string output;
};

Outcome check(const fs::path& folder, const fs::path& output,
              const std::optional<fs::path>& sources) {
    std::ostringstream out;
    CheckOptions options;
    options.output = output;
    const int code = checkCase(folder, options, out, sources);
    return {code, out.str()};
}

TEST(CheckWithFiles, QuestionsReadSourcesOnlyWithTheirFolder) {
    const fs::path folder = emptyFolder("case-sources");
    writeFile(folder / "domande.tgq", "D = DXF source=esempio\n? count D -> LINE 1\n");
    const fs::path output = kRoot / "case-sources-output";
    const Outcome without = check(folder, output, std::nullopt);
    EXPECT_EQ(without.code, kExitInvalid);
    expectContains(without.output, "--sources");
    // With it the case is valid: passed once io reads DXF, failed before.
    const Outcome with = check(folder, output, sourcesFolder());
    EXPECT_NE(with.code, kExitInvalid) << with.output;
}

TEST(CheckWithFiles, ScriptsWorkOnFreshCopiesOfTheFilesOfTheCase) {
    const fs::path folder = emptyFolder("case-copies");
    writeFile(folder / "script.tgs", "PLINE\n0,0\n10,0\n\n");
    // Not the report the script gives, but with the echo of every line, so
    // that the case runs and fails (docs/adr/0037, point 3).
    writeFile(folder / "atteso.txt",
              "messages:\n  Command: PLINE\n  Specify start point: 0,0\n"
              "  Specify next point or [Arc/Undo]: 10,0\n  Specify next point or [Arc/Undo]:\n");
    writeFile(folder / "plan.dxf", kDxf);
    writeFile(folder / "dati" / "altro.dxf", kDxf);
    const fs::path output = kRoot / "case-copies-output";
    const fs::path files = output / std::string{kCaseFilesFolder};
    writeFile(files / "stale.dxf", "saved by a previous run");

    EXPECT_EQ(check(folder, output, std::nullopt).code, kExitFailed);
    EXPECT_EQ(readFile(files / "plan.dxf"), kDxf);
    EXPECT_EQ(readFile(files / "dati" / "altro.dxf"), kDxf);
    EXPECT_FALSE(fs::exists(files / "stale.dxf"));
    EXPECT_FALSE(fs::exists(files / "script.tgs"));
    EXPECT_FALSE(fs::exists(files / "atteso.txt"));
}

TEST(CheckWithFiles, RunAcceptsSources) {
    const fs::path folder = emptyFolder("run-sources");
    writeFile(folder / "script.tgs", "PLINE\n0,0\n10,0\n\n");
    std::ostringstream withSources;
    std::ostringstream without;
    std::ostringstream err;
    EXPECT_EQ(runScriptFile(folder / "script.tgs", std::nullopt, withSources, err, sourcesFolder()),
              kExitPassed);
    EXPECT_EQ(runScriptFile(folder / "script.tgs", std::nullopt, without, err), kExitPassed);
    EXPECT_EQ(withSources.str(), without.str());
}

// ---------------------------------------------------------------------------
// The report

TEST(Level1WithFiles, AReportOfMilestone04StaysTheSame) {
    const std::string text = readFile(fs::path{TG_SCRIPT_TEST_DATA} / "pline.tgs");
    std::variant<Script, ParseError> parsed = parseScript(text);
    ASSERT_TRUE(std::holds_alternative<Script>(parsed));
    CaseFiles files{emptyFolder("level1"), sourcesFolder()};
    const std::string report = runLevel1(std::get<Script>(parsed), {&files});
    EXPECT_EQ(report, runLevel1(std::get<Script>(parsed)));
    EXPECT_EQ(report.find("drawing:"), std::string::npos);
    EXPECT_EQ(report.find("units:"), std::string::npos);
    EXPECT_EQ(report.find("unhandled:"), std::string::npos);
}

/// The section of the report for this editor, with an idle runner.
std::string section(edit::Editor& editor) {
    const cmd::CommandRegistry registry;
    const cmd::CommandRunner runner(registry, editor);
    std::string out;
    writeState({"final", editor, runner, {}, std::nullopt}, out);
    return out;
}

/// The line of `field` in `text`, or "" if it is not written.
std::string field(const std::string& text, std::string_view name) {
    const std::string start = "\n" + std::string{name} + ": ";
    const std::size_t at = text.find(start);
    if (at == std::string::npos) {
        return {};
    }
    const std::size_t end = text.find('\n', at + 1);
    return text.substr(at + 1, end - at - 1);
}

TEST(ReportFields, WrittenOnlyWhenDifferentFromANewSession) {
    core::Document document;
    edit::Editor editor(document);
    std::string text = section(editor);
    EXPECT_EQ(field(text, "drawing"), "");
    EXPECT_EQ(field(text, "units"), "");
    EXPECT_EQ(field(text, "unhandled"), "");

    // A drawing never saved: its changes alone write nothing.
    editor.beginTransaction("Line");
    static_cast<void>(editor.add(core::Line{{0.0, 0.0}, {1.0, 0.0}}));
    editor.commit();
    EXPECT_EQ(field(section(editor), "drawing"), "");

    editor.markSaved("plan.dxf");
    text = section(editor);
    EXPECT_EQ(field(text, "drawing"), "drawing: plan.dxf");
    // After ortho, before pointer, in this order.
    expectContains(text, "ortho: off\ndrawing: plan.dxf\npointer: none\n");

    editor.beginTransaction("INSUNITS");
    EXPECT_TRUE(editor.setUnits(6));
    editor.commit();
    text = section(editor);
    expectContains(text, "ortho: off\ndrawing: plan.dxf, modified\nunits: 6\npointer: none\n");

    // Undo back to the saved position: not modified, units of a new drawing.
    EXPECT_TRUE(editor.undo());
    text = section(editor);
    EXPECT_EQ(field(text, "drawing"), "drawing: plan.dxf");
    EXPECT_EQ(field(text, "units"), "");
}

TEST(ReportFields, UnhandledEntitiesByTypeInAlphabeticalOrder) {
    core::Document read;
    read.insertRaw({core::EntityId{0x30}, 1, "INSERT", {}});
    read.insertRaw({core::EntityId{0x31}, 2, "HATCH", {}});
    read.insertRaw({core::EntityId{0x32}, 3, "HATCH", {}});
    core::Document document;
    edit::Editor editor(document);
    editor.replaceDocument(std::move(read), "plan.dxf");
    const std::string text = section(editor);
    expectContains(text, "ortho: off\ndrawing: plan.dxf\nunhandled: HATCH 2, INSERT 1\n");
    // Not drawn, not listed among the entities nor in the statistics.
    expectContains(text, "entities:\nmessages:\n");
    expectContains(text, "statistics:\n");
    EXPECT_EQ(text.find("HATCH 2\n"), std::string::npos);
}

}  // namespace
}  // namespace tg::script
