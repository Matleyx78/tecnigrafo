// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Definitions and questions about drawings read from files
// (docs/verifica/domande.md, "File"; docs/adr/0018, point 6).

#include "question_table.hpp"
#include "text.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/script/case_files.hpp"
#include "tg/script/report.hpp"
#include "tg/script/text_format.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <map>
#include <span>
#include <string>
#include <system_error>
#include <utility>

namespace tg::script::detail {
namespace {

using Arguments = std::span<const std::string_view>;

constexpr std::string_view kNone = "none";

std::string named(std::string_view text) {
    return "'" + std::string{text} + "'";
}

ArgumentError wrongArguments(std::string_view form) {
    return ArgumentError{"wrong arguments, expected: " + std::string{form}};
}

/// The message of an io error, with its line as the file commands write it.
std::string describe(const io::Error& error) {
    return (error.line > 0 ? "line " + std::to_string(error.line) + ": " : "") + error.message;
}

/// A handle written in hexadecimal, in either case, never 0.
std::optional<std::uint64_t> parseHandle(std::string_view text) {
    std::uint64_t value = 0;
    const char* end = text.data() + text.size();
    const auto [stop, error] = std::from_chars(text.data(), end, value, 16);
    if (text.empty() || error != std::errc{} || stop != end || value == 0) {
        return std::nullopt;
    }
    return value;
}

/// "source=<name>" or "file=<name>" as a name for io::FileAccess.
std::optional<std::string> fileArgument(std::string_view argument) {
    constexpr std::string_view kSource = "source=";
    constexpr std::string_view kFile = "file=";
    if (argument.starts_with(kSource) && argument.size() > kSource.size()) {
        return std::string{kSourcesPrefix} + std::string{argument.substr(kSource.size())};
    }
    if (argument.starts_with(kFile) && argument.size() > kFile.size()) {
        return std::string{argument.substr(kFile.size())};
    }
    return std::nullopt;
}

/// The bytes of the file `name`: an error says which file and why.
io::Result<std::string> readBytes(io::FileAccess* access, const std::string& name) {
    if (access == nullptr) {
        return io::Error{io::Error::Kind::File, 0, "this question file cannot read files"};
    }
    io::Result<std::string> bytes = access->read(name);
    if (!bytes.ok()) {
        return io::Error{bytes.error().kind, 0,
                         "cannot read " + name + ": " + describe(bytes.error())};
    }
    return bytes;
}

/// "TYPE n; TYPE n" in alphabetical order, or "none".
template <typename Range, typename TypeOf>
std::string countText(const Range& range, const TypeOf& typeOf) {
    std::map<std::string, int, std::less<>> counts;
    for (const auto& item : range) {
        ++counts[std::string{typeOf(item)}];
    }
    std::string text;
    for (const auto& [type, count] : counts) {
        text += (text.empty() ? "" : "; ") + type + " " + std::to_string(count);
    }
    return text.empty() ? std::string{kNone} : text;
}

/// The question "<question> <drawing> [<more>…]" with `more` arguments at
/// most, answered by `answer(drawing, more)`.
template <typename Answer>
QuestionResult askAboutDrawing(const QuestionModel& model, Arguments arguments,
                               std::string_view form, std::size_t more, const Answer& answer) {
    if (arguments.empty() || arguments.size() > more + 1) {
        return wrongArguments(form);
    }
    const auto found = model.files.find(arguments[0]);
    if (found == model.files.end()) {
        return ArgumentError{"no drawing named " + named(arguments[0]) + " is read above"};
    }
    return QuestionResult{answer(found->second, arguments.subspan(1))};
}

// ---------------------------------------------------------------------------
// roundtrip: the definition of "identical" of docs/adr/0018, point 6.

/// The DXF version of $ACADVER, in the order of io::DxfVersion.
std::optional<io::DxfVersion> versionOf(std::string_view acadver) {
    constexpr std::array<std::string_view, 6> kNames{"AC1015", "AC1018", "AC1021",
                                                     "AC1024", "AC1027", "AC1032"};
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (kNames.at(i) == acadver) {
            return static_cast<io::DxfVersion>(i);
        }
    }
    return std::nullopt;
}

std::string groupText(const core::RawGroup& group) {
    return std::to_string(group.code) + " " + quoteText(group.value);
}

/// True if the entity starting at group `i` is handled by the model.
bool isHandled(const core::RawGroups& groups, std::size_t i, const core::Document& document) {
    for (std::size_t j = i + 1; j < groups.size() && groups[j].code != 0; ++j) {
        if (groups[j].code == 5) {
            const std::optional<std::uint64_t> handle = parseHandle(groups[j].value);
            return handle && document.find(core::EntityId{*handle}) != nullptr;
        }
    }
    return false;
}

/// The first difference between the groups of the original file and those
/// of the rewritten one, or std::nullopt.
std::optional<std::string> firstDifference(const io::GroupFile& original,
                                           const io::GroupFile& rewritten,
                                           const core::Document& document,
                                           const Tolerances& tolerances) {
    const core::RawGroups& before = original.groups;
    const core::RawGroups& after = rewritten.groups;
    const bool converting = original.version != rewritten.version;
    std::string_view section;
    std::string_view variable;
    bool handled = false;
    for (std::size_t i = 0; i < before.size(); ++i) {
        const core::RawGroup& old = before[i];
        const std::string where = "line " + std::to_string(original.lines[i]) + ": ";
        if (i == after.size()) {
            return where + groupText(old) + " is missing";
        }
        if (old.code == 2 && i > 0 && before[i - 1].code == 0 && before[i - 1].value == "SECTION") {
            section = old.value;
        } else if (old.code == 9 && section == "HEADER") {
            variable = old.value;
        } else if (old.code == 0) {
            variable = {};
            handled = section == "ENTITIES" && isHandled(before, i, document);
        }
        const core::RawGroup& now = after[i];
        // Only $HANDSEED changes, and converting also the version and the
        // code page. Handled entities are written again from the model:
        // their numbers match within the tolerances.
        const bool free = variable == "$HANDSEED" ||
                          (converting && (variable == "$ACADVER" || variable == "$DWGCODEPAGE"));
        const std::optional<double> x = io::parseNumber(old.value);
        const std::optional<double> y = io::parseNumber(now.value);
        const bool equal = old.value == now.value || (old.code != 9 && free) ||
                           (handled && x && y && numbersMatch(*x, *y, tolerances));
        if (old.code != now.code || !equal) {
            return where + groupText(old) + " became " + groupText(now);
        }
    }
    if (after.size() > before.size()) {
        return "the rewritten file has more groups: " + groupText(after[before.size()]);
    }
    return std::nullopt;
}

std::string roundtrip(const FileDrawing& drawing, bool toCadx, const Tolerances& tolerances) {
    const core::Document& document = drawing.read.document;
    const io::DxfVersion version =
        drawing.cadx ? io::DxfVersion::R2018
                     : versionOf(document.raw().version).value_or(io::DxfVersion::R2018);
    // A .cadx starts from its drawing.dxf as the model writes it: tg_script
    // does not open archives.
    const io::Result<std::string> original =
        drawing.cadx ? io::writeDxf(document, io::DxfVersion::R2018) : drawing.bytes;
    const io::Result<std::string> rewritten =
        toCadx ? io::writeCadx(document) : io::writeDxf(document, version);
    if (!original.ok() || !rewritten.ok()) {
        return "error: " + describe(original.ok() ? rewritten.error() : original.error());
    }
    const io::Result<io::ReadResult> reread =
        toCadx ? io::readCadx(rewritten.value()) : io::readDxf(rewritten.value());
    if (!reread.ok()) {
        return "different: the rewritten file cannot be read: " + describe(reread.error());
    }
    const io::Result<std::string> rewrittenDxf =
        toCadx ? io::writeDxf(reread.value().document, io::DxfVersion::R2018) : rewritten;
    if (!rewrittenDxf.ok()) {
        return "error: " + describe(rewrittenDxf.error());
    }
    const io::Result<io::GroupFile> before = io::readGroups(original.value());
    const io::Result<io::GroupFile> after = io::readGroups(rewrittenDxf.value());
    if (!before.ok() || !after.ok()) {
        return "different: the groups cannot be read: " +
               describe(before.ok() ? after.error() : before.error());
    }
    const std::optional<std::string> difference =
        firstDifference(before.value(), after.value(), document, tolerances);
    return difference ? "different: " + *difference : std::string{"identical"};
}

}  // namespace

