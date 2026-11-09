// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_protocol.hpp"

#include "text.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace tg::cmd {

bool matchesKeyword(const Keyword& keyword, std::string_view text) {
    return detail::equalsIgnoringCase(text, keyword.name) ||
           (!keyword.shortcut.empty() && detail::equalsIgnoringCase(text, keyword.shortcut));
}

const Keyword* findKeyword(const Prompt& prompt, std::string_view text) {
    const auto keyword = std::ranges::find_if(prompt.keywords, [text](const Keyword& candidate) {
        return matchesKeyword(candidate, text);
    });
    return keyword == prompt.keywords.end() ? nullptr : &*keyword;
}

std::string textAnswer(const Prompt& prompt, std::string_view text) {
    if (prompt.file != FilePrompt::None && text.size() >= 2 && text.front() == '"' &&
        text.back() == '"') {
        text = text.substr(1, text.size() - 2);
    }
    return std::string{text};
}

}  // namespace tg::cmd
