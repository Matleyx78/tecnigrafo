// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <utility>

namespace tg::io {
namespace {

constexpr double kTolerance = 1e-9;

// Integer engine output scaled directly keeps fixtures reproducible on every platform.
class FixtureNumbers {
public:
    explicit FixtureNumbers(std::uint32_t seed) : m_engine(seed) {}

    /// A number in [-214748.3648, 214748.3647], with four decimals.
    double next() {
        return static_cast<double>(static_cast<std::int64_t>(m_engine()) - 2147483648LL) / 10000.0;
    }

    /// A number in (0, 429496.7296].
    double positive() { return static_cast<double>(m_engine() + 1ULL) / 10000.0; }

    /// Whole degrees in [0, 360).
    double degrees() { return static_cast<double>(m_engine() % 360U); }

private:
    std::mt19937 m_engine;
};

EntityReadContext readContext() {
    return {"AC1032", "ANSI_1252", {0x20}, {0x51}, 1};
}

EntityWriteContext writeContext() {
    return {"AC1032", "ANSI_1252", {0x20}};
}

const EntityCodec& codec(std::string_view type) {
    const EntityCodec* found = findCodec(type);
    EXPECT_NE(found, nullptr) << type;
    return found != nullptr ? *found : kLwpolylineCodec;
}

core::EntityExtras sampleExtras(core::RawGroups placement) {
    core::EntityExtras extras;
    extras.beforeEntity = {{102, "{ACAD_REACTORS"}, {330, "A"}, {102, "}"}, {330, "20"}};
    extras.common = {{8, "Layer"}, {62, "3"}, {48, "1.000"}};
    extras.placement = std::move(placement);
    extras.specific = {{999, "opaque"}};
    extras.extended = {{1001, "APP"}, {1000, "accenté"}, {1040, "1.000"}};
    return extras;
}

std::optional<core::EntityRecord> roundtrip(const core::EntityRecord& record) {
    auto read = readContext();
    auto write = writeContext();
    const EntityCodec* found = findCodec(record.entity);
    EXPECT_NE(found, nullptr);
    if (found == nullptr) {
        return std::nullopt;
    }
    core::RawGroups groups;
    found->write(record, write, groups);
    return decodeEntity(found, groups, read);
}

/// The groups of an entity: handle, AcDbEntity, then `body`.
core::RawGroups entity(const core::RawGroups& body) {
    core::RawGroups groups{{5, "51"}, {100, "AcDbEntity"}, {8, "0"}};
    groups.insert(groups.end(), body.begin(), body.end());
    return groups;
}

std::optional<core::EntityRecord> decode(std::string_view type, const core::RawGroups& body) {
    auto context = readContext();
    return decodeEntity(&codec(type), entity(body), context);
}

/// The groups written for `record`, compared with `expected`: same codes in
/// the same order, values equal as text or as numbers within 1e-9.
void expectWrittenAs(const core::EntityRecord& record, std::string_view type,
                     const core::RawGroups& expected) {
    auto context = writeContext();
    core::RawGroups groups;
    codec(type).write(record, context, groups);
    // writeEntityCommon adds the owner of a record without one.
    core::RawGroups file = entity(expected);
    file.insert(file.begin() + 1, {330, "20"});
    ASSERT_EQ(groups.size(), file.size());
    for (std::size_t i = 0; i < groups.size(); ++i) {
        EXPECT_EQ(groups[i].code, file[i].code) << "group " << i;
        const auto actual = parseNumber(groups[i].value);
        const auto wanted = parseNumber(file[i].value);
        if (actual && wanted) {
            EXPECT_NEAR(*actual, *wanted, kTolerance) << "group " << i;
        } else {
            EXPECT_EQ(groups[i].value, file[i].value) << "group " << i;
        }
    }
}

TEST(DxfLineCircleArc, RandomEntitiesAndExtrasSurviveRoundtrip) {
    FixtureNumbers random(20261024);
    for (int trial = 0; trial < 300; ++trial) {
        const core::Line line{{random.next(), random.next()}, {random.next(), random.next()}};
        // In the positions of the DXF reference: no anchors are needed.
        const auto lineExtras =
            sampleExtras({{39, "1.5"}, {30, "2.00"}, {31, "-3"}, {210, "0"}, {230, "1"}});
        const core::EntityRecord lineRecord{readContext().id, line, lineExtras, 1};
        const auto readLine = roundtrip(lineRecord);
        ASSERT_TRUE(readLine.has_value());
        EXPECT_EQ(readLine->entity, lineRecord.entity);
        EXPECT_EQ(readLine->extras, lineExtras);

        const core::Circle circle{{random.next(), random.next()}, random.positive()};
        const auto circleExtras = sampleExtras({{39, "2"}, {30, "4.5"}});
        const core::EntityRecord circleRecord{readContext().id, circle, circleExtras, 1};
        const auto readCircle = roundtrip(circleRecord);
        ASSERT_TRUE(readCircle.has_value());
        EXPECT_EQ(readCircle->entity, circleRecord.entity);
        EXPECT_EQ(readCircle->extras, circleExtras);

        double start = random.degrees();
        double end = random.degrees();
        if (start == end) {
            end = std::fmod(end + 1.0, 360.0);
        }
        const core::Arc arc{{random.next(), random.next()},
                            random.positive(),
                            core::degreesToRadians(start),
                            core::degreesToRadians(end)};
        const auto arcExtras = sampleExtras({{30, "-6.25"}, {210, "0.0"}, {220, "0"}, {230, "1"}});
        const core::EntityRecord arcRecord{readContext().id, arc, arcExtras, 1};
        const auto readArc = roundtrip(arcRecord);
        ASSERT_TRUE(readArc.has_value());
        const auto& actual = std::get<core::Arc>(readArc->entity);
        EXPECT_EQ(actual.center, arc.center);
        EXPECT_EQ(actual.radius, arc.radius);
        EXPECT_NEAR(actual.startAngle, arc.startAngle, kTolerance);
        EXPECT_NEAR(actual.endAngle, arc.endAngle, kTolerance);
        EXPECT_EQ(readArc->extras, arcExtras);
    }
}

TEST(DxfLineCircleArc, ValuesAreReadAndAnglesNormalized) {
    const auto line =
        decode("LINE", {{100, "AcDbLine"}, {10, "-3"}, {20, "4"}, {11, "5"}, {21, "-8"}});
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->entity, (core::Entity{core::Line{{-3, 4}, {5, -8}}}));
    EXPECT_TRUE(line->extras.common == core::RawGroups({{8, "0"}}));
    // A line of length zero stays handled (docs/adr/0018, point 3).
    const auto point =
        decode("LINE", {{100, "AcDbLine"}, {10, "2"}, {20, "2"}, {11, "2"}, {21, "2"}});
    ASSERT_TRUE(point.has_value());
    EXPECT_EQ(point->entity, (core::Entity{core::Line{{2, 2}, {2, 2}}}));

