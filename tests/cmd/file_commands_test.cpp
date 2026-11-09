// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/file_commands.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/document_host.hpp"
#include "tg/cmd/drawing_session.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class MemoryFiles final : public io::FileAccess {
public:
    io::Result<std::string> read(std::string_view name) override {
        ++reads;
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
    int reads{0};
    bool failWrites{false};
};

class FileCommandsTest : public ::testing::Test {
public:
    FileCommandsTest() : editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setFileAccess(&files);
        runner.setDocumentFile(&file);
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
    edit::DocumentFile file;
    CommandRegistry registry;
    CommandRunner runner;
    MemoryFiles files;
    std::vector<std::string> messages;
};

TEST_F(FileCommandsTest, QsaveAsksOnceAndThenSavesImmediately) {
    runner.setDocumentFile(nullptr);
    changeUnits();
    runner.submitText("QSAVE");
    EXPECT_TRUE(runner.acceptsText());
    EXPECT_EQ(runner.promptText(), "Save drawing as <Drawing1.cadx>:");
    runner.submitText("");
    EXPECT_FALSE(runner.isActive());
    EXPECT_TRUE(files.exists("Drawing1.cadx"));
    EXPECT_TRUE(runner.documentFile().hasFile());
    EXPECT_FALSE(file.hasFile());
    EXPECT_FALSE(editor.isModified());
    inputs({"INSUNITS", "24", "QSAVE"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(files.writes, 2);
    const auto saved = io::readCadx(files.contents.at("Drawing1.cadx"));
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().document.units(), 24);
    EXPECT_TRUE(messages.empty());
}

TEST_F(FileCommandsTest, SaveasAndQsaveUseTheBorrowedDocumentFile) {
    inputs({"SAVEAS", "plan.cadx"});
    EXPECT_EQ(&runner.documentFile(), &file);
    EXPECT_TRUE(file.hasFile());
    EXPECT_EQ(file.name(), "plan.cadx");
    ASSERT_TRUE(files.exists("plan.cadx"));

    inputs({"INSUNITS", "6", "QSAVE"});
    EXPECT_EQ(files.writes, 2);
    const auto saved = io::readCadx(files.contents.at("plan.cadx"));
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().document.units(), 6);
    EXPECT_FALSE(editor.isModified());
}

TEST_F(FileCommandsTest, OpenAndNewUpdateTheFileBeforeNotifying) {
    auto drawing = core::Document::newDrawing();
    drawing.setUnits(6);
    const auto saved = io::writeDxf(drawing, io::DxfVersion::R2018);
    ASSERT_TRUE(saved.ok());
    files.contents["plan.dxf"] = saved.value();
    files.contents["invalid.cadx"] = "invalid drawing";
    ASSERT_EQ(file.name(), "Drawing1.cadx");
    ASSERT_FALSE(file.hasFile());
    int notifications = 0;
    editor.setChangeListener([&] {
        ++notifications;
        EXPECT_FALSE(editor.isModified());
        if (notifications == 1) {
            EXPECT_EQ(file.name(), "plan.dxf");
            EXPECT_TRUE(file.hasFile());
        } else {
            EXPECT_EQ(file.name(), "Drawing1.cadx");
            EXPECT_FALSE(file.hasFile());
        }
    });

    inputs({"OPEN", "plan.dxf"});
    EXPECT_EQ(notifications, 1);
    for (const char* name : {"missing.dxf", "invalid.cadx"}) {
        inputs({"OPEN", name});
        ASSERT_FALSE(messages.empty());
        EXPECT_TRUE(messages.back().starts_with(std::string{"Cannot open "} + name + ":"));
        EXPECT_EQ(notifications, 1);
        EXPECT_EQ(file.name(), "plan.dxf");
        EXPECT_TRUE(file.hasFile());
        EXPECT_EQ(document.units(), 6);
        EXPECT_FALSE(editor.isModified());
    }
    inputs({"NEW"});
    EXPECT_EQ(notifications, 2);
}

