// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_runner.hpp"

#include "commands/grip_command.hpp"
#include "object_tracking.hpp"
#include "text.hpp"
#include "tg/cmd/command_protocol.hpp"
#include "tg/cmd/commands/grid_commands.hpp"
#include "tg/cmd/coordinate_input.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/snap_points.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
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
constexpr std::string_view kInvalidNumber = "Requires a number.";
constexpr std::string_view kCancelled = "*Cancel*";
constexpr std::string_view kNothingToUndo = "Nothing to undo.";
constexpr std::string_view kNothingToRedo = "Nothing to redo.";
constexpr std::string_view kSelectObjects = "Select objects:";
constexpr std::string_view kFirstCorner = "Specify first corner:";
constexpr std::string_view kOppositeCorner = "Specify opposite corner:";
constexpr std::string_view kInvalidSelection = "Invalid selection.";
constexpr std::string_view kNoPreviousSelection = "No previous selection set.";

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

// The point the drawing aids start from, for a point given with the mouse:
// with the snap to the grid on, the grid node of `point`, computed here
// only, so that the click, the preview, tracking(), activePolar() and the
// report agree (docs/adr/0055, point 4; docs/tappe/0.8.1/PT28-griglia-cmd.md,
// point 2). The object snap looks at `point` itself, and wins.
core::Vec2 onGrid(const core::Document& document, const core::Vec2& point) {
    const core::GridSettings& grid = document.grid();
    return grid.snapOn ? gridNode(grid, point) : point;
}

// The default description check: any test that meets an undescribed prompt
// fails, wherever it runs (docs/adr/0026, point 3).
void abortOnUndescribedPrompt(const std::string& message) {
    std::cerr << message << "\n";
    std::abort();
}

// The written selection options of docs/adr/0032, point 4, read like the
// options of a prompt: the name or the shortcut, in any case.
bool isWindowOption(std::string_view text) {
    return matchesKeyword(Keyword{"Window", "W"}, text);
}

bool isCrossingOption(std::string_view text) {
    return matchesKeyword(Keyword{"Crossing", "C"}, text);
}

bool isAllOption(std::string_view text) {
    return matchesKeyword(Keyword{"ALL", ""}, text);
}

bool isLastOption(std::string_view text) {
    return matchesKeyword(Keyword{"Last", "L"}, text);
}

bool isPreviousOption(std::string_view text) {
    return matchesKeyword(Keyword{"Previous", "P"}, text);
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

// The selection never takes in an entity on a locked layer, but a layer can
// be locked, frozen or switched off while the selection stays (from the
// interface, with no command): such an entity shows no grip, so that no grip
// edits it (docs/adr/0023, point 4).
bool showsGrips(const core::Document& document, const core::EntityRecord& record) {
    return core::isVisible(document, record) && !isOnLockedLayer(document, record.id);
}

using Acquisition = CommandRunner::TrackingState::Acquisition;

bool isEmpty(const Acquisition& acquisition) {
    return !acquisition.point && !acquisition.extension && !acquisition.parallel;
}

/// True if `acquisition` holds the point, the extension (the same entity,
/// segment and end) or the direction (the same entity and segment) that
/// `wanted` holds: holding there again releases it.
bool holdsAnyOf(const Acquisition& acquisition, const Acquisition& wanted) {
    if (wanted.point && acquisition.point == wanted.point) {
        return true;
    }
    if (wanted.extension && acquisition.extension &&
        acquisition.extension->entity == wanted.extension->entity &&
        acquisition.extension->segment == wanted.extension->segment &&
        acquisition.extension->atStart == wanted.extension->atStart) {
        return true;
    }
    return wanted.parallel && acquisition.parallel &&
           acquisition.parallel->entity == wanted.parallel->entity &&
           acquisition.parallel->segment == wanted.parallel->segment;
}

/// The snap points that compete within the aperture (docs/REGOLE-NUMERICHE.md,
/// section 4, "Selezione e snap a oggetto", point 2), not Nearest and Center,
/// which only come when none does.
bool isPointSnap(core::SnapKind kind) {
    return kind != core::SnapKind::Nearest && kind != core::SnapKind::Center;
}

/// The end of a line, of an arc or of the first or last segment of an open
/// polyline within `aperture` of `pointer` that the hold takes as Endpoint
/// with Endpoint off and Extension on (docs/tappe/0.8.1/PT31, point 1): the
/// nearest; on equal distance the highest id, then the start of a line or
/// arc, the first vertex of a polyline, as findSnap() orders endpoints.
/// Only ends that give an extension: not those of a curve of zero length.
/// The points are those appendSnapPoints() gives.
std::optional<core::SnapHit> extensionEndAt(const core::Document& document,
                                            const core::Vec2& pointer, double aperture) {
    // A little larger than the aperture, as findSnap() looks: the exact
    // distance decides.
    const double margin =
        aperture + (32.0 * std::numeric_limits<double>::epsilon() *
                    std::max({std::abs(pointer.x), std::abs(pointer.y), aperture}));
    std::vector<core::EntityId> ids;
    document.query(
        core::BBox{pointer - core::Vec2{margin, margin}, pointer + core::Vec2{margin, margin}},
        ids);
    std::optional<core::SnapHit> best;
    double bestDistance = 0.0;
    const auto consider = [&](const core::Vec2& end, core::EntityId id,
                              std::optional<std::size_t> segment, bool atStart) {
        const double distance = core::distance(end, pointer);
        if (distance > aperture) {
            return;
        }
        if (!best || distance < bestDistance ||
            (distance == bestDistance && id.value > best->entity.value)) {
            best = core::SnapHit{{end, core::SnapKind::Endpoint}, id, segment, atStart};
            bestDistance = distance;
        }
    };
    core::SnapModes endpoints;  // perpendicular, tangent and the others are off
    endpoints.midpoint = false;
    endpoints.center = false;
    endpoints.insertion = false;
    endpoints.intersection = false;
    std::vector<core::SnapPoint> ends;
    for (const core::EntityId id : ids) {
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr || !core::isVisible(document, *record)) {
            continue;
        }
        if (const auto* polyline = std::get_if<core::Polyline>(&record->entity)) {
            const std::size_t count = polyline->segmentCount();
            if (polyline->isClosed() || count == 0) {
                continue;
            }
            if (const core::PolylineSegment first = polyline->segment(0);
                first.start != first.end) {
                consider(first.start, id, 0, true);
            }
            if (const core::PolylineSegment last = polyline->segment(count - 1);
                last.start != last.end) {
                consider(last.end, id, count - 1, false);
            }
            continue;
        }
        const bool isLine = std::holds_alternative<core::Line>(record->entity);
        if (!isLine && !std::holds_alternative<core::Arc>(record->entity)) {
            continue;
        }
        ends.clear();
        core::appendSnapPoints(record->entity, endpoints, ends);
        if (ends.size() != 2 || (isLine && ends[0].point == ends[1].point)) {
            continue;  // a line of zero length shows no direction
        }
        consider(ends[0].point, id, std::nullopt, true);
        consider(ends[1].point, id, std::nullopt, false);
    }
    return best;
}

