// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// tecnigrafo-script: level 1 of the control mode (docs/adr/0011,
// docs/verifica/casi.md). A development tool, never installed with the
// program.

#include "tg/script/case_check.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kUsage =
    "usage:\n"
    "  tecnigrafo-script run <script.tgs> [--report <file>] [--sources <folder>]\n"
    "  tecnigrafo-script check <case folder> [--pending] [--output <folder>]\n"
    "                          [--sources <folder>]\n"
    "\n"
    "--sources is the folder of the public files of tests/verifica/sorgenti.json,\n"
    "read as @sources/<name> by scripts and as source=<name> by questions.\n"
    "\n"
    "Exit codes: 0 passed, 1 not passed, 2 invalid case or arguments,\n"
    "77 the case needs level 2. With --pending, a case not passed exits 0\n"
    "and a case passed exits 1. See docs/verifica/casi.md.\n";

int usageError() {
    std::cerr << kUsage;
    return tg::script::kExitInvalid;
}

int run(std::span<const std::string_view> arguments) {
    std::optional<std::filesystem::path> script;
    std::optional<std::filesystem::path> report;
    std::optional<std::filesystem::path> sources;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--report" && i + 1 < arguments.size() && !report) {
            report = arguments[++i];
        } else if (arguments[i] == "--sources" && i + 1 < arguments.size() && !sources) {
            sources = arguments[++i];
        } else if (!arguments[i].starts_with("--") && !script) {
            script = arguments[i];
        } else {
            return usageError();
        }
    }
    if (!script) {
        return usageError();
    }
    return tg::script::runScriptFile(*script, report, std::cout, std::cerr, sources);
}

int check(std::span<const std::string_view> arguments) {
    std::optional<std::filesystem::path> folder;
    tg::script::CheckOptions options;
    std::optional<std::filesystem::path> sources;
    bool outputGiven = false;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i] == "--pending" && !options.pending) {
            options.pending = true;
        } else if (arguments[i] == "--output" && i + 1 < arguments.size() && !outputGiven) {
            options.output = arguments[++i];
            outputGiven = true;
        } else if (arguments[i] == "--sources" && i + 1 < arguments.size() && !sources) {
            sources = arguments[++i];
        } else if (!arguments[i].starts_with("--") && !folder) {
            folder = arguments[i];
        } else {
            return usageError();
        }
    }
    if (!folder) {
        return usageError();
    }
    return tg::script::checkCase(*folder, options, std::cout, sources);
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> arguments(argv + 1, argv + argc);
    if (arguments.empty()) {
        return usageError();
    }
    const std::string_view command = arguments.front();
    const std::span<const std::string_view> rest = std::span{arguments}.subspan(1);
    if (command == "--help" || command == "-h") {
        std::cout << kUsage;
        return tg::script::kExitPassed;
    }
    if (command == "--version") {
        std::cout << "tecnigrafo-script " << TG_VERSION << "\n";
        return tg::script::kExitPassed;
    }
    if (command == "run") {
        return run(rest);
    }
    if (command == "check") {
        return check(rest);
    }
    return usageError();
}
