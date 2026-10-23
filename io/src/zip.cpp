// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "zip.hpp"

#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <zlib.h>

namespace tg::io::detail {
namespace {

// Record signatures (PKWARE APPNOTE 4.3.7, 4.3.12, 4.3.15, 4.3.16).
constexpr std::uint32_t kLocalHeaderSignature = 0x04034B50;
constexpr std::uint32_t kCentralHeaderSignature = 0x02014B50;
constexpr std::uint32_t kZip64LocatorSignature = 0x07064B50;
constexpr std::uint32_t kEndRecordSignature = 0x06054B50;

// Fixed sizes of the records, without their variable fields (APPNOTE 4.3).
constexpr std::size_t kLocalHeaderSize = 30;
constexpr std::size_t kCentralHeaderSize = 46;
constexpr std::size_t kZip64LocatorSize = 20;
constexpr std::size_t kEndRecordSize = 22;
/// The archive comment, at the end of the file, has a 16-bit length.
constexpr std::size_t kMaxCommentSize = 0xFFFF;

// General purpose bit flags (APPNOTE 4.4.4).
constexpr std::uint16_t kFlagEncrypted = 1U << 0U;
constexpr std::uint16_t kFlagDataDescriptor = 1U << 3U;
constexpr std::uint16_t kFlagStrongEncryption = 1U << 6U;
constexpr std::uint16_t kFlagUtf8 = 1U << 11U;
constexpr std::uint16_t kFlagMaskedHeaders = 1U << 13U;
constexpr std::uint16_t kFlagsUnsupported =
    kFlagEncrypted | kFlagStrongEncryption | kFlagMaskedHeaders;

// Compression methods (APPNOTE 4.4.5).
constexpr std::uint16_t kMethodStored = 0;
constexpr std::uint16_t kMethodDeflate = 8;

/// A field set to all ones means "the value is in the ZIP64 records"
/// (APPNOTE 4.4.1.4).
constexpr std::uint16_t kZip64Marker16 = 0xFFFF;
constexpr std::uint32_t kZip64Marker32 = 0xFFFFFFFF;

// Versions needed to extract (APPNOTE 4.4.3.2): 1.0 for stored, 2.0 for
// deflate. "Made by" is 2.0 with host 0 (MS-DOS): no Unix attributes.
constexpr std::uint16_t kVersionStored = 10;
constexpr std::uint16_t kVersionDeflate = 20;
constexpr std::uint16_t kVersionMadeBy = 20;

// MS-DOS date and time (APPNOTE 4.4.6): 1980-01-01 00:00, the earliest
// representable, so that the same parts always give the same bytes.
constexpr std::uint16_t kDosTime = 0;
constexpr std::uint16_t kDosDate = (1U << 5U) | 1U;

/// zlib's default compression level (docs/tappe/0.5/P13-zip.md).
constexpr int kDeflateLevel = 6;
/// Negative window bits: raw deflate, without the zlib header and trailer,
/// as ZIP stores it; 15 is the largest window, the one deflate allows.
constexpr int kRawDeflateWindowBits = -15;
/// zlib's default memory level.
constexpr int kDeflateMemLevel = 8;

/// The first byte outside ASCII.
constexpr unsigned kFirstNonAscii = 0x80;

Error makeError(Error::Kind kind, std::string message) {
    return Error{.kind = kind, .line = 0, .message = std::move(message)};
}

Error malformed(std::string message) {
    return makeError(Error::Kind::Malformed, std::move(message));
}

Error unsupported(std::string message) {
    return makeError(Error::Kind::Unsupported, std::move(message));
}

Error tooLarge(std::string message) {
    return makeError(Error::Kind::TooLarge, std::move(message));
}

// zlib works on unsigned char; char and unsigned char may alias each other.
const Bytef* asBytes(std::string_view bytes) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<const Bytef*>(bytes.data());
}

Bytef* asBytes(std::string& bytes) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Bytef*>(bytes.data());
}

std::uint32_t crc32Of(std::string_view bytes) {
    return static_cast<std::uint32_t>(crc32_z(0, asBytes(bytes), bytes.size()));
}

unsigned byteAt(std::string_view bytes, std::size_t position) {
    return static_cast<unsigned char>(bytes[position]);
}

