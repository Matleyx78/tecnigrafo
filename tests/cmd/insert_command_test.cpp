// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// -INSERT (docs/tappe/0.8/PT21-comando-insert.md, "Test richiesti").

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/edit/editor.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

constexpr std::string_view kXScale =
    "Enter X scale factor, specify opposite corner, or [Corner/XYZ] <1>:";
constexpr std::string_view kYScale = "Enter Y scale factor <use X scale factor>:";
constexpr std::string_view kRotation = "Specify rotation angle <0>:";
constexpr std::string_view kPoint = "Specify insertion point or [Basepoint/Scale/Rotate]:";
constexpr std::string_view kName = "Enter block name or [?]:";

class MemoryFiles final : public io::FileAccess {
public:
    io::Result<std::string> read(std::string_view name) override {
        names.emplace_back(name);
        const auto found = contents.find(std::string{name});
        if (found == contents.end()) {
            return io::Error{io::Error::Kind::File, 0, "File not found."};
        }
        return found->second;
    }

    std::optional<io::Error> write(std::string_view name, std::string_view bytes) override {
        contents[std::string{name}] = bytes;
        return {};
    }

    [[nodiscard]] bool exists(std::string_view name) const override {
        return contents.contains(std::string{name});
    }

    std::map<std::string, std::string> contents;
    std::vector<std::string> names;  ///< every file read, in order
};

/// A DXF file whose model space holds `entity`, in `units`; with `layer`,
/// the entity is on a red layer of that name, which the drawing lacks.
std::string dxfFile(const core::Entity& entity, int units = 4, std::string_view layer = {}) {
    core::Document file = core::Document::newDrawing();
    file.setUnits(units);
    edit::Editor editor(file);
    editor.beginTransaction("File");
    const core::EntityId id = editor.add(entity);
    if (!layer.empty()) {
        core::Layer record;
        record.name = std::string{layer};
        record.color = core::Color::fromIndex(1);
        record.linetype = file.continuousLinetype();
        const std::optional<core::LayerId> added = editor.tables().addLayer(record);
        core::EntityProperties properties = file.find(id)->properties;
        properties.layer = added.value_or(properties.layer);
        editor.setProperties(id, properties);
    }
    editor.commit();
    return io::writeDxf(file, io::DxfVersion::R2018).value();
}

core::BlockDefinition lineBlock(std::string name, int units = 0, core::Vec2 end = {10, 0}) {
    core::BlockDefinition block{std::move(name)};
    block.units = units;
    block.entities.push_back({{}, core::Line{{0, 0}, end}});
    return block;
}

class InsertCommandTest : public ::testing::Test {
public:
    InsertCommandTest()
        : document(core::Document::newDrawing()), editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setFileAccess(&files);
        runner.setMessageSink(
            [this](std::string message) { messages.push_back(std::move(message)); });
        document.replaceBlock("Part", lineBlock("Part"));
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const std::string_view line : lines) {
            runner.submitText(line);
        }
    }

    [[nodiscard]] bool heard(std::string_view message) const {
        return std::ranges::find(messages, message) != messages.end();
    }

    /// A copy of the last insert of the drawing; a test failure, and a default
    /// insert, if there is none. A copy rather than a pointer, so that no
    /// assertion can dereference a null pointer.
    [[nodiscard]] core::Insert lastInsert() const {
        const core::Insert* last = nullptr;
        for (const core::EntityRecord& record : document.entities()) {
            if (const auto* insert = std::get_if<core::Insert>(&record.entity)) {
                last = insert;
            }
        }
        if (last == nullptr) {
            ADD_FAILURE() << "no insert in the drawing";
            return {};
        }
        return *last;
    }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    MemoryFiles files;
    std::vector<std::string> messages;
};

