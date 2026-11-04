// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/trim_extend_commands.hpp"

#include "../text.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/curve_parts.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"
#include "tg/core/tolerance.hpp"

#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

// The values of TRIMEXTENDMODE (docs/adr/0035, point 3).
constexpr int kStandardMode = 0;
constexpr int kQuickMode = 1;

// The messages of the two modes, as AutoCAD writes them when the command
// starts (docs/tappe/0.7/P16-trim-extend.md).
constexpr std::string_view kQuickSettings =
    "Current settings: Trim mode = Quick, Projection=UCS, Edge=None";
constexpr std::string_view kStandardSettings =
    "Current settings: Projection=UCS, Edge=None, Mode=Standard";
// Standard mode, an object without cuts (docs/tappe/0.7/dxf-di-prova.md,
// table of the English texts).
constexpr std::string_view kNoIntersection = "Object does not intersect an edge.";
// The operation that does not succeed (docs/adr/0035, points 22 and 23).
constexpr std::string_view kCannotTrim = "Cannot trim that object.";
constexpr std::string_view kCannotExtend = "Cannot extend that object.";

Keyword modeKeyword() {
    return {"mOde", "O"};
}

Keyword undoKeyword() {
    return {"Undo", "U"};
}

std::string objectPromptText(TrimExtendCommand::Action action, bool withUndo) {
    std::string text = action == TrimExtendCommand::Action::Trim
                           ? "Select object to trim or shift-select to extend or "
                           : "Select object to extend or shift-select to trim or ";
    text += withUndo ? "[mOde/Undo]:" : "[mOde]:";
    return text;
}

std::string_view edgesMessage(TrimExtendCommand::Action action) {
    return action == TrimExtendCommand::Action::Trim ? "Select cutting edges ..."
                                                     : "Select boundary edges ...";
}

/// The order of the places on one segment: its start, the points inside, its end.
int phaseOf(core::Location location) {
    switch (location) {
        case core::Location::BeforeStart:
            return 0;
        case core::Location::AtStart:
            return 1;
        case core::Location::Inside:
            return 2;
        case core::Location::AtEnd:
            return 3;
        case core::Location::AfterEnd:
            return 4;
    }
    return 2;
}

/// True if `a` comes before `b` along the curve: first the segment, then the
/// place on it, then the station. Used only to place the pick among the cuts,
/// which is the user's choice and not a geometric decision (docs/adr/0035,
/// point 5); the cuts keep the exact order of cutsAlong().
bool comesBefore(const core::CurvePosition& a, const core::CurvePosition& b) {
    if (a.segment != b.segment) {
        return a.segment < b.segment;
    }
    const int phaseA = phaseOf(a.location);
    const int phaseB = phaseOf(b.location);
    if (phaseA != phaseB) {
        return phaseA < phaseB;
    }
    return a.location == core::Location::Inside && a.station < b.station;
}

/// Whether a polyline segment is exactly a point: the same exact test that
/// Extension::Extended and extendTo() make, so that the segment chosen here is
/// the one they extend (docs/tappe/0.7/P15-snap-intersezione.md, B3).
bool isPointSegment(const core::PolylineSegment& segment) {
    return segment.start == segment.end;
}

/// The pick on a polyline vertex, which nearestPosition() gives as the end of
/// the segment before it (or as a place on a segment that is exactly a
/// point), is moved to the start of the first segment after it that is not
/// exactly a point, where cutsAlong() gives a cut at that vertex: a pick
/// exactly on a cut takes the piece that follows (docs/adr/0035, point 5).
/// Without such a segment (the end of an open polyline) the pick stays.
core::CurvePosition normalizedPick(const core::Entity& entity, core::CurvePosition position) {
    const auto* polyline = std::get_if<core::Polyline>(&entity);
    if (polyline == nullptr) {
        return position;
    }
    const std::size_t segments = polyline->segmentCount();
    const bool onAPoint =
        position.segment < segments && isPointSegment(polyline->segment(position.segment));
    if (position.location != core::Location::AtEnd && !onAPoint) {
        return position;
    }
    for (std::size_t step = 1; step < segments; ++step) {
        const std::size_t next = position.segment + step;
        if (next >= segments && !polyline->isClosed()) {
            break;
        }
        const std::size_t index = next % segments;
        if (!isPointSegment(polyline->segment(index))) {
            return {index, core::Location::AtStart, 0.0};
        }
    }
    return position;
}

