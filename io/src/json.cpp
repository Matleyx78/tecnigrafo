// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "json.hpp"

#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <exception>
#include <optional>
#include <set>
#include <utility>

namespace tg::io::detail {
namespace {

Error malformed() {
    return {Error::Kind::Malformed, 0, "manifest.json: invalid JSON"};
}

bool digit(char c) {
    return c >= '0' && c <= '9';
}

void appendCodepoint(std::string& out, unsigned int code) {
    constexpr unsigned int kContinuation = 0x80;
    constexpr unsigned int kLowBits = 0x3f;
    if (code < 0x80) {
        out += static_cast<char>(code);
    } else if (code < 0x800) {
        out += static_cast<char>(0xc0U | (code >> 6));
        out += static_cast<char>(kContinuation | (code & kLowBits));
    } else if (code < 0x10000) {
        out += static_cast<char>(0xe0U | (code >> 12));
        out += static_cast<char>(kContinuation | ((code >> 6) & kLowBits));
        out += static_cast<char>(kContinuation | (code & kLowBits));
    } else {
        out += static_cast<char>(0xf0U | (code >> 18));
        out += static_cast<char>(kContinuation | ((code >> 12) & kLowBits));
        out += static_cast<char>(kContinuation | ((code >> 6) & kLowBits));
        out += static_cast<char>(kContinuation | (code & kLowBits));
    }
}

class Parser {
public:
    explicit Parser(std::string_view bytes) : m_bytes(bytes) {}

    Result<Json> read() {
        auto root = value();
        if (!root) {
            return malformed();
        }
        std::vector<Frame> stack;
        if (isContainer(*root)) {
            stack.emplace_back(std::move(*root));
        }
        while (!stack.empty()) {
            auto& frame = stack.back();
            const bool object = frame.value.kind == Json::Kind::Object;
            const char closing = object ? '}' : ']';
            whitespace();
            if ((frame.first || frame.afterValue) && take(closing)) {
                Json completed = std::move(frame.value);
                stack.pop_back();
                if (stack.empty()) {
                    root = std::move(completed);
                } else {
                    append(stack.back(), std::move(completed));
                }
                continue;
            }
            if (frame.afterValue) {
                if (!take(',')) {
                    return malformed();
                }
                frame.afterValue = false;
                whitespace();
            }
            frame.first = false;
            if (object) {
                auto key = string();
                whitespace();
                if (!key || !frame.keys.insert(*key).second || !take(':')) {
                    return malformed();
                }
                frame.key = std::move(*key);
            }
            auto child = value();
            if (!child) {
                return malformed();
            }
            if (isContainer(*child)) {
                if (stack.size() >= kJsonMaxDepth) {
                    return malformed();
                }
                stack.emplace_back(std::move(*child));
            } else {
                append(frame, std::move(*child));
            }
        }
        whitespace();
        if (m_position != m_bytes.size()) {
            return malformed();
        }
        return std::move(*root);
    }

private:
    struct Frame {
        Json value;
        std::set<std::string> keys;
        std::string key;
        bool first{true};
        bool afterValue{false};

        explicit Frame(Json initial) : value(std::move(initial)) {}
    };

    static bool isContainer(const Json& value) {
        return value.kind == Json::Kind::Object || value.kind == Json::Kind::Array;
    }

    static void append(Frame& frame, Json child) {
        if (frame.value.kind == Json::Kind::Object) {
            frame.value.members.emplace_back(std::move(frame.key), std::move(child));
        } else {
            frame.value.elements.push_back(std::move(child));
        }
        frame.afterValue = true;
    }

    void whitespace() {
        while (m_position < m_bytes.size() &&
               std::string_view{" \t\r\n"}.find(m_bytes[m_position]) != std::string_view::npos) {
            ++m_position;
        }
    }

    bool take(char c) {
        if (m_position < m_bytes.size() && m_bytes[m_position] == c) {
            ++m_position;
            return true;
        }
        return false;
    }

    std::optional<unsigned int> hexQuad() {
        constexpr unsigned int kHexBase = 16;
        unsigned int value = 0;
        for (int i = 0; i < 4; ++i) {
            if (m_position == m_bytes.size()) {
                return std::nullopt;
            }
            const char c = m_bytes[m_position++];
            unsigned int nibble = 0;
            if (digit(c)) {
                nibble = static_cast<unsigned int>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                nibble = static_cast<unsigned int>(c - 'a') + 10;
            } else if (c >= 'A' && c <= 'F') {
                nibble = static_cast<unsigned int>(c - 'A') + 10;
            } else {
                return std::nullopt;
            }
            value = value * kHexBase + nibble;
        }
        return value;
    }

    bool unicode(std::string& out) {
        auto code = hexQuad();
        if (!code) {
            return false;
        }
        constexpr unsigned int kHighFirst = 0xd800;
        constexpr unsigned int kHighLast = 0xdbff;
        constexpr unsigned int kLowFirst = 0xdc00;
        constexpr unsigned int kLowLast = 0xdfff;
        if (*code >= kHighFirst && *code <= kHighLast) {
            if (!take('\\') || !take('u')) {
                return false;
            }
            const auto low = hexQuad();
            if (!low || *low < kLowFirst || *low > kLowLast) {
                return false;
            }
            constexpr unsigned int kSupplementary = 0x10000;
            *code = kSupplementary + ((*code - kHighFirst) << 10) + (*low - kLowFirst);
        } else if (*code >= kLowFirst && *code <= kLowLast) {
            return false;
        }
        appendCodepoint(out, *code);
        return true;
    }