TEST_F(InsertCommandTest, ScaleAndRotationAfterThePoint) {
    type({"-INSERT", "Part", "10,20"});
    EXPECT_EQ(runner.promptText(), kXScale);
    runner.submitText("2");
    EXPECT_EQ(runner.promptText(), kYScale);
    runner.submitText("1");
    EXPECT_EQ(runner.promptText(), kRotation);
    runner.submitText("30");
    EXPECT_FALSE(runner.isActive());
    const core::Insert insert = lastInsert();
    EXPECT_EQ(insert.block, "Part");
    EXPECT_EQ(insert.insertion, (core::Vec2{10, 20}));
    EXPECT_EQ(insert.scaleX, 2.0);
    EXPECT_EQ(insert.scaleY, 1.0);
    EXPECT_DOUBLE_EQ(insert.rotation, core::degreesToRadians(30.0));
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Insert"}));
}

TEST_F(InsertCommandTest, ScaleAndRotateBeforeThePointAskNothingAfterIt) {
    type({"INSERT", "Part", "Scale"});
    EXPECT_EQ(runner.promptText(), kXScale);
    type({"3", "Rotate"});
    EXPECT_EQ(runner.promptText(), kRotation);
    type({"45"});
    EXPECT_EQ(runner.promptText(), kPoint);
    type({"5,5"});
    EXPECT_FALSE(runner.isActive());
    const core::Insert insert = lastInsert();
    EXPECT_EQ(insert.scaleX, 3.0);
    EXPECT_EQ(insert.scaleY, 3.0);
    EXPECT_DOUBLE_EQ(insert.rotation, core::degreesToRadians(45.0));
}

TEST_F(InsertCommandTest, OnlyScaleBeforeThePointAsksOnlyTheRotation) {
    type({"-INSERT", "Part", "Scale", "2", "5,5"});
    EXPECT_EQ(runner.promptText(), kRotation);
    runner.submitText("");
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(lastInsert().scaleY, 2.0);
    EXPECT_EQ(lastInsert().rotation, 0.0);
}

TEST_F(InsertCommandTest, OnlyRotateBeforeThePointAsksBothScalesButNotTheRotation) {
    type({"-INSERT", "Part", "Rotate", "90", "5,5"});
    EXPECT_EQ(runner.promptText(), kXScale);
    runner.submitText("");
    EXPECT_EQ(runner.promptText(), kYScale);
    runner.submitText("");
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(lastInsert().scaleX, 1.0);
    EXPECT_DOUBLE_EQ(lastInsert().rotation, core::degreesToRadians(90.0));
}

TEST_F(InsertCommandTest, OppositeCornerAndCornerGiveTheScalesFromTheInsertionPoint) {
    type({"-INSERT", "Part", "10,10", "14,12"});
    EXPECT_EQ(runner.promptText(), kRotation);
    runner.submitText("");
    EXPECT_EQ(lastInsert().scaleX, 4.0);
    EXPECT_EQ(lastInsert().scaleY, 2.0);

    type({"-INSERT", "Part", "0,0", "Corner"});
    EXPECT_EQ(runner.promptText(), "Specify opposite corner:");
    type({"-2,3", ""});
    EXPECT_EQ(lastInsert().scaleX, -2.0);
    EXPECT_EQ(lastInsert().scaleY, 3.0);

    // Given before the point, the corner waits for it: the diagonal starts
    // from the insertion point.
    type({"-INSERT", "Part", "Scale", "3,4", "1,1"});
    EXPECT_EQ(runner.promptText(), kRotation);
    runner.submitText("");
    EXPECT_EQ(lastInsert().insertion, (core::Vec2{1, 1}));
    EXPECT_EQ(lastInsert().scaleX, 2.0);
    EXPECT_EQ(lastInsert().scaleY, 3.0);

    // A diagonal without width is no scale.
    type({"-INSERT", "Part", "0,0", "0,5"});
    EXPECT_TRUE(heard("Value must be nonzero."));
    EXPECT_EQ(runner.promptText(), kXScale);
}

