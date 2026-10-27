// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_runner.hpp"

#include "text.hpp"
#include "tg/cmd/coordinate_input.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/math.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

constexpr std::string_view kIdlePrompt = "Command:";
constexpr std::string_view kInvalidInput = "Invalid point or option.";
constexpr std::string_view kInvalidDistance = "Requires a distance, a point or an option.";
constexpr std::string_view kInvalidAngle = "Requires an angle, a point or an option.";
constexpr std::string_view kCancelled = "*Cancel*";
constexpr std::string_view kNothingToUndo = "Nothing to undo.";
constexpr std::string_view kNothingToRedo = "Nothing to redo.";
constexpr std::string_view kSelectObjects = "Select objects:";
constexpr std::string_view kOppositeCorner = "Specify opposite corner:";
constexpr std::string_view kInvalidSelection = "Invalid selection.";

// Pick tolerance of docs/adr/0008, in screen pixels: AutoCAD's default
// PICKBOX.
constexpr double kPickBoxPixels = 3.0;

// Object snap aperture of docs/adr/0008, in screen pixels: AutoCAD's
// default APERTURE.
constexpr double kSnapAperturePixels = 10.0;

// Commands of the runner itself. "U" is the AutoCAD shortcut of a single
// undo step; here UNDO does the same, since undo groups and marks are not
// part of stage 0.2.
constexpr std::string_view kUndo = "UNDO";
constexpr std::string_view kUndoAlias = "U";
constexpr std::string_view kRedo = "REDO";

bool isUndo(std::string_view name) {
    return detail::equalsIgnoringCase(name, kUndo) || detail::equalsIgnoringCase(name, kUndoAlias);
}

bool isRedo(std::string_view name) {
    return detail::equalsIgnoringCase(name, kRedo);
}

// The default description check: any test that meets an undescribed prompt
// fails, wherever it runs (docs/adr/0026, point 3).
void abortOnUndescribedPrompt(const std::string& message) {
    std::cerr << message << "\n";
    std::abort();
}

bool matches(const Keyword& keyword, std::string_view text) {
    return detail::equalsIgnoringCase(text, keyword.name) ||
           (!keyword.shortcut.empty() && detail::equalsIgnoringCase(text, keyword.shortcut));
}

// The questions to the model still find the entities of a locked layer, but
// no selection gesture takes them in (docs/adr/0023, point 4). An entity
// whose layer the document lacks is not locked.
bool isOnLockedLayer(const core::Document& document, core::EntityId id) {
    const core::EntityRecord* record = document.find(id);
    if (record == nullptr) {
        return false;
    }
    const core::Layer* layer = document.layer(record->properties.layer);
    return layer != nullptr && layer->locked;
}

// The sentence that tells how many entities of a gesture were on locked
// layers; numbers in a fixed sentence, as the plan of stage 0.3 allows.
std::string lockedMessage(std::size_t count) {
    if (count == 1) {
        return "1 was on a locked layer.";
    }
    return std::to_string(count) + " were on a locked layer.";
}

}  // namespace

CommandRunner::CommandRunner(const CommandRegistry& registry, edit::Editor& editor)
    : m_registry(registry),
      m_editor(editor),
      m_context{editor, std::nullopt,
                [this](std::string text) {
                    message(std::move(text));
                }},
      m_descriptionCheck(abortOnUndescribedPrompt) {}

bool CommandRunner::replaceSelection(std::vector<core::EntityId> ids) {
    if (isActive()) {
        return false;
    }
    clearSelection();
    for (const auto id : ids) {
        m_selection.add(id);
    }
    return true;
}

void CommandRunner::setFileAccess(io::FileAccess* files) {
    m_context.files = files;
}

void CommandRunner::submitText(std::string_view text) {
    if (!acceptsText()) {
        text = detail::trim(text);
    }
    if (m_askingSelection) {
        if (text.empty()) {
            endSelectionQuestion();
        } else {
            // Written options (W, C, ALL...) will be recognised here
            // (docs/adr/0008); the open rectangle, if any, stays.
            message(std::string{kInvalidSelection});
        }
        return;
    }
    if (isActive()) {
        handleTypedWhileActive(text);
        return;
    }
    m_rectangle.reset();  // typing abandons a rectangle being traced
    if (text.empty()) {
        repeatLastCommand();
    } else {
        startCommand(text);
    }
}

