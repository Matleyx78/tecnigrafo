// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// run, report, reset, open and save: the tools of the live session
// (docs/adr/0027, point 5). All but reset act on a ToolHost, so that the
// program can lend its active session (docs/adr/0051, point 4).

#include "nlohmann_json/json.hpp"
#include "tg/mcp/session.hpp"
#include "tg/mcp/tool.hpp"
#include "tg/mcp/tool_host.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::mcp {
namespace {

using nlohmann::json;

/// An error for the first argument not in `allowed`, or std::nullopt.
std::optional<ToolResult> unknownArgument(const std::string& tool, const json& arguments,
                                          std::initializer_list<std::string_view> allowed) {
    for (const auto& [key, value] : arguments.items()) {
        bool known = false;
        for (const std::string_view name : allowed) {
            known = known || key == name;
        }
        if (!known) {
            std::string message = "Unknown argument \"";
            message += key;
            message += "\" of ";
            message += tool;
            message += '.';
            return ToolResult::error(std::move(message));
        }
    }
    return std::nullopt;
}

/// The string argument `name`, or std::nullopt if it is missing or not a string.
std::optional<std::string> stringArgument(const json& arguments, std::string_view name) {
    const auto value = arguments.find(name);
    if (value == arguments.end() || !value->is_string()) {
        return std::nullopt;
    }
    return value->get<std::string>();
}

json noArguments() {
    return json{
        {"type", "object"}, {"properties", json::object()}, {"additionalProperties", false}};
}

json pathProperty(std::string_view description) {
    return json{{"type", "string"}, {"description", description}};
}

/// What every tool over a host shares: the host, which may be missing,
/// and what the tools say of paths.
class HostTool : public Tool {
public:
    HostTool(std::shared_ptr<ToolHost> host, FilePaths paths)
        : m_host(std::move(host)), m_paths(paths) {}

    [[nodiscard]] ToolResult call(const json& arguments) final {
        if (m_host == nullptr) {
            return ToolResult::error(std::string{kNotRunning});
        }
        return callOn(*m_host, arguments);
    }

protected:
    /// The call, with a host.
    [[nodiscard]] virtual ToolResult callOn(ToolHost& host, const json& arguments) = 0;

    [[nodiscard]] FilePaths paths() const { return m_paths; }

private:
    std::shared_ptr<ToolHost> m_host;
    FilePaths m_paths;
};

class Run final : public HostTool {
public:
    using HostTool::HostTool;

    [[nodiscard]] std::string name() const override { return "run"; }

    [[nodiscard]] std::string description() const override {
        return "Runs lines of a .tgs script in the live session, as tecnigrafo-script does at "
               "level 1: a normal line is typed on the command line followed by Enter (\"\" is "
               "Enter alone), and the level 1 directives !move, !click, !rightclick, !esc, "
               "!snap, !ortho, !view and !report work as in a script. Returns the lines written "
               "in the history since the previous call, the sections of any !report, and the "
               "prompt now. Invalid lines or level 2 directives are an error: nothing runs.";
    }

    [[nodiscard]] json inputSchema() const override {
        return json{{"type", "object"},
                    {"properties",
                     json{{"lines", json{{"type", "array"},
                                         {"items", json{{"type", "string"}}},
                                         {"maxItems", kMaxScriptLines},
                                         {"description",
                                          "The lines, one per string, without line breaks."}}}}},
                    {"required", json::array({"lines"})},
                    {"additionalProperties", false}};
    }

protected:
    [[nodiscard]] ToolResult callOn(ToolHost& host, const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {"lines"})) {
            return std::move(*error);
        }
        const auto lines = arguments.find("lines");
        if (lines == arguments.end() || !lines->is_array()) {
            return ToolResult::error("run needs \"lines\", an array of strings.");
        }
        if (lines->size() > kMaxScriptLines) {
            return ToolResult::error("Too many lines: at most " + std::to_string(kMaxScriptLines) +
                                     " in one request.");
        }
        std::vector<std::string> text;
        text.reserve(lines->size());
        for (const json& line : *lines) {
            if (!line.is_string()) {
                return ToolResult::error("run needs \"lines\", an array of strings.");
            }
            text.push_back(line.get<std::string>());
        }
        return host.run(text);
    }
};

class Report final : public HostTool {
public:
    using HostTool::HostTool;

    [[nodiscard]] std::string name() const override { return "report"; }

    [[nodiscard]] std::string description() const override {
        return "The report of the live session now, in the format of docs/verifica/rapporto.md: "
               "the line \"tecnigrafo-report 1\" and a section [final], whose messages are those "
               "written since the previous report or !report.";
    }

    [[nodiscard]] json inputSchema() const override { return noArguments(); }

protected:
    [[nodiscard]] ToolResult callOn(ToolHost& host, const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {})) {
            return std::move(*error);
        }
        return ToolResult::success(host.report());
    }
};

class Reset final : public Tool {
public:
    explicit Reset(std::shared_ptr<Session> session) : m_session(std::move(session)) {}

    [[nodiscard]] std::string name() const override { return "reset"; }

    [[nodiscard]] std::string description() const override {
        return "Starts the live session again from an empty document, as at the start of a "
               "script. Returns the prompt.";
    }

