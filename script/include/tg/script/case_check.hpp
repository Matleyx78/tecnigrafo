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
/// Name of the report obtained up to the frontier of a pending case that
/// `check` writes, next to ottenuto.txt (docs/adr/0037, point 6).
inline constexpr const char* kObtainedFrontierReport = "ottenuto-confine.txt";
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
    /// The checks of docs/adr/0037 run; false only for a pending case
    /// marked "esente" in tests/verifica/in-attesa.txt (point 9). For a
    /// pending script of level 1 that does not pass, they include the run
    /// up to its frontier: a difference before it, or no frontier at all,
    /// exits with kExitFailed even when pending (points 4-6).
    bool earlyChecks{true};
    /// Where the obtained report or answers are written; created if needed.
    /// A script reads and writes its files in the subfolder "file"
    /// (kCaseFilesFolder), prepared by prepareCaseFiles() (case_files.hpp).
    std::filesystem::path output{"."};
};

/// `tecnigrafo-script check <case folder> [--pending [--no-early-checks]]
/// [--output <folder>] [--sources <folder>]`: with `options.earlyChecks`,
/// checks the form of the case first (case_form.hpp), and a violation is an
/// invalid case even when pending; then runs the case (script.tgs with
/// atteso.txt, or domande.tgq), compares, writes the differences to `out` and the
/// obtained file to the output folder. A pending script of level 1 that does
/// not pass, with the early checks, runs again up to its frontier
/// (kObtainedFrontierReport) and fails if it differs before it, or has no
/// frontier (docs/adr/0037, points 4-6). The questions read "file=<name>"
/// in the case folder. `sources` is the folder of the pinned public files
/// of tests/verifica/sorgenti.json, read as "@sources/<name>" and
/// "source=<name>"; without it they are an error. Returns kExitPassed, kExitFailed, kExitInvalid or
/// kExitNeedsLevel2.
[[nodiscard]] int checkCase(const std::filesystem::path& folder, const CheckOptions& options,
                            std::ostream& out,
                            const std::optional<std::filesystem::path>& sources = std::nullopt);

}  // namespace tg::script
