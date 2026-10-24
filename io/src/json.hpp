// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/result.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tg::io::detail {

inline constexpr std::size_t kJsonMaxSize = 1024 * 1024;
inline constexpr std::size_t kJsonMaxDepth = 32;

struct JsonMember;

/// Numbers retain their RFC 8259 spelling: unknown fields need not fit a double.
/// Object members retain their order; duplicate keys are rejected by readJson.
struct Json {
    enum class Kind : std::uint8_t { Null, Boolean, Number, String, Array, Object };
    Kind kind{Kind::Null};
    std::string text;
    std::vector<Json> elements;
    std::vector<JsonMember> members;

    [[nodiscard]] const Json* find(std::string_view key) const;
    void set(std::string key, Json value);
    friend bool operator==(const Json&, const Json&);
};

// vector supports incomplete elements; pair<string, Json> does not provide
// that guarantee while Json itself is incomplete (notably with libstdc++ 14).
struct JsonMember {
    std::string first;
    Json second;

    friend bool operator==(const JsonMember&, const JsonMember&) = default;
};

[[nodiscard]] Result<Json> readJson(std::string_view bytes);
/// Validates the generated JSON against the same limits as the reader.
[[nodiscard]] Result<std::string> writeJson(const Json& value);

}  // namespace tg::io::detail
