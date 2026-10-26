// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

namespace tg::cmd {
class CommandRegistry;
}

namespace tg::app {

class MainWindow;

/// Application-only commands; registered only on the desktop window.
void registerAppCommands(cmd::CommandRegistry& registry, MainWindow& window);

}  // namespace tg::app
