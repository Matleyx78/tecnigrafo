// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// libc++ (Apple Clang, macOS) does not implement std::from_chars for floating
// point before LLVM 20, and does not define __cpp_lib_to_chars until it does.
// There the conversion falls back to a stream imbued with the classic "C"
// locale, as in cmd/src/coordinate_input.cpp. formatNumber() builds its text
// from the digits of a classic stream on every platform, so that the text is
// identical everywhere.

namespace tg::io {
namespace {

/// The longest line accepted, without its line end (docs/adr/0017, point 7).
constexpr std::size_t kMaxLineLength = std::size_t{64} * 1024;

/// The range of the DXF group codes (DXF Reference, "Group Codes in
/// Numerical Order"); -5 to -1 are the application codes.
constexpr std::int64_t kMinGroupCode = -5;
constexpr std::int64_t kMaxGroupCode = 1071;

/// The first bytes of a binary DXF (DXF Reference, "Binary DXF Files").
constexpr std::string_view kBinaryDxfSentinel = "AutoCAD Binary DXF";

/// The versions read (docs/adr/0017, point 2), and the first one in UTF-8.
constexpr std::array<std::string_view, 6> kSupportedVersions = {"AC1015", "AC1018", "AC1021",
                                                                "AC1024", "AC1027", "AC1032"};
constexpr std::string_view kFirstUtf8Version = "AC1021";
constexpr std::string_view kVersionRange = "AC1015 (AutoCAD 2000) to AC1032 (AutoCAD 2018)";

/// A handle is at most 16 hexadecimal digits (a 64-bit number).
constexpr std::size_t kMaxHandleDigits = 16;

/// Bytes of a value quoted in an error message; enough to recognise it.
constexpr std::size_t kMaxQuotedBytes = 40;

/// Spaces around a code or a number.
constexpr std::string_view kBlank = " \t";

/// Fixed notation for decimal exponents in this range, scientific outside
/// (as Python's repr() does): 0.0001 and 1e15 are fixed, 1e-05 and 1e+16 not.
constexpr int kMinFixedExponent = -4;
constexpr int kMaxFixedExponent = 15;

/// Seventeen significant digits always read back as the same double
/// (IEEE 754 binary64), so the search for the shortest text stops there.
constexpr int kMaxSignificantDigits = 17;

enum class ValueType : std::uint8_t { Text, Real, Int16, Int32, Int64, Boolean, Handle, Binary };

struct CodeRange {
    int first;
    int last;
    ValueType type;
};

// The value types of the group codes (DXF Reference, "Group Code Value
// Types"); codes not listed are text, so that nothing valid is refused.
constexpr auto kValueTypes = std::to_array<CodeRange>({
    {10, 59, ValueType::Real},       {60, 79, ValueType::Int16},
    {90, 99, ValueType::Int32},      {105, 105, ValueType::Handle},
    {110, 149, ValueType::Real},     {160, 169, ValueType::Int64},
    {170, 179, ValueType::Int16},    {210, 239, ValueType::Real},
    {270, 289, ValueType::Int16},    {290, 299, ValueType::Boolean},
    {310, 319, ValueType::Binary},   {320, 369, ValueType::Handle},
    {370, 389, ValueType::Int16},    {390, 399, ValueType::Handle},
    {400, 409, ValueType::Int16},    {420, 429, ValueType::Int32},
    {440, 459, ValueType::Int32},    {460, 469, ValueType::Real},
    {480, 481, ValueType::Handle},   {1004, 1004, ValueType::Binary},
    {1005, 1005, ValueType::Handle}, {1010, 1059, ValueType::Real},
    {1060, 1070, ValueType::Int16},  {1071, 1071, ValueType::Int32},
});

ValueType valueType(int code) {
    for (const CodeRange& range : kValueTypes) {
        if (code >= range.first && code <= range.last) {
            return range.type;
        }
    }
    return ValueType::Text;
}

/// What a value of `type` must be, for the error messages.
std::string_view expectation(ValueType type) {
    switch (type) {
        case ValueType::Real:
            return "a real number";
        case ValueType::Int16:
            return "a 16-bit integer";
        case ValueType::Int32:
            return "a 32-bit integer";
        case ValueType::Int64:
            return "a 64-bit integer";
        case ValueType::Boolean:
            return "a boolean (0 or 1)";
        case ValueType::Handle:
            return "a handle (1 to 16 hexadecimal digits)";
        case ValueType::Binary:
            return "binary data (pairs of hexadecimal digits)";
        case ValueType::Text:
            break;
    }
    return "text";
}

constexpr bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

std::string_view trimBlank(std::string_view text) {
    const std::size_t first = text.find_first_not_of(kBlank);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kBlank);
    return text.substr(first, last - first + 1);
}

/// `text` between quotes for a message: printable ASCII as is, other bytes
/// as \xNN (the message stays valid UTF-8), cut after kMaxQuotedBytes.
std::string quotedForMessage(std::string_view text) {
    constexpr std::string_view kHex = "0123456789ABCDEF";
    constexpr unsigned char kFirstPrintable = 0x20;
    constexpr unsigned char kLastPrintable = 0x7E;
    std::string out = "\"";
    const std::string_view shown = text.substr(0, kMaxQuotedBytes);
    for (const char c : shown) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte >= kFirstPrintable && byte <= kLastPrintable && c != '"' && c != '\\') {
            out += c;
        } else {
            out += "\\x";
            out += kHex[byte >> 4U];
            out += kHex[byte & 0xFU];
        }
    }
    out += '"';
    if (shown.size() < text.size()) {
        out += "...";
    }
    return out;
}

