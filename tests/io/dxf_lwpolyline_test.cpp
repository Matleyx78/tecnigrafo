// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/io/dxf_groups.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <random>

namespace tg::io {
namespace {
// Integer engine output scaled directly keeps fixtures reproducible on every platform.
class FixtureNumbers {
public:
    explicit FixtureNumbers(std::uint32_t seed) : m_engine(seed) {}

    double next() { return static_cast<double>(m_engine()) / 10000.0; }

private:
    std::mt19937 m_engine;
};

EntityReadContext readContext() {
    return {"AC1032", "ANSI_1252", {0x20}, {0x41}, 1};
}

EntityWriteContext writeContext() {
    return {"AC1032", "ANSI_1252", {0x20}};
}

TEST(DxfLwpolyline, RandomModelsAndExtrasSurviveRoundtrip) {
    FixtureNumbers random(20261023);
    auto read = readContext();
    auto write = writeContext();
    for (int trial = 0; trial < 300; ++trial) {
        std::vector<core::PolylineVertex> vertices;
        for (int i = 0; i < 1 + trial % 8; ++i) {
            const double x = random.next();
            const double y = random.next();
            vertices.push_back({{x, y}, static_cast<double>(i) / 8.0, 0.5, 1.25});
        }
        core::Polyline polyline{vertices, trial % 2 == 0};
        polyline.setConstantWidth(0.25);
        core::EntityExtras extras;
        extras.beforeEntity = {{102, "{ACAD_REACTORS"}, {330, "A"}, {102, "}"}, {330, "20"}};
        extras.common = {{8, "Layer"}, {62, "3"}, {48, "1.000"}};
        extras.placement = {{38, "2.000"}, {39, "3.000"}, {210, "0.0"}, {220, "0.0"}, {230, "1.0"}};
        extras.specific = {{70, trial % 2 == 0 ? "129" : "128"}, {999, "opaque"}};
        extras.extended = {{1001, "APP"}, {1000, "accenté"}, {1040, "1.000"}};
        const core::EntityRecord record{read.id, polyline, extras, 1};
        core::RawGroups groups;
        kLwpolylineCodec.write(record, write, groups);
        const auto decoded = decodeEntity(&kLwpolylineCodec, groups, read);
        ASSERT_TRUE(decoded.has_value());
        const auto actual = decoded.value_or(core::EntityRecord{});
        EXPECT_EQ(actual.entity, record.entity);
        EXPECT_EQ(actual.extras, extras);
    }
}

TEST(DxfLwpolyline, InvalidAndUnsupportedGeometryStaysUnhandled) {
    auto context = readContext();
    const core::RawGroups valid{{5, "41"}, {100, "AcDbEntity"}, {8, "0"}, {100, "AcDbPolyline"},
                                {90, "1"}, {10, "2"},           {20, "3"}};
    ASSERT_TRUE(decodeEntity(&kLwpolylineCodec, valid, context));
    for (const core::RawGroups& extra :
         {core::RawGroups{{91, "1"}}, {{230, "0"}}, {{210, "1"}}, {{67, "1"}}, {{330, "21"}}}) {
        auto groups = valid;
        groups.insert(groups.end(), extra.begin(), extra.end());
        EXPECT_FALSE(decodeEntity(&kLwpolylineCodec, groups, context));
    }
    for (const auto* count : {"0", "2", "-1"}) {
        auto groups = valid;
        groups[4].value = count;
        EXPECT_FALSE(decodeEntity(&kLwpolylineCodec, groups, context));
    }
    auto missingY = valid;
    missingY.pop_back();
    EXPECT_FALSE(decodeEntity(&kLwpolylineCodec, missingY, context));
    EXPECT_FALSE(decodeEntity(&kLwpolylineCodec, {}, context));
}

TEST(DxfLwpolyline, NewEntityHasModelSpaceOwnerAndLayerZero) {
    const core::EntityRecord record{{0x41}, core::Polyline{{{{1, 2}, 0.5}, {{3, 4}, 0}}, true}};
    auto context = writeContext();
    core::RawGroups groups;
    kLwpolylineCodec.write(record, context, groups);
    const core::RawGroups expected{{5, "41"},   {330, "20"},           {100, "AcDbEntity"},
                                   {8, "0"},    {100, "AcDbPolyline"}, {90, "2"},
                                   {70, "1"},   {10, "1.0"},           {20, "2.0"},
                                   {42, "0.5"}, {10, "3.0"},           {20, "4.0"}};
    EXPECT_EQ(groups, expected);
}
}  // namespace
}  // namespace tg::io
