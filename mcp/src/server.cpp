// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The MCP server over stdio (docs/adr/0027, point 3). It follows the
// specification at https://modelcontextprotocol.io/specification: revision
// 2026-07-28 for requests that carry their version in `_meta`, and the
// initialize handshake of revision 2025-11-25 for older clients (the
// "dual-era" server of the 2026-07-28 versioning page).

#include "tg/mcp/server.hpp"

#include "nlohmann_json/json.hpp"
#include "tg/mcp/tool.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::mcp {

using nlohmann::json;

namespace {

// The `_meta` keys of a modern request and result (schema of 2026-07-28).
constexpr std::string_view kVersionKey = "io.modelcontextprotocol/protocolVersion";
constexpr std::string_view kCapabilitiesKey = "io.modelcontextprotocol/clientCapabilities";
constexpr std::string_view kServerInfoKey = "io.modelcontextprotocol/serverInfo";

/// ttlMs of tools/list and server/discover: 0, always stale. The list is
/// fixed for the life of a process, but a rebuilt server may change it, and
/// on a local pipe asking again costs nothing.
constexpr int kCacheTtlMs = 0;

std::string text(const json& value) {
    // replace: a string that is not UTF-8 never throws; the parser already
    // rejects such input, so it can only come from a tool.
    return value.dump(-1, ' ', false, json::error_handler_t::replace);
}

json serverInfo() {
    return json{{"name", kServerName}, {"version", TG_VERSION}};
}

json resultResponse(const json& id, json result) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

json errorResponse(const json& id, int code, std::string message,
                   std::optional<json> data = std::nullopt) {
    json error{{"code", code}, {"message", std::move(message)}};
    if (data) {
        error["data"] = std::move(*data);
    }
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"error", std::move(error)}};
}

/// A modern result: resultType and the server's identity in `_meta`.
json modernResult(json result) {
    result["resultType"] = "complete";
    result["_meta"] = json{{kServerInfoKey, serverInfo()}};
    return result;
}

enum class LineStatus : std::uint8_t { Line, TooLong, End };

/// Reads one line without its '\n' (and a '\r' before it). A line longer
/// than `maxBytes` is read to its end but not kept, so that it costs no
/// memory: TooLong. End when nothing is left.
LineStatus readLine(std::streambuf& buffer, std::size_t maxBytes, std::string& line) {
    line.clear();
    std::size_t length = 0;
    bool any = false;
    for (;;) {
        const std::streambuf::int_type next = buffer.sbumpc();
        if (std::streambuf::traits_type::eq_int_type(next, std::streambuf::traits_type::eof())) {
            if (!any) {
                return LineStatus::End;
            }
            break;
        }
        any = true;
        const char character = std::streambuf::traits_type::to_char_type(next);
        if (character == '\n') {
            break;
        }
        ++length;
        if (length <= maxBytes) {
            line.push_back(character);
        }
    }
    if (length > maxBytes) {
        return LineStatus::TooLong;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return LineStatus::Line;
}

bool isBlank(std::string_view line) {
    return line.find_first_not_of(" \t\r") == std::string_view::npos;
}

/// A request id of MCP: a string or an integer, never null.
bool isValidId(const json& id) {
    return id.is_string() || id.is_number_integer();
}

}  // namespace

Server::Server(std::vector<std::unique_ptr<Tool>> tools, ServerOptions options)
    : m_tools(std::move(tools)), m_options(options) {}

int Server::run(std::istream& in, std::ostream& out, std::ostream& log) {
    std::streambuf* buffer = in.rdbuf();
    if (buffer == nullptr) {
        return 0;
    }
    std::string line;
    for (;;) {
        const LineStatus status = readLine(*buffer, m_options.maxMessageBytes, line);
        if (status == LineStatus::End) {
            return 0;
        }
        std::optional<json> answer;
        if (status == LineStatus::TooLong) {
            // Not parsed at all: its id is unknown (docs/adr/0027, point 3).
            log << kServerName << ": a message of more than " << m_options.maxMessageBytes
                << " bytes was refused\n";
            answer = errorResponse(nullptr, kInvalidRequest,
                                   "Message too large: the limit is " +
                                       std::to_string(m_options.maxMessageBytes) + " bytes.");
        } else if (!isBlank(line)) {
            answer = handle(line, log);
        }
        if (answer) {
            out << text(*answer) << '\n' << std::flush;
            if (!out) {
                log << kServerName << ": cannot write the answer\n";
                return 1;
            }
        }
    }
}

