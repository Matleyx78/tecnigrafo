// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/raw_group.hpp"
#include "tg/io/result.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::io {

/// How the text of a DXF file is encoded (docs/adr/0017, point 3).
enum class TextEncoding : std::uint8_t { Utf8, Ansi1252, Ansi1250 };

/// The groups of an ASCII DXF file, before any meaning is given to them.
struct GroupFile {
    core::RawGroups groups;          ///< values decoded to UTF-8
    std::vector<std::size_t> lines;  ///< line of each group code
    std::string version;             ///< $ACADVER, "" if absent
    std::string codePage;            ///< $DWGCODEPAGE, "" if absent
    std::vector<std::string> warnings;
};

/// Reads the code/value lines of an ASCII DXF file (docs/adr/0017).
///
/// - Lines end with "\n" or "\r\n". Spaces around a group code are ignored;
///   a value is kept as written, without its final "\r".
/// - Every code is an integer from -5 to 1071, and every value is checked
///   against the type of its code (text, real, 16, 32 or 64-bit integer,
///   boolean, hexadecimal handle, hexadecimal binary data).
/// - Reading stops at the group `0 EOF`, which is kept; anything else after
///   it gives a warning.
/// - A binary DXF, or a version before AC1015 or unknown, is Unsupported.
///   Without $ACADVER the text is read as UTF-8, with a warning.
/// - Text values are decoded to UTF-8: UTF-8 files from AC1021, the code page
///   of $DWGCODEPAGE before (ANSI_1252 or ANSI_1250; any other gives a
///   warning and is read as ANSI_1252). `\U+XXXX` becomes the character in
///   every version.
/// - Limits (docs/adr/0017, point 7): a line longer than 64 KiB is TooLarge;
///   a file that ends inside a group is Malformed.
[[nodiscard]] Result<GroupFile> readGroups(std::string_view bytes);

/// The groups as DXF text: each code right-aligned on three characters as
/// AutoCAD does, "\r\n" line ends, values in `encoding`. In an ANSI encoding
/// the characters the code page lacks become `\U+XXXX`.
///
/// Precondition: no value contains "\r" or "\n".
[[nodiscard]] std::string writeGroups(std::span<const core::RawGroup> groups,
                                      TextEncoding encoding);

/// The shortest text that parseNumber() reads back as `value`, always with a
/// decimal digit ("1.0", not "1"), with a decimal point whatever the system
/// language. Precondition: `value` is finite.
[[nodiscard]] std::string formatNumber(double value);

/// A finite number written as `[+-] digits [. digits] [(e|E) [+-] digits]`,
/// with a decimal point whatever the system language; std::nullopt for
/// anything else (spaces, a comma, "inf", "nan", a value out of range).
[[nodiscard]] std::optional<double> parseNumber(std::string_view text);

/// Text conversions used by readGroups() and writeGroups(); public so that
/// they can be tested on their own. Not part of the module's interface.
namespace detail {

/// True if `bytes` is well-formed UTF-8 (no overlong forms, no surrogates,
/// nothing above U+10FFFF).
[[nodiscard]] bool isValidUtf8(std::string_view bytes);

/// `bytes` in `encoding` as UTF-8, with every `\U+XXXX` replaced by its
/// character. std::nullopt if `encoding` is Utf8 and `bytes` is not valid
/// UTF-8.
[[nodiscard]] std::optional<std::string> decodeText(std::string_view bytes, TextEncoding encoding);

/// UTF-8 `text` in `encoding`: unchanged for Utf8; in an ANSI code page the
/// missing characters become `\U+XXXX` (a surrogate pair above U+FFFF), and
/// an invalid UTF-8 byte becomes `\U+FFFD`.
[[nodiscard]] std::string encodeText(std::string_view text, TextEncoding encoding);

}  // namespace detail

}  // namespace tg::io