    const auto circle = decode("CIRCLE", {{100, "AcDbCircle"}, {10, "5"}, {20, "-7"}, {40, "2.5"}});
    ASSERT_TRUE(circle.has_value());
    EXPECT_EQ(circle->entity, (core::Entity{core::Circle{{5, -7}, 2.5}}));

    const auto arc = decode("ARC", {{100, "AcDbCircle"},
                                    {10, "-2"},
                                    {20, "3"},
                                    {40, "4"},
                                    {100, "AcDbArc"},
                                    {50, "-90"},
                                    {51, "450"}});
    ASSERT_TRUE(arc.has_value());
    const auto& value = std::get<core::Arc>(arc->entity);
    EXPECT_NEAR(value.startAngle, core::degreesToRadians(270), kTolerance);
    EXPECT_NEAR(value.endAngle, core::degreesToRadians(90), kTolerance);
    EXPECT_TRUE(arc->extras.specific.empty());
}

TEST(DxfLineCircleArc, DegenerateAndInvalidEntitiesStayUnhandled) {
    for (const char* radius : {"0", "-1", "x"}) {
        EXPECT_FALSE(decode("CIRCLE", {{100, "AcDbCircle"}, {10, "2"}, {20, "3"}, {40, radius}}))
            << radius;
        EXPECT_FALSE(decode("ARC", {{100, "AcDbCircle"},
                                    {10, "1"},
                                    {20, "2"},
                                    {40, radius},
                                    {100, "AcDbArc"},
                                    {50, "20"},
                                    {51, "40"}}))
            << radius;
    }
    // Equal angles, also only after the normalization.
    for (const auto& [start, end] : {std::pair{"45", "45"}, {"0", "360"}, {"30", "390"}}) {
        EXPECT_FALSE(decode("ARC", {{100, "AcDbCircle"},
                                    {10, "1"},
                                    {20, "2"},
                                    {40, "3"},
                                    {100, "AcDbArc"},
                                    {50, start},
                                    {51, end}}))
            << start << " " << end;
    }
    // Missing or repeated values.
    EXPECT_FALSE(decode("LINE", {{100, "AcDbLine"}, {10, "0"}, {20, "0"}, {11, "1"}}));
    EXPECT_FALSE(
        decode("LINE", {{100, "AcDbLine"}, {10, "0"}, {10, "1"}, {20, "0"}, {11, "1"}, {21, "1"}}));
    EXPECT_FALSE(decode("CIRCLE", {{100, "AcDbCircle"}, {10, "0"}, {20, "0"}}));
    EXPECT_FALSE(decode("ARC", {{100, "AcDbCircle"}, {10, "0"}, {20, "0"}, {40, "1"}, {50, "0"}}));
    // An oblique extrusion (docs/adr/0018, point 3).
    EXPECT_FALSE(decode("CIRCLE", {{100, "AcDbCircle"},
                                   {10, "0"},
                                   {20, "0"},
                                   {40, "1"},
                                   {210, "0"},
                                   {220, "0.5"},
                                   {230, "0.866"}}));
}

// Extrusion (0,0,-1): the model gets world coordinates, computed here by
// hand with x -> -x; the writer gives back the values of the file.
TEST(DxfLineCircleArc, NegativeExtrusionIsConvertedAndWrittenBack) {
    const core::RawGroups down{{210, "0"}, {220, "0"}, {230, "-1"}};
    const auto with = [&](core::RawGroups body) {
        body.insert(body.end(), down.begin(), down.end());
        return body;
    };

    const auto lineFile = with({{100, "AcDbLine"}, {10, "3"}, {20, "4"}, {11, "8"}, {21, "-2"}});
    const auto line = decode("LINE", lineFile);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->entity, (core::Entity{core::Line{{3, 4}, {8, -2}}}));  // already in WCS
    EXPECT_TRUE(line->extras.flippedExtrusion);
    expectWrittenAs(*line, "LINE", lineFile);

