// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <optional>
#include <string>

namespace tg::edit {

class Editor;

/// Operations on the document's tables and header, using the owning editor's
/// transactions and history (docs/adr/0041, point 4).
class DocumentTables {
public:
    DocumentTables(const DocumentTables&) = delete;
    DocumentTables& operator=(const DocumentTables&) = delete;
    DocumentTables(DocumentTables&&) = delete;
    DocumentTables& operator=(DocumentTables&&) = delete;

    /// Changes INSUNITS (docs/adr/0021). Returns false without changes if
    /// the value is already `units`.
    /// Preconditions: a transaction is open; 0 <= units <= 24.
    bool setUnits(int units);

    /// Layer, linetype and property edits require an open transaction. They return false
    /// (or nullopt) for rejected or unchanged values, without recording a change.
    std::optional<core::LayerId> addLayer(core::Layer layer);
    bool renameLayer(core::LayerId id, std::string name);
    bool removeLayer(core::LayerId id);
    /// Changes colour, linetype, lineweight and states; preserves name/id/extras.
    /// Freezing the current layer is refused.
    bool changeLayer(const core::Layer& layer);
    std::optional<core::LinetypeId> addLinetype(core::Linetype linetype);
    /// Refuses a frozen current layer.
    bool setCurrent(const core::CurrentProperties& current);
    bool setLtscale(double scale);  ///< finite and > 0

    /// Saved view setting: modifies the drawing without an undo step.
    /// No transaction is needed. Notifies the change listener on a change.
    void setLwdisplay(bool on);

private:
    friend class Editor;

    explicit DocumentTables(Editor& editor) : m_editor(editor) {}
    ~DocumentTables() = default;

    Editor& m_editor;
};

}  // namespace tg::edit
