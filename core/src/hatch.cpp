// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch.hpp"

#include "solid_space/geom2d/clipping.hpp"
#include "solid_space/geom2d/intersection.hpp"
#include "solid_space/geom2d/region.hpp"
#include "solid_space/robust/point.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/hatch_patterns.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/tolerance.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {
// The message of the precondition supported() (docs/adr/0044, point 1);
// unused in Release, where assertions are absent.
[[maybe_unused]] constexpr const char* kNotSupported = "this Hatch is not supported";

/// An edge of a loop as the loop runs it (docs/tappe/0.8/PT30-retini-core.md,
/// point 3): the point it starts from, the point it ends at, and its bulge,
/// negative for an arc run clockwise.
struct RunEdge {
    Vec2 start;
    Vec2 end;
    double bulge{0.0};
};

/// The edge as it is written: a line from its first point to its second, an
/// arc from its start angle to its end angle, with the end points and the
/// bulge of toPolyline().
RunEdge writtenRun(const HatchEdge& edge) {
    if (const auto* line = std::get_if<Line>(&edge.curve)) {
        return {line->start, line->end, 0.0};
    }
    const Polyline polyline = toPolyline(std::get<Arc>(edge.curve));
    const auto vertices = polyline.vertices();
    return {vertices[0].position, vertices[1].position, vertices[0].bulge};
}

RunEdge reversedRun(const RunEdge& run) {
    return {run.end, run.start, -run.bulge};
}

/// The distance from `point` to the nearer end of `run`.
double distanceToEnds(const Vec2& point, const RunEdge& run) {
    return std::min(distance(point, run.start), distance(point, run.end));
}

/// The edges of a loop as the loop runs them (point 3): the first ends at its
/// end nearer to the second edge, every other one starts from its end nearer
/// to where the previous one ends; on a tie, the direction it is written in.
/// A single edge runs as it is written.
std::vector<RunEdge> runEdges(const std::vector<HatchEdge>& edges) {
    std::vector<RunEdge> runs;
    runs.reserve(edges.size());
    for (const HatchEdge& edge : edges) {
        runs.push_back(writtenRun(edge));
    }
    if (runs.size() < 2) {
        return runs;
    }
    if (distanceToEnds(runs[0].start, runs[1]) < distanceToEnds(runs[0].end, runs[1])) {
        runs[0] = reversedRun(runs[0]);
    }
    for (std::size_t i = 1; i < runs.size(); ++i) {
        const Vec2 previous = runs[i - 1].end;
        if (distance(runs[i].end, previous) < distance(runs[i].start, previous)) {
            runs[i] = reversedRun(runs[i]);
        }
    }
    return runs;
}

/// True for an arc of a whole turn: the circle= of the report.
bool isWholeTurn(const HatchEdge& edge) {
    const auto* arc = std::get_if<Arc>(&edge.curve);
    return arc != nullptr && arc->geometry().sweep == kTwoPi;
}

/// The closed polyline of an edge loop (point 3).
Polyline edgePolyline(const std::vector<HatchEdge>& edges) {
    // A half circle has bulge tan(pi / 4) = 1.
    constexpr double kHalfTurnBulge = 1.0;
    const std::vector<RunEdge> runs = runEdges(edges);
    std::vector<PolylineVertex> vertices;
    if (runs.size() == 1) {
        const RunEdge& run = runs.front();
        if (isWholeTurn(edges.front())) {
            const Vec2 center = std::get<Arc>(edges.front().curve).center;
            vertices = {{run.start, kHalfTurnBulge}, {(center * 2.0) - run.start, kHalfTurnBulge}};
        } else {
            vertices = {{run.start, run.bulge}, {run.end, 0.0}};
        }
        return Polyline{std::move(vertices), true};
    }
    vertices.reserve(runs.size());
    for (const RunEdge& run : runs) {
        vertices.push_back({run.start, run.bulge});
    }
    return Polyline{std::move(vertices), true};
}

