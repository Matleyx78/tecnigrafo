// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_form.hpp"

#include "text.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::script {
namespace {

constexpr std::string_view kScriptFile = "script.tgs";
constexpr std::string_view kExpectedFile = "atteso.txt";
constexpr std::string_view kQuestionsFile = "domande.tgq";

constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";

// The messages of docs/tappe/fuori-tappa/F1-forma-dei-casi.md, in the order
// the rules are checked.
constexpr std::string_view kCarriageReturn = "carriage return: lines end with LF only";
constexpr std::string_view kTab = "tab character";
constexpr std::string_view kEmptyLineAtEnd = "empty line at the end of the file";
constexpr std::string_view kEmptyLineNotBeforeSection = "empty line not followed by a section";
constexpr std::string_view kTrailingSpaces = "trailing spaces";
constexpr std::string_view kNoEcho =
    "no echo of this line in the messages of atteso.txt (an empty line is Enter: at Command: "
    "it repeats the last command)";

// The report (docs/verifica/rapporto.md, "Struttura"): a list is its header,
// a line ending with ':', followed by one line per element indented by two
// spaces; a section starts with '['.
constexpr std::string_view kMessagesHeader = "messages:";
constexpr std::string_view kListIndent = "  ";
constexpr char kSectionStart = '[';

/// The echo of a typed line: the prompt, a space and the text
/// (docs/verifica/rapporto.md, "La cronologia").
constexpr std::string_view kEchoSeparator = ": ";
constexpr char kPromptEnd = ':';

using Lines = std::vector<std::string_view>;

std::string_view withoutBom(std::string_view text) {
    if (text.starts_with(kUtf8Bom)) {
        text.remove_prefix(kUtf8Bom.size());
    }
    return text;
}

/// The lines of `text`, split at LF only, so that a CR stays in its line. A
/// final LF does not add an empty line (docs/verifica/script.md, "Il file").
Lines splitAtLineFeeds(std::string_view text) {
    Lines lines;
    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return lines;
}

FormError error(std::string_view file, std::size_t index, std::string_view message) {
    return {std::string{file}, static_cast<int>(index) + 1, std::string{message}};
}

/// A file read with its name.
struct File {
    std::string_view name;
    Lines lines;
};

File readFile(std::string_view name, std::string_view text) {
    return {name, splitAtLineFeeds(withoutBom(text))};
}

std::optional<FormError> findCharacter(const File& file, char character, std::string_view message) {
    for (std::size_t i = 0; i < file.lines.size(); ++i) {
        if (file.lines[i].find(character) != std::string_view::npos) {
            return error(file.name, i, message);
        }
    }
    return std::nullopt;
}

/// The first of the empty lines that end the file, if any.
std::optional<FormError> findEmptyLinesAtEnd(const File& file) {
    std::size_t end = file.lines.size();
    while (end > 0 && file.lines[end - 1].empty()) {
        --end;
    }
    if (end == file.lines.size()) {
        return std::nullopt;
    }
    return error(file.name, end, kEmptyLineAtEnd);
}

std::optional<FormError> findEmptyLineNotBeforeSection(const File& file) {
    for (std::size_t i = 0; i < file.lines.size(); ++i) {
        if (!file.lines[i].empty()) {
            continue;
        }
        if (i + 1 == file.lines.size() || !file.lines[i + 1].starts_with(kSectionStart)) {
            return error(file.name, i, kEmptyLineNotBeforeSection);
        }
    }
    return std::nullopt;
}

bool hasTrailingSpace(std::string_view line) {
    return line.ends_with(' ');
}

/// Comments and directives: the first character decides
/// (docs/verifica/script.md, "Il file").
bool isTypedLine(std::string_view line) {
    return line.empty() || (line.front() != ';' && line.front() != '!');
}

/// Trailing spaces in the script stand only on typed lines, which are text
/// written by the user.
std::optional<FormError> findScriptTrailingSpaces(const File& script) {
    for (std::size_t i = 0; i < script.lines.size(); ++i) {
        const std::string_view line = script.lines[i];
        if (!isTypedLine(line) && hasTrailingSpace(line)) {
            return error(script.name, i, kTrailingSpaces);
        }
    }
    return std::nullopt;
}

/// Trailing spaces in atteso.txt stand only on the echo of a typed line of
/// the script that ends with them: a line ending with ": " and that text.
std::optional<FormError> findExpectedTrailingSpaces(const File& expected, const File& script) {
    std::vector<std::string> echoes;
    for (const std::string_view line : script.lines) {
        if (isTypedLine(line) && hasTrailingSpace(line)) {
            echoes.push_back(std::string{kEchoSeparator} + std::string{line});
        }
    }
    for (std::size_t i = 0; i < expected.lines.size(); ++i) {
        const std::string_view line = expected.lines[i];
        if (!hasTrailingSpace(line)) {
            continue;
        }
        const bool isEcho = std::ranges::any_of(
            echoes, [line](const std::string& echo) { return line.ends_with(echo); });
        if (!isEcho) {
            return error(expected.name, i, kTrailingSpaces);
        }
    }
    return std::nullopt;
}

std::optional<FormError> findAnyTrailingSpaces(const File& file) {
    for (std::size_t i = 0; i < file.lines.size(); ++i) {
        if (hasTrailingSpace(file.lines[i])) {
            return error(file.name, i, kTrailingSpaces);
        }
    }
    return std::nullopt;
}

bool needsLevel2(const File& script) {
    return std::ranges::any_of(script.lines, [](std::string_view line) {
        const std::vector<std::string_view> words = detail::splitWords(line);
        return words.size() == 2 && words[0] == "!level" && words[1] == "2";
    });
}

/// The lines of every "messages:" list of the report, in the order of the
/// sections, without their indentation.
Lines messages(const File& expected) {
    Lines lines;
    bool inList = false;
    for (const std::string_view line : expected.lines) {
        if (line == kMessagesHeader) {
            inList = true;
        } else if (inList && line.starts_with(kListIndent)) {
            lines.push_back(line.substr(kListIndent.size()));
        } else {
            inList = false;
        }
    }
    return lines;
}

/// The first message from `from` on that is the echo of the non-empty typed
/// line `text`, or messages.size().
std::size_t findEcho(const Lines& messages, std::size_t from, std::string_view text) {
    const std::string echo = std::string{kEchoSeparator} + std::string{text};
    for (std::size_t i = from; i < messages.size(); ++i) {
        if (messages[i].ends_with(echo)) {
            return i;
        }
    }
    return messages.size();
}

/// Every typed line has its echo, in order, each after the one of the line
/// before (docs/adr/0037, point 3). The echo of an empty line, the prompt
/// alone, comes before the echo of the next non-empty typed line.
std::optional<FormError> findMissingEcho(const File& script, const File& expected) {
    const Lines list = messages(expected);
    std::vector<std::size_t> typed;
    for (std::size_t i = 0; i < script.lines.size(); ++i) {
        if (isTypedLine(script.lines[i])) {
            typed.push_back(i);
        }
    }
    std::size_t next = 0;  // the first message not yet taken by an echo
    for (std::size_t k = 0; k < typed.size(); ++k) {
        const std::string_view text = script.lines[typed[k]];
        if (!text.empty()) {
            const std::size_t found = findEcho(list, next, text);
            if (found == list.size()) {
                return error(script.name, typed[k], kNoEcho);
            }
            next = found + 1;
            continue;
        }
        std::size_t limit = list.size();
        for (std::size_t later = k + 1; later < typed.size(); ++later) {
            if (!script.lines[typed[later]].empty()) {
                limit = findEcho(list, next, script.lines[typed[later]]);
                break;
            }
        }
        std::size_t found = next;
        while (found < limit && !list[found].ends_with(kPromptEnd)) {
            ++found;
        }
        if (found >= limit) {
            return error(script.name, typed[k], kNoEcho);
        }
        next = found + 1;
    }
    return std::nullopt;
}

}  // namespace