/// The extension beyond the Endpoint `hit` (docs/adr/0057, point 3), if
/// `hit` is an end of a line, of an arc or of the first or last segment of
/// an open polyline that shows a direction.
std::optional<Acquisition::Extension> extensionOf(const core::Document& document,
                                                  const core::SnapHit& hit) {
    const core::EntityRecord* record = document.find(hit.entity);
    if (record == nullptr) {
        return std::nullopt;
    }
    Acquisition::Extension extension{hit.entity, hit.segment, hit.atStart, hit.snap.point, {}, {}};
    // The rest of the circle of an arc, from the end acquired, turning away
    // from the arc: clockwise from its start if it turns counter-clockwise.
    const auto beyond = [&](const core::ArcGeometry& arc) {
        const double rest = core::kTwoPi - std::abs(arc.sweep);
        const double away = arc.sweep > 0.0 ? rest : -rest;
        return hit.atStart
                   ? core::ArcGeometry{arc.center, arc.radius, arc.startAngle, -away}
                   : core::ArcGeometry{arc.center, arc.radius, arc.startAngle + arc.sweep, away};
    };
    const auto straight = [&](const core::Vec2& start, const core::Vec2& end) {
        return hit.atStart ? start - end : end - start;
    };
    if (const auto* line = std::get_if<core::Line>(&record->entity)) {
        extension.direction = straight(line->start, line->end);
    } else if (const auto* arc = std::get_if<core::Arc>(&record->entity)) {
        extension.beyond = beyond(arc->geometry());
    } else if (const auto* polyline = std::get_if<core::Polyline>(&record->entity)) {
        const std::size_t count = polyline->segmentCount();
        if (polyline->isClosed() || count == 0 || !hit.segment) {
            return std::nullopt;
        }
        const bool atFirstEnd = *hit.segment == 0 && hit.atStart;
        const bool atLastEnd = *hit.segment == count - 1 && !hit.atStart;
        if (!atFirstEnd && !atLastEnd) {
            return std::nullopt;  // an inner vertex has no extension
        }
        const core::PolylineSegment segment = polyline->segment(*hit.segment);
        if (segment.isArc()) {
            extension.beyond =
                beyond(core::arcFromBulge(segment.start, segment.end, segment.bulge));
        } else {
            extension.direction = straight(segment.start, segment.end);
        }
    } else {
        return std::nullopt;
    }
    if (!extension.beyond && extension.direction == core::Vec2{}) {
        return std::nullopt;  // no direction
    }
    return extension;
}

/// The line, or the straight polyline segment, whose direction a hold at
/// `pointer` acquires (docs/adr/0057, point 3): the entity a click within
/// `aperture` picks and, in a polyline, its nearest segment, the first on
/// ties; none if it is not straight, or the pointer is within `aperture` of
/// one of its ends.
std::optional<Acquisition::Parallel> parallelAt(const core::Document& document,
                                                const core::Vec2& pointer, double aperture) {
    const std::optional<core::EntityId> picked = core::pickEntity(document, pointer, aperture);
    if (!picked) {
        return std::nullopt;
    }
    const core::EntityRecord* record = document.find(*picked);
    std::optional<std::size_t> index;
    core::PolylineSegment segment;
    if (const auto* line = std::get_if<core::Line>(&record->entity)) {
        segment = {line->start, line->end, 0.0};
    } else if (const auto* polyline = std::get_if<core::Polyline>(&record->entity)) {
        double nearest = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < polyline->segmentCount(); ++i) {
            const double distance = core::distance(polyline->segment(i), pointer);
            if (!index || distance < nearest) {
                index = i;
                nearest = distance;
            }
        }
        if (!index) {
            return std::nullopt;
        }
        segment = polyline->segment(*index);
    } else {
        return std::nullopt;
    }
    if (segment.isArc() || segment.start == segment.end ||
        core::distance(segment.start, pointer) <= aperture ||
        core::distance(segment.end, pointer) <= aperture) {
        return std::nullopt;
    }
    return Acquisition::Parallel{*picked, index, segment.start, segment.end};
}

}  // namespace

