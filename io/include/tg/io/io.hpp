// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/io/result.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace tg::io {

/// The DXF versions Tecnigrafo writes (docs/adr/0017, point 2).
enum class DxfVersion : std::uint8_t { R2000, R2004, R2007, R2010, R2013, R2018 };

/// What reading did not handle or had to guess (docs/adr/0017, point 1).
struct ReadReport {
    std::map<std::string, int> unhandled;  ///< entity type -> count
    std::vector<std::string> warnings;
};

struct ReadResult {
    core::Document document;
    ReadReport report;
};

/// A drawing from the bytes of an ASCII DXF file.
[[nodiscard]] Result<ReadResult> readDxf(std::string_view bytes);

/// The bytes of an ASCII DXF file in `version`.
[[nodiscard]] Result<std::string> writeDxf(const core::Document& document, DxfVersion version);

/// A drawing from the bytes of a .cadx file (docs/adr/0006).
[[nodiscard]] Result<ReadResult> readCadx(std::string_view bytes);

/// The bytes of a .cadx file.
[[nodiscard]] Result<std::string> writeCadx(const core::Document& document);

/// The `format` of manifest.json in the .cadx files written.
inline constexpr int kCadxFormat = 1;

}  // namespace tg::io
