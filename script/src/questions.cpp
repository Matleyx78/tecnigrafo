// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/questions.hpp"

#include "question_table.hpp"
#include "text.hpp"
#include "tg/core/document.hpp"
#include "tg/core/start_handles.hpp"
#include "tg/edit/editor.hpp"
#include "tg/script/entity_reader.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>

namespace tg::script {
namespace {

using detail::AnswerShape;

constexpr std::string_view kArrow = "->";
constexpr std::string_view kNone = "none";
constexpr std::string_view kNotImplemented = "error: question not implemented yet";

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

std::string join(std::span<const std::string_view> words) {
    std::string text;
    for (const std::string_view word : words) {
        if (!text.empty()) {
            text += ' ';
        }
        text += word;
    }
    return text;
}

/// A name of a definition: a letter or '_', then letters, digits and '_'.
/// "none" is not a name: answers use it for "no entity".
bool isName(std::string_view text) {
    const auto letter = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    return !text.empty() && letter(text.front()) && text != kNone &&
           std::ranges::all_of(text, [&](char c) { return letter(c) || (c >= '0' && c <= '9'); });
}

/// The items of a list answer: "none" is the empty list.
std::vector<std::string> listItems(std::string_view answer, AnswerShape shape) {
    std::vector<std::string> items;
    if (detail::trim(answer) == kNone) {
        return items;
    }
    if (shape == AnswerShape::Names) {
        for (const std::string_view word : detail::splitWords(answer)) {
            items.emplace_back(word);
        }
        return items;
    }
    while (true) {
        const std::size_t semicolon = answer.find(';');
        items.emplace_back(detail::trim(answer.substr(0, semicolon)));
        if (semicolon == std::string_view::npos) {
            return items;
        }
        answer.remove_prefix(semicolon + 1);
    }
}

/// Lists compare as multisets: every expected item matches one obtained
/// item, in any order.
bool listsMatch(std::string_view expected, std::string_view actual, AnswerShape shape,
                const Tolerances& tolerances) {
    const std::vector<std::string> wanted = listItems(expected, shape);
    const std::vector<std::string> got = listItems(actual, shape);
    if (wanted.size() != got.size()) {
        return false;
    }
    std::vector<bool> used(got.size(), false);
    for (const std::string& item : wanted) {
        bool found = false;
        for (std::size_t i = 0; i < got.size() && !found; ++i) {
            if (!used[i] && linesMatch(item, got[i], tolerances)) {
                used[i] = true;
                found = true;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool answerMatches(AnswerShape shape, std::string_view expected, std::string_view actual,
                   const Tolerances& tolerances) {
    if (shape == AnswerShape::Single) {
        return linesMatch(expected, actual, tolerances);
    }
    return listsMatch(expected, actual, shape, tolerances);
}

/// Reads a whole .tgq file, line by line.
class QuestionFile {
public:
    explicit QuestionFile(io::FileAccess* access, bool pending = false)
        : m_editor(m_document), m_access(access), m_pending(pending) {}

    QuestionFile(const QuestionFile&) = delete;
    QuestionFile& operator=(const QuestionFile&) = delete;
    QuestionFile(QuestionFile&&) = delete;
    QuestionFile& operator=(QuestionFile&&) = delete;
    ~QuestionFile() = default;

    /// Handles one line; std::nullopt if it is fine.
    std::optional<ParseError> read(int line, std::string_view text) {
        m_line = line;
        text = detail::trim(text);
        if (text.empty() || text.starts_with(';')) {
            return std::nullopt;
        }
        if (text.starts_with('!')) {
            return readDirective(text.substr(1));
        }
        if (text.starts_with('?')) {
            return readQuestion(text.substr(1));
        }
        if (text.find('=') != std::string_view::npos) {
            return readDefinition(text);
        }
        return error("expected a definition (A = ...), a question (? ...) or !tolerance");
    }

    [[nodiscard]] std::vector<Answer> takeAnswers() { return std::move(m_answers); }

private:
    [[nodiscard]] ParseError error(std::string message, bool notImplemented = false) const {
        return ParseError{m_line, std::move(message), notImplemented};
    }

    std::optional<ParseError> readDirective(std::string_view text) {
        const std::vector<std::string_view> words = detail::splitWords(text);
        if (!words.empty() && words.front() == "current" && !text.starts_with(' ')) {
            return readCurrent(text.substr(words.front().size()));
        }
        if (words.empty() || words.front() != "tolerance" || text.starts_with(' ')) {
            return error("the directives of a question file are !tolerance and !current");
        }
        const std::vector<std::string_view> arguments(words.begin() + 1, words.end());
        std::variant<ToleranceDirective, std::string> tolerance = parseTolerance(arguments);
        if (auto* message = std::get_if<std::string>(&tolerance)) {
            return error(std::move(*message));
        }
        applyTolerance(std::get<ToleranceDirective>(tolerance), m_tolerances);
        return std::nullopt;
    }

    /// "!current <properties>": changes the current values of the
    /// document of the definitions (docs/verifica/domande.md).
    std::optional<ParseError> readCurrent(std::string_view text) {
        if (detail::trim(text).empty()) {
            return error(
                "!current needs at least one property: layer, color, linetype, "
                "ltscale, lineweight");
        }
        std::variant<WrittenProperties, EntityError> written = readProperties(text);
        if (auto* failure = std::get_if<EntityError>(&written)) {
            return error("!current: " + failure->message);
        }
        std::variant<core::EntityProperties, EntityError> current =
            applyProperties(std::get<WrittenProperties>(written), m_document.current(), m_document);
        if (auto* failure = std::get_if<EntityError>(&current)) {
            return error("!current: " + failure->message);
        }
        // The definitions build a model to ask questions about, not a
        // drawing to undo: the tables and the current values change on the
        // document itself, since edit::Editor has no operation for them yet
        // (package 0.6-P6).
        m_document.setCurrent(std::get<core::EntityProperties>(current));
        return std::nullopt;
    }

    /// "L = LAYER name=…": a new layer, or the layer 0 changed once.
    std::optional<ParseError> readLayer(std::string_view definition) {
        std::variant<LayerDefinition, EntityError> read = readLayerDefinition(definition);
        if (auto* failure = std::get_if<EntityError>(&read)) {
            return error(std::move(failure->message));
        }
        const LayerDefinition& written = std::get<LayerDefinition>(read);
        const core::Linetype* linetype = m_document.findLinetype(written.linetype);
        if (linetype == nullptr) {
            return error("unknown linetype " + quoted(written.linetype) +
                         ": define it above with LTYPE");
        }
        core::Layer layer;
        layer.name = written.name;
        layer.color = written.color;
        layer.linetype = linetype->id;
        layer.lineweight = written.lineweight;
        layer.on = !written.off;
        layer.frozen = written.frozen;
        layer.locked = written.locked;
        layer.plot = !written.noplot;
        if (const core::Layer* existing = m_document.findLayer(written.name)) {
            // The layer 0 exists in every drawing: it is defined again once,
            // to change its values (docs/verifica/domande.md).
            if (existing->id != m_document.layerZero() || m_layerZeroDefined) {
                return error("the layer " + quoted(written.name) + " is already defined");
            }
            m_layerZeroDefined = true;
            layer.id = existing->id;
            layer.name = existing->name;  // the layer 0 is never renamed
            m_document.replaceLayer(layer);
            return std::nullopt;
        }
        m_document.addLayer(std::move(layer));
        return std::nullopt;
    }

    /// "T = LTYPE name=… pattern=…".
    std::optional<ParseError> readLinetype(std::string_view definition) {
        std::variant<core::LinetypeDefinition, EntityError> read =
            readLinetypeDefinition(definition);
        if (auto* failure = std::get_if<EntityError>(&read)) {
            return error(std::move(failure->message));
        }
        auto& written = std::get<core::LinetypeDefinition>(read);
        if (m_document.findLinetype(written.name) != nullptr) {
            return error("the linetype " + quoted(written.name) + " is already defined");
        }
        core::Linetype linetype;
        linetype.name = std::move(written.name);
        linetype.description = std::move(written.description);
        linetype.pattern = std::move(written.pattern);
        m_document.addLinetype(std::move(linetype));
        return std::nullopt;
    }

    std::optional<ParseError> readDefinition(std::string_view text) {
        const std::size_t equals = text.find('=');
        const std::string_view name = detail::trim(text.substr(0, equals));
        if (!isName(name)) {
            return error("invalid name " + quoted(name) +
                         ": a letter or _, then letters, digits and _; not 'none'");
        }
        if (std::ranges::find(m_names, name) != m_names.end() || m_files.contains(name) ||
            std::ranges::find(m_tableNames, name) != m_tableNames.end()) {
            return error("the name " + quoted(name) + " is already defined");
        }
        const std::string_view definition = detail::trim(text.substr(equals + 1));
        const std::vector<std::string_view> words = detail::splitWords(definition);
        if (!words.empty() && (words.front() == "LAYER" || words.front() == "LTYPE")) {
            std::optional<ParseError> failure =
                words.front() == "LAYER" ? readLayer(definition) : readLinetype(definition);
            if (!failure) {
                m_tableNames.emplace_back(name);
            }
            return failure;
        }
        if (auto file = detail::readFileDefinition(definition, m_access)) {
            if (auto* failure = std::get_if<detail::FileDefinitionError>(&*file)) {
                return error(std::move(failure->message), failure->notImplemented);
            }
            m_files.emplace(name, std::get<detail::FileDrawing>(std::move(*file)));
            return std::nullopt;
        }
        std::variant<EntityDefinition, EntityError> read = readEntityDefinition(definition);
        if (auto* failure = std::get_if<EntityError>(&read)) {
            return error(std::move(failure->message), failure->notImplemented);
        }
        auto& entity = std::get<EntityDefinition>(read);
        // A layer or linetype not defined above is an error of the file.
        std::variant<core::EntityProperties, EntityError> properties =
            resolveProperties(entity.properties, m_document);
        if (auto* failure = std::get_if<EntityError>(&properties)) {
            return error(std::move(failure->message));
        }
        // The entities form a document numbered in the order they appear
        // (docs/adr/0011, point 3). Like every change of a document, they
        // go through the editor (docs/adr/0005).
        m_editor.beginTransaction("Definition");
        const core::EntityId id = m_editor.add(std::move(entity.entity));
        m_editor.commit();
        // The properties are those written, not the current values the
        // editor gives a new entity; see readCurrent() for why they are set
        // on the document.
        m_document.setProperties(id, std::get<core::EntityProperties>(properties));
        m_names.emplace_back(name);
        m_ids.push_back(id);
        return std::nullopt;
    }

    std::optional<ParseError> readQuestion(std::string_view text) {
        const std::optional<std::vector<std::string_view>> tokens = detail::splitTokens(text);
        if (!tokens) {
            return error("a quoted text is not closed");
        }
        const auto arrow = std::ranges::find(*tokens, kArrow);
        if (tokens->empty() || arrow == tokens->begin()) {
            return error("a question starts with its name: ? <question> ... -> <answer>");
        }
        if (arrow == tokens->end() || arrow + 1 == tokens->end()) {
            return error("a question ends with -> and the expected answer");
        }
        const std::string_view name = tokens->front();
        const detail::QuestionRow* row = detail::findQuestion(name);
        if (row == nullptr) {
            return error("unknown question " + quoted(name));
        }
        Answer answer{m_line, join(std::span{arrow + 1, tokens->end()}), {}, false};
        if (row->function == nullptr) {
            answer.skipped = m_pending;
            answer.actual = kNotImplemented;
            m_answers.push_back(std::move(answer));
            return std::nullopt;
        }
        const std::vector<std::string_view> arguments(tokens->begin() + 1, arrow);
        // "D:2F" names an entity of a drawing read from a file. A question
        // about it asks about a copy of that entity alone: the questions
        // that take a name are questions about one object.
        // An empty document, without the tables of a new drawing, whose
        // entities are numbered from the first handle of a new drawing, as
        // those of the definitions; QuestionModel finds them by name.
        core::Document fileDocument;
        fileDocument.setNextId(core::start_handles::kFirstEntity);
        edit::Editor fileEditor(fileDocument);
        std::vector<std::string> fileNames;
        std::vector<core::EntityId> fileIds;
        for (const std::string_view argument : arguments) {
            auto entity = detail::fileEntity(m_files, argument);
            if (!entity) {
                continue;
            }
            if (auto* message = std::get_if<std::string>(&*entity)) {
                return error(std::string{name} + ": " + *message);
            }
            if (const auto* unhandled = std::get_if<detail::UnhandledFileEntity>(&*entity)) {
                // Not passed rather than invalid: a later package may handle
                // the type (docs/verifica/casi.md, exit codes).
                answer.skipped = m_pending;
                answer.actual = "error: " + std::string{argument} + " is an unhandled " +
                                unhandled->type + " entity";
                m_answers.push_back(std::move(answer));
                return std::nullopt;
            }
            fileEditor.beginTransaction("Definition");
            fileIds.push_back(fileEditor.add(*std::get<const core::Entity*>(*entity)));
            fileEditor.commit();
            fileNames.emplace_back(argument);
        }
        const bool aboutFiles = !fileNames.empty();
        const detail::QuestionModel model{aboutFiles ? fileDocument : m_document,
                                          aboutFiles ? fileNames : m_names,
                                          aboutFiles ? fileIds : m_ids,
                                          m_files,
                                          m_tolerances,
                                          m_access};
        detail::QuestionResult result = row->function(model, arguments);
        if (auto* failure = std::get_if<detail::ArgumentError>(&result)) {
            if (!m_pending || !failure->notImplemented) {
                return error(std::string{name} + ": " + failure->message, failure->notImplemented);
            }
            answer.skipped = true;
            answer.actual = "error: " + failure->message;
            m_answers.push_back(std::move(answer));
            return std::nullopt;
        }
        answer.actual = std::get<std::string>(std::move(result));
        answer.matches = answerMatches(row->shape, answer.expected, answer.actual, m_tolerances);
        m_answers.push_back(std::move(answer));
        return std::nullopt;
    }

    core::Document m_document{core::Document::newDrawing()};
    edit::Editor m_editor;
    io::FileAccess* m_access;
    detail::FileDrawings m_files;           ///< drawings read from files, by name
    std::vector<std::string> m_names;       ///< m_names[n] names entity m_ids[n]
    std::vector<core::EntityId> m_ids;      ///< the defined entities, in order
    std::vector<std::string> m_tableNames;  ///< names of the LAYER and LTYPE definitions
    bool m_layerZeroDefined{false};         ///< the layer 0 is defined again at most once
    Tolerances m_tolerances;                ///< as changed by the lines read so far
    std::vector<Answer> m_answers;
    int m_line{0};
    bool m_pending{false};
};

}  // namespace

std::variant<std::vector<Answer>, ParseError> answerQuestions(std::string_view text,
                                                              io::FileAccess* files) {
    QuestionFile file{files};
    const std::vector<std::string_view> lines = splitLines(text);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (std::optional<ParseError> failure = file.read(static_cast<int>(i) + 1, lines[i])) {
            return std::move(*failure);
        }
    }
    return file.takeAnswers();
}

std::variant<QuestionPrefix, ParseError> answerQuestionsUntilFrontier(std::string_view text,
                                                                      io::FileAccess* files) {
    QuestionFile file{files, true};
    const std::vector<std::string_view> lines = splitLines(text);
    std::optional<Frontier> frontier;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const int line = static_cast<int>(i) + 1;
        if (const std::optional<int> issue = changesWith(lines[i])) {
            frontier = Frontier{line, "changes with #" + std::to_string(*issue)};
            break;
        }
        if (std::optional<ParseError> failure = file.read(line, lines[i])) {
            if (!failure->notImplemented) {
                return std::move(*failure);
            }
            frontier = Frontier{line, std::move(failure->message)};
            break;
        }
    }
    return QuestionPrefix{file.takeAnswers(), std::move(frontier)};
}

std::variant<std::vector<Difference>, ParseError> checkQuestions(std::string_view text) {
    std::variant<std::vector<Answer>, ParseError> answers = answerQuestions(text);
    if (auto* failure = std::get_if<ParseError>(&answers)) {
        return std::move(*failure);
    }
    std::vector<Difference> differences;
    for (Answer& answer : std::get<std::vector<Answer>>(answers)) {
        if (!answer.matches) {
            differences.push_back(
                {answer.line, std::move(answer.expected), std::move(answer.actual)});
        }
    }
    return differences;
}

std::string withActualAnswers(std::string_view text, const std::vector<Answer>& answers) {
    const std::vector<std::string_view> lines = splitLines(text);
    std::string out;
    auto answer = answers.begin();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const int line = static_cast<int>(i) + 1;
        std::string_view content = lines[i];
        if (answer != answers.end() && answer->line == line) {
            // The arrow of a question is its first "->" outside quotes.
            const std::optional<std::vector<std::string_view>> tokens =
                detail::splitTokens(content);
            const auto arrow = tokens ? std::ranges::find(*tokens, kArrow)
                                      : std::vector<std::string_view>::const_iterator{};
            if (tokens && arrow != tokens->end()) {
                const auto offset = static_cast<std::size_t>(arrow->data() - content.data());
                content = content.substr(0, offset + kArrow.size());
            }
            out += content;
            out += ' ';
            out += answer->actual;
            ++answer;
        } else {
            out += content;
        }
        out += '\n';
    }
    return out;
}

}  // namespace tg::script
