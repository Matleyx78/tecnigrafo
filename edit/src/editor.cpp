// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/editor.hpp"

#include <cassert>
#include <ranges>
#include <utility>

namespace tg::edit {

Editor::Editor(core::Document& document) : m_document(document) {}

void Editor::beginTransaction(std::string name) {
    assert(!m_open && "a transaction is already open");
    m_open = Step{std::move(name), {}};
}

core::EntityId Editor::add(core::Entity entity) {
    assert(m_open && "add() needs an open transaction");
    const core::EntityId id = m_document.add(std::move(entity));
    m_open->changes.push_back(Change{Change::Kind::Added, id, std::nullopt});
    return id;
}

bool Editor::remove(core::EntityId id) {
    assert(m_open && "remove() needs an open transaction");
    std::optional<core::EntityRecord> removed = m_document.remove(id);
    if (!removed) {
        return false;
    }
    m_open->changes.push_back(Change{Change::Kind::Removed, id, std::move(removed)});
    return true;
}

void Editor::commit() {
    assert(m_open && "commit() needs an open transaction");
    Step step = std::move(*m_open);
    m_open.reset();
    if (!step.changes.empty()) {
        m_undo.push_back(std::move(step));
        m_redo.clear();
    }
    notify();
}

void Editor::rollback() {
    assert(m_open && "rollback() needs an open transaction");
    for (Change& change : std::views::reverse(m_open->changes)) {
        revert(change);
    }
    m_open.reset();
    notify();
}

bool Editor::canUndo() const {
    return !m_open && !m_undo.empty();
}

bool Editor::canRedo() const {
    return !m_open && !m_redo.empty();
}

std::string Editor::undoName() const {
    return m_undo.empty() ? std::string{} : m_undo.back().name;
}

std::string Editor::redoName() const {
    return m_redo.empty() ? std::string{} : m_redo.back().name;
}

bool Editor::undo() {
    if (!canUndo()) {
        return false;
    }
    Step step = std::move(m_undo.back());
    m_undo.pop_back();
    for (Change& change : std::views::reverse(step.changes)) {
        revert(change);
    }
    m_redo.push_back(std::move(step));
    notify();
    return true;
}

bool Editor::redo() {
    if (!canRedo()) {
        return false;
    }
    Step step = std::move(m_redo.back());
    m_redo.pop_back();
    for (Change& change : step.changes) {
        apply(change);
    }
    m_undo.push_back(std::move(step));
    notify();
    return true;
}

void Editor::setChangeListener(Listener listener) {
    m_listener = std::move(listener);
}

// apply() and revert() run only in the order recorded by the transaction,
// so the entity they look for is always where the change left it.

void Editor::apply(Change& change) {
    switch (change.kind) {
        case Change::Kind::Added:
            assert(change.detached && "the added entity must be outside the document");
            m_document.restore(std::move(*change.detached));
            change.detached.reset();
            break;
        case Change::Kind::Removed:
            change.detached = m_document.remove(change.id);
            assert(change.detached && "the removed entity must be in the document");
            break;
    }
}

void Editor::revert(Change& change) {
    switch (change.kind) {
        case Change::Kind::Added:
            change.detached = m_document.remove(change.id);
            assert(change.detached && "the added entity must be in the document");
            break;
        case Change::Kind::Removed:
            assert(change.detached && "the removed entity must be outside the document");
            m_document.restore(std::move(*change.detached));
            change.detached.reset();
            break;
    }
}

void Editor::notify() const {
    if (m_listener) {
        m_listener();
    }
}

}  // namespace tg::edit
