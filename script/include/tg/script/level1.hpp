// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/script/script.hpp"

#include <string>

namespace tg::script {

/// Runs a level 1 script on an empty document and returns the report.
/// A directive that needs level 2 is an error at level 1: parseScript()
/// rejects it unless the script says "!level 2", and such a script does not
/// run here.
///
/// The script drives a cmd::CommandRunner with the built-in commands, as
/// the main window does (docs/verifica/script.md): a normal line is written
/// in the history after the prompt, as the command line echoes it, and
/// then submitted; !move, !click, !rightclick and !esc are the pointer, the
/// clicks and the Esc key of the view; !view sets the size of a pixel.
/// Precondition: script.level == 1.
[[nodiscard]] std::string runLevel1(const Script& script);

}  // namespace tg::script