std::optional<json> Server::handle(std::string_view line, std::ostream& log) {
    const json message = json::parse(line, nullptr, /*allow_exceptions=*/false);
    if (message.is_discarded()) {
        log << kServerName << ": a message that is not valid JSON was refused\n";
        return errorResponse(nullptr, kParseError, "Parse error: the message is not valid JSON.");
    }
    if (!message.is_object()) {
        // Arrays too: MCP has no JSON-RPC batches since revision 2025-06-18.
        return errorResponse(nullptr, kInvalidRequest,
                             "Invalid request: a message is one JSON-RPC object.");
    }
    const auto idIt = message.find("id");
    const bool hasId = idIt != message.end();
    const json id = hasId && isValidId(*idIt) ? *idIt : json(nullptr);
    const auto version = message.find("jsonrpc");
    const auto method = message.find("method");
    if (version == message.end() || *version != "2.0" || method == message.end() ||
        !method->is_string() || (hasId && !isValidId(*idIt))) {
        return errorResponse(id, kInvalidRequest,
                             "Invalid request: a JSON-RPC 2.0 request needs \"jsonrpc\": \"2.0\", "
                             "a string \"method\" and a string or integer \"id\".");
    }
    const auto paramsIt = message.find("params");
    const bool paramsValid = paramsIt == message.end() || paramsIt->is_object();
    if (!hasId) {
        // A notification is never answered, not even when its params are wrong.
        // notifications/initialized needs no action: initialize alone
        // already enables the other methods.
        return std::nullopt;
    }
    if (!paramsValid) {
        return errorResponse(id, kInvalidParams, "Invalid params: \"params\" must be an object.");
    }
    const json params = paramsIt == message.end() ? json::object() : *paramsIt;
    return handleRequest(id, method->get_ref<const std::string&>(), params);
}

json Server::handleRequest(const json& id, const std::string& method, const json& params) {
    if (method == "initialize") {
        return initialize(id, params);
    }
    const auto meta = params.find("_meta");
    if (meta != params.end() && meta->is_object() && meta->contains(kVersionKey)) {
        return handleModern(id, method, params);
    }
    if (method == "server/discover") {
        return errorResponse(id, kInvalidParams,
                             "Invalid params: _meta must give \"" + std::string(kVersionKey) +
                                 "\" and \"" + std::string(kCapabilitiesKey) + "\".");
    }
    return handleLegacy(id, method, params);
}

json Server::handleModern(const json& id, const std::string& method, const json& params) {
    const json& meta = params.at("_meta");
    const json& version = meta.at(kVersionKey);
    const auto capabilities = meta.find(kCapabilitiesKey);
    if (!version.is_string() || capabilities == meta.end() || !capabilities->is_object()) {
        return errorResponse(id, kInvalidParams,
                             "Invalid params: _meta must give \"" + std::string(kVersionKey) +
                                 "\" as a string and \"" + std::string(kCapabilitiesKey) +
                                 "\" as an object.");
    }
    if (version.get_ref<const std::string&>() != kProtocolVersion) {
        // Only the modern version is listed: the legacy one is not carried
        // in _meta, and a client that retried with it would be refused again.
        return errorResponse(
            id, kUnsupportedProtocolVersion, "Unsupported protocol version",
            json{{"supported", json::array({kProtocolVersion})}, {"requested", version}});
    }
    if (method == "server/discover") {
        return resultResponse(
            id, modernResult(json{{"supportedVersions", json::array({kProtocolVersion})},
                                  {"capabilities", json{{"tools", json::object()}}},
                                  {"ttlMs", kCacheTtlMs},
                                  {"cacheScope", "private"}}));
    }
    if (method == "tools/list") {
        return resultResponse(id, modernResult(listTools(true)));
    }
    if (method == "tools/call") {
        return callTool(id, params, true);
    }
    // ping and initialize are not methods of the modern revision.
    return errorResponse(id, kMethodNotFound, "Method not found: " + method + ".");
}

