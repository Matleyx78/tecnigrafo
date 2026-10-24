// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "json.hpp"
#include "tg/io/io.hpp"
#include "zip.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

namespace tg::io {
namespace {

class FixtureNumbers {
public:
    explicit FixtureNumbers(std::uint32_t seed) : m_engine(seed) {}

    double next() { return static_cast<double>(m_engine() % 100000) / 100.0; }

private:
    std::mt19937 m_engine;
};

std::vector<detail::ZipPart> fixture() {
    const auto drawing = writeDxf(core::Document{}, DxfVersion::R2018);
    EXPECT_TRUE(drawing.ok());
    return {{"mimetype", "application/vnd.tecnigrafo.cadx", true},
            {"manifest.json",
             R"({"format":1,"generator":"test","parts":["mimetype","drawing.dxf"]})", false},
            {"drawing.dxf", drawing.ok() ? drawing.value() : std::string{}, false}};
}

TEST(Cadx, WritesDeterministicOrderedArchiveAndPreservesOpaqueData) {
    core::Document document;
    core::RawDrawing raw;
    raw.nativeParts = {
        {"z-data", std::string("\0\xff\n", 3)},
        {"extensions.json", " {\"opaque\": 2} \n"},
        {"thumbnail.png", "opaque PNG bytes"},
        {"manifest.json",
         R"({"unknown":{"z":1e999,"a":[true,null,"中"]},"format":1,"generator":"old","parts":[]})"}};
    document.setRaw(raw);
    const auto first = writeCadx(document);
    ASSERT_TRUE(first.ok()) << first.error().message;
    const auto second = writeCadx(document);
    ASSERT_TRUE(second.ok());
    EXPECT_EQ(first.value(), second.value());
    const auto parts = detail::readZip(first.value());
    ASSERT_TRUE(parts.ok());
    ASSERT_EQ(parts.value().size(), 6);
    EXPECT_EQ(parts.value()[0].name, "mimetype");
    EXPECT_TRUE(parts.value()[0].stored);
    EXPECT_EQ(parts.value()[1].name, "manifest.json");
    EXPECT_EQ(parts.value()[2].name, "drawing.dxf");
    EXPECT_EQ(parts.value()[3].name, "extensions.json");
    EXPECT_EQ(parts.value()[4].name, "thumbnail.png");
    EXPECT_EQ(parts.value()[5].name, "z-data");
    const auto reread = readCadx(first.value());
    ASSERT_TRUE(reread.ok());
    for (const auto& part : raw.nativeParts) {
        if (part.name != "manifest.json") {
            const auto& actual = reread.value().document.raw().nativeParts;
            const auto found = std::ranges::find(actual, part.name, &core::NativePart::name);
            ASSERT_NE(found, actual.end());
            EXPECT_EQ(found->bytes, part.bytes);
        }
    }
    const auto manifest = detail::readJson(parts.value()[1].bytes);
    ASSERT_TRUE(manifest.ok());
    EXPECT_EQ(manifest.value().members.front().first, "unknown");
    const auto rewritten = writeCadx(reread.value().document);
    ASSERT_TRUE(rewritten.ok());
    EXPECT_EQ(rewritten.value(), first.value());
}

TEST(Cadx, RejectsInvalidMimetypeAndMissingParts) {
    for (int variation = 0; variation < 8; ++variation) {
        auto parts = fixture();
        switch (variation) {
            case 0:
                parts.erase(parts.begin());
                break;
            case 1:
                parts[0].stored = false;
                break;
            case 2:
                std::swap(parts[0], parts[1]);
                break;
            case 3:
                parts[0].bytes += '\n';
                break;
            case 4:
                parts.erase(parts.begin() + 1);
                break;
            case 5:
                parts.pop_back();
                break;
            case 6:
                parts[2].bytes = "damaged DXF";
                break;
            case 7:
                parts[1].bytes = "{";
                break;
            default:
                break;
        }
        const auto result = readCadx(detail::writeZip(parts));
        ASSERT_FALSE(result.ok()) << variation;
        EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
        if (variation < 4) {
            EXPECT_EQ(result.error().message, "not a Tecnigrafo drawing (no valid mimetype)");
        }
        if (variation == 6) {
            EXPECT_TRUE(result.error().message.starts_with("drawing.dxf: line "));
        }
    }
}

TEST(Cadx, ValidatesManifestAndReportsFutureVersions) {
    for (const auto* json :
         {"null", "[]", "{}", R"({"format":0})", R"({"format":-1})", R"({"format":1.5})",
          R"({"format":"1"})", R"({"format":true})", R"({"format":1,"generator":1,"parts":[]})",
          R"({"format":1,"generator":"g","parts":["mimetype"]})",
          R"({"format":1,"generator":"g","parts":["mimetype","mimetype"]})"}) {
        auto parts = fixture();
        parts[1].bytes = json;
        EXPECT_FALSE(readCadx(detail::writeZip(parts)).ok()) << json;
    }
    for (const auto* version : {"2", "3", "999999999999999999999999999999999"}) {
        auto parts = fixture();
        parts[1].bytes = std::string{R"({"format":)"} + version + '}';
        const auto result = readCadx(detail::writeZip(parts));
        ASSERT_FALSE(result.ok());
        EXPECT_EQ(result.error().kind, Error::Kind::Unsupported);
        EXPECT_EQ(result.error().message,
                  std::string{"written by a newer version of Tecnigrafo (format "} + version +
                      "; this version reads up to 1)");
    }
}

TEST(Cadx, RandomModelsSurviveWithHandlesAndTolerance) {
    FixtureNumbers random{20261024};
    const auto number = [&random] {
        return random.next();
    };
    constexpr double kTolerance = 1e-9;
    for (int trial = 0; trial < 100; ++trial) {
        const double x = number();
        const double y = number();
        const double radius = number() + 1;
        // P11's codecs may not yet be merged. Import all three base entities:
        // whether managed or opaque, CADX must preserve the same model.
        const std::string source =
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nLINE\n5\n100\n10\n" +
            std::to_string(x) + "\n20\n" + std::to_string(y) + "\n11\n" +
            std::to_string(x + radius) + "\n21\n" + std::to_string(y - radius) +
            "\n0\nCIRCLE\n5\n101\n10\n" + std::to_string(x) + "\n20\n" + std::to_string(y) +
            "\n40\n" + std::to_string(radius) + "\n0\nARC\n5\n102\n10\n" + std::to_string(x) +
            "\n20\n" + std::to_string(y) + "\n40\n" + std::to_string(radius) +
            "\n50\n15\n51\n145\n0\nENDSEC\n0\nEOF\n";
        auto imported = readDxf(source);
        ASSERT_TRUE(imported.ok());
        auto document = std::move(imported).value().document;
        core::Polyline polyline{{{{x, y}, 0.5, 0.25, 0.75}, {{x + 10, y - 5}, -0.5, 1, 2}},
                                trial % 2 == 0};
        polyline.setConstantWidth(0.125);
        document.add(polyline);
        core::Text text;
        text.position = {x, y};
        text.alignmentPoint = {x + 20, y + 10};
        text.rotation = 0.25;
        text.obliqueAngle = -0.125;
        text.horizontal = core::TextHorizontal::Center;
        text.vertical = core::TextVertical::Top;
        text.mirroredX = trial % 2 == 0;
        text.value = "è 中 %%d";
        document.add(text);
        const auto archive = writeCadx(document);
        ASSERT_TRUE(archive.ok()) << archive.error().message;
        const auto read = readCadx(archive.value());
        ASSERT_TRUE(read.ok());
        ASSERT_EQ(read.value().document.entities().size(), document.entities().size());
        for (const auto& expected : document.entities()) {
            const auto* actual = read.value().document.find(expected.id);
            ASSERT_NE(actual, nullptr);
            EXPECT_EQ(actual->entity.index(), expected.entity.index());
            const auto before = core::boundingBox(expected.entity);
            const auto after = core::boundingBox(actual->entity);
            EXPECT_NEAR(before.min().x, after.min().x, kTolerance);
            EXPECT_NEAR(before.min().y, after.min().y, kTolerance);
            EXPECT_NEAR(before.max().x, after.max().x, kTolerance);
            EXPECT_NEAR(before.max().y, after.max().y, kTolerance);
        }
        EXPECT_EQ(std::vector<core::RawEntity>(document.rawEntities().begin(),
                                               document.rawEntities().end()),
                  std::vector<core::RawEntity>(read.value().document.rawEntities().begin(),
                                               read.value().document.rawEntities().end()));
        const auto& actualPolyline = std::get<core::Polyline>(
            read.value().document.entities()[read.value().document.entities().size() - 2].entity);
        EXPECT_EQ(polyline, actualPolyline);
        const auto& actualText =
            std::get<core::Text>(read.value().document.entities().back().entity);
        EXPECT_EQ(actualText.value, text.value);
        EXPECT_EQ(actualText.mirroredX, text.mirroredX);
        EXPECT_NEAR(actualText.rotation, text.rotation, kTolerance);
        EXPECT_NEAR(actualText.obliqueAngle, text.obliqueAngle, kTolerance);
    }
}

TEST(Cadx, ConvertsR2000ToR2018) {
    core::Document document;
    core::Text text;
    text.value = "café 中";
    document.add(text);
    const auto current = writeDxf(document, DxfVersion::R2018);
    ASSERT_TRUE(current.ok());
    auto legacy = current.value();
    const auto version = legacy.find("AC1032");
    ASSERT_NE(version, std::string::npos);
    legacy.replace(version, 6, "AC1015");
    // This fixture has no non-ASCII bytes: DXF Unicode escapes are legal in R2000.
    const auto textAt = legacy.find("café 中");
    ASSERT_NE(textAt, std::string::npos);
    legacy.replace(textAt, std::string{"café 中"}.size(), R"(caf\U+00E9 \U+4E2D)");
    const auto old = readDxf(legacy);
    ASSERT_TRUE(old.ok());
    ASSERT_EQ(old.value().document.raw().version, "AC1015");
    const auto archive = writeCadx(old.value().document);
    ASSERT_TRUE(archive.ok()) << archive.error().message;
    const auto native = readCadx(archive.value());
    ASSERT_TRUE(native.ok());
    EXPECT_EQ(native.value().document.raw().version, "AC1032");
    EXPECT_EQ(std::get<core::Text>(native.value().document.entities()[0].entity).value, "café 中");
}

TEST(Cadx, ReadsArchiveConstructedWithIndependentZipTool) {
    // This seed was assembled with Info-ZIP, not with writeZip.
    std::ifstream stream{std::string{TG_IO_TEST_DATA} + "/../../fuzz/corpus/cadx/manual.cadx",
                         std::ios::binary};
    ASSERT_TRUE(stream);
    const std::string bytes{std::istreambuf_iterator<char>{stream}, {}};
    const auto result = readCadx(bytes);
    ASSERT_TRUE(result.ok()) << result.error().message;
}

TEST(Cadx, RejectsInvalidNativePartsWithoutLosingThem) {
    for (const auto* name : {"../escape", "mimetype", "drawing.dxf", "/absolute"}) {
        core::Document document;
        core::RawDrawing raw;
        raw.nativeParts.push_back({name, "data"});
        document.setRaw(raw);
        EXPECT_FALSE(writeCadx(document).ok());
        EXPECT_EQ(document.raw().nativeParts, raw.nativeParts);
    }
}

}  // namespace
}  // namespace tg::io
