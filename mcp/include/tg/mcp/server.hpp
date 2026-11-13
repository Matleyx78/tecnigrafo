// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "nlohmann_json/json.hpp"
#include "tg/mcp/tool.hpp"

#include <cstddef>
#include <cstdint>
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

/// Where the server reads and writes its messages, one JSON-RPC message per
/// line: stdin and stdout for tecnigrafo-mcp, a local socket for the
/// program (docs/adr/0051, point 2).
class LineTransport {
public:
    enum class Read : std::uint8_t {
        Line,     ///< a message, in `line`
        TooLong,  ///< a message of more than `maxBytes` bytes, not kept
        End,      ///< no more messages will come
    };

    LineTransport() = default;
    LineTransport(const LineTransport&) = delete;
    LineTransport& operator=(const LineTransport&) = delete;
    LineTransport(LineTransport&&) = delete;
    LineTransport& operator=(LineTransport&&) = delete;
    virtual ~LineTransport() = default;

    /// Waits for the next message and puts it in `line`, without its line
    /// end ("\n", or "\r\n"). The limit counts every byte before the "\n",
    /// a final "\r" included, as tecnigrafo-mcp always did: with
    /// maxBytes = 128, 128 bytes and "\n" are a Line, the same 128 bytes and
    /// "\r\n" are TooLong. A longer message than `maxBytes` is read to its
    /// end and reported as TooLong, so that it costs no memory. The final
    /// "\r" is removed only from a Line. Server::answer() counts the same
    /// bytes: a transport may hand it what comes before the "\n" instead.
    [[nodiscard]] virtual Read readLine(std::string& line, std::size_t maxBytes) = 0;

    /// Writes one message followed by "\n". False if the transport failed.
    [[nodiscard]] virtual bool writeLine(std::string_view line) = 0;
};

/// The transport over two streams: stdin and stdout, or streams in memory.
class StreamTransport final : public LineTransport {
public:
    StreamTransport(std::istream& in, std::ostream& out) : m_in(in), m_out(out) {}

    [[nodiscard]] Read readLine(std::string& line, std::size_t maxBytes) override;
    [[nodiscard]] bool writeLine(std::string_view line) override;

private:
    std::istream& m_in;
    std::ostream& m_out;
};

/// MCP over a transport of lines: one JSON-RPC message per line, logs on
/// `log` (docs/adr/0027, point 3). It offers exactly the tools it is built
/// with; a call to any other is answered as an unknown tool.
///
/// Methods: server/discover, tools/list and tools/call for requests that
/// carry the modern `_meta`; initialize, notifications/initialized, ping,
/// tools/list and tools/call for a legacy client. Before initialize a legacy
/// client gets an answer only to ping. Notifications are never answered.
class Server {
public:
    Server(std::vector<std::unique_ptr<Tool>> tools, ServerOptions options = {});

    /// Serves until the end of the transport. Returns 0, or 1 if a write
    /// failed.
    int run(LineTransport& transport, std::ostream& log);

    /// run() over StreamTransport{in, out}: serves until the end of `in`.
    /// Returns 0, or 1 if `out` failed.
    int run(std::istream& in, std::ostream& out, std::ostream& log);

    /// The answer to one message, as run() would write it, or std::nullopt
    /// for none (a notification or a blank line): for a transport that
    /// brings its messages one at a time instead of waiting for them.
    /// `message` is what comes before the "\n", and may keep the "\r" of a
    /// "\r\n": as in LineTransport::readLine(), the limit counts every byte
    /// of it, the "\r" included, and a message longer than maxMessageBytes
    /// is refused as run() refuses it; the "\r" is removed after the check.
    [[nodiscard]] std::optional<std::string> answer(std::string_view message, std::ostream& log);

private:
    /// The answer to one message, or std::nullopt for none (a notification).
    [[nodiscard]] std::optional<nlohmann::json> handle(std::string_view line, std::ostream& log);
    /// The answer to a message of more than maxMessageBytes.
    [[nodiscard]] nlohmann::json tooLarge(std::ostream& log) const;
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
