// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_properties.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::io {
namespace {
constexpr double kTolerance = 1e-12;

EntityReadContext readContext(int units = 4) {
    return {"AC1032", "ANSI_1252", {0x20}, {0x51}, 1, units};
}

core::RawGroups entity(const core::RawGroups& body) {
    core::RawGroups groups{{5, "51"}, {330, "20"}, {100, "AcDbEntity"}, {8, "0"}};
    groups.insert(groups.end(), body.begin(), body.end());
    return groups;
}

std::optional<core::EntityRecord> decode(const core::RawGroups& body, int units = 4) {
    auto context = readContext(units);
    return decodeEntity(&kHatchCodec, entity(body), context);
}

core::EntityRecord handled(const core::RawGroups& body, int units = 4) {
    const auto record = decode(body, units);
    EXPECT_TRUE(record.has_value());
    return record.value_or(core::EntityRecord{});
}

core::RawGroups written(const core::EntityRecord& record) {
    EntityWriteContext context{"AC1032", "ANSI_1252", {0x20}};
    core::RawGroups groups;
    kHatchCodec.write(record, context, groups);
    return groups;
}

core::RawGroups prefix(std::string_view pattern, bool solid, int loops) {
    return {{100, "AcDbHatch"},
            {10, "0"},
            {20, "0"},
            {30, "0"},
            {2, std::string{pattern}},
            {70, solid ? "1" : "0"},
            {71, "0"},
            {91, std::to_string(loops)}};
}

void appendSolidTail(core::RawGroups& groups) {
    groups.insert(groups.end(), {{75, "0"}, {76, "1"}, {98, "0"}});
}

core::RawGroups edgeLoopWithGap(std::string_view lastY) {
    auto groups = prefix("SOLID", true, 1);
    groups.insert(groups.end(), {{92, "0"},         {93, "4"},
                                 {72, "1"},         {10, "0"},
                                 {20, "0"},         {11, "10"},
                                 {21, "0"},         {72, "1"},
                                 {10, "10"},        {20, "0"},
                                 {11, "10"},        {21, "8"},
                                 {72, "1"},         {10, "10"},
                                 {20, "8"},         {11, "0"},
                                 {21, "8"},         {72, "1"},
                                 {10, "0"},         {20, "8"},
                                 {11, "0.0000005"}, {21, std::string{lastY}},
                                 {97, "0"}});
    appendSolidTail(groups);
    return groups;
}

TEST(DxfHatch, TheCodecIsRegistered) {
    const EntityCodec* codec = findCodec(std::string_view{"HATCH"});
    ASSERT_NE(codec, nullptr);
    EXPECT_EQ(codec, findCodec(core::Entity{core::Hatch{}}));
}

TEST(DxfHatch, SolidLoopsAndTheirAssociatedHandlesGoRoundInPlace) {
    auto groups = prefix("SOLID", true, 2);
    const core::RawGroups loops{
        {92, "2"},  {72, "0"},   {73, "1"}, {93, "4"}, {10, "0"}, {20, "0"}, {10, "4"},
        {20, "0"},  {10, "4"},   {20, "4"}, {10, "0"}, {20, "4"}, {97, "2"}, {330, "A"},
        {330, "B"}, {92, "0"},   {93, "1"}, {72, "2"}, {10, "2"}, {20, "2"}, {40, "1"},
        {50, "0"},  {51, "360"}, {73, "1"}, {97, "1"}, {330, "C"}};
    groups.insert(groups.end(), loops.begin(), loops.end());
    appendSolidTail(groups);

    const core::EntityRecord record = handled(groups);
    const auto& hatch = std::get<core::Hatch>(record.entity);
    ASSERT_EQ(hatch.loops.size(), 2U);
    EXPECT_EQ(hatch.loops[0].extras, (core::RawGroups{{97, "2"}, {330, "A"}, {330, "B"}}));
    EXPECT_EQ(hatch.loops[1].extras, (core::RawGroups{{97, "1"}, {330, "C"}}));

    const core::RawGroups output = written(record);
    const core::RawGroups first{{97, "2"}, {330, "A"}, {330, "B"}, {92, "0"}};
    const core::RawGroups second{{97, "1"}, {330, "C"}, {75, "0"}};
    EXPECT_NE(std::ranges::search(output, first).begin(), output.end());
    EXPECT_NE(std::ranges::search(output, second).begin(), output.end());
    auto context = readContext();
    const auto again = decodeEntity(&kHatchCodec, output, context);
    ASSERT_TRUE(again.has_value());
    const auto roundTrip = again.value_or(core::EntityRecord{});
    EXPECT_EQ(roundTrip.entity, record.entity);
    EXPECT_EQ(roundTrip.extras, record.extras);
}

TEST(DxfHatch, Ansi31EdgeLoopAndTwoSeedsGoRound) {
    auto groups = prefix("ANSI31", false, 1);
    groups.insert(groups.end(),
                  {{92, "0"},     {93, "4"},  {72, "1"},   {10, "0"},   {20, "0"},   {11, "2"},
                   {21, "0"},     {72, "1"},  {10, "2"},   {20, "0"},   {11, "2"},   {21, "2"},
                   {72, "1"},     {10, "2"},  {20, "2"},   {11, "0"},   {21, "2"},   {72, "1"},
                   {10, "0"},     {20, "2"},  {11, "0"},   {21, "0"},   {97, "0"},   {75, "1"},
                   {76, "1"},     {52, "45"}, {41, "2"},   {77, "0"},   {78, "1"},   {53, "45"},
                   {43, "0"},     {44, "0"},  {45, "1"},   {46, "1"},   {79, "2"},   {49, "0.5"},
                   {49, "-0.25"}, {98, "2"},  {10, "0.5"}, {20, "0.5"}, {10, "1.5"}, {20, "1.5"}});
    const core::EntityRecord record = handled(groups);
    const auto& hatch = std::get<core::Hatch>(record.entity);
    EXPECT_EQ(hatch.seeds, (std::vector<core::Vec2>{{0.5, 0.5}, {1.5, 1.5}}));
    auto context = readContext();
    const auto again = decodeEntity(&kHatchCodec, written(record), context);
    ASSERT_TRUE(again.has_value());
    EXPECT_EQ(again.value_or(core::EntityRecord{}).entity, record.entity);
}

TEST(DxfHatch, EllipseBoundaryAndPaddedGradientStayUnhandled) {
    auto ellipse = prefix("SOLID", true, 1);
    ellipse.insert(ellipse.end(), {{92, "0"}, {93, "1"}, {72, "3"}, {97, "0"}});
    appendSolidTail(ellipse);
    EXPECT_FALSE(decode(ellipse).has_value());

    auto gradient = prefix("SOLID", true, 0);
    appendSolidTail(gradient);
    gradient.push_back({450, "        1"});
    EXPECT_FALSE(decode(gradient).has_value());
}

TEST(DxfHatch, DownwardExtrusionIsMirroredAndRestored) {
    auto groups = prefix("SOLID", true, 1);
    groups.insert(groups.begin() + 4, {{210, "0"}, {220, "0"}, {230, "-1"}});
    groups.insert(groups.end(), {{92, "2"},
                                 {72, "0"},
                                 {73, "1"},
                                 {93, "3"},
                                 {10, "1"},
                                 {20, "2"},
                                 {10, "3"},
                                 {20, "2"},
                                 {10, "2"},
                                 {20, "4"},
                                 {97, "0"}});
    appendSolidTail(groups);
    const core::EntityRecord record = handled(groups);
    EXPECT_TRUE(record.extras.flippedExtrusion);
    const auto polyline =
        std::get<core::Hatch>(record.entity).loops[0].polyline.value_or(core::Polyline{});
    const auto& vertices = polyline.vertices();
    EXPECT_EQ(vertices[0].position, (core::Vec2{-1, 2}));
    auto context = readContext();
    const auto again = decodeEntity(&kHatchCodec, written(record), context);
    ASSERT_TRUE(again.has_value());
    EXPECT_EQ(again.value_or(core::EntityRecord{}).entity, record.entity);
}

TEST(DxfHatch, ClosureToleranceUsesDocumentUnits) {
    const core::RawGroups smallGap = edgeLoopWithGap("0");
    EXPECT_TRUE(decode(smallGap, 4).has_value());
    EXPECT_TRUE(decode(smallGap, 0).has_value());
    EXPECT_FALSE(decode(smallGap, 6).has_value());
    for (const int units : {4, 0, 6}) {
        EXPECT_FALSE(decode(edgeLoopWithGap("1"), units).has_value());
    }
}

TEST(DxfHatch, PatternAnglesAreReducedAndWrittenInDegrees) {
    for (const auto& values :
         {std::array{"3600", "3600", "3690"}, std::array{"-3600", "-3600", "-3510"}}) {
        auto groups = prefix("NET", false, 0);
        groups.insert(groups.end(), {{75, "0"},
                                     {76, "1"},
                                     {52, values[0]},
                                     {41, "1"},
                                     {77, "0"},
                                     {78, "2"},
                                     {53, values[1]},
                                     {43, "0"},
                                     {44, "0"},
                                     {45, "1"},
                                     {46, "0"},
                                     {79, "0"},
                                     {53, values[2]},
                                     {43, "0"},
                                     {44, "0"},
                                     {45, "0"},
                                     {46, "1"},
                                     {79, "0"},
                                     {98, "0"}});
        const core::EntityRecord record = handled(groups);
        const auto& hatch = std::get<core::Hatch>(record.entity);
        EXPECT_EQ(hatch.angle, 0.0);
        EXPECT_EQ(hatch.patternLines[0].angle, 0.0);
        EXPECT_NEAR(hatch.patternLines[1].angle, core::degreesToRadians(90), kTolerance);
        const core::RawGroups output = written(record);
        EXPECT_NE(std::ranges::search(output, core::RawGroups{{52, "0.0"}, {41, "1.0"}}).begin(),
                  output.end());
        EXPECT_NE(std::ranges::search(output, core::RawGroups{{53, "0.0"}, {43, "0.0"}}).begin(),
                  output.end());
        EXPECT_NE(std::ranges::search(output, core::RawGroups{{53, "90.0"}, {43, "0.0"}}).begin(),
                  output.end());
    }
}

TEST(DxfHatch, AChangedPatternOriginIsSavedAndReadBack) {
    // CC of autocad-hatchedit, its origin brought back from 245,5 to 0,0
    // (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md, "Test richiesti").
    auto groups = prefix("ANSI31", false, 1);
    groups.insert(groups.end(), {{92, "7"},
                                 {72, "0"},
                                 {73, "1"},
                                 {93, "4"},
                                 {10, "300.0"},
                                 {20, "40.0"},
                                 {10, "240.0"},
                                 {20, "40.0"},
                                 {10, "240.0"},
                                 {20, "0.0"},
                                 {10, "300.0"},
                                 {20, "0.0"},
                                 {97, "0"},
                                 {75, "0"},
                                 {76, "1"},
                                 {52, "0.0"},
                                 {41, "1.0"},
                                 {77, "0"},
                                 {78, "1"},
                                 {53, "45.0"},
                                 {43, "245.0"},
                                 {44, "5.0"},
                                 {45, "-2.245064030267288"},
                                 {46, "2.245064030267288"},
                                 {79, "0"},
                                 {47, "0.3215508088757433"},
                                 {98, "1"},
                                 {10, "245.0"},
                                 {20, "5.0"},
                                 {1001, "GradientColor1ACI"},
                                 {1070, "     5"},
                                 {1001, "ACAD"},
                                 {1010, "245.0"},
                                 {1020, "5.0"},
                                 {1030, "0.0"}});
    const core::EntityRecord record = handled(groups);
    EXPECT_EQ(core::patternOrigin(record.extras), (core::Vec2{245, 5}));

    const core::PropertyChange change =
        core::withOwnProperty(core::Document::newDrawing(), record, "origin", core::Vec2{0, 0});
    EXPECT_TRUE(change.entity);
    EXPECT_TRUE(change.extras);
    core::EntityRecord moved = record;
    moved.entity = change.entity.value_or(record.entity);
    moved.extras = change.extras.value_or(record.extras);

    const core::RawGroups before = written(record);
    const core::RawGroups after = written(moved);
    ASSERT_EQ(after.size(), before.size());
    for (std::size_t i = 0; i < after.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(after[i].code, before[i].code);
        const bool origin = after[i].code == 43 || after[i].code == 44 || after[i].code == 1010 ||
                            after[i].code == 1020;
        if (origin) {
            EXPECT_EQ(after[i].value, "0.0");
        } else {
            EXPECT_EQ(after[i].value, before[i].value);
        }
    }
    auto context = readContext();
    const auto again = decodeEntity(&kHatchCodec, after, context);
    ASSERT_TRUE(again.has_value());
    const core::EntityRecord reread = again.value_or(core::EntityRecord{});
    EXPECT_EQ(core::patternOrigin(reread.extras), (core::Vec2{0, 0}));
    EXPECT_EQ(std::get<core::Hatch>(reread.entity).patternLines.at(0).origin, (core::Vec2{0, 0}));
    EXPECT_EQ(reread.extras, moved.extras);
}

}  // namespace
}  // namespace tg::io
