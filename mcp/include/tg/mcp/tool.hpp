// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "nlohmann_json/json.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tg::cmd {
class CommandRegistry;
}  // namespace tg::cmd

namespace tg::mcp {

/// What a tool call gives back (a CallToolResult of MCP). The text becomes
/// the one text content block; the structured value, when present, is also
/// the structuredContent (a JSON object), and its compact text is the text.
struct ToolResult {
    std::string text;
    std::optional<nlohmann::json> structured;
    bool isError{false};

    /// A successful result with only text.
    [[nodiscard]] static ToolResult success(std::string text);
    /// A successful result with a JSON object; the text is its compact form.
    [[nodiscard]] static ToolResult object(nlohmann::json value);
    /// A failed tool call: {isError: true} with the message (docs/adr/0027,
    /// point 5). Not a protocol error: the model reads it and can retry.
    [[nodiscard]] static ToolResult error(std::string message);
};

/// One tool (docs/adr/0027, point 5).
class Tool {
public:
    Tool() = default;
    Tool(const Tool&) = delete;
    Tool& operator=(const Tool&) = delete;
    Tool(Tool&&) = delete;
    Tool& operator=(Tool&&) = delete;
    virtual ~Tool() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::string description() const = 0;
    [[nodiscard]] virtual nlohmann::json inputSchema() const = 0;  ///< JSON Schema object
    /// The result content; an error is {isError: true} with the message.
    /// `arguments` is always a JSON object (empty when the call gave none).
    [[nodiscard]] virtual ToolResult call(const nlohmann::json& arguments) = 0;
};

/// list_commands and describe_command (docs/adr/0026, docs/adr/0027 point 5)
/// over the descriptions of `registry`, in JSON with the same fields as
/// cmd::CommandDescription. Precondition: `registry` is not null.
[[nodiscard]] std::vector<std::unique_ptr<Tool>> commandTools(
    std::shared_ptr<const cmd::CommandRegistry> registry);

}  // namespace tg::mcp
