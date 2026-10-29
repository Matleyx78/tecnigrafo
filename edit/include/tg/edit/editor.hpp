// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
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

    /// Replaces geometry, preserving identity and data (docs/adr/0031, point 5).
    /// Only the same type or Circle -> Arc is allowed. Returns false without
    /// changes for an absent entity, a forbidden type change or identical data.
    /// Precondition: a transaction is open.
    bool replace(core::EntityId id, core::Entity entity);

    /// Adds a derived entity with new identity/order and the source's properties
    /// and extras, except complete 102 groups in beforeEntity (ADR 0031, point 6).
    /// Returns nullopt without changes if source does not exist.
    /// Precondition: a transaction is open.
    std::optional<core::EntityId> addCopy(core::EntityId source, core::Entity entity);

    /// A position in the open transaction (docs/adr/0031, point 11).
    struct Savepoint {
        std::size_t changes{0};
    };

    /// Precondition: a transaction is open.
    [[nodiscard]] Savepoint savepoint() const;

    /// Reverts changes after mark in reverse order, keeping the transaction open.
    /// Precondition: mark comes from the open transaction.
    void rollbackTo(Savepoint mark);

    /// Changes INSUNITS (docs/adr/0021). Returns false without changes if
    /// the value is already `units`.
    /// Preconditions: a transaction is open; 0 <= units <= 24.
    bool setUnits(int units);

    /// Layer and property edits require an open transaction. They return false
    /// (or nullopt) for rejected or unchanged values, without recording a change.
    std::optional<core::LayerId> addLayer(core::Layer layer);
    bool renameLayer(core::LayerId id, std::string name);
    bool removeLayer(core::LayerId id);
    /// Changes colour, linetype, lineweight and states; preserves name/id/extras.
    /// Freezing the current layer is refused.
    bool changeLayer(const core::Layer& layer);
    std::optional<core::LinetypeId> addLinetype(core::Linetype linetype);
    bool setProperties(core::EntityId id, const core::EntityProperties& properties);
    /// Refuses a frozen current layer.
    bool setCurrent(const core::CurrentProperties& current);
    bool setLtscale(double scale);  ///< finite and > 0

    /// Saved view setting: modifies the drawing without an undo step.
    /// No transaction is needed. Notifies the change listener on a change.
    void setLwdisplay(bool on);

    /// True if the undo stack is not at the last save or load position.
    [[nodiscard]] bool isModified() const;

    /// Records that the drawing has just been saved as `name` (UTF-8).
    void markSaved(std::string name);

    /// The drawing name, "Drawing1.cadx" before the first save.
    [[nodiscard]] const std::string& fileName() const { return m_fileName; }

    /// True once the drawing has been saved or loaded under a name.
    [[nodiscard]] bool hasFile() const { return m_hasFile; }

    /// Replaces the referenced document, clears history and notifies once.
    /// An empty name starts a new "Drawing1.cadx" without an associated file.
    /// Precondition: no transaction is open.
    void replaceDocument(core::Document document, std::string name);

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

    /// Names of every undo step, oldest first: the last one is undoName(),
    /// the step undo() would undo. Empty if there is none.
    [[nodiscard]] std::vector<std::string> undoNames() const;

    /// Names of every redo step, the most recently undone last: the last one
    /// is redoName(), the step redo() would redo. Empty if there is none.
    [[nodiscard]] std::vector<std::string> redoNames() const;

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
    struct LayerTableChange {
        core::Layer record;
        std::size_t position{0};
    };

    struct LinetypeTableChange {
        core::Linetype record;
        std::size_t position{0};
    };

    /// Only changed group values are stored, never a snapshot of the drawing.
    struct RawValueChange {
        std::optional<core::EntityId> entity;  ///< otherwise a raw section
        std::size_t section{0};
        std::size_t group{0};
        std::string value;
    };

    struct LayerUpdate {
        core::Layer record;
        std::vector<RawValueChange> rawValues;
    };

    /// One elementary change to the document. While the entity is outside the
    /// document (after an Added change is undone, or a Removed change is
    /// applied) `detached` holds it, so that it can be put back unchanged.
    struct Change {
        enum class Kind : std::uint8_t {
            Added,
            Removed,
            Replaced,
            Units,
            LayerAdded,
            LayerRemoved,
            LinetypeAdded,
            LayerUpdated,
            Properties,
            Current,
            Ltscale
        };

        Kind kind{Kind::Added};
        core::EntityId id;
        std::optional<core::EntityRecord> detached;
        int previousUnits{core::Document::kDefaultUnits};
        int units{core::Document::kDefaultUnits};
        std::variant<std::monostate, LayerTableChange, LinetypeTableChange, LayerUpdate,
                     core::EntityRecord, core::CurrentProperties, core::Entity, double>
            data;
    };

    struct Step {
        std::string name;
        std::vector<Change> changes;
    };

    void apply(Change& change);
    void revert(Change& change);
    void notify() const;
    void swapUpdate(Change& change);
    void swapRawValues(std::vector<RawValueChange>& values);

    core::Document& m_document;
    std::optional<Step> m_open;
    std::vector<Step> m_undo;
    std::vector<Step> m_redo;
    // A discarded redo branch can make the saved position unreachable.
    std::optional<std::size_t> m_savedPosition{0};
    std::string m_fileName{"Drawing1.cadx"};
    bool m_hasFile{false};
    bool m_savedLwdisplay{false};
    Listener m_listener;
};

}  // namespace tg::edit
