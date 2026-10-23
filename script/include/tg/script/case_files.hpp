// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/file_access.hpp"
#include "tg/io/result.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

/// The files of the control mode (docs/adr/0021, point 4;
/// docs/verifica/script.md, "File").
namespace tg::script {

/// The prefix of the pinned public files of tests/verifica/sorgenti.json.
inline constexpr std::string_view kSourcesPrefix = "@sources/";

/// The message of every name CaseFiles rejects (docs/adr/0021, point 4).
inline constexpr std::string_view kInvalidFileName = "Invalid file name.";

/// The folder, inside the output folder of a case, where its script reads
/// and writes files (docs/verifica/casi.md).
inline constexpr std::string_view kCaseFilesFolder = "file";

/// io::FileAccess for scripts: relative names inside `output` (read and
/// write), "@sources/<name>" inside `sources` (read only), anything else
/// rejected with "Invalid file name.".
///
/// A name is rejected if it is empty or absolute, starts with a drive
/// letter or with '@' (other than "@sources/"), has ".." as a component,
/// contains '\' or a control character. The <name> of "@sources/" follows
/// the rule of the names of sorgenti.json: lowercase letters, digits and
/// '-'. Errors are of kind Error::Kind::File and name the file as the
/// script wrote it, never the folders: reports do not depend on where the
/// case runs.
class CaseFiles final : public io::FileAccess {
public:
    CaseFiles(std::filesystem::path output, std::optional<std::filesystem::path> sources);

    [[nodiscard]] io::Result<std::string> read(std::string_view name) override;
    [[nodiscard]] std::optional<io::Error> write(std::string_view name,
                                                 std::string_view bytes) override;
    [[nodiscard]] bool exists(std::string_view name) const override;

private:
    /// The path of `name` on disk, or the error that rejects it.
    [[nodiscard]] io::Result<std::filesystem::path> resolve(std::string_view name,
                                                            bool writing) const;

    std::filesystem::path m_output;
    std::optional<std::filesystem::path> m_sources;
};

/// Empties `files` (created if needed) and copies into it every file and
/// folder of the case folder `folder` except script.tgs, domande.tgq and
/// the expected results (atteso*): the hand-written files a script opens.
/// The script then works on copies, so it never changes the repository,
/// and a file saved by a previous run never changes the next one. Returns
/// an error message, or std::nullopt.
[[nodiscard]] std::optional<std::string> prepareCaseFiles(const std::filesystem::path& folder,
                                                          const std::filesystem::path& files);

}  // namespace tg::script
