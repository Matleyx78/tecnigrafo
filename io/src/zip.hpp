// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// The ZIP container of the .cadx files (docs/adr/0019): stored and deflate
// parts, no encryption, no ZIP64, a single disk (PKWARE APPNOTE, 4.3 and 4.4).
// Internal to io: the .cadx reader and writer are its only users.
namespace tg::io::detail {

/// One file of an archive.
struct ZipPart {
    std::string name;    ///< UTF-8
    std::string bytes;   ///< uncompressed content
    bool stored{false};  ///< read: method 0; write: do not compress
};

/// Safety limits of readZip, checked before and while decompressing.
struct ZipLimits {
    std::size_t maxParts{256};
    std::uint64_t maxTotalSize{std::uint64_t{1} << 30};  ///< 1 GiB uncompressed
    std::uint32_t maxRatio{1000};                        ///< per part
};

/// Parts in the order of the central directory (docs/adr/0019, point 2).
///
/// Errors: Error::Kind::Unsupported for encryption, ZIP64, several disks and
/// methods other than 0 and 8; Error::Kind::TooLarge when a limit is
/// exceeded; Error::Kind::Malformed for everything else that breaks the
/// format: a damaged structure, a wrong CRC-32, a size that does not match
/// the data, a name that is not valid UTF-8 (with or without general purpose
/// bit 11, docs/tappe/0.5/P13-zip.md), absolute, with a `..` component
/// or a backslash, or repeated. Error::line is always 0.
[[nodiscard]] Result<std::vector<ZipPart>> readZip(std::string_view archive,
                                                   const ZipLimits& limits = {});

/// Parts in the given order, with a fixed date (1980-01-01 00:00).
///
/// Names are written as given, with general purpose bit 11 when they are not
/// ASCII; no extra fields, no comments. The same parts always give the same
/// bytes. Preconditions: fewer than 65535 parts, names shorter than 65536
/// bytes, and every part and the whole archive smaller than 4 GiB.
[[nodiscard]] std::string writeZip(std::span<const ZipPart> parts);

}  // namespace tg::io::detail
