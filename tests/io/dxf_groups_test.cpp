// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/dxf_groups.hpp"

#include "tg/core/raw_group.hpp"
#include "tg/io/io.hpp"
#include "tg/io/result.hpp"

#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::io {
namespace {

using Groups = std::vector<std::pair<int, std::string>>;

/// The groups as DXF lines, codes right-aligned as AutoCAD writes them.
std::string lines(const Groups& groups, std::string_view lineEnd = "\n") {
    std::string out;
    for (const auto& [code, value] : groups) {
        std::string text = std::to_string(code);
        out += std::string(text.size() < 3 ? 3 - text.size() : 0, ' ') + text;
        out += lineEnd;
        out += value;
        out += lineEnd;
    }
    return out;
}

/// A small DXF: a header with `version`, one LINE, then `extra` groups in
/// the ENTITIES section.
Groups minimalDxf(const std::string& version = "AC1032", const Groups& extra = {}) {
    Groups groups = {
        {0, "SECTION"},   {2, "HEADER"}, {9, "$ACADVER"}, {1, version},    {9, "$DWGCODEPAGE"},
        {3, "ANSI_1252"}, {0, "ENDSEC"}, {0, "SECTION"},  {2, "ENTITIES"}, {0, "LINE"},
        {5, "1F"},        {8, "0"},      {10, "1.5"},     {20, "-2"},      {30, "0.0"},
        {11, "4"},        {21, "6"},     {31, "0"},
    };
    groups.insert(groups.end(), extra.begin(), extra.end());
    groups.emplace_back(0, "ENDSEC");
    groups.emplace_back(0, "EOF");
    return groups;
}

/// Reads a minimal DXF with one more group with `code` and `value`.
Result<GroupFile> readWith(int code, const std::string& value) {
    return readGroups(lines(minimalDxf("AC1032", {{code, value}})));
}

// --- Lines and groups --------------------------------------------------------

TEST(DxfGroupsTest, ReadsMinimalDxfWithLineFeeds) {
    const Groups groups = minimalDxf();
    const Result<GroupFile> result = readGroups(lines(groups));
    ASSERT_TRUE(result.ok()) << result.error().message;
    const GroupFile& file = result.value();
    ASSERT_EQ(file.groups.size(), groups.size());
    for (std::size_t i = 0; i < groups.size(); ++i) {
        EXPECT_EQ(file.groups[i].code, groups[i].first);
        EXPECT_EQ(file.groups[i].value, groups[i].second);
        EXPECT_EQ(file.lines[i], (2 * i) + 1);
    }
    EXPECT_EQ(file.version, "AC1032");
    EXPECT_EQ(file.codePage, "ANSI_1252");
    EXPECT_TRUE(file.warnings.empty());
}

TEST(DxfGroupsTest, ReadsMinimalDxfWithCarriageReturns) {
    const Groups groups = minimalDxf("AC1015");
    const Result<GroupFile> lf = readGroups(lines(groups));
    const Result<GroupFile> crlf = readGroups(lines(groups, "\r\n"));
    ASSERT_TRUE(lf.ok()) << lf.error().message;
    ASSERT_TRUE(crlf.ok()) << crlf.error().message;
    EXPECT_EQ(crlf.value().groups, lf.value().groups);
    EXPECT_EQ(crlf.value().lines, lf.value().lines);
    EXPECT_EQ(crlf.value().version, "AC1015");
}

TEST(DxfGroupsTest, IgnoresSpacesAroundCodesAndKeepsThemInValues) {
    const std::string text = "  0 \nSECTION\n2\t\n  HEADER \n  70\n     0\n0\nEOF";
    const Result<GroupFile> result = readGroups(text);
    ASSERT_TRUE(result.ok()) << result.error().message;
    const core::RawGroups& groups = result.value().groups;
    ASSERT_EQ(groups.size(), 4U);
    EXPECT_EQ(groups[1].code, 2);
    EXPECT_EQ(groups[1].value, "  HEADER ");
    EXPECT_EQ(groups[2].code, 70);
    EXPECT_EQ(groups[2].value, "     0");
    EXPECT_EQ(groups[3].value, "EOF");  // the last line has no line end
}

TEST(DxfGroupsTest, StopsAtEofAndWarnsAboutWhatFollows) {
    const Result<GroupFile> blank = readGroups(lines(minimalDxf()) + "\r\n\n  \n");
    ASSERT_TRUE(blank.ok()) << blank.error().message;
    EXPECT_TRUE(blank.value().warnings.empty());

    const std::string text = lines(minimalDxf());
    const Result<GroupFile> extra = readGroups(text + "garbage\n");
    ASSERT_TRUE(extra.ok()) << extra.error().message;
    EXPECT_EQ(extra.value().groups.back().value, "EOF");
    ASSERT_EQ(extra.value().warnings.size(), 1U);
    EXPECT_NE(extra.value().warnings[0].find("EOF"), std::string::npos);
}

TEST(DxfGroupsTest, RejectsEmptyFile) {
    const Result<GroupFile> result = readGroups("");
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
}

TEST(DxfGroupsTest, RejectsCodesOutsideTheDxfRange) {
    for (const std::string code : {"-6", "1072", "abc", "", "1.0", "99999999999999999999"}) {
        const Result<GroupFile> result = readGroups(code + "\nvalue\n");
        ASSERT_FALSE(result.ok()) << code;
        EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
        EXPECT_EQ(result.error().line, 1U);
        EXPECT_NE(result.error().message.find("line 1"), std::string::npos);
        EXPECT_NE(result.error().message.find("group code"), std::string::npos);
    }
    EXPECT_TRUE(readGroups("-5\nvalue\n1071\n1\n").ok());
}

TEST(DxfGroupsTest, RejectsFileEndingInsideAGroup) {
    const Result<GroupFile> result = readGroups("0\nSECTION\n2\n");
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
    EXPECT_EQ(result.error().line, 3U);
    EXPECT_NE(result.error().message.find("line 3"), std::string::npos);
}

TEST(DxfGroupsTest, RejectsLinesLongerThan64KiB) {
    constexpr std::size_t kLimit = 64 * 1024;
    const std::string longest(kLimit, 'x');
    EXPECT_TRUE(readGroups("1\n" + longest + "\r\n0\nEOF\n").ok());

    const Result<GroupFile> result = readGroups("1\n" + longest + "x\n0\nEOF\n");
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, Error::Kind::TooLarge);
    EXPECT_EQ(result.error().line, 2U);
    EXPECT_NE(result.error().message.find("line 2"), std::string::npos);
}