/// Little-endian values (APPNOTE 4.4.1.1). Precondition: in range.
std::uint16_t readU16(std::string_view bytes, std::size_t position) {
    assert(position + 2 <= bytes.size());
    return static_cast<std::uint16_t>(byteAt(bytes, position) |
                                      (byteAt(bytes, position + 1) << 8U));
}

std::uint32_t readU32(std::string_view bytes, std::size_t position) {
    return static_cast<std::uint32_t>(readU16(bytes, position)) |
           (static_cast<std::uint32_t>(readU16(bytes, position + 2)) << 16U);
}

void appendU16(std::string& out, std::uint16_t value) {
    out.push_back(static_cast<char>(value & 0xFFU));
    out.push_back(static_cast<char>(value >> 8U));
}

void appendU32(std::string& out, std::uint32_t value) {
    appendU16(out, static_cast<std::uint16_t>(value & 0xFFFFU));
    appendU16(out, static_cast<std::uint16_t>(value >> 16U));
}

bool isAscii(std::string_view text) {
    return std::ranges::all_of(
        text, [](char c) { return static_cast<unsigned char>(c) < kFirstNonAscii; });
}

std::string partLabel(std::size_t index) {
    return "part " + std::to_string(index + 1);
}

std::string partLabel(std::size_t index, std::string_view name) {
    return partLabel(index) + " (\"" + std::string{name} + "\")";
}

/// The end of central directory record (APPNOTE 4.3.16).
struct EndRecord {
    std::size_t position{0};
    std::uint16_t disk{0};
    std::uint16_t directoryDisk{0};
    std::uint16_t entriesOnDisk{0};
    std::uint16_t entries{0};
    std::uint32_t directorySize{0};
    std::uint32_t directoryOffset{0};
};

/// The record whose comment reaches exactly the end of the archive, searched
/// from the end: the comment may itself contain the signature.
std::optional<EndRecord> findEndRecord(std::string_view archive) {
    if (archive.size() < kEndRecordSize) {
        return std::nullopt;
    }
    const std::size_t last = archive.size() - kEndRecordSize;
    const std::size_t first = last > kMaxCommentSize ? last - kMaxCommentSize : 0;
    for (std::size_t position = last + 1; position-- > first;) {
        if (readU32(archive, position) != kEndRecordSignature) {
            continue;
        }
        const std::size_t commentSize = readU16(archive, position + 20);
        if (position + kEndRecordSize + commentSize != archive.size()) {
            continue;
        }
        return EndRecord{.position = position,
                         .disk = readU16(archive, position + 4),
                         .directoryDisk = readU16(archive, position + 6),
                         .entriesOnDisk = readU16(archive, position + 8),
                         .entries = readU16(archive, position + 10),
                         .directorySize = readU32(archive, position + 12),
                         .directoryOffset = readU32(archive, position + 16)};
    }
    return std::nullopt;
}

/// A central directory file header (APPNOTE 4.3.12), with its name decoded.
struct CentralEntry {
    std::uint16_t flags{0};
    std::uint16_t method{0};
    std::uint32_t crc{0};
    std::uint32_t compressedSize{0};
    std::uint32_t size{0};
    std::uint32_t localOffset{0};
    std::string_view rawName;
    std::string name;
    std::size_t dataStart{0};  ///< filled in from the local header
};

/// The name in UTF-8: marked as such by bit 11, or plain ASCII. Without
/// bit 11 other bytes are in code page 437 (APPNOTE 4.4.4, appendix D).
Result<std::string> decodeName(std::string_view rawName, std::uint16_t flags, std::size_t index) {
    if ((flags & kFlagUtf8) == 0 && !isAscii(rawName)) {
        return malformed(partLabel(index) +
                         " has a name that is neither ASCII nor marked as UTF-8 (bit 11)");
    }
    if (!isValidUtf8(rawName)) {
        return malformed(partLabel(index) + " has a name that is not valid UTF-8");
    }
    return std::string{rawName};
}

