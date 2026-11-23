// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Definitions and questions about drawings read from files
// (docs/verifica/domande.md, "File"; docs/adr/0018, point 6).

#include "new_entities_format.hpp"
#include "question_table.hpp"
#include "text.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/math.hpp"
#include "tg/io/block_import.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/script/case_files.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"
#include "tg/script/text_format.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

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
        // A handled number is compared as a number: AutoCAD and ezdxf pad
        // integers with spaces ("     4"), the model writes "4".
        const std::optional<double> x = io::parseNumber(detail::trim(old.value));
        const std::optional<double> y = io::parseNumber(detail::trim(now.value));
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

/// The text of a DXF value without the blanks around it ("  ATTRIB" as "ATTRIB").
std::string_view trimmed(std::string_view text) {
    const std::size_t begin = text.find_first_not_of(" \t\r");
    if (begin == std::string_view::npos) {
        return {};
    }
    return text.substr(begin, text.find_last_not_of(" \t\r") - begin + 1);
}

/// The handles of the ATTRIB kept inside an unhandled INSERT: its raw groups
/// hold a group 0 "ATTRIB" for each, followed by its groups (io keeps an
/// INSERT whole, with its ATTRIB and SEQEND). Empty for any other entity.
std::vector<std::uint64_t> rawAttributeHandles(const core::RawEntity& raw) {
    std::vector<std::uint64_t> handles;
    if (trimmed(raw.type) != "INSERT") {
        return handles;
    }
    bool attribute = false;  // inside an ATTRIB, before its handle
    for (const core::RawGroup& group : raw.groups) {
        if (group.code == 0) {
            attribute = trimmed(group.value) == "ATTRIB";
            if (attribute) {
                handles.push_back(0);  // an ATTRIB without a handle still counts
            }
        } else if (attribute && group.code == 5) {
            handles.back() = parseHandle(trimmed(group.value)).value_or(0);
            attribute = false;
        }
    }
    return handles;
}

/// The line of "entities:" of the ATTRIB `handle` of a handled INSERT,
/// without its identifier, or std::nullopt.
std::optional<std::string> attributeLine(const core::Document& document, std::uint64_t handle) {
    for (const core::EntityRecord& record : document.entities()) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr ||
            std::ranges::find(insert->attributes, core::EntityId{handle}, &core::Attribute::id) ==
                insert->attributes.end()) {
            continue;
        }
        const std::array<const core::EntityRecord*, 1> records{&record};
        for (const EntityLine& line : entityLines(records, document)) {
            if (line.id == handle) {
                return line.text.substr(line.text.find(' ') + 1);
            }
        }
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
                writeEntity(*record, document, text);
                return text;
            }
            if (std::optional<std::string> line = attributeLine(document, *handle)) {
                return *std::move(line);
            }
            const auto raw = std::ranges::find(document.rawEntities(), core::EntityId{*handle},
                                               &core::RawEntity::id);
            if (raw != document.rawEntities().end()) {
                return "unhandled " + raw->type;
            }
            // An ATTRIB of an unhandled INSERT stays inside it (docs/adr/0044,
            // point 4), with its own handle.
            const bool inside = std::ranges::any_of(
                document.rawEntities(), [&handle](const core::RawEntity& entity) {
                    const std::vector<std::uint64_t> handles = rawAttributeHandles(entity);
                    return std::ranges::find(handles, *handle) != handles.end();
                });
            return inside ? std::string{"unhandled ATTRIB"} : std::string{kNone};
        });
}

QuestionResult askCount(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(
        model, arguments, "count <drawing>", 0, [](const FileDrawing& drawing, Arguments /*more*/) {
            // The ATTRIB of an INSERT are counted as "statistics:" counts them.
            std::vector<const core::EntityRecord*> records;
            for (const core::EntityRecord& record : drawing.read.document.entities()) {
                records.push_back(&record);
            }
            std::string text;
            for (const std::string& item : statisticsLines(records)) {
                text += (text.empty() ? "" : "; ") + item;
            }
            return text.empty() ? std::string{kNone} : text;
        });
}

