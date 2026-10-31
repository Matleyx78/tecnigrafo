// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// tecnigrafo-script: level 1 of the control mode (docs/adr/0011,
// docs/verifica/casi.md). A development tool, never installed with the
// program.

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/script/case_check.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kUsage =
    "usage:\n"
    "  tecnigrafo-script run <script.tgs> [--report <file>] [--sources <folder>]\n"
    "  tecnigrafo-script check <case folder> [--pending [--no-early-checks]]\n"
    "                          [--output <folder>] [--sources <folder>]\n"
    "  tecnigrafo-script describe [<command>]\n"
    "\n"
    "describe prints the description of every built-in command, or of one\n"
    "named by name or alias (docs/adr/0026).\n"
    "\n"
    "check first runs the early checks (docs/adr/0037): the form of the files\n"
    "of the case and the echo of the lines of the script; a case that does not\n"
    "pass them is invalid, also with --pending. --no-early-checks, only with\n"
    "--pending, leaves them out for a pending case marked \"esente\" in\n"
    "the folder tests/verifica/in-attesa/.\n"
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

std::string_view valueName(tg::cmd::ValueType value) {
    using tg::cmd::ValueType;
    switch (value) {
        case ValueType::Point:
            return "point";
        case ValueType::Distance:
            return "distance";
        case ValueType::Angle:
            return "angle";
        case ValueType::Integer:
            return "integer";
        case ValueType::Real:
            return "real";
        case ValueType::Text:
            return "text";
        case ValueType::Name:
            return "name";
        case ValueType::Color:
            return "color";
        case ValueType::Keyword:
            return "keyword";
        case ValueType::Object:
            return "object";
        case ValueType::Selection:
            return "selection";
        case ValueType::FileOpen:
            return "file-open";
        case ValueType::FileSave:
            return "file-save";
    }
    return "unknown";
}

/// One field per line:
///   command: PLINE
///   aliases: PL
///   summary: Draws a polyline of line and arc segments.
///   prompt next-point: Specify next point or [Arc/Undo]: (point; Arc=A Undo=U; Enter)
/// "selects objects first" follows the summary of commands that select first.
void printDescription(const tg::cmd::CommandDescription& description, std::ostream& out) {
    out << "command: " << description.name << "\n";
    out << "aliases:";
    for (const std::string& alias : description.aliases) {
        out << " " << alias;
    }
    out << "\n";
    out << "summary: " << description.summary << "\n";
    if (description.selectsFirst) {
        out << "selects objects first\n";
    }
    for (const tg::cmd::PromptDescription& prompt : description.prompts) {
        out << "prompt " << prompt.id << ": " << prompt.text << " (" << valueName(prompt.value);
        if (!prompt.keywords.empty()) {
            out << ";";
            for (const tg::cmd::Keyword& keyword : prompt.keywords) {
                out << " " << keyword.name << "=" << keyword.shortcut;
            }
        }
        if (prompt.acceptsEmpty) {
            out << "; Enter";
        }
        out << ")\n";
    }
}

int describe(std::span<const std::string_view> arguments) {
    if (arguments.size() > 1 || (!arguments.empty() && arguments.front().starts_with("--"))) {
        return usageError();
    }
    tg::cmd::CommandRegistry registry;
    tg::cmd::registerBuiltinCommands(registry);
    if (!arguments.empty()) {
        const tg::cmd::CommandDescription* description = registry.describe(arguments.front());
        if (description == nullptr) {
            std::cerr << "Unknown command \"" << arguments.front() << "\".\n";
            return tg::script::kExitInvalid;
        }
        printDescription(*description, std::cout);
        return tg::script::kExitPassed;
    }
    bool first = true;
    for (const tg::cmd::CommandDescription* description : registry.descriptions()) {
        if (!first) {
            std::cout << "\n";  // an empty line between two commands
        }
        first = false;
        printDescription(*description, std::cout);
    }
    return tg::script::kExitPassed;
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
        } else if (arguments[i] == "--no-early-checks" && options.earlyChecks) {
            options.earlyChecks = false;
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
    // Only a pending case can be exempt from the early checks.
    if (!folder || (!options.earlyChecks && !options.pending)) {
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
    if (command == "describe") {
        return describe(rest);
    }
    return usageError();
}