void CommandRunner::submit(const Input& input) {
    if (m_askingSelection) {
        submitWhileAskingSelection(input);
        return;
    }
    if (!isActive()) {
        // Points, options and selections mean nothing without a command.
        if (std::holds_alternative<CancelInput>(input)) {
            if (m_rectangle) {
                m_rectangle.reset();
            } else {
                m_selection.clear();
            }
        } else if (std::holds_alternative<ConfirmInput>(input)) {
            m_rectangle.reset();
            repeatLastCommand();
        }
        return;
    }
    if (std::holds_alternative<CancelInput>(input)) {
        cancel();
        return;
    }
    if (std::holds_alternative<SelectionInput>(input)) {
        return;  // only the runner hands selections to commands
    }
    forward(input);
}

void CommandRunner::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    if (m_rectangle) {
        m_rectangle->second = point;
        m_rectangle->crossing = point.x < m_rectangle->first.x;
    }
    refreshPointer();
}

void CommandRunner::setPixelSize(double drawingUnitsPerPixel) {
    assert(drawingUnitsPerPixel > 0.0);
    m_pixelSize = drawingUnitsPerPixel;
    refreshPointer();
}

void CommandRunner::click(const core::Vec2& point, PickAction action) {
    if (isSelecting()) {
        selectionClick(point, action);
    } else if (asksForPoint()) {
        forward(PointInput{aidedPoint(point, m_command->basePoint())});
    }
}

void CommandRunner::setDrawingAids(const DrawingAids& aids) {
    if (aids == m_drawingAids) {
        return;
    }
    m_drawingAids = aids;
    refreshPointer();
}

std::optional<core::SnapPoint> CommandRunner::activeSnap() const {
    if (!asksForPoint() || !m_pointer) {
        return std::nullopt;
    }
    return snapAt(*m_pointer);
}

bool CommandRunner::acceptsText() const {
    return isActive() && !m_askingSelection && m_command->prompt().kind == PromptKind::Text;
}

std::string CommandRunner::promptText() const {
    if (m_rectangle) {
        return std::string{kOppositeCorner};
    }
    if (m_askingSelection) {
        return std::string{kSelectObjects};
    }
    return isActive() ? m_command->prompt().text : std::string{kIdlePrompt};
}

std::span<const core::Entity> CommandRunner::preview() const {
    return isActive() && !m_askingSelection ? m_command->preview()
                                            : std::span<const core::Entity>{};
}

void CommandRunner::setMessageSink(std::function<void(std::string)> sink) {
    m_sink = std::move(sink);
}

void CommandRunner::setDescriptionCheck(DescriptionCheck check) {
    m_descriptionCheck = std::move(check);
}

void CommandRunner::startCommand(std::string_view text) {
    if (isUndo(text)) {
        m_lastCommand = kUndo;
        clearSelection();
        undo();
        return;
    }
    if (isRedo(text)) {
        m_lastCommand = kRedo;
        clearSelection();
        redo();
        return;
    }
    const std::optional<std::string> name = m_registry.canonicalName(text);
    if (!name) {
        // Composed from a template with one argument, as the plan of stage
        // 0.2 prescribes; translation is decided in stage 0.9.
        message("Unknown command: " + std::string{text} + ".");
        return;
    }
    m_command = m_registry.create(*name);
    assert(m_command && "a registered factory returned no command");
    if (!m_command) {
        return;
    }
    m_lastCommand = *name;
    m_description = m_registry.describe(*name);
    if (m_command->needsSelection() && m_selection.isEmpty()) {
        m_askingSelection = true;
        return;
    }
    beginCommand();
}

void CommandRunner::beginCommand() {
    // The selection is handed over, then cleared: it never outlives the
    // start of a command (docs/adr/0008).
    SelectionInput selection;
    selection.ids.assign(m_selection.ids().begin(), m_selection.ids().end());
    clearSelection();
    m_command->start(m_context);
    // Immediate commands finish in start() and have no question to ask.
    if (m_command->prompt().text.empty()) {
        finish();
        return;
    }
    if (m_command->needsSelection()) {
        assert(!selection.ids.empty() && "a selection is never handed over empty");
        forward(selection);  // checks the prompt that follows
        return;
    }
    checkPrompt();
}