bool isClosedCurve(const core::Entity& entity) {
    if (std::holds_alternative<core::Circle>(entity)) {
        return true;
    }
    const auto* polyline = std::get_if<core::Polyline>(&entity);
    return polyline != nullptr && polyline->isClosed();
}

/// A piece that the rule finds degenerate does not stay (docs/adr/0036,
/// point 5). A polyline piece is degenerate when every one of its segments
/// is, each judged alone: no threshold of Tecnigrafo's own (P16, B4).
bool isDegeneratePiece(const core::Entity& piece, const core::ToleranceRule& rule) {
    if (const auto* polyline = std::get_if<core::Polyline>(&piece)) {
        for (std::size_t i = 0; i < polyline->segmentCount(); ++i) {
            if (!core::isDegenerate(polyline->segment(i), rule)) {
                return false;
            }
        }
        return true;
    }
    return core::isDegenerate(piece, rule);
}

/// One part of the object to rebuild with partOf().
struct Part {
    std::optional<core::CurvePoint> from;
    std::optional<core::CurvePoint> to;
    core::PartCourse course{core::PartCourse::Direct};
};

/// The parts that stay when the piece holding the pick goes. `after` is the
/// index of the first cut after the pick, so the cuts before it are not
/// after the pick. Precondition: `cuts` is not empty.
std::vector<Part> partsLeft(const core::Entity& entity, const std::vector<core::Cut>& cuts,
                            std::size_t after) {
    const auto at = [&cuts](std::size_t index) {
        return core::CurvePoint{cuts[index].point, cuts[index].onHost};
    };
    const std::size_t count = cuts.size();
    if (isClosedCurve(entity)) {
        // The cuts go round: the piece between the last and the first passes
        // through the start (angle 0, vertex 0).
        if (count == 1) {
            return {{at(0), at(0), core::PartCourse::SamePlace}};
        }
        if (after == 0 || after == count) {
            return {{at(0), at(count - 1), core::PartCourse::Direct}};
        }
        return {{at(after), at(after - 1), core::PartCourse::AcrossStart}};
    }
    if (after == 0) {
        return {{at(0), std::nullopt, core::PartCourse::Direct}};
    }
    if (after == count) {
        return {{std::nullopt, at(count - 1), core::PartCourse::Direct}};
    }
    return {{std::nullopt, at(after - 1), core::PartCourse::Direct},
            {at(after), std::nullopt, core::PartCourse::Direct}};
}

/// The segment that an end extends with, and whether it is curved.
struct EndSegment {
    std::size_t index{0};
    bool curved{false};
};

/// For a line, an arc or an open polyline, the segment that `end` extends
/// with: the first or the last segment that is not exactly a point.
std::optional<EndSegment> endSegment(const core::Entity& entity, core::EntityEnd end) {
    if (std::holds_alternative<core::Line>(entity)) {
        return EndSegment{0, false};
    }
    if (std::holds_alternative<core::Arc>(entity)) {
        return EndSegment{0, true};
    }
    const auto* polyline = std::get_if<core::Polyline>(&entity);
    if (polyline == nullptr || polyline->isClosed()) {
        return std::nullopt;
    }
    const std::size_t segments = polyline->segmentCount();
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i < segments; ++i) {
        if (!isPointSegment(polyline->segment(i))) {
            found = i;
            if (end == core::EntityEnd::Start) {
                break;
            }
        }
    }
    if (!found) {
        return std::nullopt;
    }
    return EndSegment{*found, polyline->segment(*found).isArc()};
}