QuestionResult askUnhandled(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(
        model, arguments, "unhandled <drawing>", 0,
        [](const FileDrawing& drawing, Arguments /*more*/) {
            // An unhandled INSERT stays unhandled as a whole, with its ATTRIB
            // (docs/adr/0044, point 4): they are counted too.
            std::vector<std::string_view> types;
            for (const core::RawEntity& raw : drawing.read.document.rawEntities()) {
                types.emplace_back(raw.type);
                types.insert(types.end(), rawAttributeHandles(raw).size(), "ATTRIB");
            }
            return countText(types, [](std::string_view type) { return type; });
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

QuestionResult askGrid(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(
        model, arguments, "grid <drawing>", 0, [](const FileDrawing& drawing, Arguments /*more*/) {
            const core::GridSettings& grid = drawing.read.document.grid();
            const auto onOff = [](bool on) {
                return on ? "on" : "off";
            };
            return std::string{"grid="} + onOff(grid.gridOn) + " snap=" + onOff(grid.snapOn) +
                   " grid-unit=" + formatPoint(grid.gridUnit) +
                   " snap-unit=" + formatPoint(grid.snapUnit) +
                   " snap-base=" + formatPoint(grid.snapBase) +
                   " display=" + std::to_string(grid.gridDisplay) +
                   " major=" + std::to_string(grid.gridMajor) +
                   " ucsicon=" + std::to_string(grid.ucsIcon) +
                   " snap-angle=" + formatNumber(grid.snapAngle) +
                   " snap-style=" + std::to_string(grid.snapStyle);
        });
}

QuestionResult askAttdisp(const QuestionModel& model, Arguments arguments) {
    return askAboutDrawing(
        model, arguments, "attdisp <drawing>", 0,
        [](const FileDrawing& drawing, Arguments /*more*/) {
            // In the order of core::AttributeDisplay.
            constexpr std::array<std::string_view, 3> kNames{"off", "normal", "on"};
            return std::string{
                kNames.at(static_cast<std::size_t>(drawing.read.document.attributeDisplay()))};
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

// ---------------------------------------------------------------------------
// Importing a file as a block (docs/verifica/domande.md, "Importazione di un
// file come blocco"; package 0.8-PT19).

namespace {

/// The files of a definition BLOCK-IMPORT: it remembers whether reading the
/// file failed, an error of the definition rather than of the import.
class RecordingFiles final : public io::FileAccess {
public:
    explicit RecordingFiles(io::FileAccess& files) : m_files(files) {}

    [[nodiscard]] io::Result<std::string> read(std::string_view name) override {
        io::Result<std::string> bytes = m_files.read(name);
        if (!bytes.ok()) {
            m_failure = bytes.error();
        }
        return bytes;
    }

    [[nodiscard]] std::optional<io::Error> write(std::string_view name,
                                                 std::string_view bytes) override {
        return m_files.write(name, bytes);
    }

    [[nodiscard]] bool exists(std::string_view name) const override { return m_files.exists(name); }

    [[nodiscard]] const std::optional<io::Error>& failure() const { return m_failure; }

private:
    io::FileAccess& m_files;
    std::optional<io::Error> m_failure;
};

constexpr std::string_view kImportForm =
    "BLOCK-IMPORT source=<name> [target-units=N] or BLOCK-IMPORT file=<name> [target-units=N]";

/// "target-units=N", N an integer from 0 to 24.
std::optional<int> targetUnits(std::string_view argument) {
    constexpr std::string_view kPrefix = "target-units=";
    if (!argument.starts_with(kPrefix)) {
        return std::nullopt;
    }
    argument.remove_prefix(kPrefix.size());
    int units = -1;
    const char* end = argument.data() + argument.size();
    const auto [stop, error] = std::from_chars(argument.data(), end, units);
    if (argument.empty() || error != std::errc{} || stop != end || units < 0 ||
        units > core::Document::kMaxUnits) {
        return std::nullopt;
    }
    return units;
}

/// A handle in upper case hexadecimal, without prefix.
std::string hexHandle(std::uint64_t value) {
    std::array<char, 16> buffer{};
    const auto [end, error] =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
    std::string text(buffer.data(), error == std::errc{} ? end : buffer.data());
    std::ranges::transform(text, text.begin(), [](char c) {
        return c >= 'a' && c <= 'f' ? static_cast<char>(c - 'a' + 'A') : c;
    });
    return text;
}

std::string joinItems(const std::vector<std::string>& items) {
    if (items.empty()) {
        return std::string{kNone};
    }
    std::string text;
    for (const std::string& item : items) {
        text += (text.empty() ? "" : "; ") + item;
    }
    return text;
}

/// "item; item" with `text` of each element of `range`, or "none".
template <typename Range, typename Text>
std::string listText(const Range& range, const Text& text) {
    std::vector<std::string> items;
    items.reserve(std::size(range));
    for (const auto& item : range) {
        items.push_back(text(item));
    }
    return joinItems(items);
}

/// The import named `name`, or the error of the question file.
std::variant<const ImportedBlock*, ArgumentError> importArgument(const QuestionModel& model,
                                                                 std::string_view name) {
    const auto found = model.imports.find(name);
    if (found == model.imports.end()) {
        return ArgumentError{"no BLOCK-IMPORT named " + named(name) + " is defined above"};
    }
    return &found->second;
}

/// The question "<question> <import>" answered by `answer(import, names)`
/// when the import succeeded; "error: <message>" when it failed.
template <typename Answer>
QuestionResult askAboutImport(const QuestionModel& model, Arguments arguments,
                              std::string_view question, const Answer& answer) {
    if (arguments.size() != 1) {
        return wrongArguments(std::string{question} + " <import>");
    }
    auto found = importArgument(model, arguments[0]);
    if (auto* failure = std::get_if<ArgumentError>(&found)) {
        return std::move(*failure);
    }
    const ImportedBlock& imported = *std::get<const ImportedBlock*>(found);
    if (!imported.result) {
        return "error: " + imported.error.message;
    }
    return QuestionResult{answer(*imported.result, imported.names)};
}

/// A definition: name="NAME" base=x,y units=N entities=[H <entity>; …], the
/// handled and unhandled entities in the order of the definition, each
/// handled one followed by its ATTRIB.
std::string definitionText(const core::BlockDefinition& block, const core::Document& names) {
    std::vector<std::string> entities;
    auto handled = block.entities.begin();
    auto raw = block.rawEntities.begin();
    while (handled != block.entities.end() || raw != block.rawEntities.end()) {
        if (raw == block.rawEntities.end() ||
            (handled != block.entities.end() && handled->order <= raw->order)) {
            const std::array<const core::EntityRecord*, 1> alone{&*handled};
            for (const EntityLine& line : entityLines(alone, names)) {
                entities.push_back(hexHandle(line.id) + line.text.substr(line.text.find(' ')));
            }
            ++handled;
        } else {
            entities.push_back(hexHandle(raw->id.value) + " unhandled " +
                               std::string{trimmed(raw->type)});
            ++raw;
        }
    }
    std::string list;
    for (const std::string& entity : entities) {
        list += (list.empty() ? "" : "; ") + entity;
    }
    return "name=" + quoteText(block.name) + " base=" + formatPoint(block.base) +
           " units=" + std::to_string(block.units) +
           " entities=" + (entities.empty() ? std::string{kNone} : "[" + list + "]");
}

std::string recordStart(std::uint64_t id, std::string_view name) {
    return "id=" + hexHandle(id) + " name=" + quoteText(name);
}

std::string linetypeText(const core::Linetype& linetype) {
    std::string text = recordStart(linetype.id.value, linetype.name) + " pattern=";
    if (linetype.pattern.continuous()) {
        text += "continuous";
    }
    for (std::size_t i = 0; i < linetype.pattern.elements.size(); ++i) {
        text += (i == 0 ? "" : ",") + formatNumber(linetype.pattern.elements[i]);
    }
    return text + " description=" + quoteText(linetype.description);
}

std::string textStyleText(const core::TextStyle& style) {
    return recordStart(style.id, style.name) + " font=" + quoteText(style.font) +
           " height=" + formatNumber(style.fixedHeight) +
           " width-factor=" + formatNumber(style.widthFactor) +
           " oblique=" + formatNumber(core::radiansToDegrees(style.oblique)) +
           " backwards=" + (style.backwards ? "yes" : "no") +
           " upside-down=" + (style.upsideDown ? "yes" : "no");
}

/// Every essential variable, defaults included, NAME:value in alphabetical
/// order, so that the record does not depend on the target.
std::string dimStyleText(const core::DimStyle& style) {
    std::string text = recordStart(style.id, style.name);
    for (const core::DimVariableInfo& info : core::dimVariableInfos()) {
        const core::DimValue value = core::dimValue(style.vars, info.name);
        text += " " + std::string{info.name} + ":";
        if (const auto* number = std::get_if<double>(&value)) {
            text += formatNumber(*number);
        } else {
            text += std::get<std::string>(value);
        }
    }
    return text;
}

/// "code="value"" for each group, separated by blanks.
void appendGroups(const core::RawGroups& groups, std::string& out) {
    for (const core::RawGroup& group : groups) {
        out += (out.empty() ? "" : " ") + std::to_string(group.code) + "=" + quoteText(group.value);
    }
}

std::string handleGroup(std::uint64_t id) {
    return "5=" + quoteText(hexHandle(id));
}

std::string extrasGroups(std::uint64_t id, const core::EntityExtras& extras) {
    std::string text = handleGroup(id);
    for (const core::RawGroups* groups : {&extras.beforeEntity, &extras.common, &extras.specific,
                                          &extras.placement, &extras.extended}) {
        appendGroups(*groups, text);
    }
    return text;
}

/// The groups of the object with handle `id` in `block`, if it has one.
std::optional<std::string> blockGroups(const core::BlockDefinition& block, std::uint64_t id) {
    const std::array<std::pair<std::uint64_t, const core::RawGroups*>, 3> own{
        {{block.recordId, &block.recordExtras},
         {block.blockId, &block.blockExtras},
         {block.endBlockId, &block.endBlockExtras}}};
    for (const auto& [handle, groups] : own) {
        if (handle == id) {
            std::string text = handleGroup(id);
            appendGroups(*groups, text);
            return text;
        }
    }
    for (const core::EntityRecord& record : block.entities) {
        if (record.id.value == id) {
            return extrasGroups(id, record.extras);
        }
        if (const auto* insert = std::get_if<core::Insert>(&record.entity)) {
            for (const core::Attribute& attribute : insert->attributes) {
                if (attribute.id.value == id) {
                    return extrasGroups(id, attribute.extras);
                }
            }
        }
    }
    for (const core::RawEntity& raw : block.rawEntities) {
        if (raw.id.value == id) {
            // Every group kept, its type (group 0) first, the handle where it was.
            std::string text = "0=" + quoteText(trimmed(raw.type));
            appendGroups(raw.groups, text);
            return text;
        }
    }
    return std::nullopt;
}

/// The groups of the object with handle `id` returned by the import.
std::optional<std::string> importGroups(const io::BlockImport& imported, std::uint64_t id) {
    if (id == 0) {
        return std::nullopt;
    }
    if (auto text = blockGroups(imported.definition, id)) {
        return text;
    }
    for (const core::BlockDefinition& block : imported.nested) {
        if (auto text = blockGroups(block, id)) {
            return text;
        }
    }
    const auto record = [id](std::uint64_t handle, const core::RawGroups& extras) {
        std::optional<std::string> text;
        if (handle == id) {
            text = handleGroup(id);
            appendGroups(extras, *text);
        }
        return text;
    };
    for (const core::Layer& layer : imported.layers) {
        if (auto text = record(layer.id.value, layer.extras)) {
            return text;
        }
    }
    for (const core::Linetype& linetype : imported.linetypes) {
        if (auto text = record(linetype.id.value, linetype.extras)) {
            return text;
        }
    }
    for (const core::TextStyle& style : imported.textStyles) {
        if (auto text = record(style.id, style.extras)) {
            return text;
        }
    }
    for (const core::DimStyle& style : imported.dimStyles) {
        if (auto text = record(style.id, style.extras)) {
            return text;
        }
    }
    return std::nullopt;
}

/// The target with the layers and linetypes that enter, for their names.
core::Document namesDocument(core::Document target, const io::BlockImport& imported) {
    for (const core::Linetype& linetype : imported.linetypes) {
        target.addLinetype(linetype);
    }
    for (const core::Layer& layer : imported.layers) {
        target.addLayer(layer);
    }
    return target;
}

}  // namespace

std::optional<std::variant<ImportedBlock, FileDefinitionError>> readImportDefinition(
    std::string_view text, io::FileAccess* access, const core::Document& target) {
    const std::vector<std::string_view> words = splitWords(text);
    if (words.empty() || words.front() != "BLOCK-IMPORT") {
        return std::nullopt;
    }
    std::optional<std::string> name;
    std::optional<int> units;
    bool valid = words.size() == 2 || words.size() == 3;
    for (std::size_t i = 1; valid && i < words.size(); ++i) {
        if (auto file = fileArgument(words[i]); file && !name) {
            name = std::move(file);
        } else if (auto value = targetUnits(words[i]); value && !units) {
            units = value;
        } else {
            valid = false;
        }
    }
    if (!valid || !name) {
        return FileDefinitionError{"wrong arguments, expected: " + std::string{kImportForm}};
    }
    if (access == nullptr) {
        return FileDefinitionError{"cannot read " + *name +
                                   ": this question file cannot read files"};
    }
    core::Document copy = target;
    if (units) {
        copy.setUnits(*units);
    }
    RecordingFiles files{*access};
    const std::filesystem::path path{std::u8string{name->begin(), name->end()}};
    io::Result<io::BlockImport> imported = io::importBlock(path, copy, files);
    if (const std::optional<io::Error>& failure = files.failure()) {
        return FileDefinitionError{"cannot read " + *name + ": " + describe(*failure)};
    }
    if (!imported.ok()) {
        return ImportedBlock{nullptr, imported.error(), std::move(copy)};
    }
    core::Document names = namesDocument(std::move(copy), imported.value());
    return ImportedBlock{
        std::make_shared<const io::BlockImport>(std::move(imported).value()), {}, std::move(names)};
}

QuestionResult askImportError(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 1) {
        return wrongArguments("import-error <import>");
    }
    auto found = importArgument(model, arguments[0]);
    if (auto* failure = std::get_if<ArgumentError>(&found)) {
        return std::move(*failure);
    }
    const ImportedBlock& imported = *std::get<const ImportedBlock*>(found);
    return imported.result ? std::string{kNone} : quoteText(imported.error.message);
}

QuestionResult askImportDefinition(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-definition",
                          [](const io::BlockImport& imported, const core::Document& names) {
                              return definitionText(imported.definition, names);
                          });
}

QuestionResult askImportNested(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-nested",
                          [](const io::BlockImport& imported, const core::Document& names) {
                              return listText(imported.nested,
                                              [&names](const core::BlockDefinition& block) {
                                                  return definitionText(block, names);
                                              });
                          });
}

QuestionResult askImportLayers(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-layers",
                          [](const io::BlockImport& imported, const core::Document& names) {
                              // The layers that enter come last in `names`, in their order.
                              const std::vector<std::string> lines = layerLines(names);
                              const std::size_t first = lines.size() - imported.layers.size();
                              std::vector<std::string> items;
                              items.reserve(imported.layers.size());
                              for (std::size_t i = 0; i < imported.layers.size(); ++i) {
                                  items.push_back("id=" + hexHandle(imported.layers[i].id.value) +
                                                  " name=" + lines[first + i]);
                              }
                              return joinItems(items);
                          });
}

QuestionResult askImportLinetypes(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-linetypes",
                          [](const io::BlockImport& imported, const core::Document& /*names*/) {
                              return listText(imported.linetypes, linetypeText);
                          });
}

QuestionResult askImportTextStyles(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-text-styles",
                          [](const io::BlockImport& imported, const core::Document& /*names*/) {
                              return listText(imported.textStyles, textStyleText);
                          });
}

