// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/file_commands.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace tg::cmd {
namespace {

class MemoryFiles final : public io::FileAccess {
public:
    io::Result<std::string> read(std::string_view name) override {
        const auto found = contents.find(std::string{name});
        if (found == contents.end()) {
            return io::Error{io::Error::Kind::File, 0, "File not found."};
        }
        return found->second;
    }

    std::optional<io::Error> write(std::string_view name, std::string_view bytes) override {
        if (failWrites) {
            return io::Error{io::Error::Kind::File, 0, "Permission denied."};
        }
        contents[std::string{name}] = bytes;
        ++writes;
        return {};
    }

    [[nodiscard]] bool exists(std::string_view name) const override {
        return contents.contains(std::string{name});
    }

    std::map<std::string, std::string> contents;
    int writes{0};
    bool failWrites{false};
};

class FileCommandsTest : public ::testing::Test {
public:
    FileCommandsTest() : editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setFileAccess(&files);
        runner.setMessageSink(
            [this](std::string message) { messages.push_back(std::move(message)); });
    }

    void inputs(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            runner.submitText(line);
        }
    }

    void changeUnits() { inputs({"INSUNITS", "6"}); }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    MemoryFiles files;
    std::vector<std::string> messages;
};

TEST_F(FileCommandsTest, QsaveAsksOnceAndThenSavesImmediately) {
    changeUnits();
    runner.submitText("QSAVE");
    EXPECT_TRUE(runner.acceptsText());
    EXPECT_EQ(runner.promptText(), "Save drawing as <Drawing1.cadx>:");
    runner.submitText("");
    EXPECT_FALSE(runner.isActive());
    EXPECT_TRUE(files.exists("Drawing1.cadx"));
    EXPECT_TRUE(editor.hasFile());
    EXPECT_FALSE(editor.isModified());
    inputs({"INSUNITS", "24", "QSAVE"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(files.writes, 2);
    const auto saved = io::readCadx(files.contents.at("Drawing1.cadx"));
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().document.units(), 24);
    EXPECT_TRUE(messages.empty());
}

TEST_F(FileCommandsTest, SaveCopyPreservesAssociationAndSavedUndoPosition) {
    changeUnits();
    inputs({"SAVE", "copy.cadx"});
    EXPECT_FALSE(editor.hasFile());
    EXPECT_TRUE(editor.isModified());
    inputs({"SAVEAS", "current.cadx", "INSUNITS", "0", "SAVE", "other.cadx"});
    EXPECT_EQ(editor.fileName(), "current.cadx");
    EXPECT_TRUE(editor.isModified());
    inputs({"SAVE", ""});
    EXPECT_TRUE(runner.isActive());
    runner.submitText("Yes");
    EXPECT_FALSE(editor.isModified());
    runner.submitText("U");
    EXPECT_TRUE(editor.isModified());
    runner.submitText("REDO");
    EXPECT_FALSE(editor.isModified());
}

TEST_F(FileCommandsTest, FormatsExtensionsAndQuotedNamesWithSpaces) {
    inputs({"SAVEAS", "\"a plan.CADX\""});
    EXPECT_EQ(editor.fileName(), "a plan.CADX");
    ASSERT_TRUE(io::readCadx(files.contents.at("a plan.CADX")).ok());
    inputs({"SAVEAS", "drawing.DXF"});
    ASSERT_TRUE(io::readDxf(files.contents.at("drawing.DXF")).ok());
    inputs({"SAVEAS", "folder.with.dot/plan"});
    EXPECT_EQ(editor.fileName(), "folder.with.dot/plan.cadx");
    inputs({"SAVEAS", "plan.dwg"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(editor.fileName(), "folder.with.dot/plan.cadx");
    ASSERT_EQ(messages.size(), 1U);
    EXPECT_EQ(messages.back(), "Cannot save plan.dwg: only .cadx and .dxf files are supported.");
    EXPECT_EQ(files.writes, 3);
}

TEST_F(FileCommandsTest, CurrentDxfPreservesReadVersionAndSaveasWritesR2018) {
    for (const char* version : {"AC1015", "AC1018", "AC1021", "AC1024", "AC1027", "AC1032"}) {
        // An opened drawing may be old; creating a new one always writes R2018.
        const std::string bytes = std::string{"0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n"} + version +
                                  "\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
        auto opened = io::readDxf(bytes);
        ASSERT_TRUE(opened.ok());
        const auto original = opened.value().document.raw().version;
        editor.replaceDocument(std::move(opened.value().document), "old.dxf");
        files.contents["old.dxf"] = bytes;
        inputs({"INSUNITS", "6", "QSAVE"});
        auto saved = io::readDxf(files.contents.at("old.dxf"));
        ASSERT_TRUE(saved.ok());
        EXPECT_EQ(saved.value().document.raw().version, original);
        inputs({"SAVE", "", "Y"});
        saved = io::readDxf(files.contents.at("old.dxf"));
        ASSERT_TRUE(saved.ok());
        EXPECT_EQ(saved.value().document.raw().version, original);
        files.contents.erase("new.dxf");
        inputs({"SAVEAS", "new.dxf"});
        saved = io::readDxf(files.contents.at("new.dxf"));
        ASSERT_TRUE(saved.ok());
        EXPECT_EQ(saved.value().document.raw().version, "AC1032");
    }
}

TEST_F(FileCommandsTest, ReplaceDefaultsToNoAndEscapeNeverWrites) {
    files.contents["existing.cadx"] = "original";
    changeUnits();
    runner.submitText("SAVEAS");
    runner.submit(CancelInput{});
    EXPECT_FALSE(editor.hasFile());
    for (const char* answer : {"", "N", "No"}) {
        inputs({"SAVEAS", "existing.cadx"});
        EXPECT_FALSE(runner.acceptsText());
        EXPECT_EQ(runner.promptText(),
                  "A drawing with this name already exists. Do you want to replace it? "
                  "[Yes/No] <No>:");
        runner.submitText(answer);
        EXPECT_EQ(runner.promptText(), "Save drawing as <Drawing1.cadx>:");
        runner.submit(CancelInput{});
    }
    inputs({"SAVEAS", "existing.cadx"});
    runner.submit(CancelInput{});
    EXPECT_EQ(files.writes, 0);
    EXPECT_EQ(files.contents.at("existing.cadx"), "original");
    EXPECT_TRUE(editor.isModified());
    inputs({"SAVEAS", "existing.cadx", "Y"});
    EXPECT_EQ(files.writes, 1);
    EXPECT_FALSE(editor.isModified());
}

TEST_F(FileCommandsTest, WriteErrorKeepsFileDrawingAndUndoHistory) {
    inputs({"SAVEAS", "original.cadx"});
    const std::string previous = files.contents.at("original.cadx");
    changeUnits();
    const auto history = editor.undoNames();
    files.failWrites = true;
    inputs({"SAVEAS", "other.cadx"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(editor.fileName(), "original.cadx");
    EXPECT_TRUE(editor.isModified());
    EXPECT_EQ(document.units(), 6);
    EXPECT_EQ(editor.undoNames(), history);
    EXPECT_EQ(files.contents.at("original.cadx"), previous);
    EXPECT_FALSE(files.exists("other.cadx"));
    EXPECT_EQ(messages.back(), "Cannot save other.cadx: Permission denied.");
    runner.submitText("QSAVE");
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(files.contents.at("original.cadx"), previous);
}

TEST_F(FileCommandsTest, UnitsValidateIntegersAndSupportUndoRedoAndEscape) {
    runner.submitText("INSUNITS");
    for (const char* value : {"25", "-1", "abc", "4.0", "99999999999999999"}) {
        runner.submitText(value);
        EXPECT_EQ(document.units(), 4);
        EXPECT_TRUE(runner.isActive());
        EXPECT_EQ(messages.back(), "Requires an integer between 0 and 24.");
    }
    runner.submitText("");
    EXPECT_FALSE(editor.canUndo());
    inputs({"INSUNITS", "0", "INSUNITS", "24"});
    EXPECT_EQ(document.units(), 24);
    EXPECT_EQ(editor.undoName(), "INSUNITS");
    inputs({"U"});
    EXPECT_EQ(document.units(), 0);
    inputs({"U"});
    EXPECT_EQ(document.units(), 4);
    inputs({"REDO", "REDO"});
    EXPECT_EQ(document.units(), 24);
    runner.submitText("INSUNITS");
    EXPECT_EQ(runner.promptText(), "Enter new value for INSUNITS <24>:");
    runner.submit(CancelInput{});
    EXPECT_EQ(document.units(), 24);
    EXPECT_FALSE(editor.inTransaction());
}

TEST_F(FileCommandsTest, MissingFileAccessEndsSaveCommandsAndPreservesDrawing) {
    runner.setFileAccess(nullptr);
    changeUnits();
    for (const char* command : {"QSAVE", "SAVE", "SAVEAS"}) {
        runner.submitText(command);
        EXPECT_FALSE(runner.isActive());
        EXPECT_EQ(messages.back(), "File access is not available.");
        EXPECT_TRUE(editor.isModified());
        EXPECT_FALSE(editor.hasFile());
    }
}

TEST_F(FileCommandsTest, FileNamePromptHasSaveDialogHint) {
    CommandContext context{editor, {}, [](const std::string&) {
                           }};
    context.files = &files;
    SaveCommand command;
    command.start(context);
    EXPECT_EQ(command.prompt().kind, PromptKind::Text);
    EXPECT_EQ(command.prompt().file, FilePrompt::Save);
}

TEST_F(FileCommandsTest, NewResetsDocumentAndHistoryWithoutAskingWhenClean) {
    inputs({"NEW"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_TRUE(messages.empty());
    changeUnits();
    inputs({"QSAVE", "old.cadx", "NEW"});
    EXPECT_FALSE(editor.hasFile());
    EXPECT_EQ(editor.fileName(), "Drawing1.cadx");
    EXPECT_EQ(document.units(), 4);
    EXPECT_FALSE(editor.isModified());
    EXPECT_FALSE(editor.canUndo());
    EXPECT_FALSE(editor.canRedo());
}

TEST_F(FileCommandsTest, NewStartsANewDrawingWhoseFirstEntityIs257) {
    const auto addSegment = [this] {
        core::Polyline segment;
        segment.addVertex({0.0, 0.0});
        segment.addVertex({10.0, 0.0});
        editor.beginTransaction("Segment");
        const core::EntityId id = editor.add(core::Entity{segment});
        editor.commit();
        return id;
    };
    addSegment();
    addSegment();
    inputs({"QSAVE", "old.cadx", "NEW"});
    EXPECT_TRUE(document.isEmpty());
    EXPECT_EQ(document.layers().size(), 1U);
    EXPECT_EQ(document.linetypes().size(), 3U);
    EXPECT_EQ(document.layerZero().value, 253U);
    EXPECT_EQ(document.nextId(), 257U);
    EXPECT_EQ(addSegment().value, 257U);
    EXPECT_EQ(addSegment().value, 258U);
}

TEST_F(FileCommandsTest, CancelAndEscapePreserveUnsavedDrawingForBothCommands) {
    for (const char* command : {"NEW", "OPEN"}) {
        changeUnits();
        for (const char* answer : {"C", "Cancel"}) {
            inputs({command, answer});
            EXPECT_FALSE(runner.isActive());
            EXPECT_EQ(document.units(), 6);
            EXPECT_TRUE(editor.isModified());
            EXPECT_EQ(messages.back(), "*Cancel*");
        }
        runner.submitText(command);
        runner.submit(CancelInput{});
        EXPECT_TRUE(editor.isModified());
        EXPECT_EQ(document.units(), 6);
    }
}

TEST_F(FileCommandsTest, SavingBeforeReplacementUsesQsaveWithAndWithoutCurrentFile) {
    for (const char* command : {"NEW", "OPEN"}) {
        for (const char* answer : {"Y", "Yes", ""}) {
            editor.replaceDocument(core::Document{}, "");
            files.contents.clear();
            changeUnits();
            inputs({command, answer});
            EXPECT_EQ(runner.promptText(), "Save drawing as <Drawing1.cadx>:");
            runner.submitText("saved.cadx");
            ASSERT_TRUE(files.exists("saved.cadx"));
            const auto saved = io::readCadx(files.contents.at("saved.cadx"));
            ASSERT_TRUE(saved.ok());
            EXPECT_EQ(saved.value().document.units(), 6);
            if (runner.isActive()) {
                runner.submit(CancelInput{});
            }
            editor.replaceDocument(core::Document{}, "saved.cadx");
            changeUnits();
            const int writes = files.writes;
            inputs({command, answer});
            EXPECT_EQ(files.writes, writes + 1);
            EXPECT_FALSE(editor.isModified());
            if (runner.isActive()) {
                EXPECT_EQ(runner.promptText(), "Enter name of drawing to open:");
                runner.submit(CancelInput{});
            }
        }
    }
}

TEST_F(FileCommandsTest, FailedOrCancelledSaveStopsReplacement) {
    for (const char* command : {"NEW", "OPEN"}) {
        editor.replaceDocument(core::Document{}, "");
        changeUnits();
        inputs({command, "Y"});
        runner.submit(CancelInput{});
        EXPECT_EQ(document.units(), 6);
        EXPECT_TRUE(editor.isModified());
        files.failWrites = true;
        inputs({command, "Y", "failed.cadx"});
        EXPECT_FALSE(runner.isActive());
        EXPECT_EQ(document.units(), 6);
        EXPECT_TRUE(editor.isModified());
        EXPECT_TRUE(editor.canUndo());
        EXPECT_EQ(messages.back(), "Cannot save failed.cadx: Permission denied.");
        editor.markSaved("current.cadx");
        inputs({"INSUNITS", "0", command, "Y"});
        EXPECT_FALSE(runner.isActive());
        EXPECT_TRUE(editor.isModified());
        EXPECT_EQ(document.units(), 0);
        files.failWrites = false;
    }
}

TEST_F(FileCommandsTest, OpenReadsBothFormatsAndQsaveWritesOpenedFile) {
    core::Document other;
    other.setUnits(1);
    auto cadx = io::writeCadx(other);
    auto old = io::readDxf(
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n9\n$DWGCODEPAGE\n3\nANSI_1252\n9\n$"
        "INSUNITS\n70\n1\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");
    ASSERT_TRUE(old.ok());
    auto dxf = io::writeDxf(old.value().document, io::DxfVersion::R2000);
    ASSERT_TRUE(cadx.ok());
    ASSERT_TRUE(dxf.ok());
    files.contents["plan.CADX"] = cadx.value();
    files.contents["plan.DXF"] = dxf.value();
    files.contents["@sources/public-dxf"] = dxf.value();
    for (const char* name : {"plan.CADX", "plan.DXF", "@sources/public-dxf"}) {
        inputs({"OPEN", ""});
        EXPECT_EQ(runner.promptText(), "Enter name of drawing to open:");
        runner.submitText(name);
        EXPECT_FALSE(runner.isActive());
        EXPECT_EQ(editor.fileName(), name);
        EXPECT_EQ(document.units(), 1);
        EXPECT_FALSE(editor.isModified());
        EXPECT_FALSE(editor.canUndo());
        EXPECT_FALSE(editor.canRedo());
    }
    inputs({"OPEN", "plan.DXF", "INSUNITS", "6", "QSAVE"});
    EXPECT_FALSE(editor.isModified());
    auto saved = io::readDxf(files.contents.at("plan.DXF"));
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().document.raw().version, "AC1015");
    EXPECT_EQ(saved.value().document.units(), 6);
    EXPECT_TRUE(messages.empty());
}

TEST_F(FileCommandsTest, OpenFailuresPreserveDocumentAndHistory) {
    changeUnits();
    files.contents["bad.dxf"] = "abc\nSECTION\n";
    files.contents["bad.cadx"] = "not a ZIP";
    for (const char* name : {"bad.dwg", "no-extension", "missing.dxf", "bad.dxf", "bad.cadx"}) {
        const auto history = editor.undoNames();
        inputs({"OPEN", "N", name});
        EXPECT_FALSE(runner.isActive());
        EXPECT_EQ(document.units(), 6);
        EXPECT_TRUE(editor.isModified());
        EXPECT_EQ(editor.undoNames(), history);
        EXPECT_TRUE(messages.back().starts_with(std::string{"Cannot open "} + name + ": "));
    }
    EXPECT_EQ(messages[messages.size() - 2],
              "Cannot open bad.dxf: line 1: expected a group code (an integer from -5 to 1071), "
              "found \"abc\"");
}

TEST_F(FileCommandsTest, ReplacementClearsSelectionRectangleAndLastPoint) {
    for (const char* command : {"NEW", "OPEN"}) {
        files.contents.clear();
        inputs({"LINE", "0,0", "10,0", "", "QSAVE", "current.cadx"});
        runner.click({5, 0}, PickAction::Add);
        EXPECT_FALSE(runner.selection().isEmpty());
        runner.submitText(command);
        if (runner.isActive()) {
            runner.submitText("current.cadx");
        }
        EXPECT_TRUE(runner.selection().isEmpty());
        EXPECT_FALSE(runner.selectionRectangle().has_value());
        EXPECT_FALSE(editor.canUndo());
        EXPECT_FALSE(editor.canRedo());
        runner.submitText("LINE");
        runner.submitText("@");
        EXPECT_EQ(runner.promptText(), "Specify first point:");
        runner.submit(CancelInput{});
    }
}

TEST_F(FileCommandsTest, OpenReportsUnhandledTypesAlphabeticallyAndReadWarnings) {
    const std::string header = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n";
    const std::string tail = "0\nENDSEC\n0\nEOF\n";
    files.contents["single.dxf"] = header + "0\nSECTION\n2\nENTITIES\n0\nINSERT\n5\nA\n" + tail;
    files.contents["many.dxf"] = header +
                                 "0\nSECTION\n2\nENTITIES\n0\nINSERT\n5\nA\n"
                                 "0\nHATCH\n5\nB\n0\nHATCH\n5\nC\n" +
                                 tail;
    for (const char* name : {"single.dxf", "many.dxf"}) {
        const auto read = io::readDxf(files.contents.at(name));
        ASSERT_TRUE(read.ok());
        messages.clear();
        inputs({"OPEN", name});
        ASSERT_EQ(messages.size(), 1U + read.value().report.warnings.size());
        EXPECT_EQ(messages.front(), std::string{name} == "single.dxf"
                                        ? "1 entity not supported: INSERT 1"
                                        : "3 entities not supported: HATCH 2, INSERT 1");
        for (std::size_t i = 0; i < read.value().report.warnings.size(); ++i) {
            EXPECT_EQ(messages[i + 1], read.value().report.warnings[i]);
        }
    }
}

TEST_F(FileCommandsTest, FutureCadxVersionPreservesDrawingAndReportsReaderError) {
    const auto path = std::filesystem::path{__FILE__}.parent_path().parent_path() /
                      "verifica/casi/apertura/open-errori-e-limiti/versione-3.cadx";
    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    files.contents["future.cadx"] = std::string{std::istreambuf_iterator<char>{input}, {}};
    changeUnits();
    inputs({"OPEN", "N", "future.cadx"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_TRUE(editor.isModified());
    EXPECT_TRUE(editor.canUndo());
    EXPECT_EQ(document.units(), 6);
    EXPECT_EQ(messages.back(),
              "Cannot open future.cadx: written by a newer version of Tecnigrafo "
              "(format 3; this version reads up to 1)");
}

TEST_F(FileCommandsTest, OpenEmitsEveryReaderWarningAfterUnsupportedSummary) {
    files.contents["warning.dxf"] = "0\nSECTION\n2\nENTITIES\n0\nINSERT\n0\nENDSEC\n0\nEOF\n";
    const auto read = io::readDxf(files.contents.at("warning.dxf"));
    ASSERT_TRUE(read.ok());
    ASSERT_FALSE(read.value().report.warnings.empty());
    inputs({"OPEN", "warning.dxf"});
    ASSERT_EQ(messages.size(), 1U + read.value().report.warnings.size());
    EXPECT_EQ(messages.front(), "1 entity not supported: INSERT 1");
    for (std::size_t i = 0; i < read.value().report.warnings.size(); ++i) {
        EXPECT_EQ(messages[i + 1], read.value().report.warnings[i]);
    }
}

TEST_F(FileCommandsTest, OpenPromptHasDialogHintAndMissingAccessEndsCommand) {
    CommandContext context{editor, {}, [](const std::string&) {
                           }};
    context.files = &files;
    OpenCommand command;
    command.start(context);
    EXPECT_EQ(command.prompt().kind, PromptKind::Text);
    EXPECT_EQ(command.prompt().file, FilePrompt::Open);
    runner.setFileAccess(nullptr);
    runner.submitText("OPEN");
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(messages.back(), "File access is not available.");
}

}  // namespace
}  // namespace tg::cmd
