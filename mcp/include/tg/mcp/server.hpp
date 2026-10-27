// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "nlohmann_json/json.hpp"
#include "tg/mcp/tool.hpp"

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::mcp {

/// The protocol revisions the server speaks (docs/adr/0027, point 3). The
/// latest stable revision carries its version in every request ("modern",
/// stateless); the previous one opens with initialize ("legacy"). A server
/// speaking both is "dual-era" in the words of the specification.
inline constexpr std::string_view kProtocolVersion = "2026-07-28";
inline constexpr std::string_view kLegacyProtocolVersion = "2025-11-25";

/// The name that initialize and server/discover report.
inline constexpr std::string_view kServerName = "tecnigrafo-mcp";

/// JSON-RPC 2.0 error codes, and the one of MCP for an unknown version.
inline constexpr int kParseError = -32700;
inline constexpr int kInvalidRequest = -32600;
inline constexpr int kMethodNotFound = -32601;
inline constexpr int kInvalidParams = -32602;
inline constexpr int kInternalError = -32603;
inline constexpr int kUnsupportedProtocolVersion = -32022;

struct ServerOptions {
    std::size_t maxMessageBytes{4 * 1024 * 1024};
};

/// MCP over stdio: one JSON-RPC message per line on `in`, answers on `out`,
/// logs on `log` (docs/adr/0027, point 3). Returns at the end of `in`.
///
/// Methods: server/discover, tools/list and tools/call for requests that
/// carry the modern `_meta`; initialize, notifications/initialized, ping,
/// tools/list and tools/call for a legacy client. Before initialize a legacy
/// client gets an answer only to ping. Notifications are never answered.
class Server {
public:
    Server(std::vector<std::unique_ptr<Tool>> tools, ServerOptions options = {});

    /// Serves until the end of `in`. Returns 0, or 1 if `out` failed.
    int run(std::istream& in, std::ostream& out, std::ostream& log);

private:
    /// The answer to one message, or std::nullopt for none (a notification).
    [[nodiscard]] std::optional<nlohmann::json> handle(std::string_view line, std::ostream& log);
    [[nodiscard]] nlohmann::json handleRequest(const nlohmann::json& id, const std::string& method,
                                               const nlohmann::json& params);
    [[nodiscard]] nlohmann::json handleModern(const nlohmann::json& id, const std::string& method,
                                              const nlohmann::json& params);
    [[nodiscard]] nlohmann::json handleLegacy(const nlohmann::json& id, const std::string& method,
                                              const nlohmann::json& params);
    [[nodiscard]] nlohmann::json initialize(const nlohmann::json& id, const nlohmann::json& params);
    [[nodiscard]] nlohmann::json listTools(bool modern) const;
    [[nodiscard]] nlohmann::json callTool(const nlohmann::json& id, const nlohmann::json& params,
                                          bool modern);

    std::vector<std::unique_ptr<Tool>> m_tools;
    ServerOptions m_options;
    bool m_initialized{false};  ///< a legacy client sent initialize
};

}  // namespace tg::mcp