/// The closed polyline of a loop: its own, or that of its edges.
Polyline loopPolyline(const HatchLoop& loop) {
    if (loop.polyline) {
        return *loop.polyline;
    }
    return edgePolyline(loop.edges);
}

/// True when solid-space accepts the data of the edge (docs/adr/0035,
/// point 23): finite values, within its domain.
bool inDomain(const HatchEdge& edge) {
    const std::optional<bridge::g::Curve> curve =
        std::visit([](const auto& value) { return bridge::curveOf(Entity{value}); }, edge.curve);
    return curve && bridge::g::magnitude(*curve).hasValue();
}

/// True when ToleranceRule::coincident() can compare the point: finite
/// coordinates of magnitude at most 2^50. An edge within the domain can have
/// an end beyond it (a centre near the limit and a large radius).
bool comparable(const Vec2& point) {
    const auto within = [](double value) {
        return std::isfinite(value) && std::abs(value) <= solid_space::robust::kMaxCoordinate;
    };
    return within(point.x) && within(point.y);
}

// --- The lines of a pattern (docs/tappe/0.8/PT42-righe-dei-motivi.md) -----

namespace g = bridge::g;

/// Group 92: the loop is external, and the outermost one.
constexpr std::uint32_t kExternalLoop = 1;
constexpr std::uint32_t kOutermostLoop = 16;

/// The linetype of the lines of a pattern when the document has no
/// Continuous record (a document without tables).
const Linetype kContinuousLinetype{{}, "Continuous", "Solid line", {}, {}};

/// The loop of solid-space for a loop of the hatch (point 2): the circle of a
/// single whole-turn arc, the closed polyline of point 3 of PT30 otherwise.
g::Loop regionLoopOf(const HatchLoop& loop) {
    if (!loop.polyline && loop.edges.size() == 1 && isWholeTurn(loop.edges.front())) {
        const Arc& arc = std::get<Arc>(loop.edges.front().curve);
        return g::Circle{bridge::toPoint(arc.center), arc.radius};
    }
    return bridge::closedPolylineOf(loopPolyline(loop));
}

/// The lines of a family that cross the bounding box of the region (point 3,
/// step 2): k from `first`, `count` of them.
struct FamilyLines {
    std::size_t family{0};
    SinCos direction;  ///< d = (cosine, sine)
    double first{0.0};
    std::size_t count{0};
};

/// What step 2 gives for one family.
enum class RangeOutcome : std::uint8_t {
    Lines,     ///< `lines` holds them (possibly none)
    Invalid,   ///< the range is not a number: the family is left out
    TooDense,  ///< an infinite range
};

RangeOutcome familyRange(const PatternLine& line, const BBox& box, FamilyLines& lines,
                         double& total) {
    lines.direction = cardinalSinCos(line.angle);
    const Vec2 normal{-lines.direction.sine, lines.direction.cosine};
    const double step = dot(line.offset, normal);
    if (step == 0.0 || !std::isfinite(step)) {
        return RangeOutcome::Lines;
    }
    const std::array<Vec2, 4> corners{box.min(), Vec2{box.min().x, box.max().y},
                                      Vec2{box.max().x, box.min().y}, box.max()};
    double low = std::numeric_limits<double>::infinity();
    double high = -std::numeric_limits<double>::infinity();
    for (const Vec2& corner : corners) {
        const double v = dot(corner - line.origin, normal) / step;
        if (std::isnan(v)) {
            return RangeOutcome::Invalid;
        }
        low = std::min(low, v);
        high = std::max(high, v);
    }
    if (std::isinf(low) || std::isinf(high)) {
        return RangeOutcome::TooDense;
    }
    lines.first = std::ceil(low);
    const double last = std::floor(high);
    if (last < lines.first) {
        return RangeOutcome::Lines;
    }
    total += last - lines.first + 1.0;
    if (total > static_cast<double>(kMaxHatchLines)) {
        return RangeOutcome::TooDense;
    }
    lines.count = static_cast<std::size_t>(last - lines.first) + 1;
    return RangeOutcome::Lines;
}