json Server::handleLegacy(const json& id, const std::string& method, const json& params) {
    if (method == "ping") {
        return resultResponse(id, json::object());
    }
    if (method != "tools/list" && method != "tools/call") {
        return errorResponse(id, kMethodNotFound, "Method not found: " + method + ".");
    }
    if (!m_initialized) {
        return errorResponse(id, kInvalidRequest,
                             "Invalid request: the server is not initialized; send initialize "
                             "first, or give the protocol version in _meta.");
    }
    if (method == "tools/list") {
        return resultResponse(id, listTools(false));
    }
    return callTool(id, params, false);
}

json Server::initialize(const json& id, const json& params) {
    if (m_initialized) {
        return errorResponse(id, kInvalidRequest, "Invalid request: already initialized.");
    }
    const auto version = params.find("protocolVersion");
    const auto capabilities = params.find("capabilities");
    const auto client = params.find("clientInfo");
    if (version == params.end() || !version->is_string() || capabilities == params.end() ||
        !capabilities->is_object() || (client != params.end() && !client->is_object())) {
        return errorResponse(id, kInvalidParams,
                             "Invalid params: initialize needs a string \"protocolVersion\", an "
                             "object \"capabilities\" and an object \"clientInfo\".");
    }
    m_initialized = true;
    // Revision 2025-11-25: the requested version if supported, otherwise the
    // latest one the server supports through initialize, which is the same.
    // The client then disconnects if it does not know it.
    return resultResponse(id, json{{"protocolVersion", kLegacyProtocolVersion},
                                   {"capabilities", json{{"tools", json::object()}}},
                                   {"serverInfo", serverInfo()}});
}

json Server::listTools(bool modern) const {
    json tools = json::array();
    for (const std::unique_ptr<Tool>& tool : m_tools) {
        tools.push_back(json{{"name", tool->name()},
                             {"description", tool->description()},
                             {"inputSchema", tool->inputSchema()}});
    }
    json result{{"tools", std::move(tools)}};
    if (modern) {
        result["ttlMs"] = kCacheTtlMs;
        result["cacheScope"] = "private";
    }
    return result;
}

json Server::callTool(const json& id, const json& params, bool modern) {
    const auto name = params.find("name");
    if (name == params.end() || !name->is_string()) {
        return errorResponse(id, kInvalidParams,
                             "Invalid params: tools/call needs a string \"name\".");
    }
    const auto argumentsIt = params.find("arguments");
    if (argumentsIt != params.end() && !argumentsIt->is_object()) {
        return errorResponse(id, kInvalidParams,
                             "Invalid params: \"arguments\" must be an object.");
    }
    const json arguments = argumentsIt == params.end() ? json::object() : *argumentsIt;
    const auto& toolName = name->get_ref<const std::string&>();
    Tool* tool = nullptr;
    for (const std::unique_ptr<Tool>& candidate : m_tools) {
        if (candidate->name() == toolName) {
            tool = candidate.get();
            break;
        }
    }
    if (tool == nullptr) {
        return errorResponse(id, kInvalidParams, "Unknown tool: " + toolName);
    }
    ToolResult outcome;
    try {
        outcome = tool->call(arguments);
    } catch (const std::exception& exception) {
        // A tool that throws is a bug of ours; the server keeps serving.
        return errorResponse(id, kInternalError,
                             std::string("Internal error: ") + exception.what());
    }
    json result{{"content", json::array({json{{"type", "text"}, {"text", outcome.text}}})},
                {"isError", outcome.isError}};
    if (outcome.structured) {
        result["structuredContent"] = std::move(*outcome.structured);
    }
    return resultResponse(id, modern ? modernResult(std::move(result)) : std::move(result));
}

}  // namespace tg::mcp