/// Relative, without `..` components, backslashes or NUL characters.
std::optional<Error> checkName(std::string_view name, std::size_t index) {
    const std::string label = partLabel(index, name);
    if (name.empty()) {
        return malformed(partLabel(index) + " has an empty name");
    }
    const bool driveLetter =
        name.size() >= 2 && name[1] == ':' &&
        ((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= 'a' && name[0] <= 'z'));
    if (name.front() == '/' || driveLetter) {
        return malformed(label + " has an absolute name; expected a relative one");
    }
    if (name.find('\\') != std::string_view::npos) {
        return malformed(label + " has a backslash in its name; expected '/'");
    }
    if (name.find('\0') != std::string_view::npos) {
        return malformed(partLabel(index) + " has a NUL character in its name");
    }
    std::size_t start = 0;
    while (start <= name.size()) {
        const std::size_t end = std::min(name.find('/', start), name.size());
        if (name.substr(start, end - start) == "..") {
            return malformed(label + " has a '..' component in its name");
        }
        start = end + 1;
    }
    return std::nullopt;
}

/// Reads and checks every central directory header, and the limits, without
/// decompressing anything.
Result<std::vector<CentralEntry>> readCentralDirectory(std::string_view archive,
                                                       const EndRecord& end,
                                                       const ZipLimits& limits) {
    std::vector<CentralEntry> entries;
    entries.reserve(end.entries);
    std::set<std::string, std::less<>> names;
    std::uint64_t totalSize = 0;
    std::size_t position = end.directoryOffset;
    const std::size_t directoryEnd = position + end.directorySize;
    for (std::size_t index = 0; index < end.entries; ++index) {
        if (directoryEnd - position < kCentralHeaderSize ||
            readU32(archive, position) != kCentralHeaderSignature) {
            return malformed("the central directory has " + std::to_string(index) +
                             " valid entries; the end record declares " +
                             std::to_string(end.entries));
        }
        CentralEntry entry;
        entry.flags = readU16(archive, position + 8);
        entry.method = readU16(archive, position + 10);
        entry.crc = readU32(archive, position + 16);
        entry.compressedSize = readU32(archive, position + 20);
        entry.size = readU32(archive, position + 24);
        const std::size_t nameSize = readU16(archive, position + 28);
        const std::size_t extraSize = readU16(archive, position + 30);
        const std::size_t commentSize = readU16(archive, position + 32);
        const std::uint16_t startDisk = readU16(archive, position + 34);
        entry.localOffset = readU32(archive, position + 42);
        const std::size_t headerSize = kCentralHeaderSize + nameSize + extraSize + commentSize;
        if (directoryEnd - position < headerSize) {
            return malformed("the central directory header of " + partLabel(index) +
                             " runs past the end of the central directory");
        }
        entry.rawName = archive.substr(position + kCentralHeaderSize, nameSize);
        position += headerSize;

        if ((entry.flags & kFlagsUnsupported) != 0) {
            return unsupported(partLabel(index) + " is encrypted; encryption is not supported");
        }
        if (entry.compressedSize == kZip64Marker32 || entry.size == kZip64Marker32 ||
            entry.localOffset == kZip64Marker32 || startDisk == kZip64Marker16) {
            return unsupported(partLabel(index) + " needs ZIP64; ZIP64 is not supported");
        }
        if (startDisk != 0) {
            return unsupported(partLabel(index) +
                               " is on another disk; archives split across several disks "
                               "are not supported");
        }
        if (entry.method != kMethodStored && entry.method != kMethodDeflate) {
            return unsupported(partLabel(index) + " uses compression method " +
                               std::to_string(entry.method) +
                               "; only 0 (stored) and 8 (deflate) are supported");
        }
        Result<std::string> name = decodeName(entry.rawName, entry.flags, index);
        if (!name.ok()) {
            return name.error();
        }
        entry.name = std::move(name).value();
        if (std::optional<Error> error = checkName(entry.name, index)) {
            return *std::move(error);
        }
        if (!names.insert(entry.name).second) {
            return malformed(partLabel(index, entry.name) + " repeats the name of an earlier part");
        }
        if (entry.method == kMethodStored && entry.compressedSize != entry.size) {
            return malformed(partLabel(index, entry.name) + " is stored with " +
                             std::to_string(entry.compressedSize) + " bytes but declares " +
                             std::to_string(entry.size) + " uncompressed bytes");
        }
        if (entry.size > std::uint64_t{entry.compressedSize} * limits.maxRatio) {
            return tooLarge(partLabel(index, entry.name) + " declares " +
                            std::to_string(entry.size) + " bytes from " +
                            std::to_string(entry.compressedSize) +
                            " compressed bytes; the limit is a ratio of " +
                            std::to_string(limits.maxRatio) + " to 1");
        }
        totalSize += entry.size;
        if (totalSize > limits.maxTotalSize) {
            return tooLarge("the parts declare more than " + std::to_string(limits.maxTotalSize) +
                            " uncompressed bytes in all");
        }
        entries.push_back(std::move(entry));
    }
    if (position != directoryEnd) {
        return malformed("the central directory has " + std::to_string(directoryEnd - position) +
                         " bytes after its last entry");
    }
    return entries;
}