void CommandRunner::repeatLastCommand() {
    if (!m_lastCommand.empty()) {
        const std::string name = m_lastCommand;  // startCommand() overwrites it
        startCommand(name);
    }
}

void CommandRunner::handleTypedWhileActive(std::string_view text) {
    if (text.empty()) {
        forward(ConfirmInput{});
        return;
    }
    const Prompt prompt = m_command->prompt();
    if (prompt.kind == PromptKind::Text) {
        if (prompt.file != FilePrompt::None && text.size() >= 2 && text.front() == '"' &&
            text.back() == '"') {
            text = text.substr(1, text.size() - 2);
        }
        forward(TextInput{std::string{text}});
        return;
    }
    for (const Keyword& keyword : prompt.keywords) {
        if (matches(keyword, text)) {
            forward(KeywordInput{keyword.name});
            return;
        }
    }
    if (const std::optional<PointSpec> spec = parseCoordinate(text)) {
        if (spec->kind == PointSpec::Kind::Distance &&
            (prompt.kind == PromptKind::Distance || prompt.kind == PromptKind::Angle)) {
            // Multiply by the ratio so even large finite degree values do
            // not overflow before division by 180.
            const double value =
                prompt.kind == PromptKind::Angle ? spec->a * (core::kPi / 180.0) : spec->a;
            forward(ValueInput{value});
            return;
        }
        // Before the first pointer movement a typed distance has no
        // direction: falling back to the last point makes it unresolvable.
        // The pointer is corrected now, so that the direction is the one the
        // preview shows (docs/adr/0008). Ortho aligns it to the last point,
        // the one the distance is measured from: within a command it is the
        // base point; for the first point of a command it may come from the
        // previous one, as "@" does.
        const core::Vec2 pointer = m_pointer ? aidedPoint(*m_pointer, m_context.lastPoint)
                                             : m_context.lastPoint.value_or(core::Vec2{});
        if (const std::optional<core::Vec2> point = resolve(*spec, m_context.lastPoint, pointer)) {
            forward(PointInput{*point});
            return;
        }
    }
    switch (prompt.kind) {
        case PromptKind::Distance:
            message(std::string{kInvalidDistance});
            break;
        case PromptKind::Angle:
            message(std::string{kInvalidAngle});
            break;
        case PromptKind::Point:
        case PromptKind::Text:
            message(std::string{kInvalidInput});
            break;
    }
}