// --- Values -------------------------------------------------------------------

struct ValueCase {
    int code;
    std::string value;
    bool valid;
};

TEST(DxfGroupsTest, ChecksEveryValueType) {
    const std::vector<ValueCase> cases = {
        // Text: anything.
        {1, "any text, even 1,5", true},
        {1, "", true},
        {999, "a comment", true},
        // Reals.
        {10, "1.5", true},
        {40, "  -2.5e3  ", true},
        {1040, ".5", true},
        {10, "1,5", false},
        {40, "", false},
        {50, "inf", false},
        {140, "nan", false},
        {460, "1e400", false},
        // 16-bit integers.
        {70, "32767", true},
        {70, "-32768", true},
        {62, "     7", true},
        {70, "32768", false},
        {70, "-32769", false},
        {70, "1.0", false},
        {280, "x", false},
        {1070, "", false},
        // 32-bit integers.
        {90, "2147483647", true},
        {90, "-2147483648", true},
        {420, "16777215", true},
        {90, "2147483648", false},
        {1071, "-2147483649", false},
        // 64-bit integers.
        {160, "9223372036854775807", true},
        {160, "-9223372036854775808", true},
        {160, "9223372036854775808", false},
        // Booleans.
        {290, "0", true},
        {290, "     1", true},
        {290, "2", false},
        {290, "true", false},
        // Handles.
        {330, "1F", true},
        {105, "abcdef0123456789", true},
        {1005, "FFFFFFFFFFFFFFFF", true},
        {330, "12345678901234567", false},
        {340, "", false},
        {390, "G1", false},
        // Binary data.
        {310, "00FFa0", true},
        {1004, "0A", true},
        {310, "ABC", false},
        {310, "0G", false},
    };
    for (const ValueCase& c : cases) {
        const Result<GroupFile> result = readWith(c.code, c.value);
        EXPECT_EQ(result.ok(), c.valid) << c.code << " \"" << c.value << '"';
        if (c.valid && result.ok()) {
            const core::RawGroups& groups = result.value().groups;
            const core::RawGroup expected{static_cast<std::int16_t>(c.code), c.value};
            EXPECT_EQ(groups[groups.size() - 3], expected) << "values are kept as written";
        }
        if (!c.valid && !result.ok()) {
            const Error& error = result.error();
            EXPECT_EQ(error.kind, Error::Kind::Malformed);
            // The value of the extra group: after 18 groups, its second line.
            EXPECT_EQ(error.line, 38U) << c.code;
            EXPECT_NE(error.message.find("line 38"), std::string::npos) << error.message;
            EXPECT_NE(error.message.find("expected"), std::string::npos) << error.message;
            EXPECT_NE(error.message.find("found"), std::string::npos) << error.message;
        }
    }
}

