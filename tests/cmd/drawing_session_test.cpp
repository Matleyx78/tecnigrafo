// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/drawing_session.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/io/file_access.hpp"

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace tg::cmd {
namespace {

/// Files kept in memory, so that SAVEAS needs no disk.
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
        contents[std::string{name}] = bytes;
        return {};
    }

    [[nodiscard]] bool exists(std::string_view name) const override {
        return contents.contains(std::string{name});
    }

    std::map<std::string, std::string> contents;
};

class DrawingSessionTest : public testing::Test {
protected:
    DrawingSessionTest() { registerBuiltinCommands(m_registry); }

    [[nodiscard]] const CommandRegistry& registry() const { return m_registry; }

    [[nodiscard]] MemoryFiles& files() { return m_files; }

    /// A line from (x, 0) to (x, 10), committed as one LINE.
    static void drawLine(DrawingSession& session, double x) {
        session.runner().submitText("LINE");
        session.runner().submit(PointInput{{x, 0.0}});
        session.runner().submit(PointInput{{x, 10.0}});
        session.runner().submitText("");
    }

private:
    CommandRegistry m_registry;
    MemoryFiles m_files;
};

TEST_F(DrawingSessionTest, StartsWithTheDocumentGivenAndAFreshHistoryAndFile) {
    core::Document document = core::Document::newDrawing();
    core::Line line;
    line.end = {5.0, 5.0};
    document.add(line);
    const DrawingSession session(registry(), &files(), std::move(document));
    EXPECT_EQ(session.document().entities().size(), 1U);
    EXPECT_EQ(&session.editor().document(), &session.document());
    EXPECT_FALSE(session.editor().canUndo());
    EXPECT_FALSE(session.editor().isModified());
    EXPECT_FALSE(session.file().hasFile());
    EXPECT_EQ(session.file().name(), "Drawing1.cadx");
    EXPECT_EQ(&session.runner().documentFile(), &session.file());
    EXPECT_FALSE(session.runner().isActive());
}

TEST_F(DrawingSessionTest, DefaultsToANewDrawing) {
    const DrawingSession session(registry(), nullptr);
    const core::Document fresh = core::Document::newDrawing();
    EXPECT_TRUE(session.document().entities().empty());
    EXPECT_EQ(session.document().layers().size(), fresh.layers().size());
}

TEST_F(DrawingSessionTest, CanBeDestroyedWithACommandHalfway) {
    auto session = std::make_unique<DrawingSession>(registry(), &files());
    session->runner().submitText("LINE");
    session->runner().submit(PointInput{{0.0, 0.0}});
    ASSERT_TRUE(session->runner().isActive());
    // The runner, holding the open transaction, goes before the editor and
    // the document it works on: nothing is left dangling (the sanitizers
    // of the CI check it).
    session.reset();
    SUCCEED();
}

TEST_F(DrawingSessionTest, TheRunnerWorksOnTheSessionsDocumentAndFile) {
    DrawingSession session(registry(), &files());
    drawLine(session, 1.0);
    ASSERT_EQ(session.document().entities().size(), 1U);
    EXPECT_TRUE(std::holds_alternative<core::Line>(session.document().entities()[0].entity));
    EXPECT_TRUE(session.editor().canUndo());
    EXPECT_TRUE(session.editor().isModified());

    session.runner().submitText("SAVEAS");
    session.runner().submitText("plan.cadx");
    EXPECT_FALSE(session.runner().isActive());
    EXPECT_TRUE(files().exists("plan.cadx"));
    EXPECT_TRUE(session.file().hasFile());
    EXPECT_EQ(session.file().name(), "plan.cadx");
    EXPECT_FALSE(session.editor().isModified());
}

TEST_F(DrawingSessionTest, WithoutFileAccessTheFileCommandsAreDisabled) {
    DrawingSession session(registry(), nullptr);
    drawLine(session, 1.0);
    session.runner().submitText("SAVEAS");
    session.runner().submitText("plan.cadx");
    EXPECT_FALSE(session.runner().isActive());
    EXPECT_FALSE(session.file().hasFile());
    EXPECT_TRUE(session.editor().isModified());
}

TEST_F(DrawingSessionTest, TwoSessionsHaveSeparateHistories) {
    DrawingSession first(registry(), &files());
    DrawingSession second(registry(), &files());
    drawLine(first, 1.0);
    drawLine(second, 2.0);
    drawLine(second, 3.0);
    ASSERT_EQ(first.document().entities().size(), 1U);
    ASSERT_EQ(second.document().entities().size(), 2U);

    first.runner().submitText("U");
    EXPECT_TRUE(first.document().entities().empty());
    EXPECT_FALSE(first.editor().canUndo());
    EXPECT_EQ(second.document().entities().size(), 2U);
    EXPECT_EQ(second.editor().undoNames().size(), 2U);

    second.runner().submitText("U");
    EXPECT_EQ(second.document().entities().size(), 1U);
    EXPECT_TRUE(first.editor().canRedo());
    EXPECT_TRUE(first.document().entities().empty());

    // A command half way in one session leaves the other one idle.
    first.runner().submitText("LINE");
    EXPECT_TRUE(first.runner().isActive());
    EXPECT_FALSE(second.runner().isActive());
}

}  // namespace
}  // namespace tg::cmd