CommandContext::CommandContext(ProtocolContext initialProtocol, edit::Editor& initialEditor,
                               edit::DocumentFile& initialFile,
                               std::optional<core::Vec2> initialLastPoint,
                               CommandDefaults initialDefaults, io::FileAccess* initialFiles,
                               bool initiallyReplaced, DocumentHost* initialHost)
    : ProtocolContext(std::move(initialProtocol)),
      editor(initialEditor),
      file(initialFile),
      lastPoint(initialLastPoint),
      defaults(std::move(initialDefaults)),
      files(initialFiles),
      documentReplaced(initiallyReplaced),
      host(initialHost) {}

CommandRunner::CommandRunner(const CommandRegistry& registry, edit::Editor& editor)
    : m_registry(registry),
      m_editor(editor),
      m_context(CommandContext{{[this](std::string text) {
                                   message(std::move(text));
                               }},
                               editor,
                               m_ownFile,
                               {}}),
      m_descriptionCheck(abortOnUndescribedPrompt) {}

// P23 specifies this owning signature; keep the public API used by the UI.
// NOLINTNEXTLINE(performance-unnecessary-value-param)
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

void CommandRunner::setScriptSequences(bool on) {
    context().scriptSequences = on;
}

void CommandRunner::setFileAccess(io::FileAccess* files) {
    context().files = files;
}

CommandContext& CommandRunner::context() {
    if (!m_context.has_value()) {
        std::abort();
    }
    return *m_context;
}

const CommandContext& CommandRunner::context() const {
    if (!m_context.has_value()) {
        std::abort();
    }
    return *m_context;
}

void CommandRunner::setDocumentFile(edit::DocumentFile* file) {
    // A reference cannot be rebound. Carry the command session state over
    // when rebuilding the context around a different file.
    CommandContext previous = std::move(context());
    m_context.emplace(CommandContext{{std::move(previous.message), previous.scriptSequences},
                                     previous.editor,
                                     file != nullptr ? *file : m_ownFile,
                                     previous.lastPoint,
                                     previous.defaults,
                                     previous.files,
                                     previous.documentReplaced,
                                     previous.host});
}

void CommandRunner::setDocumentHost(DocumentHost* host) {
    context().host = host;
}

void CommandRunner::submitText(std::string_view text) {
    if (!acceptsText()) {
        text = detail::trim(text);
    }
    if (m_askingSelection) {
        if (text.empty()) {
            endSelectionQuestion();
        } else {
            typedSelection(text);
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
        if (!(inSelectionQuestion() && closeRectangle())) {
            cancel();
        }
        return;
    }
    if (std::holds_alternative<SelectionInput>(input) ||
        std::holds_alternative<ObjectInput>(input)) {
        return;  // only the runner hands selections and objects to commands
    }
    if (promptKind() == PromptKind::Selection) {
        submitAtSelectionPrompt(input);
        return;
    }
    if (const auto* point = std::get_if<PointInput>(&input); point != nullptr && asksForObject()) {
        pickObject(point->point, false);
        return;
    }
    forward(input);
}

void CommandRunner::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    if (m_rectangle) {
        m_rectangle->second = point;
        m_rectangle->crossing = m_writtenMode ? *m_writtenMode == core::RectangleMode::Crossing
                                              : point.x < m_rectangle->first.x;
    }
    refreshPointer();
}

void CommandRunner::pointerHeld(const core::Vec2& point) {
    pointerMoved(point);
    if (!m_drawingAids.objectSnap || !asksForPoint()) {
        return;
    }
    const TrackingState::Acquisition wanted = acquisitionAt(point);
    if (isEmpty(wanted)) {
        return;
    }
    // Held again on what an acquisition holds, it releases the whole
    // acquisition; otherwise it acquires, and the eighth drops the oldest
    // (docs/adr/0049, point 2; docs/adr/0057, point 3).
    const auto found = std::ranges::find_if(
        m_acquisitions, [&](const auto& acquisition) { return holdsAnyOf(acquisition, wanted); });
    if (found != m_acquisitions.end()) {
        m_acquisitions.erase(found);
    } else {
        if (m_acquisitions.size() == kMaxAcquired) {
            m_acquisitions.erase(m_acquisitions.begin());
        }
        m_acquisitions.push_back(wanted);
    }
    refreshPointer();
}