Error lineError(Error::Kind kind, std::size_t line, std::string_view message) {
    return Error{kind, line, "line " + std::to_string(line) + ": " + std::string{message}};
}

/// An integer written as `[+-] digits` within [minimum, maximum].
std::optional<std::int64_t> parseInteger(std::string_view text, std::int64_t minimum,
                                         std::int64_t maximum) {
    bool negative = false;
    if (!text.empty() && (text.front() == '+' || text.front() == '-')) {
        negative = text.front() == '-';
        text.remove_prefix(1);
    }
    if (text.empty()) {
        return std::nullopt;
    }
    constexpr std::uint64_t kTen = 10;
    constexpr std::uint64_t kLimit = std::numeric_limits<std::uint64_t>::max() / kTen;
    std::uint64_t magnitude = 0;
    for (const char c : text) {
        if (!isDigit(c) || magnitude > kLimit) {
            return std::nullopt;
        }
        magnitude = (magnitude * kTen) + static_cast<std::uint64_t>(c - '0');
    }
    // The magnitudes of the limits, computed without overflowing.
    const std::uint64_t maxNegative =
        minimum < 0 ? static_cast<std::uint64_t>(-(minimum + 1)) + 1 : 0;
    const std::uint64_t maxPositive = maximum > 0 ? static_cast<std::uint64_t>(maximum) : 0;
    if (negative) {
        if (magnitude > maxNegative && magnitude != 0) {
            return std::nullopt;
        }
        return magnitude == 0 ? 0 : -static_cast<std::int64_t>(magnitude - 1) - 1;
    }
    if (magnitude > maxPositive || static_cast<std::int64_t>(magnitude) < minimum) {
        return std::nullopt;
    }
    return static_cast<std::int64_t>(magnitude);
}

bool isHexValue(std::string_view text, std::size_t minDigits, std::size_t maxDigits) {
    if (text.size() < minDigits || text.size() > maxDigits) {
        return false;
    }
    return std::ranges::all_of(text, isHexDigit);
}

/// True if the value, as written, is valid for its type; text always is
/// (its encoding is checked while decoding).
bool isValidValue(ValueType type, std::string_view raw) {
    const std::string_view text = trimBlank(raw);
    switch (type) {
        case ValueType::Real:
            return parseNumber(text).has_value();
        case ValueType::Int16:
            return parseInteger(text, std::numeric_limits<std::int16_t>::min(),
                                std::numeric_limits<std::int16_t>::max())
                .has_value();
        case ValueType::Int32:
            return parseInteger(text, std::numeric_limits<std::int32_t>::min(),
                                std::numeric_limits<std::int32_t>::max())
                .has_value();
        case ValueType::Int64:
            return parseInteger(text, std::numeric_limits<std::int64_t>::min(),
                                std::numeric_limits<std::int64_t>::max())
                .has_value();
        case ValueType::Boolean:
            return text == "0" || text == "1";
        case ValueType::Handle:
            return isHexValue(text, 1, kMaxHandleDigits);
        case ValueType::Binary:
            return text.size() % 2 == 0 && isHexValue(text, 0, text.size());
        case ValueType::Text:
            break;
    }
    return true;
}

/// A group as found in the file, before its value is checked and decoded.
struct PendingGroup {
    std::int16_t code{0};
    std::string_view value;  ///< without the final "\r"
    std::size_t line{0};     ///< line of the code
};

/// Reads the line that starts at `position`, moving past its line end.
class LineReader {
public:
    explicit LineReader(std::string_view bytes) : m_bytes(bytes) {}

