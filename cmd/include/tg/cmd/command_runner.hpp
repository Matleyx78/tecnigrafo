// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/drawing_aids.hpp"
#include "tg/cmd/input.hpp"
#include "tg/cmd/selection_set.hpp"
#include "tg/core/document.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace tg::cmd {

/// What a click does to the selection.
enum class PickAction : std::uint8_t { Add, Remove };  ///< Remove: Shift held

/// A selection rectangle being traced, in drawing coordinates.
struct SelectionRectangle {
    core::Vec2 first;
    core::Vec2 second;     ///< follows the pointer
    bool crossing{false};  ///< second.x < first.x

    friend constexpr bool operator==(const SelectionRectangle&,
                                     const SelectionRectangle&) = default;
};

/// Runs one command at a time and turns what the user types into inputs,
/// so that the syntax is the same in every command (docs/adr/0005).
///
/// With no command running, typed text is a command name or alias; empty
/// text (Enter) or a ConfirmInput repeats the last command. U, UNDO and REDO
/// are handled here, and only work with no command running.
///
/// With a command running, typed text becomes, in this order: a
/// ConfirmInput if empty, a KeywordInput if it matches an option of the
/// prompt, a PointInput if it is a coordinate that can be resolved;
/// otherwise the runner writes "Invalid point or option." and the prompt
/// stays. A CancelInput rolls the command's transaction back and ends it.
///
/// Selection (docs/adr/0008). With no command running, and while asking
/// "Select objects:" for a command that needsSelection(), clicks build the
/// selection: a click within 3 pixels of an entity adds it (or removes it,
/// with PickAction::Remove); a click in the void opens a rectangle, whose
/// second corner follows the pointer, and the next click closes it and adds
/// (or removes, by the action of that click) the entities it selects: by
/// window if the second corner is to the right of the first, by crossing if
/// it is to the left. While the rectangle is open the prompt is "Specify
/// opposite corner:", and a CancelInput closes it.
///
/// - With no command running the gestures build the current selection,
///   without messages. A CancelInput with no open rectangle clears it.
/// - A command that needsSelection() starts at once with the current
///   selection, if there is one; otherwise the runner asks "Select
///   objects:". After each gesture it writes "<n> found, <t> total", where
///   n counts the entities the gesture found, or "<n> removed, <t> total",
///   where n counts those actually removed. Enter ends the question: with a
///   selection the runner calls start() and hands it over with one
///   SelectionInput; without, the command ends with no effect. Other typed
///   text writes "Invalid selection.". A CancelInput with no open rectangle
///   cancels the command.
/// - Starting a command that does not need a selection, the end of every
///   command, U, UNDO and REDO clear the selection.
///
/// Points, options and selections given with submit() while the runner is
/// selecting are ignored, as is a SelectionInput while a command runs: only
/// the runner hands selections to commands.
///
/// Drawing aids (docs/adr/0008). While a command asks for a point, that is
/// while a command runs and the runner is not selecting, the points given
/// with the mouse are corrected: the pointer handed to the command, so the
/// preview follows, and the point of a click. If object snap is on and
/// core::findSnap() finds a snap within 10 pixels, its point is used; else,
/// with ortho on and a Command::basePoint(), orthoPoint() from it; else the
/// point is left as it is. Snap wins over ortho. The base point belongs to
/// the running command: the first point of a command is never aligned to a
/// point of the previous one. Typed coordinates are never corrected, but a
/// distance typed alone ("50") takes its direction from the pointer
/// corrected against the last point it is measured from, so with ortho on
/// it follows the horizontal or the vertical through that point.
/// Points given with submit() are never corrected: they are the exact
/// points of scripts and tests. While selecting nothing is corrected.
///
/// The registry and the editor must outlive the runner.
class CommandRunner {
public:
    CommandRunner(const CommandRegistry& registry, edit::Editor& editor);

    // The context hands commands a callback bound to this object.
    CommandRunner(const CommandRunner&) = delete;
    CommandRunner& operator=(const CommandRunner&) = delete;
    CommandRunner(CommandRunner&&) = delete;
    CommandRunner& operator=(CommandRunner&&) = delete;
    ~CommandRunner() = default;

    /// Text from the command line; empty text means Enter. Leading and
    /// trailing white space is ignored.
    void submitText(std::string_view text);