/// A line of the pattern given to solid-space: its family, p_k and d.
struct PatternHost {
    std::size_t family{0};
    Vec2 base;
    Vec2 direction;
};

Vec2 along(const Vec2& base, double u, const Vec2& direction) {
    return {base.x + (u * direction.x), base.y + (u * direction.y)};
}

/// Appends the dashes of a stretch from `start` to `end` of the line through
/// `base` with direction `direction` (point 3, step 5). False when `out`
/// gets more than kMaxHatchLines lines.
bool appendDashes(const std::vector<double>& dashes, const Vec2& base, const Vec2& direction,
                  const Vec2& start, const Vec2& end, std::vector<Line>& out) {
    // Where each element of the pattern starts within a period, and the period.
    std::vector<double> offsets{0.0};
    offsets.reserve(dashes.size() + 1);
    for (const double dash : dashes) {
        offsets.push_back(offsets.back() + std::abs(dash));
    }
    const double period = offsets.back();
    const double u0 = dot(start - base, direction);
    const double u1 = dot(end - base, direction);
    if (dashes.empty() || !(period > 0.0) || !std::isfinite(period) || !(u0 < u1)) {
        out.push_back(Line{start, end});
        return out.size() <= kMaxHatchLines;
    }
    if (std::ranges::all_of(dashes, [](double dash) { return dash < 0.0; })) {
        return true;
    }
    // The period before the one of u0 holds a dot at its end that may lie at
    // u0. Every period after that one and before the one of u1 lies within the
    // stretch and gives at least one Line, so more than kMaxHatchLines + 3
    // periods give more than kMaxHatchLines Lines.
    const double firstPeriod = std::floor(u0 / period) - 1.0;
    const double periods = std::floor(u1 / period) - firstPeriod + 1.0;
    if (!(periods <= static_cast<double>(kMaxHatchLines) + 3.0)) {
        return false;
    }
    const auto count = static_cast<std::size_t>(periods);
    for (std::size_t j = 0; j < count; ++j) {
        const double origin = (firstPeriod + static_cast<double>(j)) * period;
        for (std::size_t e = 0; e < dashes.size(); ++e) {
            const double from = origin + offsets[e];
            const double to = origin + offsets[e + 1];
            if (dashes[e] == 0.0) {
                if (u0 <= from && from <= u1) {
                    const Vec2 point = along(base, from, direction);
                    out.push_back(Line{point, point});
                }
            } else if (dashes[e] > 0.0 && std::min(u1, to) > std::max(u0, from)) {
                out.push_back(Line{from <= u0 ? start : along(base, from, direction),
                                   to >= u1 ? end : along(base, to, direction)});
            }
            if (out.size() > kMaxHatchLines) {
                return false;
            }
        }
    }
    return true;
}

bool onFrozenLayer(const Document& document, const EntityRecord& record) {
    const Layer* layer = document.layer(record.properties.layer);
    return layer != nullptr && layer->frozen;
}

/// The arc transformed; a whole turn stays one, from 0 to 2*pi, which
/// transformed() would otherwise turn into an arc just short of it.
Arc transformedArc(const Arc& arc, const Similarity& similarity) {
    if (arc.geometry().sweep == kTwoPi) {
        return Arc{similarity.apply(arc.center), arc.radius * similarity.scale(), 0.0, kTwoPi};
    }
    return std::get<Arc>(transformed(Entity{arc}, similarity));
}

/// The angle of the hatch or of a pattern line after `similarity` (point 4).
double transformedAngle(double angle, const Similarity& similarity) {
    return similarity.reflects() ? similarity.rotationAngle() - angle
                                 : angle + similarity.rotationAngle();
}

// --- Area and origin of the pattern (docs/tappe/0.8.1/PT23) ----------------