std::optional<FormError> checkScriptForm(std::string_view script, std::string_view expected) {
    const File scriptFile = readFile(kScriptFile, script);
    const File expectedFile = readFile(kExpectedFile, expected);
    for (const File* file : {&scriptFile, &expectedFile}) {
        if (auto found = findCharacter(*file, '\r', kCarriageReturn)) {
            return found;
        }
    }
    for (const File* file : {&scriptFile, &expectedFile}) {
        if (auto found = findCharacter(*file, '\t', kTab)) {
            return found;
        }
    }
    // Empty lines at the end of the script are Enter: they are typed lines,
    // whose echo the last check looks for (docs/adr/0037, point 2.4).
    if (auto found = findEmptyLinesAtEnd(expectedFile)) {
        return found;
    }
    if (auto found = findEmptyLineNotBeforeSection(expectedFile)) {
        return found;
    }
    if (auto found = findScriptTrailingSpaces(scriptFile)) {
        return found;
    }
    if (auto found = findExpectedTrailingSpaces(expectedFile, scriptFile)) {
        return found;
    }
    // At level 2 a space typed on the keyboard is Enter where the command
    // does not ask for a text: the echo of a line is not known in advance.
    if (needsLevel2(scriptFile)) {
        return std::nullopt;
    }
    return findMissingEcho(scriptFile, expectedFile);
}

std::optional<FormError> checkQuestionsForm(std::string_view questions) {
    const File file = readFile(kQuestionsFile, questions);
    if (auto found = findCharacter(file, '\r', kCarriageReturn)) {
        return found;
    }
    if (auto found = findCharacter(file, '\t', kTab)) {
        return found;
    }
    if (auto found = findEmptyLinesAtEnd(file)) {
        return found;
    }
    return findAnyTrailingSpaces(file);
}

}  // namespace tg::script
