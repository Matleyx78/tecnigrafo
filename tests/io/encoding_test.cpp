// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/raw_group.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/result.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>

namespace tg::io {
namespace {

using detail::decodeText;
using detail::encodeText;
using detail::isValidUtf8;

/// Text with letters of both code pages, one character in neither (the
/// diameter sign, U+2300) and one outside the Basic Multilingual Plane.
constexpr std::string_view kMixedText = "Città è €; Šč ąŁ; ⌀ 50; \U0001F600";

/// A DXF with header `version` and `codePage` and one TEXT value `text`.
core::RawGroups textDxf(std::string_view version, std::string_view codePage,
                        std::string_view text) {
    return {{0, "SECTION"},
            {2, "HEADER"},
            {9, "$ACADVER"},
            {1, std::string{version}},
            {9, "$DWGCODEPAGE"},
            {3, std::string{codePage}},
            {0, "ENDSEC"},
            {0, "SECTION"},
            {2, "ENTITIES"},
            {0, "TEXT"},
            {1, std::string{text}},
            {0, "ENDSEC"},
            {0, "EOF"}};
}

/// The DXF of textDxf() with its text written as the raw bytes `bytes`.
std::string rawTextDxf(std::string_view version, std::string_view codePage,
                       std::string_view bytes) {
    std::string out;
    for (const core::RawGroup& group : textDxf(version, codePage, "@")) {
        out += std::to_string(group.code) + "\n";
        out += group.value == "@" ? std::string{bytes} : group.value;
        out += "\n";
    }
    return out;
}

/// The value of the TEXT read from a DXF with the given header and bytes.
std::string readText(std::string_view version, std::string_view codePage, std::string_view bytes) {
    const Result<GroupFile> result = readGroups(rawTextDxf(version, codePage, bytes));
    if (!result.ok()) {
        ADD_FAILURE() << result.error().message;
        return {};
    }
    return result.value().groups[10].value;
}

TEST(EncodingTest, ReadsAnsi1252) {
    EXPECT_EQ(readText("AC1015", "ANSI_1252", "Citt\xE0 \xE8 \x80 \xFF"), "Città è € ÿ");
    EXPECT_EQ(readText("AC1018", "ansi_1252", "\xC0\xC9"), "ÀÉ");
}

TEST(EncodingTest, ReadsAnsi1250) {
    EXPECT_EQ(readText("AC1015", "ANSI_1250",
                       "\x8A\xE8\xB9 \xA3\xF3"
                       "d\xBF"),
              "Ščą Łódż");
}

TEST(EncodingTest, ReadsUtf8FromAutoCad2007) {
    // From AC1021 the code page no longer matters.
    EXPECT_EQ(readText("AC1021", "ANSI_1252", "Città"), "Città");
    EXPECT_EQ(readText("AC1032", "ANSI_1250", "Šč"), "Šč");
}

TEST(EncodingTest, ReadsUnicodeEscapesInEveryVersion) {
    for (const std::string_view version : {"AC1015", "AC1021", "AC1032"}) {
        EXPECT_EQ(readText(version, "ANSI_1252", "\\U+00E8 \\U+2300 \\u+00e8"), "è ⌀ \\u+00e8")
            << version;
        EXPECT_EQ(readText(version, "ANSI_1252", "\\U+D83D\\U+DE00"), "\U0001F600") << version;
    }
    // Not characters: too few digits, not hexadecimal, NUL, lone surrogates.
    EXPECT_EQ(readText("AC1032", "", "\\U+00E \\U+00G8 \\U+0000 \\U+D83D \\U+DE00"),
              "\\U+00E \\U+00G8 \\U+0000 \\U+D83D \\U+DE00");
}

TEST(EncodingTest, UnknownCodePageWarnsAndReadsAnsi1252) {
    const Result<GroupFile> result = readGroups(rawTextDxf("AC1015", "ANSI_932", "\xE8"));
    ASSERT_TRUE(result.ok()) << result.error().message;
    EXPECT_EQ(result.value().codePage, "ANSI_932");
    EXPECT_EQ(result.value().groups[10].value, "è");
    ASSERT_EQ(result.value().warnings.size(), 1U);
    EXPECT_NE(result.value().warnings[0].find("ANSI_932"), std::string::npos);
    EXPECT_NE(result.value().warnings[0].find("ANSI_1252"), std::string::npos);
}

TEST(EncodingTest, MissingCodePageWarnsBeforeAutoCad2007) {
    const Result<GroupFile> result =
        readGroups("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n0\nEOF\n");
    ASSERT_TRUE(result.ok()) << result.error().message;
    EXPECT_EQ(result.value().codePage, "");
    ASSERT_EQ(result.value().warnings.size(), 1U);
    EXPECT_NE(result.value().warnings[0].find("$DWGCODEPAGE"), std::string::npos);
}

TEST(EncodingTest, RejectsInvalidUtf8) {
    for (const std::string_view bytes :
         {std::string_view{"Citt\xE0"}, std::string_view{"\xC0\xAF"},
          std::string_view{"\xED\xA0\x80"}, std::string_view{"\xF4\x90\x80\x80"},
          std::string_view{"\xE2\x8C"}}) {
        const Result<GroupFile> result = readGroups(rawTextDxf("AC1021", "ANSI_1252", bytes));
        ASSERT_FALSE(result.ok());
        EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
        EXPECT_EQ(result.error().line, 22U);  // the value of group 1 of the TEXT
        EXPECT_NE(result.error().message.find("line 22"), std::string::npos);
        EXPECT_NE(result.error().message.find("UTF-8"), std::string::npos);
        EXPECT_TRUE(isValidUtf8(result.error().message)) << "the message quotes bytes safely";
    }
}

TEST(EncodingTest, ChecksUtf8) {
    EXPECT_TRUE(isValidUtf8(""));
    EXPECT_TRUE(isValidUtf8("plain"));
    EXPECT_TRUE(isValidUtf8(kMixedText));
    EXPECT_TRUE(isValidUtf8("\xF4\x8F\xBF\xBF"));  // U+10FFFF
    EXPECT_FALSE(isValidUtf8("\x80"));
    EXPECT_FALSE(isValidUtf8("\xC3"));
    EXPECT_FALSE(isValidUtf8("\xC3("));
    EXPECT_FALSE(isValidUtf8("\xE0\x80\x80"));  // overlong
    EXPECT_FALSE(isValidUtf8("\xFF"));
}

TEST(EncodingTest, EncodesMissingCharactersAsEscapes) {
    EXPECT_EQ(encodeText("è€", TextEncoding::Ansi1252), "\xE8\x80");
    EXPECT_EQ(encodeText("č", TextEncoding::Ansi1252), "\\U+010D");
    EXPECT_EQ(encodeText("č", TextEncoding::Ansi1250), "\xE8");
    EXPECT_EQ(encodeText("⌀", TextEncoding::Ansi1250), "\\U+2300");
    EXPECT_EQ(encodeText("\U0001F600", TextEncoding::Ansi1252), "\\U+D83D\\U+DE00");
    EXPECT_EQ(encodeText("a\xFF"
                         "b",
                         TextEncoding::Ansi1252),
              "a\\U+FFFDb");
    EXPECT_EQ(encodeText(kMixedText, TextEncoding::Utf8), kMixedText);
}

TEST(EncodingTest, DecodesWhatItEncodes) {
    for (const TextEncoding encoding :
         {TextEncoding::Utf8, TextEncoding::Ansi1252, TextEncoding::Ansi1250}) {
        EXPECT_EQ(decodeText(encodeText(kMixedText, encoding), encoding),
                  std::optional<std::string>{std::string{kMixedText}});
    }
}

TEST(EncodingTest, WritesAndReadsBackInEveryEncoding) {
    struct Case {
        TextEncoding encoding;
        std::string_view version;
        std::string_view codePage;
    };

    for (const Case& c : {Case{TextEncoding::Utf8, "AC1032", "ANSI_1252"},
                          Case{TextEncoding::Ansi1252, "AC1015", "ANSI_1252"},
                          Case{TextEncoding::Ansi1250, "AC1018", "ANSI_1250"}}) {
        const core::RawGroups groups = textDxf(c.version, c.codePage, kMixedText);
        const std::string written = writeGroups(groups, c.encoding);
        if (c.encoding != TextEncoding::Utf8) {
            EXPECT_FALSE(isValidUtf8(written)) << "the text is in the code page";
        }
        const Result<GroupFile> result = readGroups(written);
        ASSERT_TRUE(result.ok()) << result.error().message;
        EXPECT_EQ(result.value().groups, groups) << c.version;
        EXPECT_TRUE(result.value().warnings.empty());
    }
}

}  // namespace
}  // namespace tg::io
