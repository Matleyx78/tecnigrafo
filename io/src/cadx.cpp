// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "json.hpp"
#include "tg/io/io.hpp"
#include "zip.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::io {
namespace {

constexpr std::string_view kMimetype = "application/vnd.tecnigrafo.cadx";
constexpr std::size_t kLocalHeaderSize = 30;
constexpr std::size_t kCentralHeaderSize = 46;
constexpr std::size_t kEndSize = 22;

Error malformed(std::string message) {
    return {Error::Kind::Malformed, 0, std::move(message)};
}

// Used only after readZip has validated all headers and their bounds.
std::uint32_t integer(std::string_view bytes, std::size_t offset, std::size_t size) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < size; ++i) {
        value |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + i]))
                 << (8 * i);
    }
    return value;
}

bool validMimetype(std::string_view bytes, const std::vector<detail::ZipPart>& parts) {
    const auto found = std::ranges::find(parts, "mimetype", &detail::ZipPart::name);
    if (found == parts.end() || !found->stored || found->bytes != kMimetype ||
        bytes.size() < kLocalHeaderSize || bytes.substr(0, 4) != "PK\003\004" ||
        integer(bytes, 26, 2) != std::string_view{"mimetype"}.size() ||
        integer(bytes, 28, 2) != 0 || bytes.substr(kLocalHeaderSize, 8) != "mimetype") {
        return false;
    }
    // A comment may itself contain an end signature. Its declared length must
    // reach the end of the archive, as already checked by readZip.
    std::size_t end = bytes.size() - kEndSize;
    while (bytes.substr(end, 4) != "PK\005\006" ||
           integer(bytes, end + 20, 2) != bytes.size() - end - kEndSize) {
        if (end == 0) {
            return false;
        }
        --end;
    }
    std::size_t position = integer(bytes, end + 16, 4);
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (position > bytes.size() || bytes.size() - position < kCentralHeaderSize) {
            return false;
        }
        const auto nameSize = integer(bytes, position + 28, 2);
        const auto extraSize = integer(bytes, position + 30, 2);
        const auto commentSize = integer(bytes, position + 32, 2);
        if (bytes.substr(position + kCentralHeaderSize, nameSize) == "mimetype") {
            return extraSize == 0 && integer(bytes, position + 42, 4) == 0;
        }
        position += kCentralHeaderSize + nameSize + extraSize + commentSize;
    }
    return false;
}

Result<detail::Json> manifest(std::string_view bytes) {
    auto parsed = detail::readJson(bytes);
    if (!parsed.ok()) {
        return parsed.error();
    }
    const auto& root = parsed.value();
    const auto* format = root.find("format");
    if (root.kind != detail::Json::Kind::Object || format == nullptr ||
        format->kind != detail::Json::Kind::Number || format->text.empty() ||
        format->text.front() < '1' || format->text.front() > '9' ||
        format->text.find_first_not_of("0123456789") != std::string::npos) {
        return malformed("manifest.json: expected a positive integer format");
    }
    const auto supported = std::to_string(kCadxFormat);
    if (format->text.size() > supported.size() ||
        (format->text.size() == supported.size() && format->text > supported)) {
        return Error{Error::Kind::Unsupported, 0,
                     "written by a newer version of Tecnigrafo (format " + format->text +
                         "; this version reads up to " + supported + ")"};
    }
    const auto* generator = root.find("generator");
    const auto* parts = root.find("parts");
    if (generator == nullptr || generator->kind != detail::Json::Kind::String || parts == nullptr ||
        parts->kind != detail::Json::Kind::Array) {
        return malformed("manifest.json: expected generator string and parts array");
    }
    return parsed;
}

bool matchingParts(const detail::Json& root, const std::vector<detail::ZipPart>& parts) {
    std::set<std::string> expected;
    for (const auto& part : parts) {
        if (part.name != "manifest.json") {
            expected.insert(part.name);
        }
    }
    const auto* listed = root.find("parts");
    for (const auto& item : listed->elements) {
        if (item.kind != detail::Json::Kind::String || expected.erase(item.text) != 1) {
            return false;
        }
    }
    return expected.empty();
}

detail::Json textValue(std::string text, detail::Json::Kind kind = detail::Json::Kind::String) {
    detail::Json value;
    value.kind = kind;
    value.text = std::move(text);
    return value;
}

}  // namespace

