// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace tg::io {
namespace {

constexpr double kTolerance = 1e-12;

void expectNear(const core::Vec2& actual, const core::Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

EntityReadContext readContext(int units = 4) {
    return {"AC1032", "ANSI_1252", {0x20}, {0x51}, 1, units};
}

/// The groups of an ELLIPSE: handle, owner (the model space), AcDbEntity,
/// layer, then `body`.
core::RawGroups entity(const core::RawGroups& body) {
    core::RawGroups groups{{5, "51"}, {330, "20"}, {100, "AcDbEntity"}, {8, "0"}};
    groups.insert(groups.end(), body.begin(), body.end());
    return groups;
}

std::optional<core::EntityRecord> decode(const core::RawGroups& body, int units = 4) {
    auto context = readContext(units);
    return decodeEntity(findCodec(std::string_view{"ELLIPSE"}), entity(body), context);
}

/// The record read from `body`, which must be handled; an empty record
/// (whose entity is not an ellipse) otherwise.
core::EntityRecord handled(const core::RawGroups& body, int units = 4) {
    const auto record = decode(body, units);
    EXPECT_TRUE(record.has_value());
    return record.value_or(core::EntityRecord{});
}

core::Ellipse decodedEllipse(const core::RawGroups& body, int units = 4) {
    const core::EntityRecord record = handled(body, units);
    const auto* ellipse = std::get_if<core::Ellipse>(&record.entity);
    return ellipse != nullptr ? *ellipse : core::Ellipse{};
}

/// The AcDbEllipse groups of an ellipse with the given parameters.
core::RawGroups ellipseBody(std::string_view start, std::string_view end) {
    return {{100, "AcDbEllipse"},
            {10, "10"},
            {20, "20"},
            {30, "0"},
            {11, "4"},
            {21, "0"},
            {31, "0"},
            {40, "0.5"},
            {41, std::string{start}},
            {42, std::string{end}}};
}

core::RawGroups written(const core::EntityRecord& record) {
    EntityWriteContext context{"AC1032", "ANSI_1252", {0x20}};
    core::RawGroups groups;
    findCodec(record.entity)->write(record, context, groups);
    return groups;
}

/// The value of the first group with `code` after the subclass AcDbEllipse.
double numberOf(const core::RawGroups& groups, int code) {
    bool inEllipse = false;
    for (const auto& group : groups) {
        if (group.code == 100) {
            inEllipse = group.value == "AcDbEllipse";
        } else if (inEllipse && group.code == code) {
            return parseNumber(group.value).value_or(-1.0);
        }
    }
    ADD_FAILURE() << "no group " << code;
    return -1.0;
}

/// Writes the record and reads it back: the same record.
void expectRoundtrip(const core::EntityRecord& record) {
    auto context = readContext();
    const auto decoded = decodeEntity(findCodec(record.entity), written(record), context);
    ASSERT_TRUE(decoded.has_value());
    const core::EntityRecord again = decoded.value_or(core::EntityRecord{});
    EXPECT_EQ(again.extras, record.extras);
    const auto& before = std::get<core::Ellipse>(record.entity);
    const auto& after = std::get<core::Ellipse>(again.entity);
    EXPECT_EQ(after.center, before.center);
    EXPECT_EQ(after.majorAxis, before.majorAxis);
    EXPECT_EQ(after.ratio, before.ratio);
    EXPECT_NEAR(after.startParameter, before.startParameter, kTolerance);
    EXPECT_NEAR(after.endParameter, before.endParameter, kTolerance);
}

TEST(DxfEllipse, TheCodecIsRegistered) {
    const EntityCodec* codec = findCodec(std::string_view{"ELLIPSE"});
    ASSERT_NE(codec, nullptr);
    EXPECT_EQ(codec, findCodec(core::Entity{core::Ellipse{{0, 0}, {1, 0}, 1.0, 0.0, 1.0}}));
}

TEST(DxfEllipse, AFullEllipseAndAnArcGoRound) {
    const core::EntityRecord whole = handled(ellipseBody("0", "6.283185307179586"));
    EXPECT_EQ(std::get<core::Ellipse>(whole.entity),
              (core::Ellipse{{10, 20}, {4, 0}, 0.5, 0.0, core::kTwoPi}));
    EXPECT_FALSE(whole.extras.flippedExtrusion);
    expectRoundtrip(whole);
    EXPECT_EQ(numberOf(written(whole), 41), 0.0);
    EXPECT_EQ(numberOf(written(whole), 42), core::kTwoPi);

    const core::EntityRecord arc = handled(ellipseBody("0.5235987755982988", "2.0943951023931953"));
    const auto& ellipse = std::get<core::Ellipse>(arc.entity);
    EXPECT_NEAR(ellipse.startParameter, core::degreesToRadians(30), kTolerance);
    EXPECT_NEAR(ellipse.endParameter, core::degreesToRadians(120), kTolerance);
    expectRoundtrip(arc);

    // An arc across 0: the model keeps both parameters in [0, 2π), the file
    // gets 42 = 41 + the span back, as AutoCAD writes it.
    const core::EntityRecord across =
        handled(ellipseBody("6.060323388325995", "7.068583470577033"));
    const auto& wrapped = std::get<core::Ellipse>(across.entity);
    EXPECT_NEAR(wrapped.startParameter, 6.060323388325995, kTolerance);
    EXPECT_NEAR(wrapped.endParameter, 7.068583470577033 - core::kTwoPi, kTolerance);
    EXPECT_NEAR(numberOf(written(across), 41), 6.060323388325995, kTolerance);
    EXPECT_NEAR(numberOf(written(across), 42), 7.068583470577033, kTolerance);
    expectRoundtrip(across);
}

TEST(DxfEllipse, ExtrusionDownMirrorsCentreAxisAndParameters) {
    core::RawGroups wholeBody = ellipseBody("0", "6.283185307179586");
    wholeBody.insert(wholeBody.end(), {{210, "0"}, {220, "0"}, {230, "-1"}});
    const core::EntityRecord whole = handled(wholeBody);
    EXPECT_TRUE(whole.extras.flippedExtrusion);
    EXPECT_EQ(std::get<core::Ellipse>(whole.entity),
              (core::Ellipse{{-10, 20}, {-4, 0}, 0.5, 0.0, core::kTwoPi}));
    expectRoundtrip(whole);

    // 0 to 90° seen from below is 270° to 0 in the world.
    core::RawGroups arcBody = ellipseBody("0", "1.5707963267948966");
    arcBody.insert(arcBody.end(), {{210, "0"}, {220, "0"}, {230, "-1"}});
    const core::EntityRecord arc = handled(arcBody);
    const auto& ellipse = std::get<core::Ellipse>(arc.entity);
    EXPECT_EQ(ellipse.center, (core::Vec2{-10, 20}));
    EXPECT_EQ(ellipse.majorAxis, (core::Vec2{-4, 0}));
    EXPECT_NEAR(ellipse.startParameter, core::degreesToRadians(270), kTolerance);
    EXPECT_EQ(ellipse.endParameter, 0.0);
    expectNear(core::pointAt(ellipse, ellipse.startParameter), {-10, 22});
    const core::RawGroups groups = written(arc);
    EXPECT_EQ(numberOf(groups, 10), 10.0);
    EXPECT_EQ(numberOf(groups, 11), 4.0);
    EXPECT_EQ(numberOf(groups, 41), 0.0);
    EXPECT_NEAR(numberOf(groups, 42), core::kPi / 2.0, kTolerance);
    expectRoundtrip(arc);
}

TEST(DxfEllipse, AnAlmostFullEllipseIsReadAsFull) {
    // An ellipse exploded from a block scaled (2, 1), as AutoCAD writes it:
    // 42 - 41 = 2π - 8.9e-16 (decision of 2026-11-15 in #679).
    const core::Ellipse exploded =
        decodedEllipse(ellipseBody("0.1370039199315505", "6.420189227111136"));
    EXPECT_EQ(exploded.startParameter, 0.0);
    EXPECT_EQ(exploded.endParameter, core::kTwoPi);

    // A true arc with a large span and distinct ends stays an arc.
    const core::Ellipse large = decodedEllipse(ellipseBody("0.1", "6.2"));
    EXPECT_NEAR(large.startParameter, 0.1, kTolerance);
    EXPECT_NEAR(large.endParameter, 6.2, kTolerance);

    // A span not above π stays as it is, even with coincident ends.
    const core::Ellipse tiny = decodedEllipse(ellipseBody("1", "1.000000000001"));
    EXPECT_EQ(tiny.startParameter, 1.0);
    EXPECT_EQ(tiny.endParameter, 1.000000000001);

    // Equal parameters are the full ellipse.
    const core::Ellipse equal = decodedEllipse(ellipseBody("2", "2"));
    EXPECT_EQ(equal.endParameter, core::kTwoPi);
}

TEST(DxfEllipse, TheFloorOfTheToleranceComesFromInsunits) {
    // A gap of 1e-7 rad on an axis of length 4: ends about 2e-7 apart, within
    // the floor of millimetres (1e-6) but not of metres (1e-9).
    const std::string end = formatNumber(core::kTwoPi + 0.5 - 1e-7);
    const core::Ellipse millimetres = decodedEllipse(ellipseBody("0.5", end), 4);
    EXPECT_EQ(millimetres.endParameter, core::kTwoPi);
    const core::Ellipse metres = decodedEllipse(ellipseBody("0.5", end), 6);
    EXPECT_NEAR(metres.startParameter, 0.5, kTolerance);
    EXPECT_NEAR(metres.endParameter, 0.5 - 1e-7, kTolerance);
}

TEST(DxfEllipse, ExtrasAndPlacementAreKept) {
    core::RawGroups body = ellipseBody("0", "6.283185307179586");
    body.at(3).value = "7";  // 30
    body.at(6).value = "8";  // 31
    body.insert(body.end(), {{999, "opaque"}, {1001, "APP"}, {1000, "data"}});
    const core::EntityRecord record = handled(body);
    EXPECT_EQ(record.extras.specific, (core::RawGroups{{999, "opaque"}}));
    EXPECT_EQ(record.extras.extended, (core::RawGroups{{1001, "APP"}, {1000, "data"}}));
    EXPECT_EQ(record.extras.placement, (core::RawGroups{{30, "7"}, {31, "8"}}));
    expectRoundtrip(record);
}

TEST(DxfEllipse, UnsupportedEllipsesStayRaw) {
    const auto withRatio = [](std::string_view ratio) {
        core::RawGroups body = ellipseBody("0", "1");
        body.at(7).value = std::string{ratio};
        return body;
    };
    EXPECT_FALSE(decode(withRatio("0")).has_value());
    EXPECT_FALSE(decode(withRatio("-0.5")).has_value());
    EXPECT_FALSE(decode(withRatio("1.5")).has_value());
    EXPECT_TRUE(decode(withRatio("1")).has_value());

    core::RawGroups noAxis = ellipseBody("0", "1");
    noAxis.at(4).value = "0";  // 11
    EXPECT_FALSE(decode(noAxis).has_value());

    core::RawGroups missing = ellipseBody("0", "1");
    missing.pop_back();  // 42
    EXPECT_FALSE(decode(missing).has_value());

    core::RawGroups twice = ellipseBody("0", "1");
    twice.push_back({40, "0.5"});
    EXPECT_FALSE(decode(twice).has_value());

    core::RawGroups notANumber = ellipseBody("0", "1");
    notANumber.at(1).value = "x";
    EXPECT_FALSE(decode(notANumber).has_value());

    core::RawGroups sideways = ellipseBody("0", "1");
    sideways.insert(sideways.end(), {{210, "0"}, {220, "1"}, {230, "0"}});
    EXPECT_FALSE(decode(sideways).has_value());
}

TEST(DxfEllipse, AFileOfHandledEllipsesIsWrittenAgainWithoutAHandseed) {
    // No $HANDSEED and no TABLES: the last handle is that of a handled
    // entity, which a reader counts, so the writer adds no $HANDSEED.
    const std::string bytes =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n9\n$INSUNITS\n70\n4\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nELLIPSE\n5\n101\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbEllipse\n10\n10\n20\n20\n30\n0\n11\n4\n21\n0\n31\n0\n40\n0.5\n"
        "41\n0\n42\n6.283185307179586\n0\nENDSEC\n0\nEOF\n";
    const auto read = readDxf(bytes);
    ASSERT_TRUE(read.ok());
    ASSERT_EQ(read.value().document.entities().size(), 1U);
    const auto written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    EXPECT_EQ(written.value().find("$HANDSEED"), std::string::npos);
    const auto reread = readDxf(written.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_EQ(reread.value().document.nextId(), read.value().document.nextId());
}

}  // namespace
}  // namespace tg::io