CommandRunner::TrackingState::Acquisition CommandRunner::acquisitionAt(
    const core::Vec2& point) const {
    const core::Document& document = m_editor.document();
    const core::SnapModes& modes = m_drawingAids.snapModes;
    const double aperture = kSnapAperturePixels * m_pixelSize;
    const std::optional<core::Vec2> base = m_command->basePoint();
    // The snap that wins the hold, as snapAt() finds it; with Extension on,
    // the ends that give an extension compete as Endpoint even with Endpoint
    // off, and win ties, as Endpoint does (docs/tappe/0.8.1/PT31, point 1).
    std::optional<core::SnapHit> hit = core::findSnapHit(document, point, aperture, modes, base);
    if (modes.extension && !modes.endpoint) {
        const std::optional<core::SnapHit> end = extensionEndAt(document, point, aperture);
        if (end &&
            (!hit || !isPointSnap(hit->snap.kind) ||
             core::distance(end->snap.point, point) <= core::distance(hit->snap.point, point))) {
            hit = end;
        }
    }
    TrackingState::Acquisition wanted;
    if (m_drawingAids.tracking && hit &&
        (hit->snap.kind != core::SnapKind::Endpoint || modes.endpoint)) {
        wanted.point = hit->snap.point;
    }
    if (hit && isPointSnap(hit->snap.kind)) {
        // A snap point wins: an extension only from its Endpoint, and no
        // direction (#992, second round, [decisione] 2).
        if (modes.extension && hit->snap.kind == core::SnapKind::Endpoint) {
            wanted.extension = extensionOf(document, *hit);
        }
    } else if (modes.parallel && base) {
        wanted.parallel = parallelAt(document, point, aperture);
    }
    return wanted;
}

void CommandRunner::dropSwitchedOff() {
    if (!m_drawingAids.objectSnap) {
        clearAcquired();
        return;
    }
    const core::SnapModes& modes = m_drawingAids.snapModes;
    for (TrackingState::Acquisition& acquisition : m_acquisitions) {
        if (!m_drawingAids.tracking) {
            acquisition.point.reset();
        }
        if (!modes.extension) {
            acquisition.extension.reset();
        }
        if (!modes.parallel) {
            acquisition.parallel.reset();
        }
    }
    std::erase_if(m_acquisitions, isEmpty);
}

const CommandRunner::TrackingState& CommandRunner::tracking() const {
    m_tracking = TrackingState{};
    m_tracking.acquisitions = m_acquisitions;
    for (const TrackingState::Acquisition& acquisition : m_acquisitions) {
        if (acquisition.point) {
            m_tracking.acquired.push_back(*acquisition.point);
        }
    }
    if (!m_pointer || !followsGuides() || snapAt(*m_pointer)) {
        return m_tracking;
    }
    TrackingState state =
        trackedPoint(onGrid(m_editor.document(), *m_pointer), m_command->basePoint());
    m_tracking.guide = state.guide;
    m_tracking.guidePoint = state.guidePoint;
    m_tracking.from = state.from;
    m_tracking.guides = std::move(state.guides);
    return m_tracking;
}

std::optional<CommandRunner::PolarTracking> CommandRunner::activePolar() const {
    if (!m_drawingAids.polar || m_drawingAids.ortho || !m_pointer || !asksForPoint() ||
        snapAt(*m_pointer)) {
        return std::nullopt;
    }
    const auto base = m_command->basePoint();
    if (!base) {
        return std::nullopt;
    }
    // Object snap tracking comes before polar (docs/adr/0049, point 5).
    const TrackingState& state = tracking();
    if (state.guide == TrackingState::Guide::WithPolar && state.guidePoint) {
        return PolarTracking{*base, *state.guidePoint};
    }
    if (state.guide != TrackingState::Guide::None) {
        return std::nullopt;
    }
    const auto point = polarFoot(*base, onGrid(m_editor.document(), *m_pointer));
    if (!point) {
        return std::nullopt;
    }
    return PolarTracking{*base, *point};
}

void CommandRunner::setPixelSize(double drawingUnitsPerPixel) {
    assert(drawingUnitsPerPixel > 0.0);
    m_pixelSize = drawingUnitsPerPixel;
    refreshPointer();
}

void CommandRunner::click(const core::Vec2& point, PickAction action) {
    // A grip belongs to an entity already selected, so it wins over a
    // selection gesture (docs/adr/0034, "Conseguenze"); an open rectangle
    // waits for its second corner, and Shift takes entities out.
    if (!isActive() && action == PickAction::Add && !m_rectangle && clickGrip(point)) {
        return;
    }
    if (isSelecting()) {
        selectionClick(point, action);
    } else if (asksForObject()) {
        pickObject(point, action == PickAction::Remove);
    } else if (asksForPoint()) {
        forward(PointInput{aidedPoint(point, m_command->basePoint())});
    }
}

void CommandRunner::setDrawingAids(const DrawingAids& aids) {
    if (aids == m_drawingAids) {
        return;
    }
    m_drawingAids = aids;
    // Tracking works only with object snap; a mode switched off takes its
    // part of the acquisitions away for good (docs/tappe/0.8.1/PT31, point 1).
    dropSwitchedOff();
    refreshPointer();
}

std::optional<core::Vec2> CommandRunner::basePoint() const {
    if (!isActive() || m_askingSelection) {
        return std::nullopt;
    }
    return m_command->basePoint();
}

core::Vec2 CommandRunner::clickPoint(const core::Vec2& position) const {
    // The same correction as click() (docs/adr/0042, point 3).
    return asksForPoint() ? aidedPoint(position, m_command->basePoint()) : position;
}

std::optional<core::SnapPoint> CommandRunner::activeSnap() const {
    if (!asksForPoint() || !m_pointer) {
        return std::nullopt;
    }
    return snapAt(*m_pointer);
}

