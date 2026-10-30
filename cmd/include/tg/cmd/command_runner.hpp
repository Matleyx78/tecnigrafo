// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/drawing_aids.hpp"
#include "tg/cmd/input.hpp"
#include "tg/cmd/selection_set.hpp"
#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
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
#include <vector>

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
/// With a command running, empty text becomes ConfirmInput, except at a
/// Selection prompt. At a Text prompt all other text becomes TextInput
/// unchanged, spaces included. Otherwise options become KeywordInput, bare
/// numbers at Distance, Angle or Number prompts become ValueInput (angles
/// converted from degrees to radians, numbers as typed), and coordinates
/// become PointInput. Invalid input writes the message for that prompt
/// kind ("Requires a number." at a Number prompt) and the question stays
/// (docs/adr/0014 and docs/adr/0032, point 2).
///
/// Object prompts (docs/adr/0032, point 1). A click, a typed coordinate
/// (absolute, or relative to the last point) or a PointInput given with
/// submit() picks the entity there with core::pickEntity(), within 3
/// pixels, and hands it to the command as ObjectInput, with shift true for
/// a click with PickAction::Remove. An entity on a locked layer is not
/// picked: the runner writes "1 was on a locked layer." and the question
/// stays; with no entity there the question stays, without messages. Other
/// typed text writes "Invalid point or option.". Drawing aids never
/// correct these points.
///
/// Selection prompts (docs/adr/0032, point 3). The runner builds a selection
/// as it does for "Select objects:", with the same gestures, written
/// options and messages, while the command's prompt stays shown. Enter, or
/// a ConfirmInput, hands the command a SelectionInput with the ids chosen,
/// empty if none. The options of the prompt become KeywordInput, before the
/// written options; the selection built so far is then dropped, as it is
/// whenever the command receives an input at this prompt. A CancelInput
/// closes an open rectangle, or else cancels the command.
/// A CancelInput calls Command::cancel() and ends the command: the command
/// closes its transaction, keeping completed work or rolling it back
/// (docs/adr/0015).
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
///   SelectionInput; without, the command ends with no effect. A CancelInput
///   with no open rectangle cancels the command.
/// - Starting a command that does not need a selection, the end of every
///   command, U, UNDO and REDO clear the selection.
///
/// Written selection (docs/adr/0032, point 4). While asking "Select
/// objects:" or at a Selection prompt, typed text is read, in any case, as:
/// - a coordinate, absolute or relative to the last point: the same as a
///   click there with PickAction::Add;
/// - W or C (Window, Crossing): the prompt becomes "Specify first corner:";
///   the next point, typed or clicked, opens a rectangle that selects by
///   window, or by crossing, whatever the direction of its second corner;
/// - ALL: every entity on a visible layer, as one gesture, so entities on
///   locked layers are discarded and counted as for a rectangle.
/// Other text writes "Invalid selection.". While a corner is asked for,
/// "Specify first corner:" or "Specify opposite corner:", text that is not
/// a coordinate writes "Invalid point or option.". Enter ends the question
/// as usual, dropping the corner being asked for.
///
/// Locked layers (docs/adr/0023, point 4). A gesture that adds entities
/// never selects those on a locked layer: it discards them, with a command
/// or without. While the runner asks "Select objects:", a gesture that
/// discarded some writes "<n> found, <t> total", where n counts the entities
/// it took in, and then "1 was on a locked layer." or "<n> were on a locked
/// layer."; with no command running it writes nothing. A gesture that takes
/// entities out says nothing about locked layers.
///
/// Points, options and selections given with submit() while the runner is
/// selecting are ignored, except the options of a Selection prompt, which
/// reach the command. A SelectionInput or an ObjectInput given with submit()
/// is always ignored: only the runner hands selections and objects to
/// commands.
///
/// Drawing aids (docs/adr/0008 and docs/adr/0014). While a command asks for
/// a point, distance, angle or number, the points given
/// with the mouse are corrected: the pointer handed to the command, so the
/// preview follows, and the point of a click. If object snap is on and
/// core::findSnap() finds a snap within 10 pixels, its point is used; else,
/// with ortho on and a Command::basePoint(), orthoPoint() from it; else the
/// point is left as it is. Snap wins over ortho. The base point belongs to
/// the running command: the first point of a command is never aligned to a
/// point of the previous one. Typed coordinates are never corrected, but a
/// distance typed alone ("50") at a Point prompt takes its direction from the pointer
/// corrected against the last point it is measured from, so with ortho on
/// it follows the horizontal or the vertical through that point.
/// Points given with submit() are never corrected: they are the exact
/// points of scripts and tests. While selecting, and at Object prompts,
/// nothing is corrected: at an Object prompt the command receives the
/// pointer as it is.
///
/// Descriptions (docs/adr/0026, point 3). Every prompt that the running
/// command shows, after start() and after each input it handles, must
/// match one PromptDescription of its command; an empty text is not a
/// prompt. The runner's own prompts ("Select objects:", "Specify first
/// corner:", "Specify opposite corner:", "Command:") are not checked. A prompt that matches none
/// calls the description check.
///
/// The registry and the editor must outlive the runner.
class CommandRunner {
public:
    /// Called with a message when a running command shows a prompt that its
    /// description lacks (docs/adr/0026, point 3).
    using DescriptionCheck = std::function<void(const std::string& message)>;