// --- Versions -----------------------------------------------------------------

TEST(DxfGroupsTest, ReadsEverySupportedVersion) {
    for (const std::string version : {"AC1015", "AC1018", "AC1021", "AC1024", "AC1027", "AC1032"}) {
        const Result<GroupFile> result = readGroups(lines(minimalDxf(version)));
        ASSERT_TRUE(result.ok()) << version << ": " << result.error().message;
        EXPECT_EQ(result.value().version, version);
    }
}

TEST(DxfGroupsTest, RejectsBinaryDxf) {
    const std::string binary = std::string{"AutoCAD Binary DXF\r\n\x1A"} + '\0' + "\x00\x00";
    const Result<GroupFile> result = readGroups(binary);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, Error::Kind::Unsupported);
    EXPECT_NE(result.error().message.find("binary DXF"), std::string::npos);
}

TEST(DxfGroupsTest, RejectsVersionsBeforeAutoCad2000) {
    for (const std::string version : {"AC1009", "AC1014", "AC2.10"}) {
        const Result<GroupFile> result = readGroups(lines(minimalDxf(version)));
        ASSERT_FALSE(result.ok()) << version;
        EXPECT_EQ(result.error().kind, Error::Kind::Unsupported);
        EXPECT_EQ(result.error().line, 8U);  // the value of $ACADVER
        EXPECT_NE(result.error().message.find(version), std::string::npos)
            << result.error().message;
        EXPECT_NE(result.error().message.find("older"), std::string::npos)
            << result.error().message;
    }
}

TEST(DxfGroupsTest, RejectsUnknownVersions) {
    for (const std::string version : {"AC1033", "AC9999", "R14", ""}) {
        const Result<GroupFile> result = readGroups(lines(minimalDxf(version)));
        ASSERT_FALSE(result.ok()) << version;
        EXPECT_EQ(result.error().kind, Error::Kind::Unsupported);
        EXPECT_NE(result.error().message.find("unknown DXF version \"" + version + '"'),
                  std::string::npos)
            << result.error().message;
    }
}

TEST(DxfGroupsTest, WarnsWithoutVersion) {
    const Result<GroupFile> result =
        readGroups(lines({{0, "SECTION"}, {2, "ENTITIES"}, {0, "ENDSEC"}, {0, "EOF"}}));
    ASSERT_TRUE(result.ok()) << result.error().message;
    EXPECT_EQ(result.value().version, "");
    ASSERT_EQ(result.value().warnings.size(), 1U);
    EXPECT_NE(result.value().warnings[0].find("$ACADVER"), std::string::npos);
}