/// The signed area of a closed polyline, its arcs included: the shoelace sum
/// plus, for every arc, the circular segment between it and its chord, which
/// a positive bulge adds and a negative one takes away.
double signedArea(const Polyline& polyline) {
    double area = 0.0;
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        const PolylineSegment segment = polyline.segment(i);
        area += cross(segment.start, segment.end) / 2.0;
        if (segment.bulge != 0.0) {
            // Included angle 4 atan(bulge), radius c (1 + b^2) / (4 b).
            const double sweep = 4.0 * std::atan(segment.bulge);
            const double radius = distance(segment.start, segment.end) *
                                  (1.0 + (segment.bulge * segment.bulge)) / (4.0 * segment.bulge);
            area += radius * radius * (sweep - std::sin(sweep)) / 2.0;
        }
    }
    return area;
}

/// The middle of the first segment of a polyline, its arc included.
Vec2 firstSegmentMiddle(const Polyline& polyline) {
    if (polyline.segmentCount() == 0) {
        return polyline.vertices().front().position;
    }
    const PolylineSegment segment = polyline.segment(0);
    const Vec2 chord = segment.end - segment.start;
    // A positive bulge runs counter-clockwise: the arc lies right of the chord.
    return ((segment.start + segment.end) * 0.5) +
           (Vec2{chord.y, -chord.x} * (segment.bulge * 0.5));
}

/// True when `point` is inside or on the closed `loop`: the even-odd rule of
/// a filled region.
bool loopHolds(const Polyline& loop, const Vec2& point) {
    return shapeDistance(GeneratedShape{FilledRegion{{loop}, Transform2D::identity()}}, point) ==
           0.0;
}

/// XDATA: the application name, and the point of the pattern origin.
constexpr std::int16_t kApplicationCode = 1001;
constexpr std::int16_t kPointX = 1010;
constexpr std::int16_t kPointY = 1020;
constexpr std::int16_t kPointZ = 1030;
constexpr std::string_view kAcadApplication = "ACAD";

std::string_view trimmed(std::string_view text) {
    const std::size_t first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

/// The real of a group, std::nullopt when it is not a finite number. A
/// stream with the classic locale: floating-point std::from_chars is missing
/// from the macOS 14 standard library.
std::optional<double> realValue(std::string_view text) {
    const std::string_view number = trimmed(text);
    if (number.empty()) {
        return std::nullopt;
    }
    std::istringstream stream{std::string{number}};
    stream.imbue(std::locale::classic());
    double value = 0.0;
    stream >> std::noskipws >> value;
    if (stream.fail() || !stream.eof() || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

/// A real as io writes one: the fewest fixed decimals, at least one, that
/// read back as the same double; scientific with 17 digits when none does.
std::string realText(double value) {
    constexpr int kMaxDecimals = 17;
    for (int decimals = 1; decimals <= kMaxDecimals; ++decimals) {
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        stream << std::fixed << std::setprecision(decimals) << value;
        if (realValue(stream.str()) == value) {
            return stream.str();
        }
    }
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::scientific << std::setprecision(kMaxDecimals - 1) << value;
    return stream.str();
}

/// The groups of the application ACAD in `groups`: [first, last), first at
/// its 1001; first == groups.size() when there is none.
std::pair<std::size_t, std::size_t> acadApplication(const RawGroups& groups) {
    std::size_t first = groups.size();
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (groups[i].code == kApplicationCode && trimmed(groups[i].value) == kAcadApplication) {
            first = i;
            break;
        }
    }
    std::size_t last = first;
    if (first < groups.size()) {
        last = first + 1;
        while (last < groups.size() && groups[last].code != kApplicationCode) {
            ++last;
        }
    }
    return {first, last};
}

/// The index of the first 1010 of the application ACAD, or `last`.
std::size_t originIndex(const RawGroups& groups, std::size_t first, std::size_t last) {
    for (std::size_t i = first; i < last; ++i) {
        if (groups[i].code == kPointX) {
            return i;
        }
    }
    return last;
}

bool sameAsciiName(std::string_view a, std::string_view b) {
    const auto upper = [](char c) {
        return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
    };
    return std::ranges::equal(a, b, [&](char x, char y) { return upper(x) == upper(y); });
}

/// The name in capitals, ASCII letters only.
std::string capitals(std::string_view name) {
    std::string text{name};
    for (char& c : text) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return text;
}

bool finitePositive(double value) {
    return std::isfinite(value) && value > 0.0;
}

/// `vector` turned by the angle whose sine and cosine are `turn`.
Vec2 turned(const Vec2& vector, const SinCos& turn) {
    return {(vector.x * turn.cosine) - (vector.y * turn.sine),
            (vector.x * turn.sine) + (vector.y * turn.cosine)};
}

}  // namespace

bool supported(const Hatch& hatch) {
    return std::ranges::all_of(hatch.loops, [](const HatchLoop& loop) {
        if (loop.polyline) {
            return loop.polyline->isClosed() && loop.polyline->vertexCount() > 0;
        }
        return !loop.edges.empty();
    });
}

bool isClosed(const HatchLoop& loop, const ToleranceRule& rule) {
    if (loop.polyline) {
        return loop.polyline->isClosed();
    }
    if (loop.edges.empty() || !std::ranges::all_of(loop.edges, inDomain)) {
        return false;
    }
    const std::vector<RunEdge> runs = runEdges(loop.edges);
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const Vec2& end = runs[i].end;
        const Vec2& next = runs[(i + 1) % runs.size()].start;
        if (!comparable(end) || !comparable(next) || !rule.coincident(end, next)) {
            return false;
        }
    }
    return true;
}