    [[nodiscard]] json inputSchema() const override { return noArguments(); }

    [[nodiscard]] ToolResult call(const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {})) {
            return std::move(*error);
        }
        return ToolResult::success(m_session->reset());
    }

private:
    std::shared_ptr<Session> m_session;
};

class Open final : public HostTool {
public:
    using HostTool::HostTool;

    [[nodiscard]] std::string name() const override { return "open"; }

    [[nodiscard]] std::string description() const override {
        if (paths() == FilePaths::Computer) {
            return "Opens a .dxf or .cadx file of this computer with the command OPEN, as run "
                   "would with the lines OPEN and the path. Returns what run returns.";
        }
        return "Opens a .dxf or .cadx file inside --root with the command OPEN, as run would "
               "with the lines OPEN and the name; with unsaved changes it stops at the question "
               "whether to save them. Returns what run returns.";
    }

    [[nodiscard]] json inputSchema() const override {
        const std::string_view path = paths() == FilePaths::Computer
                                          ? "The path of the file on this computer."
                                          : "The file, relative to --root: no \"..\", no "
                                            "absolute path.";
        return json{{"type", "object"},
                    {"properties", json{{"path", pathProperty(path)}}},
                    {"required", json::array({"path"})},
                    {"additionalProperties", false}};
    }

protected:
    [[nodiscard]] ToolResult callOn(ToolHost& host, const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {"path"})) {
            return std::move(*error);
        }
        const std::optional<std::string> path = stringArgument(arguments, "path");
        if (!path) {
            return ToolResult::error(paths() == FilePaths::Computer
                                         ? "open needs \"path\", the path of a file."
                                         : "open needs \"path\", a file relative to --root.");
        }
        return host.open(*path);
    }
};

class Save final : public HostTool {
public:
    using HostTool::HostTool;

    [[nodiscard]] std::string name() const override { return "save"; }

    [[nodiscard]] std::string description() const override {
        if (paths() == FilePaths::Computer) {
            return "Saves the drawing to a .dxf or .cadx file of this computer with the command "
                   "SAVEAS, as run would with the lines SAVEAS and the path. If the file exists, "
                   "replace true answers Y to the question to replace it; otherwise the question "
                   "stays. Returns what run returns.";
        }
        return "Saves the drawing to a .dxf or .cadx file inside --root with the command SAVEAS, "
               "as run would with the lines SAVEAS and the name. If the file exists, replace "
               "true answers Y to the question to replace it; otherwise the question stays. "
               "Returns what run returns.";
    }

    [[nodiscard]] json inputSchema() const override {
        const std::string_view path =
            paths() == FilePaths::Computer
                ? "The path of the file on this computer. Without an extension, .cadx."
                : "The file, relative to --root: no \"..\", no absolute path. Without an "
                  "extension, .cadx.";
        return json{
            {"type", "object"},
            {"properties", json{{"path", pathProperty(path)},
                                {"replace", json{{"type", "boolean"},
                                                 {"description", "Replace the file if it exists."},
                                                 {"default", false}}}}},
            {"required", json::array({"path"})},
            {"additionalProperties", false}};
    }

protected:
    [[nodiscard]] ToolResult callOn(ToolHost& host, const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {"path", "replace"})) {
            return std::move(*error);
        }
        const std::optional<std::string> path = stringArgument(arguments, "path");
        if (!path) {
            return ToolResult::error(paths() == FilePaths::Computer
                                         ? "save needs \"path\", the path of a file."
                                         : "save needs \"path\", a file relative to --root.");
        }
        bool replace = false;
        if (const auto value = arguments.find("replace"); value != arguments.end()) {
            if (!value->is_boolean()) {
                return ToolResult::error("\"replace\" of save must be true or false.");
            }
            replace = value->get<bool>();
        }
        return host.save(*path, replace);
    }
};

}  // namespace

std::vector<std::unique_ptr<Tool>> hostTools(std::shared_ptr<ToolHost> host, FilePaths paths) {
    std::vector<std::unique_ptr<Tool>> tools;
    tools.push_back(std::make_unique<Run>(host, paths));
    tools.push_back(std::make_unique<Report>(host, paths));
    tools.push_back(std::make_unique<Open>(host, paths));
    tools.push_back(std::make_unique<Save>(std::move(host), paths));
    return tools;
}

std::vector<std::unique_ptr<Tool>> programTools(std::shared_ptr<ToolHost> host) {
    std::vector<std::unique_ptr<Tool>> tools = hostCommandTools(host);
    std::ranges::move(hostTools(std::move(host), FilePaths::Computer), std::back_inserter(tools));
    return tools;
}

std::vector<std::unique_ptr<Tool>> sessionTools(std::shared_ptr<Session> session) {
    assert(session != nullptr);
    // run, report, reset, open, save: reset sits where tecnigrafo-mcp has
    // always listed it.
    std::vector<std::unique_ptr<Tool>> tools = hostTools(session, FilePaths::Root);
    constexpr std::ptrdiff_t kResetPlace = 2;
    tools.insert(tools.begin() + kResetPlace, std::make_unique<Reset>(std::move(session)));
    return tools;
}

}  // namespace tg::mcp