TEST_F(FileCommandsTest, SaveCopyPreservesAssociationAndSavedUndoPosition) {
    changeUnits();
    inputs({"SAVE", "copy.cadx"});
    EXPECT_FALSE(file.hasFile());
    EXPECT_TRUE(editor.isModified());
    inputs({"SAVEAS", "current.cadx", "INSUNITS", "0", "SAVE", "other.cadx"});
    EXPECT_EQ(file.name(), "current.cadx");
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
    EXPECT_EQ(file.name(), "a plan.CADX");
    ASSERT_TRUE(io::readCadx(files.contents.at("a plan.CADX")).ok());
    inputs({"SAVEAS", "drawing.DXF"});
    ASSERT_TRUE(io::readDxf(files.contents.at("drawing.DXF")).ok());
    inputs({"SAVEAS", "folder.with.dot/plan"});
    EXPECT_EQ(file.name(), "folder.with.dot/plan.cadx");
    inputs({"SAVEAS", "plan.dwg"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(file.name(), "folder.with.dot/plan.cadx");
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
        file.setSaved("old.dxf");
        editor.replaceDocument(std::move(opened.value().document));
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
    EXPECT_FALSE(file.hasFile());
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
    EXPECT_EQ(file.name(), "original.cadx");
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
        EXPECT_FALSE(file.hasFile());
    }
}

TEST_F(FileCommandsTest, FileNamePromptHasSaveDialogHint) {
    CommandContext context{{[](const std::string&) {
                           }},
                           editor,
                           file,
                           {}};
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
    EXPECT_FALSE(file.hasFile());
    EXPECT_EQ(file.name(), "Drawing1.cadx");
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
            file.reset("Drawing1.cadx");
            editor.replaceDocument(core::Document{});
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
            file.setSaved("saved.cadx");
            editor.replaceDocument(core::Document{});
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
        file.reset("Drawing1.cadx");
        editor.replaceDocument(core::Document{});
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
        file.setSaved("current.cadx");
        editor.markSaved();
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
        EXPECT_EQ(file.name(), name);
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
    const std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input.is_open());
    // Not std::istreambuf_iterator: GCC 13 at -O3 warns of a false null
    // dereference in it (-Wnull-dereference).
    std::ostringstream bytes;
    bytes << input.rdbuf();
    files.contents["future.cadx"] = std::move(bytes).str();
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
    CommandContext context{{[](const std::string&) {
                           }},
                           editor,
                           file,
                           {}};
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

TEST_F(FileCommandsTest, WithoutHostNewOfAModifiedDrawingAsksAndReplaces) {
    changeUnits();
    inputs({"NEW"});
    EXPECT_EQ(runner.promptText(), "Save changes to Drawing1.cadx? [Yes/No/Cancel] <Yes>:");
    inputs({"N"});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(document.units(), 4);
    EXPECT_FALSE(editor.canUndo());
}

TEST_F(FileCommandsTest, WithoutHostSaveasOfAnExistingFileAsksToReplace) {
    inputs({"SAVEAS", "a.cadx"});
    changeUnits();
    inputs({"NEW", "N", "SAVEAS", "a.cadx"});
    EXPECT_EQ(runner.promptText(),
              "A drawing with this name already exists. Do you want to replace it? [Yes/No] <No>:");
}

/// The host of NEW and OPEN for the tests: the sessions in a list, in the
/// order they were opened, one of them active, all writing to one history.
class TestHost final : public DocumentHost {
public:
    TestHost(const CommandRegistry& registry, io::FileAccess& files,
             std::vector<std::string>& messages)
        : m_registry(registry), m_files(files), m_messages(messages) {
        m_active = &add(core::Document::newDrawing(), edit::DocumentFile{});
    }

    DrawingSession& addDocument(core::Document document, edit::DocumentFile file) override {
        DrawingSession& session = add(std::move(document), std::move(file));
        activate(session);
        return session;
    }

    [[nodiscard]] DrawingSession* findOpen(std::string_view name) override {
        for (const auto& session : m_sessions) {
            if (session->file().name() == name) {
                return session.get();
            }
        }
        return nullptr;
    }

    void activate(DrawingSession& session) override { m_active = &session; }

    [[nodiscard]] std::vector<std::string> openNames() const override {
        std::vector<std::string> names;
        for (const auto& session : m_sessions) {
            if (!session->file().hasFile()) {
                names.push_back(session->file().name());
            }
        }
        return names;
    }

    [[nodiscard]] DrawingSession& active() const { return *m_active; }

    [[nodiscard]] DrawingSession& session(std::size_t index) const { return *m_sessions.at(index); }

    [[nodiscard]] std::size_t size() const { return m_sessions.size(); }

private:
    DrawingSession& add(core::Document document, edit::DocumentFile file) {
        DrawingSession& session = *m_sessions.emplace_back(
            std::make_unique<DrawingSession>(m_registry, &m_files, std::move(document)));
        session.file() = std::move(file);
        session.runner().setDocumentHost(this);
        session.runner().setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
        return session;
    }

    const CommandRegistry& m_registry;
    io::FileAccess& m_files;
    std::vector<std::string>& m_messages;
    std::vector<std::unique_ptr<DrawingSession>> m_sessions;
    DrawingSession* m_active{nullptr};
};

class HostedFileCommandsTest : public ::testing::Test {
public:
    HostedFileCommandsTest() {
        registerBuiltinCommands(registry);
        const auto plan = io::writeCadx(core::Document::newDrawing());
        EXPECT_TRUE(plan.ok());
        files.contents["plan.cadx"] = plan.value();
    }

    /// Each line goes to the runner of the document active at that moment.
    void inputs(std::initializer_list<const char*> lines) const {
        for (const char* line : lines) {
            host.active().runner().submitText(line);
        }
    }

    void line() const { inputs({"LINE", "0,0", "10,0", ""}); }

    [[nodiscard]] DrawingSession& session(std::size_t index) const { return host.session(index); }

    CommandRegistry registry;
    MemoryFiles files;
    std::vector<std::string> messages;
    TestHost host{registry, files, messages};
};

TEST_F(HostedFileCommandsTest, NewWithAModifiedDocumentAddsDrawing2WithoutAsking) {
    line();
    inputs({"NEW"});
    ASSERT_EQ(host.size(), 2U);
    EXPECT_EQ(&host.active(), &session(1));
    EXPECT_EQ(session(1).file().name(), "Drawing2.cadx");
    EXPECT_FALSE(session(1).file().hasFile());
    EXPECT_TRUE(session(1).document().isEmpty());
    // The first one stays, as it was, with its history and no question.
    EXPECT_FALSE(session(0).runner().isActive());
    EXPECT_TRUE(session(0).editor().isModified());
    EXPECT_EQ(session(0).document().entityCount(), 1U);
    EXPECT_EQ(session(0).editor().undoName(), "Line");
    EXPECT_TRUE(messages.empty());
}

TEST_F(HostedFileCommandsTest, NewTakesTheFirstUntitledNameNotOpen) {
    inputs({"SAVEAS", "a.cadx", "NEW", "NEW"});
    ASSERT_EQ(host.size(), 3U);
    // Drawing1.cadx is now a.cadx: its name is free again.
    EXPECT_EQ(session(1).file().name(), "Drawing1.cadx");
    EXPECT_EQ(session(2).file().name(), "Drawing2.cadx");
}

TEST_F(HostedFileCommandsTest, OpenNextToANewDrawingAddsADocumentAndKeepsTheDrawing) {
    inputs({"OPEN", "plan.cadx"});
    ASSERT_EQ(host.size(), 2U);
    EXPECT_EQ(&host.active(), &session(1));
    EXPECT_TRUE(session(1).file().hasFile());
    EXPECT_EQ(session(1).file().name(), "plan.cadx");
    EXPECT_FALSE(session(1).editor().isModified());
    EXPECT_FALSE(session(0).file().hasFile());
    EXPECT_EQ(session(0).file().name(), "Drawing1.cadx");
    EXPECT_FALSE(session(0).runner().isActive());
}

TEST_F(HostedFileCommandsTest, OpenWithAModifiedDocumentAddsADocumentWithoutAsking) {
    line();
    inputs({"OPEN"});
    EXPECT_EQ(host.active().runner().promptText(), "Enter name of drawing to open:");
    inputs({"plan.cadx"});
    ASSERT_EQ(host.size(), 2U);
    EXPECT_EQ(&host.active(), &session(1));
    EXPECT_TRUE(session(0).editor().isModified());
    EXPECT_EQ(session(0).document().entityCount(), 1U);
}

TEST_F(HostedFileCommandsTest, OpenOfAFileAlreadyOpenActivatesItWithoutReading) {
    inputs({"OPEN", "plan.cadx"});
    line();
    host.activate(session(0));
    const int reads = files.reads;
    inputs({"OPEN", "plan.cadx"});
    ASSERT_EQ(host.size(), 2U);
    EXPECT_EQ(&host.active(), &session(1));
    EXPECT_EQ(files.reads, reads);
    ASSERT_FALSE(messages.empty());
    EXPECT_EQ(messages.back(), "plan.cadx is already open.");
    // Its changes and its history stay.
    EXPECT_TRUE(session(1).editor().isModified());
    EXPECT_EQ(session(1).document().entityCount(), 1U);
    EXPECT_FALSE(session(0).runner().isActive());
}

TEST_F(HostedFileCommandsTest, OpenFailuresLeaveTheDocumentsAsTheyAre) {
    line();
    inputs({"OPEN", "missing.cadx"});
    EXPECT_EQ(host.size(), 1U);
    EXPECT_EQ(&host.active(), &session(0));
    EXPECT_EQ(messages.back(), "Cannot open missing.cadx: File not found.");
    EXPECT_TRUE(session(0).editor().isModified());
}

TEST_F(HostedFileCommandsTest, SavingOnAFileOpenInAnotherDocumentIsRefused) {
    line();
    inputs({"SAVEAS", "a.cadx", "NEW"});
    line();
    inputs({"SAVEAS", "b.cadx"});
    DrawingSession& b = session(1);
    ASSERT_EQ(&host.active(), &b);
    line();
    const int writes = files.writes;
    const std::vector<std::string> undo = b.editor().undoNames();
    for (const char* command : {"SAVEAS", "SAVE"}) {
        SCOPED_TRACE(command);
        inputs({command, "a.cadx"});
        // Refused before the question to replace, and nothing is written.
        EXPECT_FALSE(b.runner().isActive());
        EXPECT_EQ(messages.back(), "Cannot save a.cadx: it is open in another document.");
        EXPECT_EQ(files.writes, writes);
        EXPECT_EQ(b.file().name(), "b.cadx");
        EXPECT_TRUE(b.editor().isModified());
        EXPECT_EQ(b.editor().undoNames(), undo);
    }
    // The document of a.cadx saves on its own file, as it always did.
    host.activate(session(0));
    inputs({"SAVEAS", "a.cadx"});
    EXPECT_EQ(session(0).runner().promptText(),
              "A drawing with this name already exists. Do you want to replace it? [Yes/No] <No>:");
    inputs({"Y"});
    EXPECT_FALSE(session(0).runner().isActive());
    EXPECT_EQ(files.writes, writes + 1);
    EXPECT_EQ(session(0).file().name(), "a.cadx");
}

TEST_F(HostedFileCommandsTest, QsaveOfAnUntitledDocumentOnAnOpenFileIsRefused) {
    inputs({"SAVEAS", "a.cadx", "NEW"});
    line();
    inputs({"QSAVE", "a"});
    EXPECT_FALSE(host.active().runner().isActive());
    EXPECT_EQ(messages.back(), "Cannot save a.cadx: it is open in another document.");
    EXPECT_FALSE(host.active().file().hasFile());
}

}  // namespace
}  // namespace tg::cmd