TEST(DxfGroupsTest, RejectsHeaderVariableWithoutItsValue) {
    const Result<GroupFile> result = readGroups(
        lines({{0, "SECTION"}, {2, "HEADER"}, {9, "$ACADVER"}, {3, "AC1032"}, {0, "EOF"}}));
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
    EXPECT_EQ(result.error().line, 7U);
}

// --- Truncation -----------------------------------------------------------------

TEST(DxfGroupsTest, TruncatedFileGivesErrorOrResult) {
    for (const std::string_view lineEnd : {"\n", "\r\n"}) {
        const std::string text = lines(minimalDxf("AC1018", {{1, "Città"}, {40, "2.5"}}), lineEnd);
        for (std::size_t length = 0; length <= text.size(); ++length) {
            const Result<GroupFile> result = readGroups(std::string_view{text}.substr(0, length));
            if (result.ok()) {
                EXPECT_EQ(result.value().groups.size(), result.value().lines.size());
            } else {
                EXPECT_FALSE(result.error().message.empty());
            }
            EXPECT_FALSE(readDxf(std::string_view{text}.substr(0, length)).ok());
        }
    }
}

// --- Writing ------------------------------------------------------------------

TEST(DxfGroupsTest, WritesCodesRightAlignedWithCarriageReturns) {
    const core::RawGroups groups = {
        {0, "SECTION"}, {10, "1.5"}, {100, "AcDbEntity"}, {1001, "APP"}, {-1, "x"}};
    EXPECT_EQ(writeGroups(groups, TextEncoding::Utf8),
              "  0\r\nSECTION\r\n 10\r\n1.5\r\n100\r\nAcDbEntity\r\n1001\r\nAPP\r\n -1\r\nx\r\n");
}

TEST(DxfGroupsTest, WrittenGroupsReadBackEqual) {
    const Result<GroupFile> first = readGroups(lines(minimalDxf("AC1032", {{62, "     7"}})));
    ASSERT_TRUE(first.ok()) << first.error().message;
    const std::string written = writeGroups(first.value().groups, TextEncoding::Utf8);
    const Result<GroupFile> second = readGroups(written);
    ASSERT_TRUE(second.ok()) << second.error().message;
    EXPECT_EQ(second.value().groups, first.value().groups);
}

// --- Numbers ------------------------------------------------------------------

TEST(DxfGroupsTest, ParsesNumbersWithADecimalPoint) {
    EXPECT_EQ(parseNumber("1"), 1.0);
    EXPECT_EQ(parseNumber("1.0"), 1.0);
    EXPECT_EQ(parseNumber("+2.5"), 2.5);
    EXPECT_EQ(parseNumber(".5"), 0.5);
    EXPECT_EQ(parseNumber("5."), 5.0);
    EXPECT_EQ(parseNumber("1e300"), 1e300);
    EXPECT_EQ(parseNumber("-1.5E-3"), -1.5e-3);
    const std::optional<double> negativeZero = parseNumber("-0.0");
    ASSERT_TRUE(negativeZero.has_value());
    EXPECT_EQ(*negativeZero, 0.0);
    EXPECT_TRUE(std::signbit(*negativeZero));
}

TEST(DxfGroupsTest, RejectsInvalidNumbers) {
    for (const std::string_view text :
         {"1e400", "-1e400", "nan", "NaN", "inf", "-inf", ",5", "1,5", "", " 1", "1 ", "0x10", "1e",
          "e5", ".", "-", "1.2.3", "--1"}) {
        EXPECT_FALSE(parseNumber(text).has_value()) << '"' << text << '"';
    }
}