    CommandRunner(const CommandRegistry& registry, edit::Editor& editor);

    // The context hands commands a callback bound to this object.
    CommandRunner(const CommandRunner&) = delete;
    CommandRunner& operator=(const CommandRunner&) = delete;
    CommandRunner(CommandRunner&&) = delete;
    CommandRunner& operator=(CommandRunner&&) = delete;
    ~CommandRunner() = default;

    /// Text from the command line; empty text means Enter. Leading and
    /// trailing white space is ignored except at a Text prompt.
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
    /// selecting it is a selection gesture; at an Object prompt it picks an
    /// entity; at a Point, Distance, Angle or Number prompt it becomes a
    /// PointInput, corrected by the drawing aids. Text prompts ignore it.
    void click(const core::Vec2& point, PickAction action);

    /// Object snap and ortho. While a command asks for a point, a change
    /// hands the command the pointer corrected again, so that the preview
    /// follows the new settings without waiting for the pointer to move.
    void setDrawingAids(const DrawingAids& aids);

    [[nodiscard]] const DrawingAids& drawingAids() const { return m_drawingAids; }

    /// The snap the pointer is on, for the marker; std::nullopt if none or
    /// if no command is asking for a point, distance, angle or number. Computed on each call, from
    /// the last pointer position and the drawing as it is now.
    [[nodiscard]] std::optional<core::SnapPoint> activeSnap() const;

    /// True while a command is running, including while asking "Select
    /// objects:" for it.
    [[nodiscard]] bool isActive() const { return m_command != nullptr; }

    /// True while the running command asks for Text: the view then treats
    /// the space bar as a character, not as Enter.
    [[nodiscard]] bool acceptsText() const;

    /// True with no command running, while asking "Select objects:" and at a
    /// Selection prompt of the running command.
    [[nodiscard]] bool isSelecting() const { return !m_command || inSelectionQuestion(); }

    /// The current selection, or the one being built for a command.
    [[nodiscard]] const SelectionSet& selection() const { return m_selection; }

    /// Replaces the idle selection without messages; false while a command runs.
    [[nodiscard]] bool replaceSelection(std::vector<core::EntityId> ids);

    /// The rectangle being traced, if any.
    [[nodiscard]] std::optional<SelectionRectangle> selectionRectangle() const {
        return m_rectangle;
    }

    /// "Specify opposite corner:" with an open rectangle, "Specify first
    /// corner:" after a written W or C, "Select objects:" while asking for a
    /// selection, otherwise the prompt of the running command, or "Command:"
    /// with none.
    [[nodiscard]] std::string promptText() const;