std::vector<core::Vec2> CommandRunner::grips() const {
    std::vector<core::Vec2> points;
    if (isActive() || m_selection.size() > kMaxGripEntities) {
        return points;
    }
    const core::Document& document = m_editor.document();
    for (const core::EntityId id : m_selection.ids()) {
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr || !showsGrips(document, *record)) {
            continue;
        }
        const std::vector<core::Vec2> own = core::gripPoints(record->entity);
        points.insert(points.end(), own.begin(), own.end());
    }
    return points;
}

bool CommandRunner::acceptsText() const {
    return promptKind() == PromptKind::Text;
}

std::string CommandRunner::promptText() const {
    if (m_rectangle) {
        return std::string{kOppositeCorner};
    }
    if (m_writtenMode) {
        return std::string{kFirstCorner};
    }
    if (m_askingSelection) {
        return std::string{kSelectObjects};
    }
    return isActive() ? m_command->prompt().text : std::string{kIdlePrompt};
}

Prompt CommandRunner::prompt() const {
    // The runner's own questions: "Command:", "Select objects:" and the
    // corners of a rectangle, also at a Selection prompt of the command.
    if (!isActive() || m_askingSelection || m_rectangle || m_writtenMode) {
        return Prompt{promptText(), {}};
    }
    return m_command->prompt();
}

