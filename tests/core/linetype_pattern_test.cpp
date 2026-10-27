// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/linetype_pattern.hpp"

#include <gtest/gtest.h>

#include <algorithm>

namespace tg::core {
namespace {
TEST(LinetypePattern, ContinuousAndLength) {
    EXPECT_TRUE(LinetypePattern{}.continuous());
    EXPECT_DOUBLE_EQ(LinetypePattern{}.length(), 0);
    const LinetypePattern pattern{{12, -3, 0, -3}};
    EXPECT_FALSE(pattern.continuous());
    EXPECT_DOUBLE_EQ(pattern.length(), 18);
    EXPECT_EQ(pattern, (LinetypePattern{{12, -3, 0, -3}}));
    EXPECT_NE(pattern, (LinetypePattern{{12, -3}}));
    EXPECT_FALSE((LinetypePattern{{0}}).continuous());
    EXPECT_DOUBLE_EQ((LinetypePattern{{0}}).length(), 0);
}

TEST(LinetypePattern, StandardDefinitions) {
    const auto dash = standardLinetype("acad_iso02w100");
    ASSERT_TRUE(dash);
    if (!dash) {  // Make the successful assertion visible to static analysis.
        return;
    }
    EXPECT_EQ(dash->name, "ACAD_ISO02W100");
    EXPECT_EQ(dash->description, "Tratto ISO __ __ __ __ __ __ __ __ __ __ __ __ ");
    EXPECT_EQ(dash->pattern, (LinetypePattern{{12, -3}}));
    EXPECT_DOUBLE_EQ(dash->pattern.length(), 15);
    const auto dashDot = standardLinetype("AcAd_IsO10W100");
    ASSERT_TRUE(dashDot);
    if (!dashDot) {  // Make the successful assertion visible to static analysis.
        return;
    }
    EXPECT_EQ(dashDot->name, "ACAD_ISO10W100");
    EXPECT_EQ(dashDot->description, "Tratto punto ISO __ . __ . __ . __ . __ . __ . ");
    EXPECT_EQ(dashDot->pattern, (LinetypePattern{{12, -3, 0, -3}}));
    auto independent = standardLinetype(dash->name);
    ASSERT_TRUE(independent);
    if (!independent) {  // Make the successful assertion visible to static analysis.
        return;
    }
    independent->pattern.elements.clear();
    EXPECT_FALSE(dash->pattern.continuous());
}

TEST(LinetypePattern, NamesAndUnknownTypes) {
    const auto names = standardLinetypeNames();
    ASSERT_EQ(names.size(), 2U);
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));
    for (const auto name : names) {
        EXPECT_TRUE(standardLinetype(name));
    }
    for (const auto* name : {"", "Continuous", "ByLayer", "DASHED", " ACAD_ISO02W100"}) {
        EXPECT_FALSE(standardLinetype(name));
    }
}
}  // namespace
}  // namespace tg::core