/// The two ends of a line, an arc or an open polyline, start first.
std::optional<std::pair<core::Vec2, core::Vec2>> endsOf(const core::Entity& entity) {
    if (const auto* line = std::get_if<core::Line>(&entity)) {
        return std::pair{line->start, line->end};
    }
    if (const auto* arc = std::get_if<core::Arc>(&entity)) {
        const core::ArcGeometry geometry = arc->geometry();
        return std::pair{core::startPoint(geometry), core::endPoint(geometry)};
    }
    const auto* polyline = std::get_if<core::Polyline>(&entity);
    if (polyline == nullptr || polyline->isClosed() || polyline->vertexCount() < 2) {
        return std::nullopt;
    }
    return std::pair{polyline->vertices().front().position, polyline->vertices().back().position};
}

double squaredDistance(const core::Vec2& a, const core::Vec2& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

/// The first edge that `end` reaches (docs/adr/0035, point 6; P16): for the
/// end, the first cut AfterEnd of its segment; for the start, the last cut
/// BeforeStart of a straight segment, or the last cut AfterEnd of a curved
/// one, whose AfterEnd covers the rest of the circle.
std::optional<core::Cut> boundaryFor(const std::vector<core::Cut>& cuts, core::EntityEnd end,
                                     const EndSegment& segment) {
    const core::Location wanted = end == core::EntityEnd::Start && !segment.curved
                                      ? core::Location::BeforeStart
                                      : core::Location::AfterEnd;
    std::optional<core::Cut> found;
    for (const core::Cut& cut : cuts) {
        if (cut.onHost.segment != segment.index || cut.onHost.location != wanted) {
            continue;
        }
        found = cut;
        if (end == core::EntityEnd::End) {
            break;
        }
    }
    return found;
}

}  // namespace

// TrimExtendCommand

void TrimExtendCommand::start(CommandContext& context) {
    // The AutoCAD scripts always use standard mode (docs/adr/0032, point 8).
    m_scriptSequence = context.scriptSequences;
    m_quick = context.defaults.trimExtendMode == kQuickMode && !m_scriptSequence;
    m_edges.reset();
    m_savepoints.clear();
    m_beforeMode = Stage::Object;
    context.editor.beginTransaction(m_action == Action::Trim ? "Trim" : "Extend");
    if (m_quick) {
        context.message(std::string{kQuickSettings});
        m_stage = Stage::Object;
    } else {
        context.message(std::string{kStandardSettings});
        context.message(std::string{edgesMessage(m_action)});
        m_stage = Stage::Edges;
    }
}

Status TrimExtendCommand::handle(CommandContext& context, const Input& input) {
    switch (m_stage) {
        case Stage::Edges:
            handleEdges(input);
            break;
        case Stage::Object:
            if (std::holds_alternative<ConfirmInput>(input)) {
                context.editor.commit();
                return Status::Finished;
            }
            handleObject(context, input);
            break;
        case Stage::Mode:
            handleMode(context, input);
            break;
    }
    return Status::Continue;
}

void TrimExtendCommand::cancel(CommandContext& context) {
    // Esc keeps the operations already done (docs/adr/0035, point 21).
    if (context.editor.inTransaction()) {
        context.editor.commit();
    }
}

Prompt TrimExtendCommand::prompt() const {
    switch (m_stage) {
        case Stage::Edges:
            return {
                "Select objects or [mOde] <select all>:", {modeKeyword()}, PromptKind::Selection};
        case Stage::Object:
            if (m_savepoints.empty()) {
                return {objectPromptText(m_action, false), {modeKeyword()}, PromptKind::Object};
            }
            return {objectPromptText(m_action, true),
                    {modeKeyword(), undoKeyword()},
                    PromptKind::Object};
        case Stage::Mode:
            return {"Enter a trim mode option [Quick/Standard] <" +
                        std::string{m_quick ? "Quick" : "Standard"} + ">:",
                    {{"Quick", "Q"}, {"Standard", "S"}}};
    }
    return {};
}

void TrimExtendCommand::handleEdges(const Input& input) {
    if (const auto* selection = std::get_if<SelectionInput>(&input)) {
        // An empty choice means every edge, as in quick mode (P16).
        if (selection->ids.empty()) {
            m_edges.reset();
        } else {
            m_edges = selection->ids;
        }
        m_stage = Stage::Object;
    } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "mOde") {
            m_beforeMode = Stage::Edges;
            m_stage = Stage::Mode;
        }
    }
}