    [[nodiscard]] bool atEnd() const { return m_position >= m_bytes.size(); }

    [[nodiscard]] std::size_t lineNumber() const { return m_line; }

    [[nodiscard]] std::string_view rest() const { return m_bytes.substr(m_position); }

    /// Puts in `text` the next line, without its "\n" or "\r\n".
    /// Precondition: !atEnd().
    [[nodiscard]] std::optional<Error> next(std::string_view& text) {
        assert(!atEnd());
        ++m_line;
        const std::size_t newline = m_bytes.find('\n', m_position);
        const std::size_t end = newline == std::string_view::npos ? m_bytes.size() : newline;
        text = m_bytes.substr(m_position, end - m_position);
        m_position = newline == std::string_view::npos ? m_bytes.size() : newline + 1;
        if (!text.empty() && text.back() == '\r') {
            text.remove_suffix(1);
        }
        if (text.size() > kMaxLineLength) {
            return lineError(Error::Kind::TooLarge, m_line,
                             "the line is " + std::to_string(text.size()) +
                                 " bytes long; the longest line accepted is " +
                                 std::to_string(kMaxLineLength) + " bytes");
        }
        return std::nullopt;
    }

private:
    std::string_view m_bytes;
    std::size_t m_position{0};
    std::size_t m_line{0};
};

/// Splits the file into groups, checking only the codes and the structure.
Result<std::vector<PendingGroup>> splitGroups(std::string_view bytes,
                                              std::vector<std::string>& warnings) {
    std::vector<PendingGroup> groups;
    LineReader reader{bytes};
    while (!reader.atEnd()) {
        std::string_view codeLine;
        if (std::optional<Error> error = reader.next(codeLine)) {
            return std::move(*error);
        }
        const std::size_t line = reader.lineNumber();
        const std::optional<std::int64_t> code =
            parseInteger(trimBlank(codeLine), kMinGroupCode, kMaxGroupCode);
        if (!code) {
            return lineError(Error::Kind::Malformed, line,
                             "expected a group code (an integer from -5 to 1071), found " +
                                 quotedForMessage(codeLine));
        }
        if (reader.atEnd()) {
            return lineError(Error::Kind::Malformed, line,
                             "the file ends after group code " + std::to_string(*code) +
                                 ": expected its value on line " + std::to_string(line + 1));
        }
        std::string_view valueLine;
        if (std::optional<Error> error = reader.next(valueLine)) {
            return std::move(*error);
        }
        groups.push_back(PendingGroup{static_cast<std::int16_t>(*code), valueLine, line});
        if (*code == 0 && trimBlank(valueLine) == "EOF") {
            if (reader.rest().find_first_not_of(" \t\r\n") != std::string_view::npos) {
                warnings.push_back("line " + std::to_string(reader.lineNumber() + 1) +
                                   ": what follows the group 0 EOF was ignored");
            }
            break;
        }
    }
    return groups;
}

/// The group that holds the value of the header variable `name` (group 9),
/// or nullptr if the variable is absent.
Result<const PendingGroup*> headerValue(const std::vector<PendingGroup>& groups,
                                        std::string_view name, std::int16_t valueCode) {
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (groups[i].code != 9 || trimBlank(groups[i].value) != name) {
            continue;
        }
        if (i + 1 == groups.size() || groups[i + 1].code != valueCode) {
            const std::size_t line = groups[i].line + 2;
            const std::string found = i + 1 == groups.size()
                                          ? std::string{"the end of the file"}
                                          : "group code " + std::to_string(groups[i + 1].code);
            return lineError(Error::Kind::Malformed, line,
                             "expected group code " + std::to_string(valueCode) +
                                 " with the value of " + std::string{name} + ", found " + found);
        }
        return &groups[i + 1];
    }
    return nullptr;
}

bool isSupportedVersion(std::string_view version) {
    return std::ranges::find(kSupportedVersions, version) != kSupportedVersions.end();
}

/// True for the versions before AutoCAD 2000: "AC1009", "AC1014", and the
/// oldest ones written with a dot ("AC2.10").
bool isOlderVersion(std::string_view version) {
    constexpr std::string_view kPrefix = "AC";
    constexpr std::int64_t kFirstSupported = 1015;
    if (version.size() <= kPrefix.size() || version.substr(0, kPrefix.size()) != kPrefix) {
        return false;
    }
    const std::string_view number = version.substr(kPrefix.size());
    if (number.find('.') != std::string_view::npos) {
        return std::ranges::all_of(number, [](char c) { return isDigit(c) || c == '.'; });
    }
    return isDigit(number.front()) && parseInteger(number, 0, kFirstSupported - 1).has_value();
}

