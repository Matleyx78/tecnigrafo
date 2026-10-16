// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

namespace tg::cmd {

class CommandRegistry;

/// Registers PLINE and its PL alias. Neither name may already be registered.
void registerBuiltinCommands(CommandRegistry& registry);

}  // namespace tg::cmd