void TrimExtendCommand::handleObject(CommandContext& context, const Input& input) {
    if (const auto* object = std::get_if<ObjectInput>(&input)) {
        // Shift does the other command (docs/adr/0035, point 7).
        const bool trims = (m_action == Action::Trim) != object->shift;
        if (trims) {
            trim(context, object->id, object->point);
        } else {
            extend(context, object->id, object->point);
        }
        return;
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "mOde") {
            m_beforeMode = Stage::Object;
            m_stage = Stage::Mode;
        } else if (keyword->keyword == "Undo" && !m_savepoints.empty()) {
            context.editor.rollbackTo(m_savepoints.back());
            m_savepoints.pop_back();
        }
    }
}

void TrimExtendCommand::handleMode(CommandContext& context, const Input& input) {
    if (std::holds_alternative<ConfirmInput>(input)) {
        m_stage = m_beforeMode;  // Enter keeps the mode
        return;
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "Quick") {
            enterMode(context, true);
        } else if (keyword->keyword == "Standard") {
            enterMode(context, false);
        }
    }
}

void TrimExtendCommand::enterMode(CommandContext& context, bool quick) {
    // The option changes TRIMEXTENDMODE too (P16), but within the AutoCAD
    // script sequences the command stays in standard mode (docs/adr/0032,
    // point 8; docs/adr/0035, point 3).
    context.defaults.trimExtendMode = quick ? kQuickMode : kStandardMode;
    if (m_scriptSequence) {
        quick = false;
    }
    if (quick == m_quick) {
        m_stage = m_beforeMode;
        return;
    }
    m_quick = quick;
    m_edges.reset();
    if (quick) {
        m_stage = Stage::Object;
        return;
    }
    // Standard mode starts with its question on the edges.
    context.message(std::string{edgesMessage(m_action)});
    m_stage = Stage::Edges;
}

std::vector<core::Entity> TrimExtendCommand::edgesFor(const CommandContext& context,
                                                      core::EntityId object,
                                                      bool& objectIsEdge) const {
    const core::Document& document = context.editor.document();
    std::vector<core::Entity> edges;
    objectIsEdge = false;
    const auto consider = [&](const core::EntityRecord& record) {
        if (std::holds_alternative<core::Text>(record.entity)) {
            return;  // a text is never an edge (docs/adr/0035, point 4)
        }
        if (record.id == object) {
            objectIsEdge = true;
            return;
        }
        edges.push_back(record.entity);
    };
    if (!m_edges) {
        // Every curve on a visible layer, locked ones included.
        for (const core::EntityRecord& record : document.entities()) {
            if (core::isVisible(document, record)) {
                consider(record);
            }
        }
    } else {
        for (const core::EntityId id : *m_edges) {
            if (const core::EntityRecord* record = document.find(id)) {
                consider(*record);
            }
        }
    }
    return edges;
}

