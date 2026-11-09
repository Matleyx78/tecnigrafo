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
#include <memory>

class QAction;
class QMenu;
class QMenuBar;

namespace tg::app {

/// The response of ActionSpec::responses that waits for the user's own
/// answer, as the backslash does in AutoCAD's menu macros.
constexpr QChar kWaitForUser = QLatin1Char('\\');

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
    /// In the quick access bar of both interfaces, in the order of the
    /// registry (docs/adr/0039, point 1, area 1).
    bool quickAccess{false};
    /// The action this one is a variant of ("draw.circle"), or empty
    /// (docs/tappe/0.7.5/P19). A variant is in the menu of that action, not
    /// in a menu or a ribbon group of its own: `menu` and `ribbon` stay
    /// empty.
    QString variantOf;
    /// What the action answers after the name of `command`, one response
    /// per prompt, as if typed in the command line: an option of that
    /// prompt ("D"), or kWaitForUser, which waits for the user's own answer
    /// (the prompt changes). The responses left when the command ends, or
    /// when its prompt does not offer the next one, are dropped.
    QStringList responses;
    // Shortcuts included (decision B7 of docs/DECISIONI.md, docs/tappe/
    // 0.7.5/P29); separate from `environments`. The run-time actions of
    // dynamicActions() keep the default: their menu decides for them.
    bool requiresDocument{true};  ///< disabled while no document is active (the Start page)
};

/// The actions of the main window. Menus, and later the ribbon, toolbars
/// and the shortcuts window, are built from it; the registry only declares,
/// the window connects what each action does.
class ActionRegistry final : public QObject {
    Q_OBJECT

public:
    explicit ActionRegistry(QObject* parent = nullptr);
    ~ActionRegistry() override;

    ActionRegistry(const ActionRegistry&) = delete;
    ActionRegistry& operator=(const ActionRegistry&) = delete;
    ActionRegistry(ActionRegistry&&) = delete;
    ActionRegistry& operator=(ActionRegistry&&) = delete;

    /// Precondition: the id is new; a variant's action (ActionSpec::
    /// variantOf) is already added. The QAction is parented to the registry,
    /// with objectName == id.
    ///
    /// The first variant of an action gives it a menu of its variants, in
    /// order, with their icons and texts: a submenu in the classic menus, a
    /// drop-down in the ribbon and the toolbars. The action remembers the
    /// variant last triggered (the first until then) and shows its icon and
    /// tooltip, as AutoCAD's buttons do; MainWindow makes it run that one.
    QAction* add(ActionSpec spec);

    [[nodiscard]] QAction* action(const QString& id) const;  ///< nullptr if unknown

    /// nullptr if unknown; valid as long as the registry.
    [[nodiscard]] const ActionSpec* spec(const QString& id) const;

    [[nodiscard]] QList<QAction*> actions() const;  ///< in order of addition

    /// The variants of the action `id`, in order; empty if it has none.
    [[nodiscard]] QList<QAction*> variants(const QString& id) const;

    /// The variant the action `id` runs: the last one triggered, the first
    /// until then; nullptr if it has no variants.
    [[nodiscard]] QAction* currentVariant(const QString& id) const;

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
        // For an action with variants: they, the one it runs, and the menu
        // that shows them, owned by the entry.
        QList<QAction*> variants;
        QAction* current{nullptr};
        std::unique_ptr<QMenu> menu;
    };

    /// Adds `variant` to the variants of `owner`.
    void addVariant(Entry& owner, QAction* variant);

    /// Makes `variant` the one that `owner` runs and shows.
    static void useVariant(Entry& owner, QAction* variant);

    // A deque keeps the specs where they are when entries are added, so
    // that the pointers returned by spec() stay valid.
    std::deque<Entry> m_entries;
    QHash<QString, Entry*> m_index;
};

}  // namespace tg::app