std::vector<std::size_t> regionLoops(const Hatch& hatch) {
    const auto flagged = [](const HatchLoop& loop, std::uint32_t flags) {
        return (loop.flags & flags) != 0;
    };
    const bool anyExternal = std::ranges::any_of(
        hatch.loops, [&](const HatchLoop& loop) { return flagged(loop, kExternalLoop); });
    std::uint32_t kept = 0;
    if (anyExternal && hatch.islands == IslandStyle::Outer) {
        kept = kExternalLoop | kOutermostLoop;
    } else if (anyExternal && hatch.islands == IslandStyle::Ignore) {
        kept = kExternalLoop;
    }
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < hatch.loops.size(); ++i) {
        if (kept == 0 || flagged(hatch.loops[i], kept)) {
            indices.push_back(i);
        }
    }
    return indices;
}

double filledArea(const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    std::vector<Polyline> loops;
    for (const std::size_t index : regionLoops(hatch)) {
        loops.push_back(loopPolyline(hatch.loops[index]));
    }
    double area = 0.0;
    for (std::size_t i = 0; i < loops.size(); ++i) {
        const Vec2 middle = firstSegmentMiddle(loops[i]);
        std::size_t depth = 0;
        for (std::size_t j = 0; j < loops.size(); ++j) {
            if (j != i && loopHolds(loops[j], middle)) {
                ++depth;
            }
        }
        const double own = std::abs(signedArea(loops[i]));
        area += depth % 2 == 0 ? own : -own;
    }
    return area;
}

Vec2 patternOrigin(const EntityExtras& extras) {
    const RawGroups& groups = extras.extended;
    const auto [first, last] = acadApplication(groups);
    const std::size_t x = originIndex(groups, first, last);
    if (x + 1 >= last || groups[x + 1].code != kPointY) {
        return {};
    }
    const std::optional<double> xValue = realValue(groups[x].value);
    const std::optional<double> yValue = realValue(groups[x + 1].value);
    if (!xValue || !yValue) {
        return {};
    }
    return {*xValue, *yValue};
}

Hatch withPatternOrigin(const Hatch& hatch, const Vec2& from, const Vec2& to) {
    Hatch result = hatch;
    const Vec2 shift = to - from;
    for (PatternLine& line : result.patternLines) {
        line.origin = line.origin + shift;
    }
    return result;
}

