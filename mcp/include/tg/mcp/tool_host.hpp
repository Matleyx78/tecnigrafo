// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/mcp/tool.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {
class CommandRegistry;
}  // namespace tg::cmd

namespace tg::mcp {

/// The error of every call to a tool that has no host (docs/adr/0051,
/// point 1): the bridge of the program lists the tools before it reaches
/// the program, and a call it cannot pass on gives this message.
inline constexpr std::string_view kNotRunning =
    "Tecnigrafo is not running, or its assistant channel is off.";

/// What the tools act on (docs/adr/0051, point 4): the private session of
/// tecnigrafo-mcp, or the active session lent by the program.
class ToolHost {
public:
    ToolHost() = default;
    ToolHost(const ToolHost&) = delete;
    ToolHost& operator=(const ToolHost&) = delete;
    ToolHost(ToolHost&&) = delete;
    ToolHost& operator=(ToolHost&&) = delete;
    virtual ~ToolHost() = default;

    /// The tool run: lines of a .tgs script, level 1 (Session::run).
    [[nodiscard]] virtual ToolResult run(std::span<const std::string> lines) = 0;
    /// The tool report: the report of the state now (Session::report).
    [[nodiscard]] virtual std::string report() = 0;
    /// The tool open: OPEN with `path` (Session::open).
    [[nodiscard]] virtual ToolResult open(std::string_view path) = 0;
    /// The tool save: SAVEAS with `path` (Session::save).
    [[nodiscard]] virtual ToolResult save(std::string_view path, bool replace) = 0;
    /// The commands that list_commands and describe_command describe.
    [[nodiscard]] virtual const cmd::CommandRegistry& commands() const = 0;
};

/// The paths that open and save take, which only change what the tools say
/// of them: the host decides what a path means.
enum class FilePaths : std::uint8_t {
    Root,      ///< tecnigrafo-mcp: names relative to --root (docs/adr/0027, point 4)
    Computer,  ///< the program: the paths of the computer (docs/adr/0051, point 4)
};

/// list_commands and describe_command over host->commands(), with the
/// same results as commandTools() over the same registry. With a null
/// `host` they still describe themselves, and every call is the error
/// kNotRunning.
[[nodiscard]] std::vector<std::unique_ptr<Tool>> hostCommandTools(
    std::shared_ptr<const ToolHost> host);

/// run, report, open and save over `host`, in this order. With a null
/// `host` they still describe themselves, and every call is the error
/// kNotRunning.
[[nodiscard]] std::vector<std::unique_ptr<Tool>> hostTools(std::shared_ptr<ToolHost> host,
                                                           FilePaths paths);

/// The tools of the program's assistant channel (docs/adr/0051, point 4):
/// list_commands, describe_command, run, report, open and save over `host`,
/// in this order, and nothing else (no reset, run_script, check_case, ask).
/// The bridge answers tools/list with programTools(nullptr), without a host.
[[nodiscard]] std::vector<std::unique_ptr<Tool>> programTools(std::shared_ptr<ToolHost> host);

}  // namespace tg::mcp