    /// The structured question, including file-dialog metadata, for the UI:
    /// the running command's, unless the runner asks its own question.
    [[nodiscard]] Prompt prompt() const;

    /// The preview of the running command, empty with none or while asking
    /// for a selection.
    [[nodiscard]] std::span<const core::Entity> preview() const;

    /// File access for commands, not owned. nullptr disables file commands.
    void setFileAccess(io::FileAccess* files);

    /// AutoCAD script sequences (ADR 0032, points 6-8). Default false.
    void setScriptSequences(bool on);

    /// Where messages go (the history of the command line). An empty
    /// function discards them.
    void setMessageSink(std::function<void(std::string)> sink);

    /// The default check writes the message to std::cerr and calls
    /// std::abort(), so that any test fails. An empty function turns the check
    /// off, as the application does outside --script.
    void setDescriptionCheck(DescriptionCheck check);

private:
    void startCommand(std::string_view text);
    void beginCommand();
    void repeatLastCommand();
    void handleTypedWhileActive(std::string_view text);
    /// Text at a Text prompt.
    void typedText(std::string_view text, const Prompt& prompt);
    /// Text at a Point, Distance or Angle prompt.
    void typedPoint(std::string_view text, const Prompt& prompt);
    /// Text at a Number prompt.
    void typedNumber(std::string_view text, const Prompt& prompt);
    /// Text at an Object prompt.
    void typedObject(std::string_view text, const Prompt& prompt);
    /// Text at a Selection prompt of the running command.
    void typedAtSelectionPrompt(std::string_view text, const Prompt& prompt);
    /// Written selection, shared by "Select objects:" and Selection prompts.
    void typedSelection(std::string_view text);
    /// Forwards the keyword of `prompt` that `text` names; false if none.
    bool forwardKeyword(std::string_view text, const Prompt& prompt);
    /// A typed coordinate, absolute or relative to the last point, as a point
    /// that needs no pointer; std::nullopt if `text` is none.
    [[nodiscard]] std::optional<core::Vec2> typedPosition(std::string_view text) const;
    void submitWhileAskingSelection(const Input& input);
    void submitAtSelectionPrompt(const Input& input);
    void endSelectionQuestion();
    /// Enter at a Selection prompt: hands over what has been chosen.
    void endSelectionPrompt();
    void selectionClick(const core::Vec2& point, PickAction action);
    void applyToSelection(std::span<const core::EntityId> ids, PickAction action);
    /// Picks the entity at `point` for an Object prompt.
    void pickObject(const core::Vec2& point, bool shift);
    /// Closes an open rectangle, and the written W or C that opened it, if
    /// any; false if no rectangle is open.
    bool closeRectangle();
    void forward(const Input& input);
    /// Calls the description check if the running command shows a prompt
    /// that its description lacks.
    void checkPrompt() const;

    /// The kind of the running command's prompt; std::nullopt with no
    /// command, or while the runner asks "Select objects:" before starting
    /// it.
    [[nodiscard]] std::optional<PromptKind> promptKind() const;

    /// Point, Distance, Angle and Number prompts accept points from the
    /// mouse, corrected by the drawing aids.
    [[nodiscard]] bool asksForPoint() const;

    [[nodiscard]] bool asksForObject() const { return promptKind() == PromptKind::Object; }

    /// "Select objects:" or a Selection prompt of the running command: the
    /// questions where gestures write messages.
    [[nodiscard]] bool inSelectionQuestion() const {
        return m_askingSelection || promptKind() == PromptKind::Selection;
    }

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
    DescriptionCheck m_descriptionCheck;
    const CommandDescription* m_description{nullptr};  ///< of the running command
    SelectionSet m_selection;
    std::optional<SelectionRectangle> m_rectangle;
    /// After a written W or C: the mode of the rectangle, whatever its
    /// direction, asked for or open.
    std::optional<core::RectangleMode> m_writtenMode;
    DrawingAids m_drawingAids;
    double m_pixelSize{1.0};
    bool m_askingSelection{false};  ///< m_command exists but is not started yet
};

}  // namespace tg::cmd
