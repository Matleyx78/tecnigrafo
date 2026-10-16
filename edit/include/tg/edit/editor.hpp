// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tg::edit {

/// The only way to modify a document (docs/adr/0005).
///
/// Every change happens inside a named transaction. A committed transaction
/// that changed something becomes one undo step; a rolled back transaction
/// leaves no trace. Undo and redo put entities back with their original
/// identifiers.
///
/// The editor does not own the document, which must outlive it.
class Editor {
public:
    using Listener = std::function<void()>;

    explicit Editor(core::Document& document);

    // Copies would share the document but not the undo history.
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor(Editor&&) = delete;
    Editor& operator=(Editor&&) = delete;
    ~Editor() = default;

    [[nodiscard]] const core::Document& document() const { return m_document; }

    /// Opens a transaction. `name` is shown to the user ("Undo Polyline").
    /// Precondition: no transaction is open.
    void beginTransaction(std::string name);

    /// Adds an entity to the document and returns its new identifier.
    /// Precondition: a transaction is open.
    core::EntityId add(core::Entity entity);

    /// Removes the entity with identifier `id`. Returns false, changing
    /// nothing, if the document has no such entity.
    /// Precondition: a transaction is open.
    bool remove(core::EntityId id);

    /// Closes the open transaction, keeping its changes. A transaction that
    /// changed something becomes an undo step and clears the redo steps; an
    /// empty one leaves both stacks as they are.
    /// Precondition: a transaction is open.
    void commit();

    /// Closes the open transaction, undoing its changes.
    /// Precondition: a transaction is open.
    void rollback();

    [[nodiscard]] bool inTransaction() const { return m_open.has_value(); }

    /// True if undo() would succeed: there is a step and no open transaction.
    [[nodiscard]] bool canUndo() const;

    /// True if redo() would succeed: there is a step and no open transaction.
    [[nodiscard]] bool canRedo() const;

    /// Name of the most recent undo step, or an empty string if there is none.
    [[nodiscard]] std::string undoName() const;

    /// Name of the most recent redo step, or an empty string if there is none.
    [[nodiscard]] std::string redoName() const;

    /// Undoes the most recent step. Returns false, changing nothing, if there
    /// is no step or a transaction is open.
    bool undo();

    /// Redoes the most recently undone step. Returns false, changing nothing,
    /// if there is no step or a transaction is open.
    bool redo();

    /// `listener` is called after every commit() and rollback(), even of an
    /// empty transaction, and after every undo() or redo() that succeeded.
    /// An empty function removes the listener.
    void setChangeListener(Listener listener);

private:
    /// One elementary change to the document. While the entity is outside the
    /// document (after an Added change is undone, or a Removed change is
    /// applied) `detached` holds it, so that it can be put back unchanged.
    struct Change {
        enum class Kind : std::uint8_t { Added, Removed };

        Kind kind{Kind::Added};
        core::EntityId id;
        std::optional<core::EntityRecord> detached;
    };

    struct Step {
        std::string name;
        std::vector<Change> changes;
    };

    void apply(Change& change);
    void revert(Change& change);
    void notify() const;

    core::Document& m_document;
    std::optional<Step> m_open;
    std::vector<Step> m_undo;
    std::vector<Step> m_redo;
    Listener m_listener;
};

}  // namespace tg::edit