void TrimExtendCommand::trim(CommandContext& context, core::EntityId id, const core::Vec2& pick) {
    const core::EntityRecord* record = context.editor.document().find(id);
    if (record == nullptr || std::holds_alternative<core::Text>(record->entity)) {
        return;  // a text is not trimmed
    }
    const core::Entity entity = record->entity;
    bool objectIsEdge = false;
    const std::vector<core::Entity> edges = edgesFor(context, id, objectIsEdge);
    // A polyline among the edges is an edge of itself: its vertices and its
    // crossings cut it (docs/adr/0035, point 4).
    const bool cutsItself = std::holds_alternative<core::Polyline>(entity) && objectIsEdge;
    const core::HostCuts found = core::cutsAlong(entity, core::Extension::None, edges, cutsItself);
    if (found.cuts.empty()) {
        if (!found.complete) {
            // solid-space refused some data: no conclusion (P16, B18).
            context.message(std::string{kCannotTrim});
        } else if (m_quick) {
            beginOperation(context);
            context.editor.remove(id);
        } else {
            context.message(std::string{kNoIntersection});
        }
        return;
    }
    const core::CurvePosition picked = normalizedPick(entity, core::nearestPosition(entity, pick));
    std::size_t after = 0;
    while (after < found.cuts.size() && !comesBefore(picked, found.cuts[after].onHost)) {
        ++after;
    }
    const core::ToleranceRule rule = core::ToleranceRule::of(context.editor.document());
    std::vector<core::Entity> kept;
    for (const Part& part : partsLeft(entity, found.cuts, after)) {
        core::PartResult result = core::partOf(entity, part.from, part.to, part.course);
        if (result.outcome == core::PartOutcome::NotRepresentable) {
            context.message(std::string{kCannotTrim});
            return;  // the object stays as it is (P16, B1)
        }
        if (result.outcome == core::PartOutcome::Done && result.entity &&
            !isDegeneratePiece(*result.entity, rule)) {
            kept.push_back(std::move(*result.entity));
        }
    }
    beginOperation(context);
    if (kept.empty()) {
        context.editor.remove(id);
        return;
    }
    context.editor.replace(id, kept.front());
    if (kept.size() > 1) {
        // The second piece is a copy, with the data of the original
        // (docs/adr/0031, point 6).
        context.editor.addCopy(id, kept[1]);
    }
}

void TrimExtendCommand::extend(CommandContext& context, core::EntityId id, const core::Vec2& pick) {
    const core::EntityRecord* record = context.editor.document().find(id);
    if (record == nullptr) {
        return;
    }
    const core::Entity entity = record->entity;
    // Circles, closed polylines and texts are not extended.
    const auto ends = endsOf(entity);
    if (!ends) {
        return;
    }
    // The end nearer the pick; on a tie the end (P16).
    const core::EntityEnd end =
        squaredDistance(pick, ends->first) < squaredDistance(pick, ends->second)
            ? core::EntityEnd::Start
            : core::EntityEnd::End;
    const std::optional<EndSegment> segment = endSegment(entity, end);
    if (!segment) {
        return;  // every segment is exactly a point: there is no direction
    }
    bool objectIsEdge = false;
    const std::vector<core::Entity> edges = edgesFor(context, id, objectIsEdge);
    const core::HostCuts found = core::cutsAlong(entity, core::Extension::Extended, edges);
    const std::optional<core::Cut> boundary = boundaryFor(found.cuts, end, *segment);
    if (!boundary) {
        if (!found.complete) {
            context.message(std::string{kCannotExtend});  // P16, B18
        }
        return;
    }
    core::PartResult result =
        core::extendTo(entity, end, core::CurvePoint{boundary->point, boundary->onHost});
    if (result.outcome == core::PartOutcome::NotRepresentable) {
        context.message(std::string{kCannotExtend});
        return;
    }
    if (result.outcome != core::PartOutcome::Done || !result.entity) {
        return;
    }
    beginOperation(context);
    context.editor.replace(id, std::move(*result.entity));
}

void TrimExtendCommand::beginOperation(CommandContext& context) {
    m_savepoints.push_back(context.editor.savepoint());
}

// TrimextendmodeCommand

void TrimextendmodeCommand::start(CommandContext& context) {
    m_value = context.defaults.trimExtendMode;
}

Status TrimextendmodeCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        std::string_view value = detail::trim(text->text);
        // from_chars accepts a minus sign but not an explicit plus sign.
        if (value.starts_with('+')) {
            value.remove_prefix(1);
        }
        int number = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
            number < kStandardMode || number > kQuickMode) {
            context.message("Requires an integer between 0 and 1.");
            return Status::Continue;
        }
        context.defaults.trimExtendMode = number;
        return Status::Finished;
    }
    // Enter keeps the current value; no other input answers this question.
    return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
}

Prompt TrimextendmodeCommand::prompt() const {
    return {"Enter new value for TRIMEXTENDMODE <" + std::to_string(m_value) + ">:",
            {},
            PromptKind::Text};
}

}  // namespace tg::cmd
