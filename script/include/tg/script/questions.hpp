// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/file_access.hpp"
#include "tg/script/compare.hpp"
#include "tg/script/script.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

/// Questions to the model (docs/adr/0011, point 3; docs/verifica/domande.md).
namespace tg::script {

/// The answer to one question of a .tgq file.
struct Answer {
    int line;              ///< 1-based
    std::string expected;  ///< as written after "->", blanks collapsed
    std::string actual;    ///< the model's answer, or "error: <reason>"
    bool matches{false};   ///< by the tolerances in force at that line

    bool skipped{false};  ///< not compared: the program cannot answer it yet

    friend bool operator==(const Answer&, const Answer&) = default;
};

/// Reads a .tgq file and answers every question against core, in the order
/// of the file: a question sees the entities defined above it. Returns the
/// answers, or the first error in the file.
///
/// The definitions and questions about files (docs/verifica/domande.md,
/// "File") read them through `files`; without it they are an error.
[[nodiscard]] std::variant<std::vector<Answer>, ParseError> answerQuestions(
    std::string_view text, io::FileAccess* files = nullptr);

/// The answers above the frontier of a pending question file.
struct QuestionPrefix {
    std::vector<Answer> answers;       ///< some may be skipped
    std::optional<Frontier> frontier;  ///< none: every line was read
};

/// Reads as answerQuestions(), skipping questions not implemented yet.
/// An unimplemented definition/directive or changesWith() stops the file;
/// malformed lines above that frontier are errors (docs/adr/0037, point 4).
[[nodiscard]] std::variant<QuestionPrefix, ParseError> answerQuestionsUntilFrontier(
    std::string_view text, io::FileAccess* files = nullptr);

/// Reads a .tgq file of questions (docs/verifica/domande.md), evaluates
/// every question against core and returns the differences from the
/// expected answers written in the file; empty if all match.
[[nodiscard]] std::variant<std::vector<Difference>, ParseError> checkQuestions(
    std::string_view text);

/// `text` with the expected answer of each question replaced by the
/// obtained one: the file a reference writer would write for the program
/// as it is. Lines without a question are copied as they are.
[[nodiscard]] std::string withActualAnswers(std::string_view text,
                                            const std::vector<Answer>& answers);

}  // namespace tg::script
