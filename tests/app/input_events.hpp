// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// Keyboard and mouse input for the tests of module app. The events are built
// here and sent with QCoreApplication::sendEvent, as QTest does, so event
// filters run as with a real keyboard. QTest::keyClick and mouseClick are
// avoided: clang-tidy's analyser reports a false use-after-free inside their
// inline code, in a Qt header where it cannot be suppressed.

#include <QChar>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QWidget>

#include <string>

namespace tg::app::events {

/// A key press sent to `widget`.
inline void press(QWidget* widget, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                  const QString& text = {}) {
    QKeyEvent event(QEvent::KeyPress, key, modifiers, text);
    QCoreApplication::sendEvent(widget, &event);
}

/// `text` (ASCII) typed in `widget`, one key press per character. For ASCII
/// the Qt key code is the code of the upper-case character.
inline void type(QWidget* widget, const std::string& text) {
    for (const char c : text) {
        const QChar character = QLatin1Char(c);
        press(widget, static_cast<Qt::Key>(character.toUpper().unicode()), Qt::NoModifier,
              QString(character));
    }
}

/// A click of `button` at `position`, in the coordinates of `widget`.
inline void click(QWidget* widget, Qt::MouseButton button, QPoint position) {
    const QPointF local(position);
    const QPointF global = widget->mapToGlobal(local);
    QMouseEvent pressEvent(QEvent::MouseButtonPress, local, global, button, button, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &pressEvent);
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, local, global, button, Qt::NoButton,
                             Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &releaseEvent);
}

/// The pointer moved to `position`, with no button pressed.
inline void move(QWidget* widget, QPoint position) {
    const QPointF local(position);
    QMouseEvent event(QEvent::MouseMove, local, widget->mapToGlobal(local), Qt::NoButton,
                      Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &event);
}

}  // namespace tg::app::events