std::optional<std::variant<FileDrawing, FileDefinitionError>> readFileDefinition(
    std::string_view text, io::FileAccess* access) {
    const std::vector<std::string_view> words = splitWords(text);
    if (words.empty() || (words.front() != "DXF" && words.front() != "CADX")) {
        return std::nullopt;
    }
    const bool cadx = words.front() == "CADX";
    const std::optional<std::string> name =
        words.size() == 2 ? fileArgument(words[1]) : std::nullopt;
    if (!name) {
        return FileDefinitionError{"wrong arguments, expected: " + std::string{words.front()} +
                                   " source=<name> or " + std::string{words.front()} +
                                   " file=<name>"};
    }
    io::Result<std::string> bytes = readBytes(access, *name);
    if (!bytes.ok()) {
        return FileDefinitionError{bytes.error().message};
    }
    io::Result<io::ReadResult> read =
        cadx ? io::readCadx(bytes.value()) : io::readDxf(bytes.value());
    if (!read.ok()) {
        return FileDefinitionError{"cannot read " + *name + ": " + describe(read.error()),
                                   read.error().kind == io::Error::Kind::NotImplemented};
    }
    return FileDrawing{cadx, std::move(bytes).value(), std::move(read).value()};
}

std::optional<std::variant<const core::Entity*, UnhandledFileEntity, std::string>> fileEntity(
    const FileDrawings& files, std::string_view name) {
    const std::size_t colon = name.find(':');
    const auto drawing =
        colon == std::string_view::npos ? files.end() : files.find(name.substr(0, colon));
    if (drawing == files.end()) {
        return std::nullopt;
    }
    const std::optional<std::uint64_t> handle = parseHandle(name.substr(colon + 1));
    const core::EntityRecord* record =
        handle ? drawing->second.read.document.find(core::EntityId{*handle}) : nullptr;
    if (record != nullptr) {
        return &record->entity;
    }
    if (handle) {
        const std::span<const core::RawEntity> raw = drawing->second.read.document.rawEntities();
        const auto found = std::ranges::find(raw, core::EntityId{*handle}, &core::RawEntity::id);
        if (found != raw.end()) {
            return UnhandledFileEntity{found->type};
        }
    }
    return drawing->first + " has no entity with handle " + named(name.substr(colon + 1));
}