    /// Input from the mouse and the keyboard.
    void submit(const Input& input);

    /// The pointer moved to `point`, in drawing coordinates. Gives the
    /// direction of distances typed alone ("50") and the second corner of an
    /// open selection rectangle. While a command asks for a point, the
    /// command receives it corrected by the drawing aids.
    void pointerMoved(const core::Vec2& point);

    /// Drawing units per screen pixel; the view sets it at every zoom.
    /// Pick box and snap aperture, defined in pixels, follow it. While a
    /// command asks for a point, a change hands the command the pointer
    /// corrected again, since the snap may have changed.
    /// Precondition: > 0. Default 1.
    void setPixelSize(double drawingUnitsPerPixel);

    /// Left click in the view, in drawing coordinates (docs/adr/0008). While
    /// selecting it is a selection gesture; while a command asks for a point
    /// it becomes a PointInput, corrected by the drawing aids.
    void click(const core::Vec2& point, PickAction action);

    /// Object snap and ortho. While a command asks for a point, a change
    /// hands the command the pointer corrected again, so that the preview
    /// follows the new settings without waiting for the pointer to move.
    void setDrawingAids(const DrawingAids& aids);

    [[nodiscard]] const DrawingAids& drawingAids() const { return m_drawingAids; }

    /// The snap the pointer is on, for the marker; std::nullopt if none or
    /// if no command is asking for a point. Computed on each call, from the
    /// last pointer position and the drawing as it is now.
    [[nodiscard]] std::optional<core::SnapPoint> activeSnap() const;

    /// True while a command is running, including while asking "Select
    /// objects:" for it.
    [[nodiscard]] bool isActive() const { return m_command != nullptr; }

    /// True with no command running, or while asking "Select objects:".
    [[nodiscard]] bool isSelecting() const { return !m_command || m_askingSelection; }

    /// The current selection, or the one being built for a command.
    [[nodiscard]] const SelectionSet& selection() const { return m_selection; }

    /// The rectangle being traced, if any.
    [[nodiscard]] std::optional<SelectionRectangle> selectionRectangle() const {
        return m_rectangle;
    }

    /// "Specify opposite corner:" with an open rectangle, "Select objects:"
    /// while asking for a selection, otherwise the prompt of the running
    /// command, or "Command:" with none.
    [[nodiscard]] std::string promptText() const;

    /// The preview of the running command, empty with none or while asking
    /// for a selection.
    [[nodiscard]] std::span<const core::Entity> preview() const;

    /// Where messages go (the history of the command line). An empty
    /// function discards them.
    void setMessageSink(std::function<void(std::string)> sink);

private:
    void startCommand(std::string_view text);
    void beginCommand();
    void repeatLastCommand();
    void handleTypedWhileActive(std::string_view text);
    void submitWhileAskingSelection(const Input& input);
    void endSelectionQuestion();
    void selectionClick(const core::Vec2& point, PickAction action);
    void applyToSelection(std::span<const core::EntityId> ids, PickAction action);
    void forward(const Input& input);

    [[nodiscard]] bool asksForPoint() const { return isActive() && !m_askingSelection; }

    [[nodiscard]] std::optional<core::SnapPoint> snapAt(const core::Vec2& point) const;
    /// `point` corrected by the snap or, failing that, by ortho from
    /// `orthoBase`, if any.
    [[nodiscard]] core::Vec2 aidedPoint(const core::Vec2& point,
                                        const std::optional<core::Vec2>& orthoBase) const;
    void refreshPointer();
    void cancel();
    void finish();
    void clearSelection();
    void undo();
    void redo();
    void message(std::string text) const;

    const CommandRegistry& m_registry;
    edit::Editor& m_editor;
    CommandContext m_context;
    std::unique_ptr<Command> m_command;
    std::string m_lastCommand;            ///< canonical name, empty before the first one
    std::optional<core::Vec2> m_pointer;  ///< as given, before the drawing aids
    std::function<void(std::string)> m_sink;
    SelectionSet m_selection;
    std::optional<SelectionRectangle> m_rectangle;
    DrawingAids m_drawingAids;
    double m_pixelSize{1.0};
    bool m_askingSelection{false};  ///< m_command exists but is not started yet
};

}  // namespace tg::cmd
