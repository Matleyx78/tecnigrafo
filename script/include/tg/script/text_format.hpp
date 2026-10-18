// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <optional>
#include <string>
#include <string_view>

/// Texts inside reports and question files (docs/verifica/rapporto.md):
/// the value and the style of a TEXT entity, the argument of text-width.
namespace tg::script {

/// `text` between double quotes, with '"' and '\' escaped by a backslash
/// and the control characters (U+0000 to U+001F, U+007F) written as
/// \u{XX}. Every other byte, UTF-8 included, is copied as it is.
[[nodiscard]] std::string quoteText(std::string_view text);

/// The text of a quoted string, or std::nullopt if `token` is not one.
/// Besides the escapes of quoteText(), \u{X...} accepts one to six
/// hexadecimal digits of any Unicode code point but the surrogates, and
/// writes it in UTF-8. An unescaped '"' inside is an error.
[[nodiscard]] std::optional<std::string> unquoteText(std::string_view token);

}  // namespace tg::script
