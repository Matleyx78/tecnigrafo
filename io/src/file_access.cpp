// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/file_access.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace tg::io {
namespace {

namespace fs = std::filesystem;

/// Added to the name of the file being replaced, for the temporary copy
/// written next to it.
constexpr std::string_view kTemporarySuffix = ".tecnigrafo-tmp";

/// A UTF-8 name as a path: through std::u8string, so that it is not read in
/// the system's narrow encoding (which on Windows is not UTF-8).
fs::path toPath(std::string_view name) {
    std::u8string utf8;
    utf8.reserve(name.size());
    for (const char c : name) {
        utf8.push_back(static_cast<char8_t>(c));
    }
    return fs::path{utf8};
}

Error fileError(std::string_view name, std::string_view what) {
    return Error{Error::Kind::File, 0, "\"" + std::string{name} + "\": " + std::string{what}};
}

/// True if `bytes` were written completely to the new file `path`.
bool writeWhole(const fs::path& path, std::string_view bytes) {
    std::ofstream stream{path, std::ios::binary | std::ios::trunc};
    if (!stream) {
        return false;
    }
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    return !stream.fail();
}

}  // namespace

// The file system can throw (a name the system cannot represent, memory):
// no exception may leave the module (docs/adr/0017, point 6), so each
// operation catches them and reports an error.

Result<std::string> DiskFiles::read(std::string_view name) {
    try {
        const fs::path path = toPath(name);
        std::error_code error;
        if (fs::is_directory(path, error)) {
            return fileError(name, "is a folder, expected a file");
        }
        std::ifstream stream{path, std::ios::binary};
        if (!stream) {
            return fileError(
                name, fs::exists(path, error) ? "cannot be opened for reading" : "does not exist");
        }
        std::string bytes{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
        if (stream.bad()) {
            return fileError(name, "cannot be read");
        }
        return bytes;
    } catch (const std::exception& exception) {
        return fileError(name, std::string{"cannot be read: "} + exception.what());
    }
}

std::optional<Error> DiskFiles::write(std::string_view name, std::string_view bytes) {
    try {
        const fs::path path = toPath(name);
        fs::path temporary = path;
        temporary += toPath(kTemporarySuffix);
        std::error_code error;
        if (!writeWhole(temporary, bytes)) {
            static_cast<void>(
                fs::remove(temporary, error));  // best effort: the error is reported anyway
            return fileError(name, "cannot be written");
        }
        // rename() replaces the old file in one step, or leaves it as it was.
        fs::rename(temporary, path, error);
        if (error) {
            std::error_code ignored;
            static_cast<void>(
                fs::remove(temporary, ignored));  // best effort: the error is reported anyway
            return fileError(name, "cannot be replaced by the new content");
        }
        return std::nullopt;
    } catch (const std::exception& exception) {
        return fileError(name, std::string{"cannot be written: "} + exception.what());
    }
}

bool DiskFiles::exists(std::string_view name) const {
    try {
        std::error_code error;
        return fs::exists(toPath(name), error);
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace tg::io