bool equalsIgnoringCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    const auto lower = [](char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c;
    };
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (lower(a[i]) != lower(b[i])) {
            return false;
        }
    }
    return true;
}

/// The encoding of the text (docs/adr/0017, point 3), with a warning when
/// it is a guess.
TextEncoding textEncoding(const GroupFile& file, std::vector<std::string>& warnings) {
    if (file.version.empty()) {
        warnings.emplace_back("no $ACADVER in the file: text read as UTF-8");
        return TextEncoding::Utf8;
    }
    if (file.version >= kFirstUtf8Version) {
        return TextEncoding::Utf8;
    }
    if (equalsIgnoringCase(file.codePage, "ANSI_1252")) {
        return TextEncoding::Ansi1252;
    }
    if (equalsIgnoringCase(file.codePage, "ANSI_1250")) {
        return TextEncoding::Ansi1250;
    }
    if (file.codePage.empty()) {
        warnings.emplace_back("no $DWGCODEPAGE in the file: text read as ANSI_1252");
    } else {
        warnings.push_back("code page " + quotedForMessage(file.codePage) +
                           " is not supported: text read as ANSI_1252");
    }
    return TextEncoding::Ansi1252;
}

/// `value` in scientific notation with `significantDigits` digits, as
/// "-d.ddde+XX", in the classic locale.
std::string scientific(double value, int significantDigits) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::scientific << std::setprecision(significantDigits - 1) << value;
    return stream.str();
}

/// The text written by scientific() as fixed or scientific notation, with
/// trailing zeros dropped but always one decimal digit.
std::string render(std::string_view text) {
    std::string out;
    std::size_t position = 0;
    if (text[position] == '-') {
        out += '-';
        ++position;
    }
    std::string digits;
    while (position < text.size() && text[position] != 'e') {
        if (isDigit(text[position])) {
            digits += text[position];
        }
        ++position;
    }
    while (digits.size() > 1 && digits.back() == '0') {
        digits.pop_back();
    }
    // The stream writes 'e', a sign and at least two digits.
    const bool negativeExponent = text[position + 1] == '-';
    int magnitude = 0;
    for (position += 2; position < text.size(); ++position) {
        magnitude = (magnitude * 10) + (text[position] - '0');
    }
    const int exponent = negativeExponent ? -magnitude : magnitude;

    if (exponent < kMinFixedExponent || exponent > kMaxFixedExponent) {
        out += digits.front();
        out += '.';
        out += digits.size() > 1 ? digits.substr(1) : "0";
        out += negativeExponent ? "e-" : "e+";
        if (magnitude < 10) {
            out += '0';
        }
        out += std::to_string(magnitude);
        return out;
    }
    if (exponent < 0) {
        out += "0.";
        out.append(static_cast<std::size_t>(-exponent - 1), '0');
        out += digits;
        return out;
    }
    const auto integerDigits = static_cast<std::size_t>(exponent) + 1;
    if (digits.size() <= integerDigits) {
        out += digits;
        out.append(integerDigits - digits.size(), '0');
        out += ".0";
        return out;
    }
    out.append(digits, 0, integerDigits);
    out += '.';
    out.append(digits, integerDigits);
    return out;
}

/// True if `text` is exactly `[+-] digits [. digits] [(e|E) [+-] digits]`,
/// with at least one mantissa digit. Checking the grammar here makes the
/// accepted syntax the same on every platform.
bool isDecimalNumber(std::string_view text) {
    std::size_t position = 0;
    const auto skipDigits = [&text, &position] {
        const std::size_t start = position;
        while (position < text.size() && isDigit(text[position])) {
            ++position;
        }
        return position - start;
    };
    if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
        ++position;
    }
    std::size_t mantissaDigits = skipDigits();
    if (position < text.size() && text[position] == '.') {
        ++position;
        mantissaDigits += skipDigits();
    }
    if (mantissaDigits == 0) {
        return false;
    }
    if (position < text.size() && (text[position] == 'e' || text[position] == 'E')) {
        ++position;
        if (position < text.size() && (text[position] == '+' || text[position] == '-')) {
            ++position;
        }
        if (skipDigits() == 0) {
            return false;
        }
    }
    return position == text.size();
}

}  // namespace