EntityExtras withPatternOriginXdata(EntityExtras extras, const Vec2& origin) {
    RawGroups& groups = extras.extended;
    const std::string x = realText(origin.x);
    const std::string y = realText(origin.y);
    const auto [first, last] = acadApplication(groups);
    if (first == groups.size()) {
        groups.insert(groups.end(), {{kApplicationCode, std::string{kAcadApplication}},
                                     {kPointX, x},
                                     {kPointY, y},
                                     {kPointZ, "0.0"}});
        return extras;
    }
    const std::size_t at = originIndex(groups, first, last);
    const auto position = [&groups](std::size_t index) {
        return groups.begin() + static_cast<std::ptrdiff_t>(index);
    };
    if (at == last) {
        groups.insert(position(last), {{kPointX, x}, {kPointY, y}, {kPointZ, "0.0"}});
        return extras;
    }
    groups[at].value = x;
    if (at + 1 < last && groups[at + 1].code == kPointY) {
        groups[at + 1].value = y;
    } else {
        groups.insert(position(at + 1), RawGroup{kPointY, y});
    }
    return extras;
}

std::optional<Hatch> withPattern(const Hatch& hatch, const Vec2& origin, std::string_view name,
                                 double scale, double angle, int measurement) {
    if (!finitePositive(scale) || !finitePositive(hatch.scale) || !std::isfinite(angle)) {
        return std::nullopt;
    }
    Hatch result = hatch;
    result.scale = scale;
    result.angle = angle;
    if (sameAsciiName(name, "SOLID")) {
        result.pattern = "SOLID";
        result.solid = true;
        result.patternLines.clear();
        return result;
    }
    std::vector<PatternLine> lines = predefinedPatternLines(name, scale, angle, measurement);
    if (!lines.empty()) {
        for (PatternLine& line : lines) {
            line.origin = line.origin + origin;
        }
        result.pattern = capitals(name);
        result.solid = false;
        result.patternLines = std::move(lines);
        return result;
    }
    if (hatch.solid || !sameAsciiName(name, hatch.pattern)) {
        return std::nullopt;
    }
    // The families of the file, in proportion around the origin.
    const SinCos turn = cardinalSinCos(angle - hatch.angle);
    const double factor = scale / hatch.scale;
    for (PatternLine& line : result.patternLines) {
        line.angle += angle - hatch.angle;
        line.origin = origin + (turned(line.origin - origin, turn) * factor);
        line.offset = turned(line.offset, turn) * factor;
        for (double& dash : line.dashes) {
            dash *= factor;
        }
    }
    return result;
}