TEST_F(InsertCommandTest, AFileWithANewNameBecomesABlockWithItsLayer) {
    files.contents["nuovo.dxf"] = dxfFile(core::Line{{2, 3}, {12, 3}}, 4, "IMPORTED");
    type({"-INSERT", "nuovo.dxf"});
    EXPECT_EQ(runner.promptText(), kPoint);
    type({"10,0", "", "", ""});
    EXPECT_FALSE(runner.isActive());
    const core::BlockDefinition* block = document.blocks().find("nuovo");
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->entities.size(), 1U);
    EXPECT_EQ(std::get<core::Line>(block->entities[0].entity).start, (core::Vec2{2, 3}));
    EXPECT_NE(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(lastInsert().block, "nuovo");
    EXPECT_EQ(files.names, (std::vector<std::string>{"nuovo.dxf"}));
    EXPECT_FALSE(heard("Block \"nuovo\" defined"));
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Insert"}));

    // The name it proposes next is that of the block.
    runner.submitText("-INSERT");
    EXPECT_EQ(runner.promptText(), "Enter block name or [?] <nuovo>:");
    runner.submitText("");
    EXPECT_EQ(runner.promptText(), kPoint);
    runner.submit(CancelInput{});

    // U takes back the block and its layer with the insert.
    runner.submitText("U");
    EXPECT_EQ(document.blocks().find("nuovo"), nullptr);
    EXPECT_EQ(document.findLayer("IMPORTED"), nullptr);
}

TEST_F(InsertCommandTest, EscAfterAPlainFileLeavesNothing) {
    files.contents["nuovo.dxf"] = dxfFile(core::Line{{2, 3}, {12, 3}});
    type({"-INSERT", "nuovo.dxf"});
    runner.submit(CancelInput{});
    EXPECT_EQ(document.blocks().find("nuovo"), nullptr);
    EXPECT_TRUE(editor.undoNames().empty());
}

TEST_F(InsertCommandTest, AFileNamedAsABlockInsertsTheBlockWithoutReadingIt) {
    files.contents["lib/Part.dxf"] = dxfFile(core::Circle{{0, 0}, 1});
    type({"-INSERT", "lib/Part.dxf"});
    EXPECT_EQ(runner.promptText(), kPoint);
    type({"0,0", "", "", ""});
    EXPECT_TRUE(files.names.empty());
    const core::BlockDefinition* block = document.blocks().find("Part");
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(std::holds_alternative<core::Line>(block->entities[0].entity));
    EXPECT_EQ(lastInsert().block, "Part");
}

TEST_F(InsertCommandTest, NameEqualsPathRedefinesAnExistingBlockAndItsInserts) {
    editor.beginTransaction("Insert");
    editor.add(core::Insert{"Part", {5, 5}, 1, 1, 0, {}});
    editor.commit();
    EXPECT_EQ(document.extents().max().x, 15.0);
    files.contents["simbolo.dxf"] = dxfFile(core::Line{{20, 0}, {30, 0}});

    type({"-INSERT", "part=simbolo.dxf"});
    EXPECT_TRUE(heard("Block \"Part\" redefined"));
    EXPECT_EQ(runner.promptText(), kPoint);
    type({"0,0", "", "", ""});
    const core::BlockDefinition* block = document.blocks().find("Part");
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->name, "Part");
    EXPECT_EQ(std::get<core::Line>(block->entities[0].entity).end, (core::Vec2{30, 0}));
    // The insert that was there shows the new definition.
    EXPECT_EQ(document.extents().max().x, 35.0);

    type({"-INSERT", "Nuovo=simbolo.dxf"});
    EXPECT_TRUE(heard("Block \"Nuovo\" defined"));
    EXPECT_NE(document.blocks().find("Nuovo"), nullptr);
}

