// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "../../cmd/src/name_list.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tg::cmd::detail {
namespace {

TEST(NameList, TrimsEachElementAndPreservesEmptyNames) {
    EXPECT_EQ(splitNameList(" Muri , Porte interne,\tFinestre "),
              (std::vector<std::string>{"Muri", "Porte interne", "Finestre"}));
    EXPECT_EQ(splitNameList("A,,B,"), (std::vector<std::string>{"A", "", "B", ""}));
    EXPECT_EQ(splitNameList(""), (std::vector<std::string>{""}));
}

TEST(NameList, MatchesOnlyCompleteNamesWithoutCase) {
    EXPECT_TRUE(matchesName("M?R*", "muri"));
    EXPECT_TRUE(matchesName("*a*b?", "AAAbc"));
    EXPECT_TRUE(matchesName("a**b", "ab"));
    EXPECT_TRUE(matchesName("*", ""));
    EXPECT_TRUE(matchesName("", ""));
    EXPECT_FALSE(matchesName("?", ""));
    EXPECT_FALSE(matchesName("a*b", "abcd"));
    EXPECT_FALSE(matchesName("Mur", "Muri"));
    EXPECT_FALSE(matchesName("*a*b", "abx"));
    EXPECT_FALSE(matchesName("[]", "A"));
}

TEST(NameList, ReturnsEachMatchingLayerOnceInTableOrder) {
    auto document = core::Document::newDrawing();
    core::Layer layer;
    layer.name = "Muri";
    layer.linetype = document.continuousLinetype();
    const auto walls = document.addLayer(layer);
    layer.id = {};
    layer.name = "Porte";
    const auto doors = document.addLayer(layer);
    EXPECT_EQ(matchingLayers(document, "p*, M?ri, m*"),
              (std::vector<core::LayerId>{walls, doors}));
    EXPECT_TRUE(matchingLayers(document, "Z*").empty());
    EXPECT_EQ(matchingLayers(document, "*,0"),
              (std::vector<core::LayerId>{document.layerZero(), walls, doors}));
}

}  // namespace
}  // namespace tg::cmd::detail