Result<ReadResult> readCadx(std::string_view bytes) {
    try {
        auto archive = detail::readZip(bytes);
        if (!archive.ok()) {
            return archive.error();
        }
        const auto& parts = archive.value();
        if (!validMimetype(bytes, parts)) {
            return malformed("not a Tecnigrafo drawing (no valid mimetype)");
        }
        const auto header = std::ranges::find(parts, "manifest.json", &detail::ZipPart::name);
        if (header == parts.end()) {
            return malformed("manifest.json: missing required part");
        }
        auto parsed = manifest(header->bytes);
        if (!parsed.ok()) {
            return parsed.error();
        }
        if (!matchingParts(parsed.value(), parts)) {
            return malformed("manifest.json: parts do not match the archive");
        }
        const auto drawing = std::ranges::find(parts, "drawing.dxf", &detail::ZipPart::name);
        if (drawing == parts.end()) {
            return malformed("drawing.dxf: missing required part");
        }
        auto result = readDxf(drawing->bytes);
        if (!result.ok()) {
            auto error = result.error();
            error.message =
                "drawing.dxf: " +
                (error.line == 0 ? std::string{} : "line " + std::to_string(error.line) + ": ") +
                error.message;
            // The location is now relative to a named ZIP part, not the outer file.
            error.line = 0;
            return error;
        }
        if (result.value().document.raw().version != "AC1032") {
            return malformed("drawing.dxf: expected DXF R2018 (AC1032)");
        }
        auto raw = result.value().document.raw();
        for (const auto& part : parts) {
            if (part.name != "mimetype" && part.name != "drawing.dxf") {
                raw.nativeParts.push_back({part.name, part.bytes});
            }
        }
        result.value().document.setRaw(std::move(raw));
        return result;
    } catch (const std::exception&) {
        return Error{Error::Kind::TooLarge, 0, "unable to allocate native drawing data"};
    }
}

Result<std::string> writeCadx(const core::Document& document) {
    try {
        auto drawing = writeDxf(document, DxfVersion::R2018);
        if (!drawing.ok()) {
            return drawing.error();
        }
        detail::Json root;
        root.kind = detail::Json::Kind::Object;
        std::vector<detail::ZipPart> other;
        std::set<std::string> names{"mimetype", "drawing.dxf"};
        for (const auto& part : document.raw().nativeParts) {
            if (!names.insert(part.name).second) {
                return malformed("duplicate or reserved native part: " + part.name);
            }
            if (part.name == "manifest.json") {
                auto parsed = manifest(part.bytes);
                if (!parsed.ok()) {
                    return parsed.error();
                }
                root = std::move(parsed).value();
            } else {
                other.push_back({part.name, part.bytes, false});
            }
        }
        std::ranges::sort(other, {}, &detail::ZipPart::name);
        detail::Json listed;
        listed.kind = detail::Json::Kind::Array;
        listed.elements.push_back(textValue("mimetype"));
        listed.elements.push_back(textValue("drawing.dxf"));
        for (const auto& part : other) {
            listed.elements.push_back(textValue(part.name));
        }
        root.set("format", textValue(std::to_string(kCadxFormat), detail::Json::Kind::Number));
        root.set("generator", textValue("Tecnigrafo " TG_VERSION));
        root.set("parts", std::move(listed));
        auto header = detail::writeJson(root);
        if (!header.ok()) {
            return header.error();
        }
        std::vector<detail::ZipPart> parts;
        parts.push_back({"mimetype", std::string{kMimetype}, true});
        parts.push_back({"manifest.json", std::move(header).value(), false});
        parts.push_back({"drawing.dxf", std::move(drawing).value(), false});
        for (auto& part : other) {
            parts.push_back(std::move(part));
        }
        constexpr std::size_t kMaxParts = 256;
        constexpr std::size_t kMaxNameSize = 65535;
        constexpr std::size_t kMaxTotalSize = std::size_t{1} << 30;
        std::size_t total = 0;
        if (parts.size() > kMaxParts) {
            return Error{Error::Kind::TooLarge, 0, "too many native parts"};
        }
        for (const auto& part : parts) {
            if (part.name.size() > kMaxNameSize || part.bytes.size() > kMaxTotalSize - total) {
                return Error{Error::Kind::TooLarge, 0, "native parts exceed ZIP size limits"};
            }
            total += part.bytes.size();
        }
        auto bytes = detail::writeZip(parts);
        // Validate names and container limits even for NativeParts supplied by callers.
        const auto checked = detail::readZip(bytes);
        if (!checked.ok()) {
            return checked.error();
        }
        return bytes;
    } catch (const std::exception&) {
        return Error{Error::Kind::TooLarge, 0, "unable to allocate native drawing data"};
    }
}

}  // namespace tg::io
