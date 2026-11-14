// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

class QString;
class QWidget;

namespace tg::app {

/// Tells the user where to find the previous session's crash log.
void showCrashNotice(QWidget& parent, const QString& previousPath);

}  // namespace tg::app
