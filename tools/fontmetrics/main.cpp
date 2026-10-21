// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
// Offsets and field widths below are defined by the TrueType table formats.
class Table {
public:
    explicit Table(std::span<const unsigned char> bytes) : m_bytes(bytes) {}

    [[nodiscard]] std::uint32_t read(std::size_t offset, std::size_t count = 2) const {
        const auto bytes = slice(offset, count);
        std::uint32_t value = 0;
        for (const auto byte : bytes.m_bytes) {
            value = (value << 8U) | byte;
        }
        return value;
    }

    [[nodiscard]] int signedWord(std::size_t offset) const {
        const auto word = read(offset);
        return word < 0x8000U ? static_cast<int>(word) : static_cast<int>(word) - 0x10000;
    }

    [[nodiscard]] Table slice(std::size_t offset, std::size_t count) const {
        if (offset > m_bytes.size() || count > m_bytes.size() - offset) {
            throw std::runtime_error("truncated font table");
        }
        return Table{m_bytes.subspan(offset, count)};
    }

private:
    std::span<const unsigned char> m_bytes;
};

using CharacterMap = std::map<std::uint32_t, std::uint32_t>;

void readFormat4(Table table, CharacterMap& characters) {
    const std::size_t count = table.read(6) / 2;
    const std::size_t ends = 14;
    const std::size_t starts = ends + 2 * count + 2;
    const std::size_t deltas = starts + 2 * count;
    const std::size_t ranges = deltas + 2 * count;
    for (std::size_t i = 0; i < count; ++i) {
        const auto end = table.read(ends + 2 * i);
        const auto start = table.read(starts + 2 * i);
        const auto delta = table.read(deltas + 2 * i);
        const auto range = table.read(ranges + 2 * i);
        for (auto code = start; code <= end && code < 0xFFFFU; ++code) {
            auto glyph = range == 0 ? code
                                    : table.read(ranges + 2 * i + range +
                                                 2 * static_cast<std::size_t>(code - start));
            if (range == 0 || glyph != 0) {
                glyph = (glyph + delta) & 0xFFFFU;
            }
            if (glyph != 0) {
                characters[code] = glyph;
            }
        }
    }
}

void readFormat12(Table table, CharacterMap& characters) {
    const auto groups = table.read(12, 4);
    for (std::size_t i = 0; i < groups; ++i) {
        const auto start = table.read(16 + 12 * i, 4);
        const auto end = table.read(20 + 12 * i, 4);
        const auto first = table.read(24 + 12 * i, 4);
        if (start > end || end > 0x10FFFFU || first > 0xFFFFU || end - start > 0xFFFFU - first) {
            throw std::runtime_error("invalid cmap group");
        }
        for (auto code = start; code <= end; ++code) {
            const auto glyph = first + code - start;
            if (glyph != 0) {
                characters[code] = glyph;
            }
        }
    }
}

void generate(const char* inputPath, const char* outputPath) {
    std::ifstream input(inputPath, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open font");
    }
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>{input}, {}};
    const Table font{bytes};
    std::map<std::string, Table> tables;
    for (std::size_t i = 0; i < font.read(4); ++i) {
        const auto offset = 12 + 16 * i;
        std::string tag;
        for (std::size_t j = 0; j < 4; ++j) {
            tag += static_cast<char>(font.read(offset + j, 1));
        }
        tables.emplace(tag, font.slice(font.read(offset + 8, 4), font.read(offset + 12, 4)));
    }
    const auto head = tables.at("head");
    const auto hhea = tables.at("hhea");
    const auto os2 = tables.at("OS/2");
    const auto cmap = tables.at("cmap");
    const auto hmtx = tables.at("hmtx");
    const auto capHeight = os2.signedWord(88);
    const auto metricsCount = hhea.read(34);
    if (os2.read(0) < 2 || capHeight <= 0 || metricsCount == 0 || head.read(18) == 0) {
        throw std::runtime_error("unsupported font metrics");
    }
    CharacterMap characters;
    // A full Unicode subtable takes precedence over the BMP table.
    for (const auto format : {4U, 12U}) {
        for (std::size_t i = 0; i < cmap.read(2); ++i) {
            const auto record = 4 + 8 * i;
            const auto platform = cmap.read(record);
            const auto encoding = cmap.read(record + 2);
            if (platform != 0 && (platform != 3 || (encoding != 1 && encoding != 10))) {
                continue;
            }
            const auto offset = cmap.read(record + 4, 4);
            if (cmap.read(offset) != format) {
                continue;
            }
            const auto length = format == 4 ? cmap.read(offset + 2) : cmap.read(offset + 4, 4);
            const auto table = cmap.slice(offset, length);
            if (format == 4) {
                readFormat4(table, characters);
            } else {
                readFormat12(table, characters);
            }
        }
    }
    if (characters.empty()) {
        throw std::runtime_error("no supported Unicode cmap");
    }
    std::ofstream output(outputPath, std::ios::binary);
    output << "// Generated from the bundled font. Do not edit.\n"
           << "constexpr double kUnitsPerEm = " << head.read(18) << ";\n"
           << "static_assert(kUnitsPerEm > 0);\nconstexpr double kCapHeight = " << capHeight
           << ";\n"
           << "constexpr FontMetrics kMetrics{" << hhea.signedWord(4) << " / kCapHeight, "
           << hhea.signedWord(6) << " / kCapHeight};\n"
           << "constexpr double kMissingAdvance = " << hmtx.read(0) << " / kCapHeight;\n"
           << "constexpr std::array<CharacterWidth, " << characters.size() << "> kWidths{{\n";
    for (const auto& [code, glyph] : characters) {
        // Glyphs after numberOfHMetrics share the final advance.
        const std::size_t metric = glyph < metricsCount ? glyph : metricsCount - 1;
        output << "    CharacterWidth{" << code << ", " << hmtx.read(4 * metric)
               << " / kCapHeight},\n";
    }
    output << "}};\n";
    output.close();
    if (!output) {
        throw std::runtime_error("cannot write metrics");
    }
}
}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: fontmetrics font.ttf output.inc\n";
        return 1;
    }
    try {
        generate(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "fontmetrics: " << error.what() << '\n';
        return 1;
    }
}