QuestionResult askImportDimStyles(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-dim-styles",
                          [](const io::BlockImport& imported, const core::Document& /*names*/) {
                              return listText(imported.dimStyles, dimStyleText);
                          });
}

QuestionResult askImportUnitFactor(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-unit-factor",
                          [](const io::BlockImport& imported, const core::Document& /*names*/) {
                              return formatNumber(imported.unitFactor);
                          });
}

QuestionResult askImportNextId(const QuestionModel& model, Arguments arguments) {
    return askAboutImport(model, arguments, "import-next-id",
                          [](const io::BlockImport& imported, const core::Document& /*names*/) {
                              return hexHandle(imported.nextId);
                          });
}

QuestionResult askImportGroups(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "import-groups <import> <handle>";
    if (arguments.size() != 2) {
        return wrongArguments(kForm);
    }
    const std::optional<std::uint64_t> handle = parseHandle(arguments[1]);
    if (!handle) {
        return wrongArguments(kForm);
    }
    auto found = importArgument(model, arguments[0]);
    if (auto* failure = std::get_if<ArgumentError>(&found)) {
        return std::move(*failure);
    }
    const ImportedBlock& imported = *std::get<const ImportedBlock*>(found);
    if (!imported.result) {
        return "error: " + imported.error.message;
    }
    std::optional<std::string> groups = importGroups(*imported.result, *handle);
    if (!groups) {
        return ArgumentError{named(arguments[0]) + " returned nothing with handle " +
                             named(arguments[1])};
    }
    return groups->empty() ? std::string{kNone} : *std::move(groups);
}

}  // namespace tg::script::detail
