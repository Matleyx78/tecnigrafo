// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_description.hpp"

// The descriptions of the built-in commands (docs/adr/0026), used by
// registerBuiltinCommands(). Not installed, not public. The prompt texts
// repeat those in cmd/src/commands/: a change goes in both places, and the
// runner's description check reports the one that is missing.
namespace tg::cmd::detail {

[[nodiscard]] CommandDescription arcDescription();
[[nodiscard]] CommandDescription circleDescription();
[[nodiscard]] CommandDescription eraseDescription();
[[nodiscard]] CommandDescription insunitsDescription();
[[nodiscard]] CommandDescription lineDescription();
[[nodiscard]] CommandDescription newDescription();
[[nodiscard]] CommandDescription openDescription();
[[nodiscard]] CommandDescription plineDescription();
[[nodiscard]] CommandDescription qsaveDescription();
[[nodiscard]] CommandDescription saveDescription();
[[nodiscard]] CommandDescription saveasDescription();
[[nodiscard]] CommandDescription textDescription();

}  // namespace tg::cmd::detail