TEST(DxfGroupsTest, FormatsNumbersWithADecimalDigit) {
    EXPECT_EQ(formatNumber(1.0), "1.0");
    EXPECT_EQ(formatNumber(0.0), "0.0");
    EXPECT_EQ(formatNumber(-0.0), "-0.0");
    EXPECT_EQ(formatNumber(-2.5), "-2.5");
    EXPECT_EQ(formatNumber(0.1), "0.1");
    EXPECT_EQ(formatNumber(100.0), "100.0");
    EXPECT_EQ(formatNumber(1.0 / 3.0), "0.3333333333333333");
    EXPECT_EQ(formatNumber(0.0001), "0.0001");
    EXPECT_EQ(formatNumber(1e-5), "1.0e-05");
    EXPECT_EQ(formatNumber(1e15), "1000000000000000.0");
    EXPECT_EQ(formatNumber(1e16), "1.0e+16");
    EXPECT_EQ(formatNumber(1e300), "1.0e+300");
    EXPECT_EQ(formatNumber(1.25e-300), "1.25e-300");
}

TEST(DxfGroupsTest, FormattedNumbersReadBackEqual) {
    std::mt19937_64 generator{20261022};
    std::uniform_int_distribution<std::uint64_t> bits;
    std::uniform_real_distribution<double> coordinates{-1e6, 1e6};
    const auto check = [](double value) {
        const std::string text = formatNumber(value);
        const std::optional<double> back = parseNumber(text);
        ASSERT_TRUE(back.has_value()) << text;
        EXPECT_EQ(*back, value) << text;
        EXPECT_NE(text.find('.'), std::string::npos) << text;
    };
    for (int i = 0; i < 2000; ++i) {
        check(coordinates(generator));
        // Any normal double, from its bits. Subnormal numbers are left out:
        // the stream used on macOS (see dxf_groups.cpp) reports them as out
        // of range.
        const double value = std::bit_cast<double>(bits(generator));
        if (std::isnormal(value)) {
            check(value);
        }
    }
    check(std::numeric_limits<double>::max());
    check(std::numeric_limits<double>::lowest());
    check(std::numeric_limits<double>::min());
    check(std::numeric_limits<double>::epsilon());
}

// --- The module's interface -------------------------------------------------

TEST(IoTest, ReadDxfReadsTheGroupsThenIsNotImplemented) {
    const Result<ReadResult> valid = readDxf(lines(minimalDxf()));
    ASSERT_FALSE(valid.ok());
    EXPECT_EQ(valid.error().kind, Error::Kind::NotImplemented);
    EXPECT_NE(valid.error().message.find("P10"), std::string::npos);

    const Result<ReadResult> invalid = readDxf("0\nSECTION\n70\nx\n");
    ASSERT_FALSE(invalid.ok());
    EXPECT_EQ(invalid.error().kind, Error::Kind::Malformed);
    EXPECT_EQ(invalid.error().line, 4U);
}

TEST(IoTest, OtherFunctionsAreNotImplemented) {
    const core::Document document;
    const Result<std::string> dxf = writeDxf(document, DxfVersion::R2018);
    ASSERT_FALSE(dxf.ok());
    EXPECT_EQ(dxf.error().kind, Error::Kind::NotImplemented);
    EXPECT_NE(dxf.error().message.find("P10"), std::string::npos);

    const Result<ReadResult> readCadxResult = readCadx("");
    ASSERT_FALSE(readCadxResult.ok());
    EXPECT_EQ(readCadxResult.error().kind, Error::Kind::NotImplemented);
    EXPECT_NE(readCadxResult.error().message.find("P14"), std::string::npos);

    const Result<std::string> cadx = writeCadx(document);
    ASSERT_FALSE(cadx.ok());
    EXPECT_EQ(cadx.error().kind, Error::Kind::NotImplemented);
    EXPECT_NE(cadx.error().message.find("P14"), std::string::npos);
    EXPECT_EQ(kCadxFormat, 1);
}

}  // namespace
}  // namespace tg::io
