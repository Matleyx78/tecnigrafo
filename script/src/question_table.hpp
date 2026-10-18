// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/entity_id.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// The catalogue of questions (docs/verifica/domande.md). Not installed, not
// public: questions.cpp reads the file, this table answers.
namespace tg::script::detail {

/// The drawing the questions of a file ask about: the entities defined so
/// far, with the names the file gave them. Entity n (1-based identifier) is
/// names[n - 1].
struct QuestionModel {
    const core::Document& document;
    const std::vector<std::string>& names;

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

}  // namespace tg::script::detail
