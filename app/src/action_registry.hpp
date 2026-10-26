// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <deque>

class QAction;
class QMenuBar;

namespace tg::app {

/// One action of the interface, declared once (docs/adr/0009, point 1).
///
/// A spec with a menu and an empty text is a separator of that menu: the
/// registry owns separators too, so that no QAction exists outside it.
struct ActionSpec {
    QString id;             ///< stable, English: "draw.line", "edit.undo"
    QString text;           ///< menu text, with & for the mnemonic
    QString description;    ///< tooltip and shortcuts window
    QString icon;           ///< icon name, "draw-line"; empty until P18-P19
    QKeySequence shortcut;  ///< default; empty for none
    QString command;        ///< cmd command it runs ("LINE"), or empty
    bool checkable{false};
    QStringList environments{QStringLiteral("drawing2d")};
    QString menu;    ///< classic menu path: "&Edit", "&Tools/Object Snap &Modes"
    QString ribbon;  ///< ribbon place "home/draw"; empty: menu only (P16)
};

/// The actions of the main window. Menus, and later the ribbon, toolbars
/// and the shortcuts window, are built from it; the registry only declares,
/// the window connects what each action does.
class ActionRegistry final : public QObject {
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);

    /// Precondition: the id is new. The QAction is parented to the registry,
    /// with objectName == id.
    QAction* add(ActionSpec spec);

    [[nodiscard]] QAction* action(const QString& id) const;  ///< nullptr if unknown

    /// nullptr if unknown; valid as long as the registry.
    [[nodiscard]] const ActionSpec* spec(const QString& id) const;

    [[nodiscard]] QList<QAction*> actions() const;  ///< in order of addition

    /// Actions created at run time by the registry itself, for lists such
    /// as the recent files: ids "<prefix>.1"... "<prefix>.<count>". Actions
    /// created by an earlier call are returned again, not created twice.
    QList<QAction*> dynamicActions(const QString& prefix, int count);

    /// Builds the classic menus into `bar` from ActionSpec::menu, in order:
    /// each menu and submenu appears where its first action is added.
    void buildMenus(QMenuBar* bar) const;

private:
    struct Entry {
        ActionSpec spec;
        QAction* action{nullptr};
    };

    // A deque keeps the specs where they are when entries are added, so
    // that the pointers returned by spec() stay valid.
    std::deque<Entry> m_entries;
    QHash<QString, const Entry*> m_index;
};

}  // namespace tg::app