TEST_F(InsertCommandTest, TheFirstEqualsSignSeparatesTheNameFromThePath) {
    files.contents["a=b.dxf"] = dxfFile(core::Line{{100, 100}, {110, 100}});
    files.contents["b.dxf"] = dxfFile(core::Circle{{0, 0}, 2});
    type({"-INSERT", "a=b.dxf"});
    EXPECT_EQ(files.names, (std::vector<std::string>{"b.dxf"}));
    EXPECT_TRUE(heard("Block \"a\" defined"));
    const core::BlockDefinition* block = document.blocks().find("a");
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(std::holds_alternative<core::Circle>(block->entities[0].entity));
    EXPECT_EQ(document.blocks().find("b"), nullptr);
}

TEST_F(InsertCommandTest, ASecondEqualsSignIsAnInvalidFileName) {
    files.contents["a=b.dxf"] = dxfFile(core::Line{{100, 100}, {110, 100}});
    type({"-INSERT", "Part=a=b.dxf"});
    EXPECT_TRUE(heard("Invalid file name."));
    EXPECT_EQ(runner.promptText(), kName);
    EXPECT_TRUE(files.names.empty());

    runner.submitText("Unknown");
    EXPECT_TRUE(heard("Unknown block name: \"Unknown\"."));
    EXPECT_EQ(runner.promptText(), kName);
    runner.submitText("?");
    EXPECT_TRUE(heard("Part"));
}

TEST_F(InsertCommandTest, APathReusingABlockInMillimetresInMetresConvertsTheProposedScale) {
    document.setUnits(6);
    document.replaceBlock("assente", lineBlock("assente", 4));
    type({"-INSERT", "assente.dxf", "0,0"});
    EXPECT_EQ(runner.promptText(), kXScale);
    type({"", "", ""});
    EXPECT_FALSE(runner.isActive());
    EXPECT_TRUE(files.names.empty());
    EXPECT_DOUBLE_EQ(lastInsert().scaleX, 0.001);
    EXPECT_DOUBLE_EQ(lastInsert().scaleY, 0.001);
}

TEST_F(InsertCommandTest, EveryScaleFactorIsMultipliedByTheUnitFactor) {
    document.setUnits(6);
    document.replaceBlock("Mm", lineBlock("Mm", 4));
    type({"-INSERT", "Mm", "0,0", "", "", ""});
    EXPECT_DOUBLE_EQ(lastInsert().scaleX, 0.001);
    EXPECT_DOUBLE_EQ(lastInsert().scaleY, 0.001);

    type({"-INSERT", "Mm", "0,0", "2", "", ""});
    EXPECT_DOUBLE_EQ(lastInsert().scaleX, 0.002);
    EXPECT_DOUBLE_EQ(lastInsert().scaleY, 0.002);

    type({"-INSERT", "Mm", "Scale", "XYZ"});
    EXPECT_EQ(runner.promptText(), "Enter X scale factor <1>:");
    type({"2", "1"});
    EXPECT_EQ(runner.promptText(), "Enter Z scale factor <use X scale factor>:");
    type({"3", "0,0", ""});
    EXPECT_DOUBLE_EQ(lastInsert().scaleX, 0.002);
    EXPECT_DOUBLE_EQ(lastInsert().scaleY, 0.001);
    const core::EntityRecord& record = document.entities().back();
    ASSERT_FALSE(record.extras.specific.empty());
    EXPECT_EQ(record.extras.specific.back().code, 43);

    type({"-INSERT", "Mm", "0,0", "4,2", ""});
    EXPECT_DOUBLE_EQ(lastInsert().scaleX, 0.004);
    EXPECT_DOUBLE_EQ(lastInsert().scaleY, 0.002);
}

TEST_F(InsertCommandTest, UnitCodesZeroAndOutsideTheTableGiveFactorOne) {
    document.setUnits(6);
    document.replaceBlock("U0", lineBlock("U0", 0));
    document.replaceBlock("U24", lineBlock("U24", 24));
    document.replaceBlock("U25", lineBlock("U25", 25));
    type({"-INSERT", "U0", "0,0", "", "", ""});
    EXPECT_EQ(lastInsert().scaleX, 1.0);
    type({"-INSERT", "U24", "0,10", "", "", ""});
    EXPECT_DOUBLE_EQ(lastInsert().scaleX, 6336000.0 / 3937.0);
    EXPECT_DOUBLE_EQ(lastInsert().scaleY, 6336000.0 / 3937.0);
    type({"-INSERT", "U25", "0,20", "", "", ""});
    EXPECT_EQ(lastInsert().scaleX, 1.0);
    EXPECT_EQ(lastInsert().scaleY, 1.0);
    EXPECT_EQ(document.blocks().find("U25")->units, 25);
}