Result<GroupFile> readGroups(std::string_view bytes) {
    if (bytes.substr(0, kBinaryDxfSentinel.size()) == kBinaryDxfSentinel) {
        return lineError(Error::Kind::Unsupported, 1,
                         "found a binary DXF (\"AutoCAD Binary DXF\"); only ASCII DXF files "
                         "can be read");
    }
    if (bytes.empty()) {
        return Error{Error::Kind::Malformed, 0, "the file is empty: expected DXF groups"};
    }

    GroupFile file;
    Result<std::vector<PendingGroup>> split = splitGroups(bytes, file.warnings);
    if (!split.ok()) {
        return split.error();
    }
    const std::vector<PendingGroup>& pending = split.value();

    const Result<const PendingGroup*> version = headerValue(pending, "$ACADVER", 1);
    if (!version.ok()) {
        return version.error();
    }
    if (version.value() != nullptr) {
        const PendingGroup& group = *version.value();
        file.version = std::string{trimBlank(group.value)};
        if (!isSupportedVersion(file.version)) {
            const std::string what = isOlderVersion(file.version)
                                         ? "DXF version " + quotedForMessage(file.version) +
                                               " is older than AutoCAD 2000"
                                         : "unknown DXF version " + quotedForMessage(file.version);
            return lineError(Error::Kind::Unsupported, group.line + 1,
                             what + "; Tecnigrafo reads " + std::string{kVersionRange});
        }
    }
    const Result<const PendingGroup*> codePage = headerValue(pending, "$DWGCODEPAGE", 3);
    if (!codePage.ok()) {
        return codePage.error();
    }
    if (codePage.value() != nullptr) {
        file.codePage = std::string{trimBlank(codePage.value()->value)};
    }
    const TextEncoding encoding = textEncoding(file, file.warnings);

    for (const PendingGroup& group : pending) {
        const ValueType type = valueType(group.code);
        const std::size_t valueLine = group.line + 1;
        if (type != ValueType::Text) {
            if (!isValidValue(type, group.value)) {
                return lineError(Error::Kind::Malformed, valueLine,
                                 "expected " + std::string{expectation(type)} + " for group code " +
                                     std::to_string(group.code) + ", found " +
                                     quotedForMessage(group.value));
            }
            file.groups.push_back(core::RawGroup{group.code, std::string{group.value}});
        } else {
            std::optional<std::string> text = detail::decodeText(group.value, encoding);
            if (!text) {
                return lineError(Error::Kind::Malformed, valueLine,
                                 "expected UTF-8 text for group code " +
                                     std::to_string(group.code) + ", found invalid bytes in " +
                                     quotedForMessage(group.value));
            }
            file.groups.push_back(core::RawGroup{group.code, std::move(*text)});
        }
        file.lines.push_back(group.line);
    }
    return file;
}

std::string writeGroups(std::span<const core::RawGroup> groups, TextEncoding encoding) {
    constexpr std::size_t kCodeWidth = 3;
    std::string out;
    for (const core::RawGroup& group : groups) {
        assert(group.value.find_first_of("\r\n") == std::string::npos);
        const std::string code = std::to_string(group.code);
        if (code.size() < kCodeWidth) {
            out.append(kCodeWidth - code.size(), ' ');
        }
        out += code;
        out += "\r\n";
        out += detail::encodeText(group.value, encoding);
        out += "\r\n";
    }
    return out;
}

std::string formatNumber(double value) {
    assert(std::isfinite(value));
    if (std::isnan(value)) {
        return "nan";  // a broken precondition stays visible in the file
    }
    if (std::isinf(value)) {
        return value > 0.0 ? "inf" : "-inf";
    }
    if (value == 0.0) {
        return std::signbit(value) ? "-0.0" : "0.0";
    }
    // The fewest significant digits that read back as the same value. The
    // stream rounds correctly on every supported platform, so the text with
    // 17 digits always reads back exactly.
    std::string text;
    for (int digits = 1; digits <= kMaxSignificantDigits; ++digits) {
        text = scientific(value, digits);
        const std::optional<double> back = parseNumber(text);
        if (back && *back == value) {
            break;
        }
    }
    return render(text);
}

std::optional<double> parseNumber(std::string_view text) {
    if (!isDecimalNumber(text)) {
        return std::nullopt;
    }
    // std::from_chars does not accept a leading '+'.
    if (text.front() == '+') {
        text.remove_prefix(1);
    }
    double value = 0.0;
#if defined(__cpp_lib_to_chars)
    const char* const end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value);
    if (error != std::errc{} || stop != end) {
        return std::nullopt;
    }
#else
    std::istringstream stream{std::string{text}};
    stream.imbue(std::locale::classic());
    stream >> value;
    if (stream.fail()) {
        return std::nullopt;
    }
#endif
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

}  // namespace tg::io