/// Checks the local header of `entry` against the central directory and
/// finds where its data starts. `dataLimit` is the start of the central
/// directory: every part lies before it.
std::optional<Error> readLocalHeader(std::string_view archive, std::size_t dataLimit,
                                     std::size_t index, CentralEntry& entry) {
    const std::string label = partLabel(index, entry.name);
    const std::size_t position = entry.localOffset;
    if (position > dataLimit || dataLimit - position < kLocalHeaderSize ||
        readU32(archive, position) != kLocalHeaderSignature) {
        return malformed("the local header of " + label + " is missing at offset " +
                         std::to_string(position));
    }
    const std::uint16_t flags = readU16(archive, position + 6);
    const std::uint16_t method = readU16(archive, position + 8);
    const std::uint32_t crc = readU32(archive, position + 14);
    const std::uint32_t compressedSize = readU32(archive, position + 18);
    const std::uint32_t size = readU32(archive, position + 22);
    const std::size_t nameSize = readU16(archive, position + 26);
    const std::size_t extraSize = readU16(archive, position + 28);
    const std::size_t headerSize = kLocalHeaderSize + nameSize + extraSize;
    if (dataLimit - position < headerSize) {
        return malformed("the local header of " + label + " runs into the central directory");
    }
    if ((flags & kFlagsUnsupported) != 0) {
        return unsupported(label + " is encrypted; encryption is not supported");
    }
    if (compressedSize == kZip64Marker32 || size == kZip64Marker32) {
        return unsupported(label + " needs ZIP64; ZIP64 is not supported");
    }
    const bool sameSizes =
        (flags & kFlagDataDescriptor) != 0 ||
        (crc == entry.crc && compressedSize == entry.compressedSize && size == entry.size);
    if (method != entry.method || !sameSizes ||
        archive.substr(position + kLocalHeaderSize, nameSize) != entry.rawName) {
        return malformed("the local header of " + label +
                         " does not match its central directory header");
    }
    entry.dataStart = position + headerSize;
    if (dataLimit - entry.dataStart < entry.compressedSize) {
        return malformed("the data of " + label + " runs into the central directory");
    }
    return std::nullopt;
}

/// Each part's header and data must not overlap another part's: overlapping
/// parts are a known way to make a small archive decompress to a huge size.
std::optional<Error> checkOverlaps(const std::vector<CentralEntry>& entries) {
    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    ranges.reserve(entries.size());
    for (const CentralEntry& entry : entries) {
        ranges.emplace_back(entry.localOffset, entry.dataStart + entry.compressedSize);
    }
    std::ranges::sort(ranges);
    for (std::size_t i = 1; i < ranges.size(); ++i) {
        if (ranges[i].first < ranges[i - 1].second) {
            return malformed("two parts overlap at offset " + std::to_string(ranges[i].first));
        }
    }
    return std::nullopt;
}

/// A raw inflate stream, ended on every path out of the function.
class Inflater {
public:
    Inflater() {
        const int status = inflateInit2(&m_stream, kRawDeflateWindowBits);
        if (status == Z_MEM_ERROR) {
            throw std::bad_alloc();
        }
        assert(status == Z_OK);
    }

    ~Inflater() { inflateEnd(&m_stream); }

    Inflater(const Inflater&) = delete;
    Inflater& operator=(const Inflater&) = delete;
    Inflater(Inflater&&) = delete;
    Inflater& operator=(Inflater&&) = delete;

    z_stream& stream() { return m_stream; }

private:
    z_stream m_stream{};
};

/// A raw deflate stream, ended on every path out of the function.
class Deflater {
public:
    Deflater() {
        const int status = deflateInit2(&m_stream, kDeflateLevel, Z_DEFLATED, kRawDeflateWindowBits,
                                        kDeflateMemLevel, Z_DEFAULT_STRATEGY);
        if (status == Z_MEM_ERROR) {
            throw std::bad_alloc();
        }
        assert(status == Z_OK);
    }

