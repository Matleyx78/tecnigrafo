// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <filesystem>
#include <optional>
#include <ostream>

/// The commands of tecnigrafo-script (docs/verifica/casi.md). They live in
/// the library so that the tests drive them without starting a process.
namespace tg::script {

/// Exit codes of tecnigrafo-script.
inline constexpr int kExitPassed = 0;
inline constexpr int kExitFailed = 1;
/// A malformed file, a missing file or wrong arguments: never a valid
/// case, not even a pending one.
inline constexpr int kExitInvalid = 2;
/// The case needs level 2: ctest counts it as skipped (SKIP_RETURN_CODE).
inline constexpr int kExitNeedsLevel2 = 77;

/// Name of the obtained report that `check` writes, next to atteso.txt.
inline constexpr const char* kObtainedReport = "ottenuto.txt";
/// Name of the question file with the obtained answers that `check` writes.
inline constexpr const char* kObtainedAnswers = "ottenuto.tgq";

/// `tecnigrafo-script run <script.tgs> [--report <file>] [--sources <folder>]`:
/// runs the script at level 1 and writes the report to `report`, or to
/// `out` without one. Errors go to `err`. The script reads and writes files
/// in the current folder, and reads "@sources/<name>" in `sources`
/// (docs/verifica/script.md, "File"). Returns kExitPassed, kExitInvalid, or
/// kExitNeedsLevel2 for a script that says "!level 2".
[[nodiscard]] int runScriptFile(const std::filesystem::path& script,
                                const std::optional<std::filesystem::path>& report,
                                std::ostream& out, std::ostream& err,
                                const std::optional<std::filesystem::path>& sources = std::nullopt);

struct CheckOptions {
    /// The case is listed in tests/verifica/in-attesa.txt: a case not
    /// passed exits with kExitPassed, with a warning, and a case passed with
    /// kExitFailed, asking to remove it from the list.
    bool pending{false};
    /// Where the obtained report or answers are written; created if needed.
    /// A script reads and writes its files in the subfolder "file"
    /// (kCaseFilesFolder), prepared by prepareCaseFiles() (case_files.hpp).
    std::filesystem::path output{"."};
};

/// `tecnigrafo-script check <case folder> [--pending] [--output <folder>]
/// [--sources <folder>]`: runs the case (script.tgs with atteso.txt, or
/// domande.tgq), compares, writes the differences to `out` and the
/// obtained file to the output folder. The questions read "file=<name>"
/// in the case folder. `sources` is the folder of the pinned public files
/// of tests/verifica/sorgenti.json, read as "@sources/<name>" and
/// "source=<name>"; without it they are an error. Returns kExitPassed, kExitFailed, kExitInvalid or
/// kExitNeedsLevel2.
[[nodiscard]] int checkCase(const std::filesystem::path& folder, const CheckOptions& options,
                            std::ostream& out,
                            const std::optional<std::filesystem::path>& sources = std::nullopt);

}  // namespace tg::script