TEST_F(InsertCommandTest, EscAfterNameEqualsPathKeepsTheRedefinitionInOneStep) {
    editor.beginTransaction("Insert");
    editor.add(core::Insert{"Part", {5, 5}, 1, 1, 0, {}});
    editor.commit();
    const core::BlockDefinition original = *document.blocks().find("Part");
    files.contents["simbolo.dxf"] = dxfFile(core::Line{{20, 0}, {30, 0}}, 4, "IMPORTED");

    type({"-INSERT", "Part=simbolo.dxf"});
    runner.submit(CancelInput{});
    EXPECT_TRUE(heard("*Cancel*"));
    EXPECT_EQ(document.entityCount(), 1U);
    EXPECT_NE(*document.blocks().find("Part"), original);
    EXPECT_NE(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Insert", "Insert"}));
    runner.submitText("U");
    EXPECT_EQ(*document.blocks().find("Part"), original);
    EXPECT_EQ(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(document.entityCount(), 1U);

    // With a new name, also when Esc comes at the scale.
    type({"-INSERT", "Nuovo=simbolo.dxf", "0,0"});
    EXPECT_EQ(runner.promptText(), kXScale);
    runner.submit(CancelInput{});
    EXPECT_NE(document.blocks().find("Nuovo"), nullptr);
    EXPECT_NE(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(document.entityCount(), 1U);
    runner.submitText("U");
    EXPECT_EQ(document.blocks().find("Nuovo"), nullptr);
    EXPECT_EQ(document.findLayer("IMPORTED"), nullptr);
}

TEST_F(InsertCommandTest, BasepointChangesTheBasePointOfThisInsertOnly) {
    type({"-INSERT", "Part", "Basepoint"});
    EXPECT_EQ(runner.promptText(), "Specify base point <0,0>:");
    type({"5,0", "15,20", "", "", ""});
    EXPECT_EQ(lastInsert().insertion, (core::Vec2{10, 20}));
    EXPECT_EQ(document.blocks().find("Part")->base, (core::Vec2{0, 0}));

    // The base point lands on the insertion point after scale and rotation.
    type({"-INSERT", "Part", "Scale", "2", "Rotate", "90", "Basepoint", "5,0", "15,20"});
    EXPECT_NEAR(lastInsert().insertion.x, 15.0, 1e-12);
    EXPECT_NEAR(lastInsert().insertion.y, 10.0, 1e-12);
}

TEST_F(InsertCommandTest, ScaleZeroRepeatsThePromptAndUUndoesTheInsert) {
    type({"-INSERT", "Part", "0,0", "0"});
    EXPECT_TRUE(heard("Value must be nonzero."));
    EXPECT_EQ(runner.promptText(), kXScale);
    type({"1", "0"});
    EXPECT_EQ(runner.promptText(), kYScale);
    type({"", ""});
    EXPECT_EQ(document.entityCount(), 1U);
    runner.submitText("U");
    EXPECT_EQ(document.entityCount(), 0U);
}

TEST_F(InsertCommandTest, ThePreviewFollowsThePointer) {
    type({"-INSERT", "Part"});
    runner.pointerMoved({3, 4});
    ASSERT_EQ(runner.preview().size(), 1U);
    EXPECT_EQ(std::get<core::Insert>(runner.preview()[0]).insertion, (core::Vec2{3, 4}));
    runner.submit(CancelInput{});
    EXPECT_TRUE(runner.preview().empty());
}

}  // namespace
}  // namespace tg::cmd
