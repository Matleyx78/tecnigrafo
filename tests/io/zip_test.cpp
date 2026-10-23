// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "zip.hpp"

#include "tg/io/result.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace tg::io {
namespace {

using detail::readZip;
using detail::writeZip;
using detail::ZipLimits;
using detail::ZipPart;

/// The contents that tests/io/dati/zip/crea.sh puts in its archives.
std::string repeated(std::string_view line, int times) {
    std::string text;
    for (int i = 0; i < times; ++i) {
        text += line;
    }
    return text;
}

const std::string kText = repeated("Tecnigrafo: archivio di prova.\n", 40);
const std::string kUnicodeText = repeated("Città, perché, €.\n", 20);
constexpr std::string_view kMimetype = "application/x-tecnigrafo";
constexpr std::size_t kMebibyte = std::size_t{1024} * 1024;

/// An archive of tests/io/dati/zip, made by crea.sh with other tools.
std::string archiveFile(std::string_view name) {
    const std::string path = std::string{TG_IO_TEST_DATA} + "/zip/" + std::string{name};
    std::ifstream file{path, std::ios::binary};
    EXPECT_TRUE(file.good()) << path;
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

struct Expected {
    std::string_view name;
    std::string_view bytes;
    bool stored;
};

void expectParts(const Result<std::vector<ZipPart>>& result,
                 const std::vector<Expected>& expected) {
    ASSERT_TRUE(result.ok()) << result.error().message;
    const std::vector<ZipPart>& parts = result.value();
    ASSERT_EQ(parts.size(), expected.size());
    for (std::size_t i = 0; i < parts.size(); ++i) {
        EXPECT_EQ(parts[i].name, expected[i].name) << "part " << i;
        EXPECT_EQ(parts[i].bytes, expected[i].bytes) << "part " << i;
        EXPECT_EQ(parts[i].stored, expected[i].stored) << "part " << i;
    }
}

void expectError(const Result<std::vector<ZipPart>>& result, Error::Kind kind,
                 std::string_view fragment) {
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, kind) << result.error().message;
    EXPECT_EQ(result.error().line, 0U);
    EXPECT_NE(result.error().message.find(fragment), std::string::npos) << result.error().message;
}

// Little-endian fields of an archive, to damage it on purpose.
std::uint32_t getU32(const std::string& bytes, std::size_t position) {
    std::uint32_t value = 0;
    for (std::size_t i = 4; i-- > 0;) {
        value = (value << 8U) | static_cast<unsigned char>(bytes.at(position + i));
    }
    return value;
}

void setU16(std::string& bytes, std::size_t position, std::uint16_t value) {
    bytes.at(position) = static_cast<char>(value & 0xFFU);
    bytes.at(position + 1) = static_cast<char>(value >> 8U);
}

void setU32(std::string& bytes, std::size_t position, std::uint32_t value) {
    setU16(bytes, position, static_cast<std::uint16_t>(value & 0xFFFFU));
    setU16(bytes, position + 2, static_cast<std::uint16_t>(value >> 16U));
}

// Offsets in the records (PKWARE APPNOTE 4.3.7, 4.3.12, 4.3.16).
constexpr std::size_t kEndRecordSize = 22;
constexpr std::size_t kCentralHeaderSize = 46;
constexpr std::size_t kLocalFlags = 6;
constexpr std::size_t kLocalMethod = 8;
constexpr std::size_t kLocalCrc = 14;
constexpr std::size_t kLocalSize = 22;
constexpr std::size_t kCentralFlags = 8;
constexpr std::size_t kCentralMethod = 10;
constexpr std::size_t kCentralCrc = 16;
constexpr std::size_t kCentralSize = 24;
constexpr std::size_t kCentralLocalOffset = 42;

/// Where the central directory of a writeZip() archive starts.
std::size_t centralDirectory(const std::string& archive) {
    return getU32(archive, archive.size() - kEndRecordSize + 16);
}

/// Sets a 16-bit field of the first part in both its headers.
void setFirstPartU16(std::string& archive, std::size_t local, std::size_t central,
                     std::uint16_t value) {
    const std::size_t directory = centralDirectory(archive);
    setU16(archive, local, value);
    setU16(archive, directory + central, value);
}

void setFirstPartU32(std::string& archive, std::size_t local, std::size_t central,
                     std::uint32_t value) {
    const std::size_t directory = centralDirectory(archive);
    setU32(archive, local, value);
    setU32(archive, directory + central, value);
}

/// The default limits with one of them changed.
template <typename T>
ZipLimits limitsWith(T ZipLimits::*limit, std::type_identity_t<T> value) {
    ZipLimits limits;
    limits.*limit = value;
    return limits;
}

std::string oneDeflatedPart() {
    const std::vector<ZipPart> parts{{.name = "testo.txt", .bytes = kText, .stored = false}};
    return writeZip(parts);
}

// --- Archives made by other tools ---

TEST(ZipRead, InfoZipDeflate) {
    expectParts(readZip(archiveFile("zip-deflate.zip")),
                {
                    {"testo.txt", kText, false},
                    {"vuoto.txt", "", true},
                    {"cartella/", "", true},
                    {"cartella/dati.txt", kUnicodeText, false},
                });
}

TEST(ZipRead, InfoZipStored) {
    expectParts(readZip(archiveFile("zip-stored.zip")),
                {
                    {"testo.txt", kText, true},
                    {"vuoto.txt", "", true},
                    {"cartella/", "", true},
                    {"cartella/dati.txt", kUnicodeText, true},
                });
}

TEST(ZipRead, InfoZipDataDescriptors) {
    const std::string archive = archiveFile("zip-descriptor.zip");
    // Bit 3: sizes and CRC-32 follow the data, the local header has zeros.
    ASSERT_GE(archive.size(), 8U);
    EXPECT_NE(static_cast<unsigned char>(archive[kLocalFlags]) & 0x08U, 0U);
    expectParts(readZip(archive), {
                                      {"testo.txt", kText, false},
                                      {"vuoto.txt", "", true},
                                  });
}

TEST(ZipRead, PythonZipfile) {
    // Bit 11 on the non-ASCII name, a part comment and an archive comment.
    expectParts(readZip(archiveFile("python.zip")), {
                                                        {"mimetype", kMimetype, true},
                                                        {"testo.txt", kText, false},
                                                        {"vuoto.txt", "", false},
                                                        {"cartella/città.txt", kUnicodeText, false},
                                                    });
}

TEST(ZipRead, PythonZipfileDataDescriptors) {
    expectParts(readZip(archiveFile("python-descriptor.zip")),
                {
                    {"mimetype", kMimetype, true},
                    {"testo.txt", kText, false},
                    {"vuoto.txt", "", false},
                    {"cartella/città.txt", kUnicodeText, false},
                });
}

TEST(ZipRead, PythonEmptyArchive) {
    expectParts(readZip(archiveFile("python-empty.zip")), {});
}

TEST(ZipRead, RefusesInfoZipNonAsciiNameWithoutBit11) {
    expectError(readZip(archiveFile("zip-nome-non-marcato.zip")), Error::Kind::Malformed, "bit 11");
}

TEST(ZipRead, RefusesInfoZipEncryption) {
    expectError(readZip(archiveFile("zip-encrypted.zip")), Error::Kind::Unsupported, "encrypt");
}

TEST(ZipRead, RefusesInfoZipZip64) {
    expectError(readZip(archiveFile("zip-zip64.zip")), Error::Kind::Unsupported, "ZIP64");
}

TEST(ZipRead, RefusesInfoZipBzip2) {
    expectError(readZip(archiveFile("zip-bzip2.zip")), Error::Kind::Unsupported, "method 12");
}

// --- Writing ---

TEST(ZipWrite, RoundTripsRandomParts) {
    // A fixed seed makes failures reproducible; this is not security-sensitive.
    // NOLINTNEXTLINE(cert-msc32-c,cert-msc51-cpp)
    std::mt19937 generator{0x5A1F00DU};
    std::uniform_int_distribution<int> byte{0, 255};
    const auto randomBytes = [&](std::size_t size) {
        std::string bytes(size, '\0');
        for (char& c : bytes) {
            c = static_cast<char>(byte(generator));
        }
        return bytes;
    };
    // Several MB of text with some variety: compresses, but not to nothing.
    std::string large;
    while (large.size() < 3 * kMebibyte) {
        large += "line " + std::to_string(byte(generator)) + " of a large part\n";
    }

    const std::vector<ZipPart> parts{
        {.name = "mimetype", .bytes = std::string{kMimetype}, .stored = true},
        {.name = "empty-stored", .bytes = "", .stored = true},
        {.name = "empty-deflated", .bytes = "", .stored = false},
        {.name = "random.bin", .bytes = randomBytes(10000), .stored = false},
        {.name = "random-stored.bin", .bytes = randomBytes(4096), .stored = true},
        {.name = "large/text.txt", .bytes = large, .stored = false},
        {.name = "large/random.bin", .bytes = randomBytes(2 * kMebibyte), .stored = false},
        {.name = "cartella/città €.txt", .bytes = kUnicodeText, .stored = false},
    };
    const std::string archive = writeZip(parts);
    EXPECT_LT(archive.size(), large.size() + 3 * kMebibyte);

    const Result<std::vector<ZipPart>> read = readZip(archive);
    ASSERT_TRUE(read.ok()) << read.error().message;
    ASSERT_EQ(read.value().size(), parts.size());
    for (std::size_t i = 0; i < parts.size(); ++i) {
        EXPECT_EQ(read.value()[i].name, parts[i].name);
        EXPECT_EQ(read.value()[i].bytes, parts[i].bytes) << parts[i].name;
        EXPECT_EQ(read.value()[i].stored, parts[i].stored) << parts[i].name;
    }
}

TEST(ZipWrite, SamePartsGiveSameBytes) {
    const std::vector<ZipPart> parts{
        {.name = "mimetype", .bytes = std::string{kMimetype}, .stored = true},
        {.name = "testo.txt", .bytes = kText, .stored = false},
        {.name = "cartella/città.txt", .bytes = kUnicodeText, .stored = false},
    };
    EXPECT_EQ(writeZip(parts), writeZip(parts));
}

TEST(ZipWrite, EmptyArchiveIsOnlyTheEndRecord) {
    const std::string archive = writeZip({});
    EXPECT_EQ(archive.size(), kEndRecordSize);
    expectParts(readZip(archive), {});
}

TEST(ZipWrite, FirstStoredPartIsPlainAfterItsHeader) {
    // docs/adr/0006: "mimetype" first, stored, no extra field, so that its
    // name and content sit at fixed offsets.
    const std::vector<ZipPart> parts{
        {.name = "mimetype", .bytes = std::string{kMimetype}, .stored = true},
        {.name = "testo.txt", .bytes = kText, .stored = false},
    };
    const std::string archive = writeZip(parts);
    EXPECT_EQ(archive.substr(0, 4), std::string_view("PK\x03\x04", 4));
    EXPECT_EQ(archive[kLocalMethod], 0);                                   // stored
    EXPECT_EQ(archive.substr(10, 4), std::string("\x00\x00\x21\x00", 4));  // 1980-01-01 00:00
    EXPECT_EQ(archive.substr(28, 2), std::string("\x00\x00", 2));          // no extra field
    EXPECT_EQ(archive.substr(30, 8), "mimetype");
    EXPECT_EQ(archive.substr(38, kMimetype.size()), kMimetype);
}

TEST(ZipWrite, MarksNonAsciiNamesAsUtf8) {
    const std::vector<ZipPart> parts{
        {.name = "ascii.txt", .bytes = "a", .stored = false},
        {.name = "città.txt", .bytes = "b", .stored = false},
    };
    const std::string archive = writeZip(parts);
    const std::size_t directory = centralDirectory(archive);
    const std::size_t second =
        directory + kCentralHeaderSize + std::string_view{"ascii.txt"}.size();
    EXPECT_EQ(archive[directory + kCentralFlags + 1] & 0x08, 0);
    EXPECT_EQ(archive[second + kCentralFlags + 1] & 0x08, 0x08);  // bit 11
}

// --- Refusals ---

TEST(ZipRefuse, Encrypted) {
    std::string archive = oneDeflatedPart();
    setFirstPartU16(archive, kLocalFlags, kCentralFlags, 0x0001);
    expectError(readZip(archive), Error::Kind::Unsupported, "encrypt");
}

TEST(ZipRefuse, Zip64Markers) {
    std::string archive = oneDeflatedPart();
    setFirstPartU32(archive, kLocalSize, kCentralSize, 0xFFFFFFFFU);
    expectError(readZip(archive), Error::Kind::Unsupported, "ZIP64");

    archive = oneDeflatedPart();
    setU16(archive, archive.size() - kEndRecordSize + 10, 0xFFFF);  // total entries
    expectError(readZip(archive), Error::Kind::Unsupported, "ZIP64");
}

TEST(ZipRefuse, SeveralDisks) {
    std::string archive = oneDeflatedPart();
    setU16(archive, archive.size() - kEndRecordSize + 4, 1);  // number of this disk
    expectError(readZip(archive), Error::Kind::Unsupported, "disk");
}

TEST(ZipRefuse, Method12) {
    std::string archive = oneDeflatedPart();
    setFirstPartU16(archive, kLocalMethod, kCentralMethod, 12);
    expectError(readZip(archive), Error::Kind::Unsupported, "method 12");
}

TEST(ZipRefuse, UnsafeNames) {
    for (const std::string_view name : {"../evil.txt", "dir/../../evil.txt", "dir/..",
                                        "/etc/evil.txt", "C:/evil.txt", "dir\\evil.txt", ""}) {
        const std::vector<ZipPart> parts{
            {.name = std::string{name}, .bytes = "x", .stored = false}};
        const Result<std::vector<ZipPart>> read = readZip(writeZip(parts));
        ASSERT_FALSE(read.ok()) << name;
        EXPECT_EQ(read.error().kind, Error::Kind::Malformed) << name;
    }
    // ".." inside a component is just a name.
    const std::vector<ZipPart> parts{{.name = "dir/a..b/..c", .bytes = "x", .stored = false}};
    EXPECT_TRUE(readZip(writeZip(parts)).ok());
}

TEST(ZipRefuse, NameNotValidUtf8) {
    const std::vector<ZipPart> parts{{.name = "bad\xC3(.txt", .bytes = "x", .stored = false}};
    expectError(readZip(writeZip(parts)), Error::Kind::Malformed, "UTF-8");
}

TEST(ZipRefuse, DuplicateName) {
    const std::vector<ZipPart> parts{
        {.name = "same.txt", .bytes = "one", .stored = false},
        {.name = "same.txt", .bytes = "two", .stored = false},
    };
    expectError(readZip(writeZip(parts)), Error::Kind::Malformed, "repeats");
}

TEST(ZipRefuse, WrongCrc) {
    std::string archive = oneDeflatedPart();
    const std::uint32_t crc = getU32(archive, kLocalCrc);
    setFirstPartU32(archive, kLocalCrc, kCentralCrc, crc ^ 1U);
    expectError(readZip(archive), Error::Kind::Malformed, "CRC-32");
}

TEST(ZipRefuse, LocalHeaderDifferentFromCentralDirectory) {
    std::string archive = oneDeflatedPart();
    const std::uint32_t crc = getU32(archive, kLocalCrc);
    setU32(archive, kLocalCrc, crc ^ 1U);  // only the local header
    expectError(readZip(archive), Error::Kind::Malformed, "does not match");
}

TEST(ZipRefuse, FalseDeclaredSize) {
    for (const std::uint32_t size : {static_cast<std::uint32_t>(kText.size() - 1),
                                     static_cast<std::uint32_t>(kText.size() + 1)}) {
        std::string archive = oneDeflatedPart();
        setFirstPartU32(archive, kLocalSize, kCentralSize, size);
        expectError(readZip(archive), Error::Kind::Malformed, "declare");
    }
    // A stored part must declare as many bytes as it has.
    const std::vector<ZipPart> stored{{.name = "a.txt", .bytes = "abc", .stored = true}};
    std::string archive = writeZip(stored);
    setFirstPartU32(archive, kLocalSize, kCentralSize, 4);
    expectError(readZip(archive), Error::Kind::Malformed, "declares");
}

TEST(ZipRefuse, Ratio2000To1BeforeDecompressing) {
    std::string archive = oneDeflatedPart();
    const std::uint32_t compressed = getU32(archive, 18);
    setFirstPartU32(archive, kLocalSize, kCentralSize, compressed * 2000U);
    // TooLarge, not Malformed: refused before inflating a single byte.
    expectError(readZip(archive), Error::Kind::TooLarge, "ratio");

    // The limit is the caller's.
    expectError(readZip(oneDeflatedPart(), limitsWith(&ZipLimits::maxRatio, 10)),
                Error::Kind::TooLarge, "ratio");
}

TEST(ZipRefuse, TotalSize) {
    expectError(readZip(oneDeflatedPart(), limitsWith(&ZipLimits::maxTotalSize, kText.size() - 1)),
                Error::Kind::TooLarge, "in all");
    EXPECT_TRUE(
        readZip(oneDeflatedPart(), limitsWith(&ZipLimits::maxTotalSize, kText.size())).ok());
}

TEST(ZipRefuse, MoreThan256Parts) {
    std::vector<ZipPart> parts;
    parts.reserve(257);
    for (int i = 0; i < 256; ++i) {
        parts.push_back({.name = std::to_string(i), .bytes = "", .stored = true});
    }
    EXPECT_TRUE(readZip(writeZip(parts)).ok());
    parts.push_back({.name = "256", .bytes = "", .stored = true});
    expectError(readZip(writeZip(parts)), Error::Kind::TooLarge, "257 parts");
}

TEST(ZipRefuse, OverlappingParts) {
    // Part "b.txt" again inside the stored data of "a.txt": the central
    // directory entry of b points there, a classic of zip bombs.
    const std::vector<ZipPart> single{{.name = "b.txt", .bytes = "bbbb", .stored = true}};
    const std::string localB = writeZip(single).substr(0, 30 + 5 + 4);
    const std::vector<ZipPart> parts{
        {.name = "a.txt", .bytes = localB, .stored = true},
        single.front(),
    };
    std::string archive = writeZip(parts);
    const std::size_t secondEntry = centralDirectory(archive) + kCentralHeaderSize + 5;
    setU32(archive, secondEntry + kCentralLocalOffset, 30 + 5);
    expectError(readZip(archive), Error::Kind::Malformed, "overlap");
}

TEST(ZipRefuse, TruncatedAtEveryLength) {
    const std::vector<ZipPart> parts{
        {.name = "mimetype", .bytes = std::string{kMimetype}, .stored = true},
        {.name = "testo.txt", .bytes = kText, .stored = false},
    };
    const std::string archive = writeZip(parts);
    for (std::size_t size = 0; size < archive.size(); ++size) {
        const Result<std::vector<ZipPart>> read =
            readZip(std::string_view{archive}.substr(0, size));
        ASSERT_FALSE(read.ok()) << size;
        EXPECT_EQ(read.error().kind, Error::Kind::Malformed) << size;
    }
}

TEST(ZipRefuse, DamagedDeflateData) {
    std::string archive = oneDeflatedPart();
    const std::size_t data = 30 + std::string_view{"testo.txt"}.size();
    archive.at(data) = static_cast<char>(0xFF);  // block type 3 is reserved (RFC 1951)
    expectError(readZip(archive), Error::Kind::Malformed, "damaged");
}

TEST(ZipRefuse, NotAnArchive) {
    expectError(readZip(""), Error::Kind::Malformed, "end of central directory");
    expectError(readZip(kText), Error::Kind::Malformed, "end of central directory");
}

}  // namespace
}  // namespace tg::io
