// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/file_access.hpp"

#include "tg/io/result.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#ifndef TG_IO_TEST_OUTPUT
#error "TG_IO_TEST_OUTPUT must name a folder for the files of these tests"
#endif

namespace tg::io {
namespace {

namespace fs = std::filesystem;

/// A UTF-8 name as a path, as DiskFiles does.
fs::path pathOf(std::string_view name) {
    std::u8string utf8;
    for (const char c : name) {
        utf8.push_back(static_cast<char8_t>(c));
    }
    return fs::path{utf8};
}

/// A UTF-8 path as the name DiskFiles takes.
std::string nameOf(const fs::path& path) {
    std::string name;
    for (const char8_t c : path.u8string()) {
        name.push_back(static_cast<char>(c));
    }
    return name;
}

class DiskFilesTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* const info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_folder = fs::path{TG_IO_TEST_OUTPUT} / info->name();
        std::error_code error;
        fs::remove_all(m_folder, error);
        fs::create_directories(m_folder);
    }

    /// The name of `file` in this test's folder.
    [[nodiscard]] std::string name(std::string_view file) const {
        return nameOf(m_folder / pathOf(file));
    }

    /// How many files and folders are in this test's folder.
    [[nodiscard]] std::size_t fileCount() const {
        std::size_t count = 0;
        for ([[maybe_unused]] const auto& entry : fs::directory_iterator{m_folder}) {
            ++count;
        }
        return count;
    }

    [[nodiscard]] const fs::path& folder() const { return m_folder; }

    [[nodiscard]] DiskFiles& files() { return m_files; }

private:
    fs::path m_folder;
    DiskFiles m_files;
};

TEST_F(DiskFilesTest, WritesAndReadsBytes) {
    const std::string bytes = std::string{"line 1\r\nline 2\n"} + '\0' + "\xFF end";
    const std::string file = name("drawing.dxf");
    EXPECT_FALSE(files().exists(file));
    EXPECT_EQ(files().write(file, bytes), std::nullopt);
    EXPECT_TRUE(files().exists(file));
    const Result<std::string> read = files().read(file);
    ASSERT_TRUE(read.ok()) << read.error().message;
    EXPECT_EQ(read.value(), bytes);
    EXPECT_EQ(fileCount(), 1U) << "no temporary file is left";
}

TEST_F(DiskFilesTest, ReplacesAnExistingFile) {
    const std::string file = name("drawing.dxf");
    ASSERT_EQ(files().write(file, "old content, longer than the new one"), std::nullopt);
    ASSERT_EQ(files().write(file, "new"), std::nullopt);
    const Result<std::string> read = files().read(file);
    ASSERT_TRUE(read.ok()) << read.error().message;
    EXPECT_EQ(read.value(), "new");
    EXPECT_EQ(fileCount(), 1U);
}

TEST_F(DiskFilesTest, FailedWriteLeavesTheOldContent) {
    // A folder cannot be replaced by a file: the write fails after the
    // temporary file is complete, at the rename.
    const std::string folder = name("drawing.dxf");
    fs::create_directory(pathOf(folder));
    ASSERT_EQ(files().write(name("drawing.dxf/inside.txt"), "kept"), std::nullopt);

    const std::optional<Error> error = files().write(folder, "new content");
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error.value_or(Error{}).kind, Error::Kind::File);
    EXPECT_NE(error.value_or(Error{}).message.find("drawing.dxf"), std::string::npos);
    EXPECT_TRUE(fs::is_directory(pathOf(folder)));
    const Result<std::string> inside = files().read(name("drawing.dxf/inside.txt"));
    ASSERT_TRUE(inside.ok()) << inside.error().message;
    EXPECT_EQ(inside.value(), "kept");
    EXPECT_EQ(fileCount(), 1U) << "the temporary file is removed";
}

TEST_F(DiskFilesTest, FailsInAMissingFolder) {
    const std::string file = name("missing/drawing.dxf");
    const std::optional<Error> error = files().write(file, "content");
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(error.value_or(Error{}).kind, Error::Kind::File);
    EXPECT_FALSE(files().exists(file));
    EXPECT_EQ(fileCount(), 0U);
}

TEST_F(DiskFilesTest, ReadingAMissingFileIsAnError) {
    const Result<std::string> read = files().read(name("missing.dxf"));
    ASSERT_FALSE(read.ok());
    EXPECT_EQ(read.error().kind, Error::Kind::File);
    EXPECT_EQ(read.error().line, 0U);
    EXPECT_NE(read.error().message.find("missing.dxf"), std::string::npos);
    EXPECT_NE(read.error().message.find("does not exist"), std::string::npos);
}

TEST_F(DiskFilesTest, ReadingAFolderIsAnError) {
    const Result<std::string> read = files().read(nameOf(folder()));
    ASSERT_FALSE(read.ok());
    EXPECT_EQ(read.error().kind, Error::Kind::File);
}

TEST_F(DiskFilesTest, NamesWithAccentedLetters) {
    const std::string file = name("Città già è più ščř ⌀.dxf");
    ASSERT_EQ(files().write(file, "accented"), std::nullopt);
    EXPECT_TRUE(files().exists(file));
    EXPECT_TRUE(fs::exists(folder() / pathOf("Città già è più ščř ⌀.dxf")));
    const Result<std::string> read = files().read(file);
    ASSERT_TRUE(read.ok()) << read.error().message;
    EXPECT_EQ(read.value(), "accented");
    EXPECT_EQ(fileCount(), 1U);
}

TEST_F(DiskFilesTest, WorksThroughTheInterface) {
    FileAccess& access = files();
    const std::string file = name("interface.dxf");
    ASSERT_EQ(access.write(file, "x"), std::nullopt);
    EXPECT_TRUE(access.exists(file));
    EXPECT_TRUE(access.read(file).ok());
}

}  // namespace
}  // namespace tg::io