std::span<const core::EntityId> CommandRunner::highlighted() const {
    return isActive() && !m_askingSelection ? m_command->highlighted()
                                            : std::span<const core::EntityId>{};
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

void CommandRunner::setEnvironment(std::string id) {
    assert(!isActive() && "the environment can only change while idle");
    m_environment = std::move(id);
}

const std::string& CommandRunner::environment() const {
    return m_environment;
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
    const CommandDescription* description = m_registry.describe(text);
    if (description == nullptr) {
        // Composed from a template with one argument, as the plan of stage
        // 0.2 prescribes; translation is decided in stage 0.9.
        message("Unknown command: " + std::string{text} + ".");
        return;
    }
    const auto& environments = description->environments;
    if (!environments.empty() &&
        std::ranges::find(environments, m_environment) == environments.end()) {
        message("Command \"" + description->name + "\" is not available in this environment.");
        return;
    }
    m_command = m_registry.create(description->name);
    assert(m_command && "a registered factory returned no command");
    if (!m_command) {
        return;
    }
    m_lastCommand = description->name;
    m_description = description;
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
    // The objects a command starts with, chosen at "Select objects:" or
    // preselected, become the previous selection set (PT34, point 2).
    if (m_command->needsSelection() && !selection.ids.empty()) {
        m_previous = selection.ids;
    }
    m_command->start(context());
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
    // TEXTEDIT accepts one pickfirst object without becoming a
    // needsSelection() command: that would add a runner-owned selection
    // prompt when there is no pickfirst selection (issue #939).
    if (m_description != nullptr && m_description->name == "TEXTEDIT" &&
        selection.ids.size() == 1) {
        forward(ObjectInput{selection.ids.front(), {}, false});
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
    const Prompt prompt = m_command->prompt();
    if (prompt.kind == PromptKind::Selection) {
        typedAtSelectionPrompt(text, prompt);  // Enter hands over the selection
        return;
    }
    if (text.empty()) {
        forward(ConfirmInput{});
        return;
    }
    // One function per kind of prompt: the later packages of stage 0.7 extend
    // them separately.
    switch (prompt.kind) {
        case PromptKind::Point:
        case PromptKind::Distance:
        case PromptKind::Angle:
            typedPoint(text, prompt);
            break;
        case PromptKind::Text:
            typedText(text, prompt);
            break;
        case PromptKind::Name:
            // submitText() trims non-text answers before dispatching them.
            forward(TextInput{std::string{text}});
            break;
        case PromptKind::Object:
            typedObject(text, prompt);
            break;
        case PromptKind::Number:
            typedNumber(text, prompt);
            break;
        case PromptKind::Selection:
            break;  // handled above
    }
}

void CommandRunner::typedText(std::string_view text, const Prompt& prompt) {
    forward(TextInput{textAnswer(prompt, text)});
}

void CommandRunner::typedPoint(std::string_view text, const Prompt& prompt) {
    if (forwardKeyword(text, prompt)) {
        return;
    }
    if (const std::optional<PointSpec> spec = parseCoordinate(text)) {
        if (spec->kind == PointSpec::Kind::Distance &&
            (prompt.kind == PromptKind::Distance || prompt.kind == PromptKind::Angle)) {
            // Multiply by the ratio so even large finite degree values do
            // not overflow before division by 180.
            const double value =
                prompt.kind == PromptKind::Angle ? core::reducedDegreesToRadians(spec->a) : spec->a;
            forward(ValueInput{value});
            return;
        }
        // Before the first pointer movement a typed distance has no
        // direction: falling back to the last point makes it unresolvable.
        // The pointer is corrected now, so that the direction is the one the
        // preview shows (docs/adr/0008). Ortho aligns it to the last point,
        // the one the distance is measured from, and polar tracking puts it
        // on the polar ray the pointer is on (docs/adr/0033, point 9):
        // within a command it is the base point; for the first point of a
        // command it may come from the previous one, as "@" does.
        const auto& lastPoint = context().lastPoint;
        // With a single guide of object snap tracking active, the distance
        // is measured from its acquired point, along it (docs/adr/0049,
        // point 6): from the end of an extension, along an arc extension
        // along the arc, from the base point along a parallel (docs/adr/0057,
        // point 4); where the guide gives no point, as without guides.
        if (spec->kind == PointSpec::Kind::Distance && m_pointer && followsGuides() &&
            !snapAt(*m_pointer)) {
            if (const std::optional<core::Vec2> point = detail::pointAlongGuide(
                    trackedPoint(onGrid(m_editor.document(), *m_pointer), lastPoint), spec->a)) {
                forward(PointInput{*point});
                return;
            }
        }
        const core::Vec2 pointer =
            m_pointer ? aidedPoint(*m_pointer, lastPoint) : lastPoint.value_or(core::Vec2{});
        if (const std::optional<core::Vec2> point = resolve(*spec, lastPoint, pointer)) {
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
        default:
            message(std::string{kInvalidInput});
            break;
    }
}

void CommandRunner::typedNumber(std::string_view text, const Prompt& prompt) {
    if (forwardKeyword(text, prompt)) {
        return;
    }
    if (const std::optional<PointSpec> spec = parseCoordinate(text);
        spec && spec->kind == PointSpec::Kind::Distance) {
        forward(ValueInput{spec->a});  // a pure number: no unit, no conversion
        return;
    }
    if (const std::optional<core::Vec2> point = typedPosition(text)) {
        forward(PointInput{*point});  // the command measures from its base point
        return;
    }
    message(std::string{kInvalidNumber});
}

void CommandRunner::typedObject(std::string_view text, const Prompt& prompt) {
    if (forwardKeyword(text, prompt)) {
        return;
    }
    if (const std::optional<core::Vec2> point = typedPosition(text)) {
        pickObject(*point, false);
        return;
    }
    message(std::string{kInvalidInput});
}

void CommandRunner::typedAtSelectionPrompt(std::string_view text, const Prompt& prompt) {
    if (text.empty()) {
        endSelectionPrompt();
        return;
    }
    // While a corner is asked for, the question is the runner's, not the
    // command's: its options do not apply.
    if (!m_rectangle && !m_writtenMode && forwardKeyword(text, prompt)) {
        return;
    }
    typedSelection(text);
}

void CommandRunner::typedSelection(std::string_view text) {
    if (m_rectangle || m_writtenMode) {
        // "Specify first corner:" or "Specify opposite corner:" ask for a point.
        if (const std::optional<core::Vec2> point = typedPosition(text)) {
            selectionClick(*point, PickAction::Add);
        } else {
            message(std::string{kInvalidInput});
        }
        return;
    }
    if (isWindowOption(text)) {
        m_writtenMode = core::RectangleMode::Window;
        return;
    }
    if (isCrossingOption(text)) {
        m_writtenMode = core::RectangleMode::Crossing;
        return;
    }
    if (isAllOption(text)) {
        // Everything a gesture could reach, as one gesture: entities on locked
        // layers are discarded and counted by applyToSelection().
        const core::Document& document = m_editor.document();
        std::vector<core::EntityId> ids;
        for (const core::EntityRecord& record : document.entities()) {
            if (core::isVisible(document, record)) {
                ids.push_back(record.id);
            }
        }
        // The most recent first, as AutoCAD gives them (docs/adr/0056,
        // point 1).
        std::ranges::sort(ids, std::ranges::greater{}, &core::EntityId::value);
        applyToSelection(ids, PickAction::Add);
        return;
    }
    if (isLastOption(text)) {
        selectLast();
        return;
    }
    if (isPreviousOption(text)) {
        selectPrevious();
        return;
    }
    if (const std::optional<core::Vec2> point = typedPosition(text)) {
        selectionClick(*point, PickAction::Add);
        return;
    }
    message(std::string{kInvalidSelection});
}

bool CommandRunner::forwardKeyword(std::string_view text, const Prompt& prompt) {
    const Keyword* keyword = findKeyword(prompt, text);
    if (keyword == nullptr) {
        return false;
    }
    forward(KeywordInput{keyword->name});
    return true;
}

std::optional<core::Vec2> CommandRunner::typedPosition(std::string_view text) const {
    const std::optional<PointSpec> spec = parseCoordinate(text);
    // A distance alone needs a direction from the pointer: it is not a
    // position.
    if (!spec || spec->kind == PointSpec::Kind::Distance) {
        return std::nullopt;
    }
    return resolve(*spec, context().lastPoint, core::Vec2{});
}

void CommandRunner::submitWhileAskingSelection(const Input& input) {
    if (std::holds_alternative<CancelInput>(input)) {
        if (!closeRectangle()) {
            cancel();
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        endSelectionQuestion();
    }
}

void CommandRunner::submitAtSelectionPrompt(const Input& input) {
    if (std::holds_alternative<ConfirmInput>(input)) {
        endSelectionPrompt();
    } else if (std::holds_alternative<KeywordInput>(input)) {
        forward(input);
    }
    // Points and values given with submit() are not selection gestures.
}

void CommandRunner::endSelectionQuestion() {
    m_rectangle.reset();
    m_writtenMode.reset();
    m_askingSelection = false;
    if (m_selection.isEmpty()) {
        m_command.reset();  // nothing chosen: the command ends with no effect
        return;
    }
    beginCommand();
}

void CommandRunner::endSelectionPrompt() {
    SelectionInput selection;
    selection.ids.assign(m_selection.ids().begin(), m_selection.ids().end());
    // An answer without objects leaves the previous selection set as it is.
    if (!selection.ids.empty()) {
        m_previous = selection.ids;
    }
    forward(selection);  // possibly empty: the command decides what that means
}

void CommandRunner::selectLast() {
    // The visible entity with the highest identifier, the one created last;
    // one on a locked layer is discarded as by any gesture.
    const core::Document& document = m_editor.document();
    std::optional<core::EntityId> last;
    for (const core::EntityRecord& record : document.entities()) {
        if (core::isVisible(document, record) && (!last || record.id.value > last->value)) {
            last = record.id;
        }
    }
    applyToSelection(last ? std::span{&*last, 1} : std::span<const core::EntityId>{},
                     PickAction::Add);
}

void CommandRunner::selectPrevious() {
    // Erased entities are no longer part of it (PT34, point 2).
    const core::Document& document = m_editor.document();
    std::vector<core::EntityId> ids;
    for (const core::EntityId id : m_previous) {
        if (document.find(id) != nullptr) {
            ids.push_back(id);
        }
    }
    if (ids.empty()) {
        message(std::string{kNoPreviousSelection});
        return;
    }
    applyToSelection(ids, PickAction::Add);
}

void CommandRunner::selectionClick(const core::Vec2& point, PickAction action) {
    const core::Document& document = m_editor.document();
    if (m_rectangle) {
        const core::Vec2 first = m_rectangle->first;
        // A written W or C fixes the mode; otherwise the direction decides.
        const core::RectangleMode mode = m_writtenMode.value_or(
            point.x < first.x ? core::RectangleMode::Crossing : core::RectangleMode::Window);
        m_rectangle.reset();
        m_writtenMode.reset();
        applyToSelection(core::selectInRectangle(document, core::BBox{first, point}, mode), action);
        return;
    }
    if (m_writtenMode) {
        // The first corner of W or C never picks the entity under it.
        m_rectangle =
            SelectionRectangle{point, point, *m_writtenMode == core::RectangleMode::Crossing};
        return;
    }
    if (const std::optional<core::EntityId> picked =
            core::pickEntity(document, point, kPickBoxPixels * m_pixelSize)) {
        applyToSelection(std::span{&*picked, 1}, action);
        return;
    }
    m_rectangle = SelectionRectangle{point, point, false};
}

bool CommandRunner::clickGrip(const core::Vec2& point) {
    if (m_selection.size() > kMaxGripEntities) {
        return false;  // no grip is shown (docs/adr/0034, point 1)
    }
    const core::Document& document = m_editor.document();
    const double halfSide = kGripSizePixels / 2.0 * m_pixelSize;

    struct Hit {
        const core::EntityRecord* record{nullptr};
        std::size_t index{0};
        core::Vec2 grip;
        double distance{0.0};
    };

    std::optional<Hit> best;
    for (const core::EntityId id : m_selection.ids()) {
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr || !showsGrips(document, *record)) {
            continue;
        }
        const std::vector<core::Vec2> points = core::gripPoints(record->entity);
        for (std::size_t index = 0; index < points.size(); ++index) {
            const double dx = points[index].x - point.x;
            const double dy = points[index].y - point.y;
            if (std::abs(dx) > halfSide || std::abs(dy) > halfSide) {
                continue;  // outside the square, edges included in it
            }
            const double distance = tg::core::hypot(dx, dy);
            // The nearest grip; at equal distance the entity with the highest
            // identifier, as core::pickEntity() orders its candidates
            // (docs/adr/0034, point 3); within one entity the first grip.
            if (!best || distance < best->distance ||
                (!(best->distance < distance) && id.value > best->record->id.value)) {
                best = Hit{record, index, points[index], distance};
            }
        }
    }
    if (!best) {
        return false;
    }
    m_hotGrip = best->grip;
    m_description = nullptr;  // its prompt is the runner's (docs/adr/0026, point 3)
    m_command = std::make_unique<GripCommand>(best->record->id, best->record->entity, best->index,
                                              best->grip);
    m_command->start(context());
    refreshPointer();  // the preview follows the pointer from the start
    return true;
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
    if (!inSelectionQuestion()) {
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

void CommandRunner::pickObject(const core::Vec2& point, bool shift) {
    const core::Document& document = m_editor.document();
    const std::optional<core::EntityId> picked =
        core::pickEntity(document, point, kPickBoxPixels * m_pixelSize);
    if (!picked) {
        return;  // nothing there: the question stays, silently
    }
    if (isOnLockedLayer(document, *picked)) {
        message(lockedMessage(1));
        return;
    }
    forward(ObjectInput{*picked, point, shift});
}

bool CommandRunner::closeRectangle() {
    if (!m_rectangle) {
        return false;
    }
    m_rectangle.reset();
    m_writtenMode.reset();
    return true;
}

void CommandRunner::forward(const Input& input) {
    // Any answer ends the question, and with it the points acquired for it
    // (docs/adr/0049, point 2).
    clearAcquired();
    // Any answer to a Selection prompt ends that question: what was chosen
    // goes with the SelectionInput, if that is the answer, or is dropped.
    if (promptKind() == PromptKind::Selection) {
        clearSelection();
    }
    if (m_command->handle(context(), input) == Status::Finished) {
        finish();
        return;
    }
    checkPrompt();
}

void CommandRunner::checkPrompt() const {
    if (!m_descriptionCheck || !isActive() || m_askingSelection || m_hotGrip) {
        return;  // the grip command's prompt is the runner's own
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
    // Perpendicular starts from the base point of the running command, like
    // ortho (docs/adr/0033, point 4); the first point of a command has none.
    const std::optional<core::Vec2> from =
        isActive() ? m_command->basePoint() : std::optional<core::Vec2>{};
    return core::findSnap(m_editor.document(), point, kSnapAperturePixels * m_pixelSize,
                          m_drawingAids.snapModes, from);
}

bool CommandRunner::followsGuides() const {
    return m_drawingAids.objectSnap && asksForPoint() && !m_acquisitions.empty();
}

std::optional<core::Vec2> CommandRunner::polarFoot(const core::Vec2& base,
                                                   const core::Vec2& point) const {
    // The polar aperture is the snap aperture (docs/adr/0033, point 9).
    return polarPoint(base, point, context().defaults.polarAngle,
                      kSnapAperturePixels * m_pixelSize);
}

CommandRunner::TrackingState CommandRunner::trackedPoint(
    const core::Vec2& point, const std::optional<core::Vec2>& polarBase) const {
    if (m_acquisitions.empty()) {
        return {};
    }
    // The polar ray counts as a guide (docs/adr/0049, point 4); ortho, if a
    // caller left it on with polar, applies instead of polar.
    std::optional<detail::PolarRay> ray;
    if (m_drawingAids.polar && !m_drawingAids.ortho && polarBase) {
        if (const std::optional<core::Vec2> foot = polarFoot(*polarBase, point);
            foot && *foot != *polarBase) {
            ray = detail::PolarRay{*polarBase, *foot};
        }
    }
    // A parallel goes through the base point of the command, which a typed
    // distance along it is measured from too (docs/tappe/0.8.1/PT31, point 3).
    return detail::trackPoint(m_acquisitions, m_command->basePoint(), point,
                              kSnapAperturePixels * m_pixelSize, ray);
}

core::Vec2 CommandRunner::aidedPoint(const core::Vec2& point,
                                     const std::optional<core::Vec2>& orthoBase) const {
    // Snap wins over ortho (docs/adr/0008) and over polar tracking
    // (docs/adr/0033, point 9). Ortho and polar start from the same base
    // point; if a caller left both on, ortho applies. Object snap tracking
    // comes after the snap and before both (docs/adr/0049, point 5).
    if (const std::optional<core::SnapPoint> snap = snapAt(point)) {
        return snap->point;
    }
    // Then the grid node, which the guides, ortho and polar project; the
    // crossing of two guides stays its own point (docs/adr/0055, point 4).
    const core::Vec2 node = onGrid(m_editor.document(), point);
    if (followsGuides()) {
        if (const TrackingState state = trackedPoint(node, orthoBase); state.guidePoint) {
            return *state.guidePoint;
        }
    }
    if (!orthoBase) {
        return node;
    }
    if (m_drawingAids.ortho) {
        return orthoPoint(*orthoBase, node);
    }
    if (m_drawingAids.polar) {
        if (const std::optional<core::Vec2> polar = polarFoot(*orthoBase, node)) {
            return *polar;
        }
    }
    return node;
}

std::optional<PromptKind> CommandRunner::promptKind() const {
    if (!isActive() || m_askingSelection) {
        return std::nullopt;
    }
    return m_command->prompt().kind;
}

bool CommandRunner::asksForPoint() const {
    const std::optional<PromptKind> kind = promptKind();
    return kind == PromptKind::Point || kind == PromptKind::Distance || kind == PromptKind::Angle ||
           kind == PromptKind::Number;
}

void CommandRunner::refreshPointer() {
    if (!m_pointer) {
        return;
    }
    if (asksForPoint()) {
        m_command->pointerMoved(aidedPoint(*m_pointer, m_command->basePoint()));
    } else if (asksForObject()) {
        m_command->pointerMoved(*m_pointer);  // one picks, one does not draw
    }
}

void CommandRunner::cancel() {
    // Keep the command alive for cancel(), while editor listeners already
    // see the runner idle when its transaction is closed.
    const std::unique_ptr<Command> command = std::move(m_command);
    clearAcquired();
    const bool started = !m_askingSelection;
    m_askingSelection = false;
    // Esc on a hot grip leaves the selection with its grips (docs/adr/0034,
    // point 3).
    if (!std::exchange(m_hotGrip, std::nullopt)) {
        clearSelection();
    }
    if (started) {
        command->cancel(context());
    }
    const bool closed = !m_editor.inTransaction();
    assert(closed && "a command cancelled without closing its transaction");
    if (!closed) {
        m_editor.rollback();
    }
    message(std::string{kCancelled});
}

void CommandRunner::finish() {
    if (context().documentReplaced) {
        context().lastPoint.reset();
        context().documentReplaced = false;
        // The previous selection set belongs to the document (PT34, point 2).
        m_previous.clear();
    }
    m_command.reset();
    clearAcquired();
    // A grip edit leaves the selection with its grips (docs/adr/0034,
    // point 3); every other command clears it (docs/adr/0008).
    if (!std::exchange(m_hotGrip, std::nullopt)) {
        clearSelection();
    }
    const bool closed = !m_editor.inTransaction();
    assert(closed && "a command finished without closing its transaction");
    if (!closed) {
        m_editor.rollback();
    }
}

void CommandRunner::clearSelection() {
    m_selection.clear();
    m_rectangle.reset();
    m_writtenMode.reset();
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
