// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "json.hpp"

#include <gtest/gtest.h>

#include <string>

namespace tg::io::detail {
namespace {

TEST(Json, PreservesOrderTypesAndNumberSpelling) {
    const auto parsed = readJson(R"({"z":[null,true,false,-0,1.25e+1000],"a":{"x":"è中"}})");
    ASSERT_TRUE(parsed.ok());
    EXPECT_EQ(parsed.value().members.front().first, "z");
    const auto written = writeJson(parsed.value());
    ASSERT_TRUE(written.ok());
    const auto reread = readJson(written.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_EQ(reread.value(), parsed.value());
    EXPECT_EQ(written.value(), R"({"z":[null,true,false,-0,1.25e+1000],"a":{"x":"è中"}})");
}

TEST(Json, DecodesEscapesAndSurrogatePairs) {
    const auto parsed = readJson(R"("\"\\\/\b\f\n\r\t\u0000\u00e8\u4e2d\ud83d\ude00")");
    ASSERT_TRUE(parsed.ok());
    EXPECT_EQ(parsed.value().text, std::string("\"\\/\b\f\n\r\t\0", 9) + "è中😀");
    const auto written = writeJson(parsed.value());
    ASSERT_TRUE(written.ok());
    const auto reread = readJson(written.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_EQ(parsed.value(), reread.value());
}

TEST(Json, RejectsMalformedInput) {
    for (const auto* bytes : {"",
                              " ",
                              "{}x",
                              "[1,]",
                              R"({"a":1,})",
                              R"({"a":1,"a":2})",
                              R"({"a":1,"\u0061":2})",
                              "00",
                              "01",
                              "-01",
                              "+1",
                              ".1",
                              "1.",
                              "1e",
                              "1e+",
                              "NaN",
                              "Infinity",
                              "True",
                              R"("\x")",
                              "\"\n\"",
                              R"("\ud800")",
                              R"("\udc00")",
                              R"("\ud800\u0000")",
                              R"("\uZZZZ")",
                              R"("unterminated)",
                              "{1:2}",
                              "[true false]"}) {
        EXPECT_FALSE(readJson(bytes).ok()) << bytes;
    }
    EXPECT_FALSE(readJson(std::string{'"', static_cast<char>(0xff), '"'}).ok());
}

TEST(Json, EnforcesSizeAndDepthLimits) {
    const std::string allowed =
        std::string(kJsonMaxDepth, '[') + "0" + std::string(kJsonMaxDepth, ']');
    EXPECT_TRUE(readJson(allowed).ok());
    EXPECT_FALSE(readJson("[" + allowed + "]").ok());
    const std::string large = '"' + std::string(kJsonMaxSize - 2, 'x') + '"';
    EXPECT_TRUE(readJson(large).ok());
    const auto tooLarge = readJson(large + ' ');
    ASSERT_FALSE(tooLarge.ok());
    EXPECT_EQ(tooLarge.error().kind, Error::Kind::TooLarge);
    Json value;
    value.kind = Json::Kind::String;
    value.text = std::string(kJsonMaxSize, 'x');
    EXPECT_FALSE(writeJson(value).ok());
}

TEST(Json, SetReplacesInPlaceAndAppendsUnknownFields) {
    auto parsed = readJson(R"({"first":null,"format":1,"last":false})");
    ASSERT_TRUE(parsed.ok());
    Json value;
    value.kind = Json::Kind::String;
    value.text = "new";
    parsed.value().set("format", value);
    parsed.value().set("extra", value);
    const auto written = writeJson(parsed.value());
    ASSERT_TRUE(written.ok());
    EXPECT_EQ(written.value(), R"({"first":null,"format":"new","last":false,"extra":"new"})");
}

}  // namespace
}  // namespace tg::io::detail
