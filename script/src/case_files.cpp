// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_files.hpp"

#include <algorithm>
#include <string>
#include <system_error>
#include <utility>

namespace tg::script {
namespace {

namespace fs = std::filesystem;

io::Error fileError(std::string message) {
    return io::Error{io::Error::Kind::File, 0, std::move(message)};
}

std::string inQuotes(std::string_view name) {
    return "\"" + std::string{name} + "\"";
}

/// A UTF-8 name as a path, through std::u8string: on Windows the narrow
/// encoding is not UTF-8.
fs::path toPath(std::string_view name) {
    return fs::path{std::u8string(name.begin(), name.end())};
}

std::string toUtf8(const fs::path& path) {
    const std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

bool isControl(char c) {
    const auto code = static_cast<unsigned char>(c);
    return code < 0x20 || code == 0x7F;
}

bool isAsciiLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/// A name relative to the output folder that cannot leave it.
bool isRelativeName(std::string_view name) {
    if (name.empty() || name.front() == '/' || name.front() == '@') {
        return false;
    }
    if (name.size() >= 2 && name[1] == ':' && isAsciiLetter(name[0])) {
        return false;  // a drive letter, "C:plan.dxf" included
    }
    if (std::ranges::any_of(name, [](char c) { return c == '\\' || isControl(c); })) {
        return false;
    }
    while (true) {
        const std::size_t slash = name.find('/');
        if (name.substr(0, slash) == "..") {
            return false;
        }
        if (slash == std::string_view::npos) {
            return true;
        }
        name.remove_prefix(slash + 1);
    }
}

/// The rule of the names of sorgenti.json (docs/verifica/casi.md).
bool isSourceName(std::string_view name) {
    return !name.empty() && std::ranges::all_of(name, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    });
}

/// `error` with the path on disk replaced by the name the script wrote:
/// io::DiskFiles quotes the path at the start of its messages.
io::Error renamed(io::Error error, const fs::path& path, std::string_view name) {
    const std::string disk = inQuotes(toUtf8(path));
    if (error.message.starts_with(disk)) {
        error.message.replace(0, disk.size(), inQuotes(name));
    }
    return error;
}

}  // namespace

CaseFiles::CaseFiles(fs::path output, std::optional<fs::path> sources)
    : m_output(std::move(output)), m_sources(std::move(sources)) {}

io::Result<fs::path> CaseFiles::resolve(std::string_view name, bool writing) const {
    if (!name.starts_with(kSourcesPrefix)) {
        if (!isRelativeName(name)) {
            return fileError(std::string{kInvalidFileName});
        }
        return m_output / toPath(name);
    }
    const std::string_view source = name.substr(kSourcesPrefix.size());
    if (!isSourceName(source)) {
        return fileError(std::string{kInvalidFileName});
    }
    if (writing) {
        return fileError(inQuotes(name) + ": the files of " + std::string{kSourcesPrefix} +
                         " are read only");
    }
    if (!m_sources) {
        return fileError(inQuotes(name) + ": no folder of public files was given (--sources)");
    }
    return *m_sources / toPath(source);
}

io::Result<std::string> CaseFiles::read(std::string_view name) {
    const io::Result<fs::path> path = resolve(name, false);
    if (!path.ok()) {
        return path.error();
    }
    io::DiskFiles disk;
    io::Result<std::string> bytes = disk.read(toUtf8(path.value()));
    if (!bytes.ok()) {
        return renamed(bytes.error(), path.value(), name);
    }
    return bytes;
}

std::optional<io::Error> CaseFiles::write(std::string_view name, std::string_view bytes) {
    const io::Result<fs::path> path = resolve(name, true);
    if (!path.ok()) {
        return path.error();
    }
    io::DiskFiles disk;
    if (std::optional<io::Error> error = disk.write(toUtf8(path.value()), bytes)) {
        return renamed(std::move(*error), path.value(), name);
    }
    return std::nullopt;
}

bool CaseFiles::exists(std::string_view name) const {
    const io::Result<fs::path> path = resolve(name, false);
    return path.ok() && io::DiskFiles{}.exists(toUtf8(path.value()));
}

std::optional<std::string> prepareCaseFiles(const fs::path& folder, const fs::path& files) {
    std::error_code error;
    fs::remove_all(files, error);
    if (error || !fs::create_directories(files, error)) {
        return "cannot prepare the folder " + files.generic_string();
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, error)) {
        const std::string name = entry.path().filename().generic_string();
        if (name == "script.tgs" || name == "domande.tgq" || name.starts_with("atteso")) {
            continue;
        }
        // The output may be inside the case folder (or the same folder).
        // Never copy a directory that contains the destination into itself.
        if (entry.is_directory(error)) {
            const fs::path relative = fs::relative(files, entry.path(), error);
            if (error) {
                return "cannot resolve the folder " + entry.path().generic_string();
            }
            if (!relative.empty() && *relative.begin() != "..") {
                continue;
            }
        }
        fs::copy(entry.path(), files / entry.path().filename(), fs::copy_options::recursive, error);
        if (error) {
            return "cannot copy " + name + " to " + files.generic_string();
        }
    }
    if (error) {
        return "cannot read the folder " + folder.generic_string();
    }
    return std::nullopt;
}

}  // namespace tg::script