    ~Deflater() { deflateEnd(&m_stream); }

    Deflater(const Deflater&) = delete;
    Deflater& operator=(const Deflater&) = delete;
    Deflater(Deflater&&) = delete;
    Deflater& operator=(Deflater&&) = delete;

    z_stream& stream() { return m_stream; }

private:
    z_stream m_stream{};
};

/// Inflates `input` into exactly `size` bytes. Stops as soon as the data
/// gives one byte more than declared, so a lie costs nothing.
Result<std::string> inflatePart(std::string_view input, std::uint32_t size,
                                const std::string& label) {
    Inflater inflater;
    z_stream& stream = inflater.stream();
    std::string out(size, '\0');
    // Where the first byte past the declared size would go.
    std::array<Bytef, 1> beyond{};
    bool pastSize = false;
    stream.next_in = asBytes(input);
    stream.avail_in = static_cast<uInt>(input.size());
    stream.next_out = asBytes(out);
    stream.avail_out = static_cast<uInt>(out.size());
    while (true) {
        if (stream.avail_out == 0) {
            if (pastSize) {
                return malformed(label + " decompresses to more than its declared " +
                                 std::to_string(size) + " bytes");
            }
            pastSize = true;
            stream.next_out = beyond.data();
            stream.avail_out = static_cast<uInt>(beyond.size());
        }
        const int status = inflate(&stream, Z_NO_FLUSH);
        if (status == Z_STREAM_END) {
            break;
        }
        if (status == Z_MEM_ERROR) {
            throw std::bad_alloc();
        }
        if (status != Z_OK) {
            // Z_DATA_ERROR, or Z_BUF_ERROR: the input ended mid-stream.
            return malformed("the compressed data of " + label + " is damaged or truncated");
        }
    }
    if (stream.total_out != size) {
        return malformed(label + " decompresses to " + std::to_string(stream.total_out) +
                         " bytes; it declares " + std::to_string(size));
    }
    return out;
}

std::string deflatePart(std::string_view input) {
    assert(input.size() < kZip64Marker32);
    Deflater deflater;
    z_stream& stream = deflater.stream();
    std::string out(deflateBound(&stream, static_cast<uLong>(input.size())), '\0');
    stream.next_in = asBytes(input);
    stream.avail_in = static_cast<uInt>(input.size());
    stream.next_out = asBytes(out);
    stream.avail_out = static_cast<uInt>(out.size());
    // deflateBound() leaves room for everything: one call finishes.
    [[maybe_unused]] const int status = deflate(&stream, Z_FINISH);
    assert(status == Z_STREAM_END);
    out.resize(stream.total_out);
    return out;
}

/// The fields that local and central headers share, from "version needed
/// to extract" to "extra field length" (APPNOTE 4.3.7, 4.3.12).
struct HeaderFields {
    std::uint16_t version{0};
    std::uint16_t flags{0};
    std::uint16_t method{0};
    std::uint32_t crc{0};
    std::uint32_t compressedSize{0};
    std::uint32_t size{0};
    std::uint16_t nameSize{0};
};

void appendHeaderFields(std::string& out, const HeaderFields& fields) {
    appendU16(out, fields.version);
    appendU16(out, fields.flags);
    appendU16(out, fields.method);
    appendU16(out, kDosTime);
    appendU16(out, kDosDate);
    appendU32(out, fields.crc);
    appendU32(out, fields.compressedSize);
    appendU32(out, fields.size);
    appendU16(out, fields.nameSize);
    appendU16(out, 0);  // no extra field
}

}  // namespace

