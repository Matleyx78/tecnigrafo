// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// tecnigrafo-mcp: the development MCP server over stdio (docs/adr/0027). A
// development tool, never installed with the program.

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/mcp/server.hpp"
#include "tg/mcp/session.hpp"
#include "tg/mcp/tool.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#endif

namespace {

constexpr std::string_view kUsage =
    "usage:\n"
    "  tecnigrafo-mcp [--root <folder>] [--sources <folder>] [--questions]\n"
    "\n"
    "An MCP server on stdin and stdout, one JSON-RPC message per line; logs\n"
    "go to stderr. It ends at the end of stdin (docs/adr/0027).\n"
    "\n"
    "--root       the only folder where the file commands read and write\n"
    "--sources    the public files of tests/verifica/sorgenti.json, as @sources/<name>\n"
    "--questions  also offer the tool ask, for questions to the model (.tgq)\n";

constexpr int kExitOk = 0;
constexpr int kExitUsage = 2;

using Options = tg::mcp::SessionOptions;

bool isFolder(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::is_directory(path, error);
}

/// The options, or std::nullopt after printing what is wrong.
std::optional<Options> parseOptions(const std::vector<std::string_view>& arguments) {
    Options options;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        const std::string_view argument = arguments[i];
        if (argument == "--root" && i + 1 < arguments.size() && !options.root) {
            options.root = arguments[++i];
        } else if (argument == "--sources" && i + 1 < arguments.size() && !options.sources) {
            options.sources = arguments[++i];
        } else if (argument == "--questions" && !options.questions) {
            options.questions = true;
        } else {
            std::cerr << kUsage;
            return std::nullopt;
        }
    }
    for (const auto& [name, folder] :
         {std::pair{"--root", options.root}, std::pair{"--sources", options.sources}}) {
        if (folder && !isFolder(*folder)) {
            std::cerr << "tecnigrafo-mcp: " << name << " " << folder->string()
                      << " is not an existing folder.\n";
            return std::nullopt;
        }
    }
    return options;
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> arguments(argv + 1, argv + argc);
    if (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "-h")) {
        std::cout << kUsage;
        return kExitOk;
    }
    if (arguments.size() == 1 && arguments.front() == "--version") {
        std::cout << "tecnigrafo-mcp " << TG_VERSION << "\n";
        return kExitOk;
    }
    const std::optional<Options> options = parseOptions(arguments);
    if (!options) {
        return kExitUsage;
    }

#ifdef _WIN32
    // One message per line ends with '\n' alone, never "\r\n".
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    auto registry = std::make_shared<tg::cmd::CommandRegistry>();
    tg::cmd::registerBuiltinCommands(*registry);
    // list_commands, describe_command, the live session, then the tools of
    // tecnigrafo-script (docs/adr/0027, point 5).
    std::vector<std::unique_ptr<tg::mcp::Tool>> tools = tg::mcp::commandTools(std::move(registry));
    std::ranges::move(tg::mcp::sessionTools(std::make_shared<tg::mcp::Session>(*options)),
                      std::back_inserter(tools));
    std::ranges::move(tg::mcp::caseTools(*options), std::back_inserter(tools));
    tg::mcp::Server server(std::move(tools));
    return server.run(std::cin, std::cout, std::cerr) == 0 ? kExitOk : EXIT_FAILURE;
}