    const auto circleFile = with({{100, "AcDbCircle"}, {10, "5"}, {20, "6"}, {40, "3"}});
    const auto circle = decode("CIRCLE", circleFile);
    ASSERT_TRUE(circle.has_value());
    EXPECT_EQ(circle->entity, (core::Entity{core::Circle{{-5, 6}, 3}}));
    EXPECT_TRUE(circle->extras.flippedExtrusion);
    expectWrittenAs(*circle, "CIRCLE", circleFile);

    // 150..210 in the object system: 180-210 = -30 -> 330, 180-150 = 30.
    // The mirrored arc crosses 0 degrees.
    const auto arcFile = with({{100, "AcDbCircle"},
                               {10, "2"},
                               {20, "3"},
                               {40, "4"},
                               {100, "AcDbArc"},
                               {50, "150"},
                               {51, "210"}});
    const auto arc = decode("ARC", arcFile);
    ASSERT_TRUE(arc.has_value());
    const auto& mirrored = std::get<core::Arc>(arc->entity);
    EXPECT_EQ(mirrored.center, (core::Vec2{-2, 3}));
    EXPECT_NEAR(mirrored.startAngle, core::degreesToRadians(330), kTolerance);
    EXPECT_NEAR(mirrored.endAngle, core::degreesToRadians(30), kTolerance);
    expectWrittenAs(*arc, "ARC", arcFile);

    // 300..30 crosses 0 in the file: 180-30 = 150, 180-300 = -120 -> 240.
    const auto crossingFile = with({{100, "AcDbCircle"},
                                    {10, "-4"},
                                    {20, "2"},
                                    {40, "5"},
                                    {100, "AcDbArc"},
                                    {50, "300"},
                                    {51, "30"}});
    const auto crossing = decode("ARC", crossingFile);
    ASSERT_TRUE(crossing.has_value());
    const auto& value = std::get<core::Arc>(crossing->entity);
    EXPECT_EQ(value.center, (core::Vec2{4, 2}));
    EXPECT_NEAR(value.startAngle, core::degreesToRadians(150), kTolerance);
    EXPECT_NEAR(value.endAngle, core::degreesToRadians(240), kTolerance);
    expectWrittenAs(*crossing, "ARC", crossingFile);
}

TEST(DxfLineCircleArc, PlacementGroupsAreWrittenWhereTheyWereRead) {
    // Extrusion before thickness, after the end point: not the order of the
    // DXF reference, written again as read.
    const core::RawGroups unusual{{100, "AcDbLine"}, {10, "3"},   {20, "4"},    {30, "1.2500"},
                                  {11, "8"},         {21, "-2"},  {31, "-2.5"}, {210, "0"},
                                  {220, "0"},        {230, "-1"}, {39, "-0.75"}};
    const auto line = decode("LINE", unusual);
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(line->extras.placementAnchors.size(), line->extras.placement.size());
    expectWrittenAs(*line, "LINE", unusual);

    // The order of the DXF reference needs no anchors.
    const core::RawGroups usual{{100, "AcDbCircle"}, {39, "2"},  {10, "1"},  {20, "2"}, {30, "3"},
                                {40, "4"},           {210, "0"}, {220, "0"}, {230, "1"}};
    const auto circle = decode("CIRCLE", usual);
    ASSERT_TRUE(circle.has_value());
    EXPECT_TRUE(circle->extras.placementAnchors.empty());
    expectWrittenAs(*circle, "CIRCLE", usual);

    // Placement groups an entity does not use are kept as well.
    const core::RawGroups stray{{100, "AcDbCircle"}, {10, "1"}, {20, "2"}, {40, "4"}, {38, "5"}};
    const auto kept = decode("CIRCLE", stray);
    ASSERT_TRUE(kept.has_value());
    expectWrittenAs(*kept, "CIRCLE", stray);
}

}  // namespace
}  // namespace tg::io
