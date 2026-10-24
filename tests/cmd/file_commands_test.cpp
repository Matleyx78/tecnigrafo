// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/file_commands.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

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
    for (const auto version :
         {io::DxfVersion::R2000, io::DxfVersion::R2004, io::DxfVersion::R2007,
          io::DxfVersion::R2010, io::DxfVersion::R2013, io::DxfVersion::R2018}) {
        const auto bytes = io::writeDxf(core::Document{}, version);
        ASSERT_TRUE(bytes.ok());
        auto opened = io::readDxf(bytes.value());
        ASSERT_TRUE(opened.ok());
        const auto original = opened.value().document.raw().version;
        editor.replaceDocument(std::move(opened.value().document), "old.dxf");
        files.contents["old.dxf"] = bytes.value();
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

}  // namespace
}  // namespace tg::cmd
