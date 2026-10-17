// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

namespace tg::cmd {

class CommandRegistry;

/// Registers PLINE (PL) and ERASE (E). None of these names may already be registered.
void registerBuiltinCommands(CommandRegistry& registry);

}  // namespace tg::cmd
