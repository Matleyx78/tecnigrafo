// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"
#include "tg/script/compare.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// The catalogue of questions (docs/verifica/domande.md). Not installed, not
// public: questions.cpp reads the file, this table answers.
namespace tg::script::detail {

/// A drawing read by a definition "D = DXF ..." or "C = CADX ..."
/// (docs/verifica/domande.md, "File").
struct FileDrawing {
    bool cadx{false};   ///< read by io::readCadx, else by io::readDxf
    std::string bytes;  ///< the file as read
    io::ReadResult read;
};

/// The drawings read by the definitions so far, by name.
using FileDrawings = std::map<std::string, FileDrawing, std::less<>>;

/// The drawing the questions of a file ask about: the entities defined so
/// far, with the names the file gave them, and the drawings read from
/// files. Entity ids[n] is named names[n]: the identifiers grow in the order
/// of the definitions, but the layers and linetypes defined between them
/// take handles too (docs/adr/0023, point 1).
struct QuestionModel {
    const core::Document& document;
    const std::vector<std::string>& names;
    const std::vector<core::EntityId>& ids;  ///< as many as names
    const FileDrawings& files;
    /// The tolerances in force at the question (roundtrip compares with them).
    const Tolerances& tolerances;
    /// Where the definitions and questions about files read them; nullptr
    /// if they cannot (answerQuestions() without files).
    io::FileAccess* access;

    /// The entity named `name`, or std::nullopt.
    [[nodiscard]] std::optional<core::EntityId> find(std::string_view name) const;

    /// The name the file gave entity `id`.
    [[nodiscard]] std::string_view nameOf(core::EntityId id) const;
};

/// A question whose arguments are wrong: the file is malformed.
struct ArgumentError {
    std::string message;
};

/// The answer as text, in the form of docs/verifica/domande.md.
using QuestionResult = std::variant<std::string, ArgumentError>;

using QuestionFunction = QuestionResult (*)(const QuestionModel& model,
                                            std::span<const std::string_view> arguments);

/// How an answer is compared with the expected one.
enum class AnswerShape : std::uint8_t {
    Single,  ///< one value: numbers within the tolerances, the rest equal
    Names,   ///< names separated by blanks, or "none", in any order
    Snaps,   ///< "Kind x,y" items separated by ';', or "none", in any order
    Items,   ///< any items separated by ';', or "none", in any order
};

struct QuestionRow {
    std::string_view name;
    AnswerShape shape{AnswerShape::Single};
    /// nullptr for a question of the catalogue that the program does not
    /// answer yet: it gets "error: question not implemented yet".
    QuestionFunction function{nullptr};
};

/// The row of question `name`, or nullptr if the catalogue has none.
[[nodiscard]] const QuestionRow* findQuestion(std::string_view name);

/// Why a definition of a drawing read from a file failed.
struct FileDefinitionError {
    std::string message;
    bool notImplemented{false};  ///< io does not read this format yet
};

/// Reads the drawing of a definition "DXF source=<name>", "DXF file=<name>",
/// "CADX source=<name>" or "CADX file=<name>" (the text after "D =").
/// std::nullopt if `text` is not one, so that it is read as an entity.
/// Defined in file_questions.cpp.
[[nodiscard]] std::optional<std::variant<FileDrawing, FileDefinitionError>> readFileDefinition(
    std::string_view text, io::FileAccess* access);

/// An entity of a drawing read from a file that io keeps unhandled: a
/// question about it is not passed, not invalid (docs/verifica/casi.md),
/// because a later package may handle its type.
struct UnhandledFileEntity {
    std::string type;  ///< the DXF type, "LINE"
};

/// The handled entity named "<drawing>:<handle>" ("D:2F"): std::nullopt if
/// `name` does not start with the name of a drawing of `files` and ':',
/// UnhandledFileEntity if the entity with that handle is unhandled, an error
/// message if the drawing has no entity with that handle.
/// Defined in file_questions.cpp.
[[nodiscard]] std::optional<std::variant<const core::Entity*, UnhandledFileEntity, std::string>>
fileEntity(const FileDrawings& files, std::string_view name);

}  // namespace tg::script::detail