QuestionResult askEntity(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "entity <drawing> <handle>";
    if (arguments.size() != 2) {
        return wrongArguments(kForm);
    }
    return askAboutDrawing(
        model, arguments, kForm, 1,
        [kForm](const FileDrawing& drawing, Arguments more) -> QuestionResult {
            const std::optional<std::uint64_t> handle = parseHandle(more[0]);
            if (!handle) {
                return wrongArguments(kForm);
            }
            const core::Document& document = drawing.read.document;
            if (const core::EntityRecord* record = document.find(core::EntityId{*handle})) {
                std::string text;
                writeEntity(record->entity, text);
                return text;
            }
            const auto raw = std::ranges::find(document.rawEntities(), core::EntityId{*handle},
                                               &core::RawEntity::id);
            return raw == document.rawEntities().end() ? std::string{kNone}
                                                       : "unhandled " + raw->type;
        });
}

QuestionResult askCount(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(
        model, arguments, "count <drawing>", 0, [](const FileDrawing& drawing, Arguments /*more*/) {
            return countText(
                drawing.read.document.entities(),
                [](const core::EntityRecord& record) { return dxfType(record.entity); });
        });
}

QuestionResult askUnhandled(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(model, arguments, "unhandled <drawing>", 0,
                           [](const FileDrawing& drawing, Arguments /*more*/) {
                               return countText(drawing.read.document.rawEntities(),
                                                [](const core::RawEntity& raw) -> std::string_view {
                                                    return raw.type;
                                                });
                           });
}

QuestionResult askVersion(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(model, arguments, "version <drawing>", 0,
                           [](const FileDrawing& drawing, Arguments /*more*/) {
                               const std::string& version = drawing.read.document.raw().version;
                               return version.empty() ? std::string{kNone} : version;
                           });
}

QuestionResult askUnits(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(model, arguments, "units <drawing>", 0,
                           [](const FileDrawing& drawing, Arguments /*more*/) {
                               return std::to_string(drawing.read.document.units());
                           });
}

QuestionResult askWarnings(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(model, arguments, "warnings <drawing>", 0,
                           [](const FileDrawing& drawing, Arguments /*more*/) {
                               std::string text;
                               for (const std::string& warning : drawing.read.report.warnings) {
                                   text += (text.empty() ? "" : "; ") + warning;
                               }
                               return text.empty() ? std::string{kNone} : text;
                           });
}

QuestionResult askRoundtrip(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "roundtrip <drawing> [as=dxf|cadx]";
    return askAboutDrawing(
        model, arguments, kForm, 1,
        [&model, kForm](const FileDrawing& drawing, Arguments more) -> QuestionResult {
            if (!more.empty() && more[0] != "as=dxf" && more[0] != "as=cadx") {
                return wrongArguments(kForm);
            }
            const bool toCadx = more.empty() ? drawing.cadx : more[0] == "as=cadx";
            return roundtrip(drawing, toCadx, model.tolerances);
        });
}

QuestionResult askOpenError(const QuestionModel& model, Arguments arguments) {
    const std::optional<std::string> name =
        arguments.size() == 1 ? fileArgument(arguments[0]) : std::nullopt;
    if (!name) {
        return wrongArguments("open-error source=<name> or open-error file=<name>");
    }
    // The format comes from the extension, as for OPEN; the public files of
    // sorgenti.json have none and are DXF.
    const bool cadx = name->ends_with(".cadx");
    if (!cadx && !name->ends_with(".dxf") && !name->starts_with(kSourcesPrefix)) {
        return ArgumentError{"open-error reads .dxf and .cadx files, not " + named(*name)};
    }
    const io::Result<std::string> bytes = readBytes(model.access, *name);
    if (!bytes.ok()) {
        return ArgumentError{bytes.error().message};
    }
    const io::Result<io::ReadResult> read =
        cadx ? io::readCadx(bytes.value()) : io::readDxf(bytes.value());
    return read.ok() ? std::string{kNone} : quoteText(describe(read.error()));
}

}  // namespace tg::script::detail
