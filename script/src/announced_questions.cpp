// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/questions.hpp"

#include <string>
#include <utility>

namespace tg::script::detail {

Answer announcedQuestionAnswer(int line, std::string expected, bool pending) {
    return Answer{line, std::move(expected), "error: question not implemented yet", false, pending};
}

}  // namespace tg::script::detail