Result<std::vector<ZipPart>> readZip(std::string_view archive, const ZipLimits& limits) {
    const std::optional<EndRecord> end = findEndRecord(archive);
    if (!end) {
        return malformed("no end of central directory record; expected a ZIP archive");
    }
    if (end->position >= kZip64LocatorSize &&
        readU32(archive, end->position - kZip64LocatorSize) == kZip64LocatorSignature) {
        return unsupported("the archive has ZIP64 records; ZIP64 is not supported");
    }
    if (end->entries == kZip64Marker16 || end->entriesOnDisk == kZip64Marker16 ||
        end->directorySize == kZip64Marker32 || end->directoryOffset == kZip64Marker32) {
        return unsupported("the archive needs ZIP64; ZIP64 is not supported");
    }
    if (end->disk != 0 || end->directoryDisk != 0 || end->entriesOnDisk != end->entries) {
        return unsupported("the archive is split across several disks; not supported");
    }
    if (end->entries > limits.maxParts) {
        return tooLarge("the archive has " + std::to_string(end->entries) +
                        " parts; the limit is " + std::to_string(limits.maxParts));
    }
    if (std::uint64_t{end->directoryOffset} + end->directorySize > end->position) {
        return malformed("the central directory lies outside the archive");
    }

    Result<std::vector<CentralEntry>> directory = readCentralDirectory(archive, *end, limits);
    if (!directory.ok()) {
        return directory.error();
    }
    std::vector<CentralEntry> entries = std::move(directory).value();
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (std::optional<Error> error =
                readLocalHeader(archive, end->directoryOffset, index, entries[index])) {
            return *std::move(error);
        }
    }
    if (std::optional<Error> error = checkOverlaps(entries)) {
        return *std::move(error);
    }

    std::vector<ZipPart> parts;
    parts.reserve(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index) {
        CentralEntry& entry = entries[index];
        const std::string label = partLabel(index, entry.name);
        const std::string_view data = archive.substr(entry.dataStart, entry.compressedSize);
        ZipPart part{
            .name = std::move(entry.name), .bytes = {}, .stored = entry.method == kMethodStored};
        if (part.stored) {
            part.bytes = std::string{data};
        } else {
            Result<std::string> bytes = inflatePart(data, entry.size, label);
            if (!bytes.ok()) {
                return bytes.error();
            }
            part.bytes = std::move(bytes).value();
        }
        if (crc32Of(part.bytes) != entry.crc) {
            return malformed(label + " fails its CRC-32 check");
        }
        parts.push_back(std::move(part));
    }
    return parts;
}

std::string writeZip(std::span<const ZipPart> parts) {
    assert(parts.size() < kZip64Marker16);
    std::string archive;
    std::string directory;
    for (const ZipPart& part : parts) {
        assert(part.name.size() <= kZip64Marker16);
        assert(part.bytes.size() < kZip64Marker32);
        const std::string compressed = part.stored ? std::string{} : deflatePart(part.bytes);
        const std::string_view data = part.stored ? std::string_view{part.bytes} : compressed;
        const HeaderFields fields{
            .version = part.stored ? kVersionStored : kVersionDeflate,
            .flags = isAscii(part.name) ? std::uint16_t{0} : kFlagUtf8,
            .method = part.stored ? kMethodStored : kMethodDeflate,
            .crc = crc32Of(part.bytes),
            .compressedSize = static_cast<std::uint32_t>(data.size()),
            .size = static_cast<std::uint32_t>(part.bytes.size()),
            .nameSize = static_cast<std::uint16_t>(part.name.size()),
        };
        assert(archive.size() < kZip64Marker32);
        const auto offset = static_cast<std::uint32_t>(archive.size());

        appendU32(archive, kLocalHeaderSignature);
        appendHeaderFields(archive, fields);
        archive += part.name;
        archive += data;

        appendU32(directory, kCentralHeaderSignature);
        appendU16(directory, kVersionMadeBy);
        appendHeaderFields(directory, fields);
        appendU16(directory, 0);  // file comment length
        appendU16(directory, 0);  // disk number start
        appendU16(directory, 0);  // internal file attributes
        appendU32(directory, 0);  // external file attributes
        appendU32(directory, offset);
        directory += part.name;
    }
    assert(archive.size() + directory.size() < kZip64Marker32);
    const auto directoryOffset = static_cast<std::uint32_t>(archive.size());
    archive += directory;

    const auto count = static_cast<std::uint16_t>(parts.size());
    appendU32(archive, kEndRecordSignature);
    appendU16(archive, 0);  // number of this disk
    appendU16(archive, 0);  // disk with the central directory
    appendU16(archive, count);
    appendU16(archive, count);
    appendU32(archive, static_cast<std::uint32_t>(directory.size()));
    appendU32(archive, directoryOffset);
    appendU16(archive, 0);  // comment length
    return archive;
}

}  // namespace tg::io::detail