void CommandRunner::submitWhileAskingSelection(const Input& input) {
    if (std::holds_alternative<CancelInput>(input)) {
        if (m_rectangle) {
            m_rectangle.reset();
        } else {
            cancel();
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        endSelectionQuestion();
    }
}

void CommandRunner::endSelectionQuestion() {
    m_rectangle.reset();
    m_askingSelection = false;
    if (m_selection.isEmpty()) {
        m_command.reset();  // nothing chosen: the command ends with no effect
        return;
    }
    beginCommand();
}

void CommandRunner::selectionClick(const core::Vec2& point, PickAction action) {
    const core::Document& document = m_editor.document();
    if (m_rectangle) {
        const core::Vec2 first = m_rectangle->first;
        m_rectangle.reset();
        const core::RectangleMode mode =
            point.x < first.x ? core::RectangleMode::Crossing : core::RectangleMode::Window;
        applyToSelection(core::selectInRectangle(document, core::BBox{first, point}, mode), action);
        return;
    }
    if (const std::optional<core::EntityId> picked =
            core::pickEntity(document, point, kPickBoxPixels * m_pixelSize)) {
        applyToSelection(std::span{&*picked, 1}, action);
        return;
    }
    m_rectangle = SelectionRectangle{point, point, false};
}

void CommandRunner::applyToSelection(std::span<const core::EntityId> ids, PickAction action) {
    const core::Document& document = m_editor.document();
    std::size_t found = 0;
    std::size_t locked = 0;
    std::size_t removed = 0;
    for (const core::EntityId id : ids) {
        if (action == PickAction::Add) {
            // A locked layer's entity is discarded, not counted as found.
            if (isOnLockedLayer(document, id)) {
                ++locked;
                continue;
            }
            m_selection.add(id);
            ++found;
        } else if (m_selection.remove(id)) {
            // Nothing on a locked layer is ever selected, so there is nothing
            // to discard, and nothing to say, when a gesture takes entities out.
            ++removed;
        }
    }
    if (!m_askingSelection) {
        return;  // at rest the gestures are silent
    }
    // Numbers in a fixed sentence, as the plan of stage 0.3 allows.
    const std::string total = std::to_string(m_selection.size()) + " total";
    if (action == PickAction::Add) {
        message(std::to_string(found) + " found, " + total);
        if (locked > 0) {
            message(lockedMessage(locked));
        }
    } else {
        message(std::to_string(removed) + " removed, " + total);
    }
}

void CommandRunner::forward(const Input& input) {
    if (m_command->handle(m_context, input) == Status::Finished) {
        finish();
        return;
    }
    checkPrompt();
}

void CommandRunner::checkPrompt() const {
    if (!m_descriptionCheck || !isActive() || m_askingSelection) {
        return;
    }
    const Prompt prompt = m_command->prompt();
    if (prompt.text.empty()) {
        return;  // nothing shown
    }
    if (m_description != nullptr &&
        std::ranges::any_of(m_description->prompts, [&prompt](const PromptDescription& described) {
            return cmd::matches(described, prompt);
        })) {
        return;
    }
    // Composed for developers, never shown to users: not translated.
    m_descriptionCheck(m_lastCommand + ": undescribed prompt \"" + prompt.text + "\"");
}

std::optional<core::SnapPoint> CommandRunner::snapAt(const core::Vec2& point) const {
    if (!m_drawingAids.objectSnap) {
        return std::nullopt;
    }
    return core::findSnap(m_editor.document(), point, kSnapAperturePixels * m_pixelSize,
                          m_drawingAids.snapModes);
}

core::Vec2 CommandRunner::aidedPoint(const core::Vec2& point,
                                     const std::optional<core::Vec2>& orthoBase) const {
    // Snap wins over ortho (docs/adr/0008).
    if (const std::optional<core::SnapPoint> snap = snapAt(point)) {
        return snap->point;
    }
    if (m_drawingAids.ortho && orthoBase) {
        return orthoPoint(*orthoBase, point);
    }
    return point;
}

void CommandRunner::refreshPointer() {
    if (asksForPoint() && m_pointer) {
        m_command->pointerMoved(aidedPoint(*m_pointer, m_command->basePoint()));
    }
}

void CommandRunner::cancel() {
    // Keep the command alive for cancel(), while editor listeners already
    // see the runner idle when its transaction is closed.
    const std::unique_ptr<Command> command = std::move(m_command);
    const bool started = !m_askingSelection;
    m_askingSelection = false;
    clearSelection();
    if (started) {
        command->cancel(m_context);
    }
    const bool closed = !m_editor.inTransaction();
    assert(closed && "a command cancelled without closing its transaction");
    if (!closed) {
        m_editor.rollback();
    }
    message(std::string{kCancelled});
}

void CommandRunner::finish() {
    if (m_context.documentReplaced) {
        m_context.lastPoint.reset();
        m_context.documentReplaced = false;
    }
    m_command.reset();
    clearSelection();
    const bool closed = !m_editor.inTransaction();
    assert(closed && "a command finished without closing its transaction");
    if (!closed) {
        m_editor.rollback();
    }
}

void CommandRunner::clearSelection() {
    m_selection.clear();
    m_rectangle.reset();
}

void CommandRunner::undo() {
    const std::string name = m_editor.undoName();
    if (!m_editor.undo()) {
        message(std::string{kNothingToUndo});
        return;
    }
    message("Undone: " + name + ".");
}

void CommandRunner::redo() {
    const std::string name = m_editor.redoName();
    if (!m_editor.redo()) {
        message(std::string{kNothingToRedo});
        return;
    }
    message("Redone: " + name + ".");
}

void CommandRunner::message(std::string text) const {
    if (m_sink) {
        m_sink(std::move(text));
    }
}

}  // namespace tg::cmd