PatternLines patternLines(const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    assert(!hatch.solid && "patternLines() of a solid fill");
    PatternLines result;
    std::vector<g::Loop> loops;
    BBox box;
    for (const std::size_t index : regionLoops(hatch)) {
        loops.push_back(regionLoopOf(hatch.loops[index]));
        box.expand(loopPolyline(hatch.loops[index]).boundingBox());
    }
    if (box.isEmpty()) {
        return result;
    }
    // Step 2: which lines, and step 3: how many.
    std::vector<FamilyLines> families;
    double total = 0.0;
    for (std::size_t f = 0; f < hatch.patternLines.size(); ++f) {
        FamilyLines lines;
        lines.family = f;
        switch (familyRange(hatch.patternLines[f], box, lines, total)) {
            case RangeOutcome::Lines:
                families.push_back(lines);
                break;
            case RangeOutcome::Invalid:
                result.complete = false;
                break;
            case RangeOutcome::TooDense:
                return {PatternOutcome::TooDense, {}, result.complete};
        }
    }
    const double length = std::max({box.width(), box.height(), 1.0});
    std::vector<g::Curve> hosts;
    std::vector<PatternHost> bases;
    hosts.reserve(static_cast<std::size_t>(total));
    bases.reserve(static_cast<std::size_t>(total));
    for (const FamilyLines& lines : families) {
        const PatternLine& family = hatch.patternLines[lines.family];
        const Vec2 direction{lines.direction.cosine, lines.direction.sine};
        for (std::size_t i = 0; i < lines.count; ++i) {
            const double k = lines.first + static_cast<double>(i);
            const Vec2 base{family.origin.x + (k * family.offset.x),
                            family.origin.y + (k * family.offset.y)};
            hosts.emplace_back(
                g::Segment{bridge::toPoint(base), bridge::toPoint(along(base, length, direction))});
            bases.push_back({lines.family, base, direction});
        }
    }
    // Step 4: the cut, and step 5: the dashes.
    const g::RegionClipping clipping = g::clipToRegion(hosts, g::Extension::extended, loops);
    if (!clipping.skippedHosts.empty() || !clipping.skippedLoops.empty()) {
        result.complete = false;
    }
    for (std::size_t i = 0; i < bases.size(); ++i) {
        const PatternHost& host = bases[i];
        const auto append = [&](const auto& stretch) {
            return appendDashes(hatch.patternLines[host.family].dashes, host.base, host.direction,
                                bridge::fromPoint(stretch.from.point),
                                bridge::fromPoint(stretch.to.point), result.lines);
        };
        const g::ClippedHost& clipped = clipping.hosts[i];
        if (!std::ranges::all_of(clipped.inside, append) ||
            !std::ranges::all_of(clipped.boundary, append)) {
            return {PatternOutcome::TooDense, {}, result.complete};
        }
    }
    return result;
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    if (onFrozenLayer(document, record)) {
        return {};
    }
    EffectiveProperties properties = effectiveProperties(document, record);
    if (hatch.solid) {
        FilledRegion region;
        for (const std::size_t index : regionLoops(hatch)) {
            region.loops.push_back(loopPolyline(hatch.loops[index]));
        }
        return {GeneratedItem{std::move(region), properties}};
    }
    const PatternLines pattern = patternLines(hatch);
    const Linetype* continuous = document.linetype(document.continuousLinetype());
    properties.linetype = continuous != nullptr ? continuous : &kContinuousLinetype;
    std::vector<GeneratedItem> items;
    items.reserve(pattern.lines.size());
    for (const Line& line : pattern.lines) {
        items.push_back(GeneratedItem{line, properties});
    }
    return items;
}

BBox extentOf(const Document& document, const EntityRecord& record, const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    BBox box;
    if (onFrozenLayer(document, record)) {
        return box;
    }
    for (const HatchLoop& loop : hatch.loops) {
        box.expand(loopPolyline(loop).boundingBox());
    }
    return box;
}

double distanceOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                  const Vec2& point) {
    assert(supported(hatch) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, hatch), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                const BBox& rectangle) {
    assert(supported(hatch) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, hatch), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                  const BBox& rectangle) {
    assert(supported(hatch) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, hatch), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Hatch& hatch, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(hatch) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Hatch& hatch,
                               const Similarity& similarity, MirrorText /*mirrorText*/) {
    assert(supported(hatch) && kNotSupported);
    const double factor = similarity.scale();
    const Transform2D linear = similarity.matrix();
    Hatch result = hatch;
    result.angle = transformedAngle(hatch.angle, similarity);
    result.scale = hatch.scale * factor;
    for (PatternLine& line : result.patternLines) {
        line.angle = transformedAngle(line.angle, similarity);
        line.origin = similarity.apply(line.origin);
        line.offset = linear.applyToVector(line.offset);
        for (double& dash : line.dashes) {
            dash *= factor;
        }
    }
    for (Vec2& seed : result.seeds) {
        seed = similarity.apply(seed);
    }
    for (HatchLoop& loop : result.loops) {
        if (loop.polyline) {
            loop.polyline = std::get<Polyline>(transformed(Entity{*loop.polyline}, similarity));
        }
        for (HatchEdge& edge : loop.edges) {
            if (auto* line = std::get_if<Line>(&edge.curve)) {
                *line = Line{similarity.apply(line->start), similarity.apply(line->end)};
            } else {
                edge.curve = transformedArc(std::get<Arc>(edge.curve), similarity);
            }
        }
    }
    return {Entity{std::move(result)}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Hatch& hatch, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    // Never reached: a hatch has no grip, so no index is valid.
    assert(supported(hatch) && kNotSupported);
    return {GripOutcome::Invalid, Entity{hatch}, {}};
}

}  // namespace tg::core