    std::optional<std::string> string() {
        if (!take('"')) {
            return std::nullopt;
        }
        std::string out;
        while (m_position < m_bytes.size()) {
            const char c = m_bytes[m_position++];
            if (c == '"') {
                return out;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                return std::nullopt;
            }
            if (c != '\\') {
                out += c;
                continue;
            }
            if (m_position == m_bytes.size()) {
                return std::nullopt;
            }
            const char escaped = m_bytes[m_position++];
            constexpr std::string_view kEscapes = "\"\\/bfnrt";
            constexpr std::string_view kDecoded = "\"\\/\b\f\n\r\t";
            const auto index = kEscapes.find(escaped);
            if (index != std::string_view::npos) {
                out += kDecoded[index];
            } else if (escaped != 'u' || !unicode(out)) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    bool digits() {
        const auto start = m_position;
        while (m_position < m_bytes.size() && digit(m_bytes[m_position])) {
            ++m_position;
        }
        return m_position > start;
    }

    std::optional<Json> number() {
        const auto start = m_position;
        (void)take('-');
        if (!take('0') && !digits()) {
            return std::nullopt;
        }
        if (take('.') && !digits()) {
            return std::nullopt;
        }
        if (take('e') || take('E')) {
            if (!take('+')) {
                (void)take('-');
            }
            if (!digits()) {
                return std::nullopt;
            }
        }
        Json result;
        result.kind = Json::Kind::Number;
        result.text = m_bytes.substr(start, m_position - start);
        return result;
    }

    std::optional<Json> value() {
        whitespace();
        if (m_position == m_bytes.size()) {
            return std::nullopt;
        }
        const char c = m_bytes[m_position];
        if (c == '{' || c == '[') {
            ++m_position;
            Json result;
            result.kind = c == '{' ? Json::Kind::Object : Json::Kind::Array;
            return result;
        }
        if (c == '"') {
            auto text = string();
            if (!text) {
                return std::nullopt;
            }
            Json result;
            result.kind = Json::Kind::String;
            result.text = std::move(*text);
            return result;
        }
        for (const std::string_view literal : {"true", "false", "null"}) {
            if (m_bytes.substr(m_position).starts_with(literal)) {
                m_position += literal.size();
                Json result;
                result.kind = literal == "null" ? Json::Kind::Null : Json::Kind::Boolean;
                if (result.kind == Json::Kind::Boolean) {
                    result.text = literal;
                }
                return result;
            }
        }
        return number();
    }

    std::string_view m_bytes;
    std::size_t m_position{0};
};

void quote(std::string& out, std::string_view text) {
    constexpr std::string_view kHex = "0123456789abcdef";
    out += '"';
    for (const char c : text) {
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            const auto byte = static_cast<unsigned char>(c);
            out += "\\u00";
            out += kHex[byte >> 4];
            out += kHex[byte & 0xfU];
        } else {
            out += c;
        }
    }
    out += '"';
}

bool serialize(const Json& value, std::string& out) {
    struct Frame {
        const Json* value;
        std::size_t next{0};
        bool started{false};
    };

    std::vector<Frame> stack{{&value}};
    while (!stack.empty()) {
        auto& frame = stack.back();
        const auto& current = *frame.value;
        const bool object = current.kind == Json::Kind::Object;
        const bool container = object || current.kind == Json::Kind::Array;
        if (!frame.started) {
            if (container) {
                if (stack.size() > kJsonMaxDepth) {
                    return false;
                }
                out += object ? '{' : '[';
                frame.started = true;
            } else {
                switch (current.kind) {
                    case Json::Kind::Null:
                        out += "null";
                        break;
                    case Json::Kind::Boolean:
                    case Json::Kind::Number:
                        out += current.text;
                        break;
                    case Json::Kind::String:
                        quote(out, current.text);
                        break;
                    case Json::Kind::Array:
                    case Json::Kind::Object:
                        break;
                }
                stack.pop_back();
            }
        } else {
            const std::size_t size = object ? current.members.size() : current.elements.size();
            if (frame.next == size) {
                out += object ? '}' : ']';
                stack.pop_back();
            } else {
                const auto index = frame.next++;
                if (index != 0) {
                    out += ',';
                }
                if (object) {
                    quote(out, current.members[index].first);
                    out += ':';
                    stack.push_back({&current.members[index].second});
                } else {
                    stack.push_back({&current.elements[index]});
                }
            }
        }
        if (out.size() > kJsonMaxSize) {
            return false;
        }
    }
    return true;
}

}  // namespace

const Json* Json::find(std::string_view key) const {
    const auto found = std::ranges::find(members, key, &decltype(members)::value_type::first);
    return found == members.end() ? nullptr : &found->second;
}

void Json::set(std::string key, Json value) {
    for (auto& member : members) {
        if (member.first == key) {
            member.second = std::move(value);
            return;
        }
    }
    members.emplace_back(std::move(key), std::move(value));
}

Result<Json> readJson(std::string_view bytes) {
    try {
        if (bytes.size() > kJsonMaxSize) {
            return Error{Error::Kind::TooLarge, 0, "manifest.json: JSON exceeds 1 MiB"};
        }
        if (!isValidUtf8(bytes)) {
            return malformed();
        }
        return Parser{bytes}.read();
    } catch (const std::exception&) {
        return Error{Error::Kind::TooLarge, 0, "unable to allocate native drawing data"};
    }
}

Result<std::string> writeJson(const Json& value) {
    try {
        std::string out;
        if (!serialize(value, out)) {
            return Error{Error::Kind::TooLarge, 0,
                         "manifest.json: JSON exceeds size or depth limit"};
        }
        const auto checked = readJson(out);
        if (!checked.ok()) {
            return checked.error();
        }
        return out;
    } catch (const std::exception&) {
        return Error{Error::Kind::TooLarge, 0, "unable to allocate native drawing data"};
    }
}

}  // namespace tg::io::detail
