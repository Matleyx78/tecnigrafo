// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <optional>
#include <string>
#include <string_view>

/// The form of the files of a verification case and the echo of the lines
/// of a script: the early checks of docs/adr/0037, points 2 and 3. They read
/// the files only, never run the program.
namespace tg::script {

/// The first violation of the form of a case (docs/adr/0037, points 2 and 3).
struct FormError {
    std::string file;     ///< "script.tgs", "atteso.txt" or "domande.tgq"
    int line;             ///< 1-based, in `file`
    std::string message;  ///< English, one line

    friend bool operator==(const FormError&, const FormError&) = default;
};

/// Checks script.tgs and atteso.txt of a script case: the rules of point 2
/// on both files, then the echo of point 3 unless the script has a line
/// "!level 2". Reads the texts as they are; a UTF-8 BOM does not count.
[[nodiscard]] std::optional<FormError> checkScriptForm(std::string_view script,
                                                       std::string_view expected);

/// Checks domande.tgq (point 2).
[[nodiscard]] std::optional<FormError> checkQuestionsForm(std::string_view questions);

}  // namespace tg::script
