// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

namespace tg::app {

class ActionRegistry;

/// Declares the actions of the main window in `registry`, in the order of
/// the classic menus: File, Edit, View, Tools, Draw, Help. Only the
/// declarations: MainWindow connects what each action does, and actions
/// with a command run it as if typed in the command line.
///
/// Ids: file.new, file.open, file.save, file.saveas, file.recent (the
/// Recent Files submenu; its entries are file.recent.1... created at run
/// time), file.quit; edit.undo, edit.redo, edit.erase; view.zoom-extents;
/// aids.osnap, aids.ortho, aids.snap.endpoint, aids.snap.midpoint,
/// aids.snap.center, aids.snap.insertion; draw.line, draw.pline,
/// draw.circle, draw.arc, draw.text; help.about. Ids ending in
/// ".separator-<name>" are menu separators.
void addBuiltinActions(ActionRegistry& registry);

}  // namespace tg::app
