// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/document_file.hpp"

#include <gtest/gtest.h>

namespace tg::edit {
namespace {

TEST(DocumentFile, StartsUntitledWithoutAFile) {
    const DocumentFile file;
    EXPECT_EQ(file.name(), "Drawing1.cadx");
    EXPECT_FALSE(file.hasFile());
}

TEST(DocumentFile, RemembersSavedNameUntilReset) {
    DocumentFile file;
    file.setSaved("pianta-caffè.cadx");
    EXPECT_EQ(file.name(), "pianta-caffè.cadx");
    EXPECT_TRUE(file.hasFile());

    file.reset("Drawing2.cadx");
    EXPECT_EQ(file.name(), "Drawing2.cadx");
    EXPECT_FALSE(file.hasFile());
}

}  // namespace
}  // namespace tg::edit
