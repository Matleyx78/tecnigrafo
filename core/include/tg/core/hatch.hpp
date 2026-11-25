// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/raw_group.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::core {

class Document;
class Similarity;
struct EntityRecord;
struct GeneratedItem;
struct GripResult;
struct SnapModes;
struct SnapPoint;
struct TransformOutcome;
class ToleranceRule;
enum class MirrorText : std::uint8_t;

/// DXF group 75 (docs/adr/0048, point 1).
enum class IslandStyle : std::uint8_t { Normal, Outer, Ignore };

/// An edge of a boundary path; a full circle is an Arc.
struct HatchEdge {
    std::variant<Line, Arc> curve;

    friend bool operator==(const HatchEdge&, const HatchEdge&) = default;
};

/// A boundary path: a polyline, or a list of edges.
struct HatchLoop {
    std::uint32_t flags{0};            ///< 92
    std::optional<Polyline> polyline;  ///< a polyline path, otherwise the edges
    std::vector<HatchEdge> edges;
    RawGroups extras;  ///< 97 and 330, in their original order

    friend bool operator==(const HatchLoop&, const HatchLoop&) = default;
};

/// A line family of a pattern, in drawing coordinates (docs/adr/0048).
struct PatternLine {
    double angle{0.0};           ///< 53, radians
    Vec2 origin;                 ///< 43, 44
    Vec2 offset;                 ///< 45, 46
    std::vector<double> dashes;  ///< 49

    friend bool operator==(const PatternLine&, const PatternLine&) = default;
};

/// DXF HATCH (docs/adr/0048, point 1). Angles in radians.
struct Hatch {
    std::string pattern;                       ///< 2
    bool solid{false};                         ///< 70
    IslandStyle islands{IslandStyle::Normal};  ///< 75
    int patternType{1};                        ///< 76
    double angle{0.0};                         ///< 52
    double scale{1.0};                         ///< 41
    bool doubled{false};                       ///< 77
    std::vector<PatternLine> patternLines;
    std::vector<Vec2> seeds;  ///< 10, 20 after 98
    std::vector<HatchLoop> loops;

    friend bool operator==(const Hatch&, const Hatch&) = default;
};

// The functions of Hatch that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(hatch).

/// True when every loop is a closed polyline with a vertex or a loop of at
/// least one edge (docs/tappe/0.8/PT30-retini-core.md, point 1). Whether the
/// edges follow one another is not checked: that depends on the unit of the
/// drawing, and isClosed() answers it for whoever makes a hatch. So the
/// answer changes neither with the units nor with a transformation.
[[nodiscard]] bool supported(const Hatch& hatch);

/// Whether `loop` is closed under `rule` (docs/tappe/0.8/PT30-retini-core.md,
/// point 1): a polyline loop when its polyline is closed; an edge loop when
/// it has an edge and the end of every edge, run as point 3 says, coincides
/// under `rule` with the start of the next one, the last with the first (one
/// edge: its own start). supported() does not check it: whoever makes a
/// hatch does, with the rule of its document (the DXF reader, -HATCH).
/// An edge beyond solid-space's domain (docs/adr/0035, point 23) leaves the
/// loop open.
[[nodiscard]] bool isClosed(const HatchLoop& loop, const ToleranceRule& rule);

/// The loops of `hatch` that its island style keeps (docs/adr/0048, point 3),
/// by increasing index in Hatch::loops. Normal: all. Outer: those flagged
/// external (group 92, bit 1) or outermost (bit 16). Ignore: those flagged
/// external. When no loop is flagged external (a file written by another
/// program), all of them, whatever the style.
[[nodiscard]] std::vector<std::size_t> regionLoops(const Hatch& hatch);

/// The area of the region that the loops of regionLoops() bound with the
/// even-odd rule (the ~area of docs/tappe/0.8.1/PT23-proprieta-retini-quote.md,
/// point 2), whatever the fill: the sum of the areas of their closed
/// polylines (those of generateOf(), arcs included), each with the sign
/// (-1)^depth, depth being how many other kept loops enclose the middle of
/// its first segment. Exact for loops that do not cross one another, as the
/// loops of a hatch made by AutoCAD or by -HATCH; for loops that cross, the
/// even-odd area is not that sum, and this one is only an estimate.
/// Precondition: supported(hatch).
[[nodiscard]] double filledArea(const Hatch& hatch);

// The origin of the pattern (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md,
// point 1). Origin and units are not in the Hatch: the origin is in the
// ACAD XDATA 1010/1020 of the EntityRecord, and the step of NET depends on
// $MEASUREMENT (3.175 or 0.125), which the caller passes.

/// The origin of the pattern: the ACAD XDATA 1010/1020 of `extras`, 0,0 when
/// missing (AutoCAD's HPORIGIN of a new drawing). The first 1010 of the
/// application ACAD (from its 1001 to the next 1001), followed by a 1020; a
/// value that is not a finite number counts as missing.
[[nodiscard]] Vec2 patternOrigin(const EntityExtras& extras);

/// The families of `hatch`, whose origin is `from`, with origin `to`: every
/// base point (43/44) moves by to - from, generated pattern or read from the
/// file alike (docs/REGOLE-NUMERICHE.md, section 2, point 9: the families
/// stand relative to the origin). Offsets and dashes do not change.
[[nodiscard]] Hatch withPatternOrigin(const Hatch& hatch, const Vec2& from, const Vec2& to);

/// The ACAD XDATA 1010/1020 of `extras` set to `origin`, added when missing
/// (docs/tappe/0.8.1/dxf-di-prova.md); the other groups untouched. The
/// values are written as io writes a real, with the fewest decimals (at
/// least one) that read back as the same double. Missing: a 1020 is inserted
/// after a 1010 without one; without a 1010, the point 1010, 1020, 1030 0.0
/// goes at the end of the application ACAD; without ACAD, 1001 ACAD and the
/// point go at the end of the XDATA, as AutoCAD writes them.
[[nodiscard]] EntityExtras withPatternOriginXdata(EntityExtras extras, const Vec2& origin);

/// The hatch with another predefined pattern, scale or angle, its origin
/// being `origin` and its drawing's $MEASUREMENT `measurement`: the families
/// of predefinedPatternLines(name, scale, angle, measurement) with every base
/// point moved by `origin`. With the hatch's own pattern name when
/// predefinedPatternLines() cannot generate it, the families of the file
/// transformed in proportion to the new scale and angle around `origin`
/// (docs/REGOLE-NUMERICHE.md, section 2, point 9). nullopt for any other
/// name that cannot be generated (today ANSI31 and NET; SOLID gives a solid
/// fill) and for a scale, old or new, that is not finite and positive.
///
/// The names compare as ASCII letters of any case; a predefined name is
/// stored in capitals ("ANSI31", "NET", "SOLID"), the hatch's own name as it
/// is. SOLID gives solid yes and no family; a predefined pattern gives solid
/// no. The proportion turns every family by angle - hatch.angle (its
/// direction through cardinalSinCos, so a quarter turn is exact; its angle
/// brought into [0, 2π) with normalizeAngle) and
/// multiplies its base point from `origin`, its offset and its dashes by
/// scale / hatch.scale. A finite angle is taken as it is: the caller reduces
/// it (reducedDegreesToRadians); one that is not finite gives nullopt.
[[nodiscard]] std::optional<Hatch> withPattern(const Hatch& hatch, const Vec2& origin,
                                               std::string_view name, double scale, double angle,
                                               int measurement);

/// HPMAXLINES of AutoCAD at its initial value: a pattern that needs more
/// lines, or more dashes, is not drawn.
inline constexpr std::size_t kMaxHatchLines = 1'000'000;

enum class PatternOutcome : std::uint8_t {
    Done,      ///< `lines` holds the pattern (possibly none)
    TooDense,  ///< more than kMaxHatchLines lines or dashes: `lines` is empty
};

struct PatternLines {
    PatternOutcome outcome{PatternOutcome::Done};
    std::vector<Line> lines;  ///< drawing coordinates, in the order of point 4
    /// False when solid-space left out a loop or a line (beyond its domain,
    /// docs/adr/0035, point 23): what it left out is not drawn.
    bool complete{true};
};

/// The lines of the pattern of `hatch` cut on its region (docs/adr/0048,
/// points 2 and 3). Precondition: supported(hatch) and !hatch.solid.
///
/// docs/tappe/0.8/PT42-righe-dei-motivi.md, point 3, gives the steps: the
/// loops of regionLoops() (a single whole-turn arc as a circle, every other
/// loop as its closed polyline), the lines of each family that cross the
/// bounding box of those loops, taken whole, cut by solid-space's
/// clipToRegion with the stretches inside and then those on the boundary of
/// each line, and the dashes of line k measured from p_k = origin + k *
/// offset, the point of the line that the origin of its family gives.
/// A family whose step is 0 or not finite gives no line; one whose range of
/// lines is not a number (an origin that is not finite) gives none either,
/// and makes the result incomplete, as a line left out by solid-space does.
/// A pattern whose dashes add up to 0, or to a value that is not finite, is
/// drawn continuous.
///
/// Band (solid-space ADR 0013, point 4): the ends of a stretch are rounded
/// within the errors of clipping.hpp, while the dashes are placed by
/// comparing them with the positions of the pattern. A dash or a dot that
/// starts or ends within those errors of an end of a stretch, and a stretch
/// shorter than them, may give one Line more or less, or a dash shorter by
/// as much, than the exact ends would; the same holds for the count against
/// kMaxHatchLines.
[[nodiscard]] PatternLines patternLines(const Hatch& hatch);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
///
/// A solid fill gives one FilledRegion with a closed polyline per loop of
/// regionLoops(), in order, for every island style (docs/adr/0048, point 3).
/// An edge loop becomes the polyline of docs/tappe/0.8/PT30-retini-core.md,
/// point 3: one vertex per edge, at the point where the edge starts, each
/// edge run in the direction its ends give; a single whole-turn arc becomes
/// two half turns from the point of its start angle. A pattern gives the
/// Lines of patternLines(), nothing when it is TooDense, with the effective
/// properties of the hatch but the linetype Continuous: the dashes come from
/// the pattern, as in AutoCAD with HPLINETYPE 0.
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record, const Hatch& hatch);

/// The extent registered in the spatial index: the bounding box of the loops,
/// as generateOf() makes them, whatever the fill (docs/tappe/0.8/
/// PT30-retini-core.md, point 6); empty on a frozen layer.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const Hatch& hatch);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const Hatch& hatch, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const Hatch& hatch, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const Hatch& hatch, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none, as a filled region gives none
/// (docs/adr/0048, point 4).
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                           const SnapModes& modes, std::vector<SnapPoint>& out,
                           const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, never refused (docs/tappe/0.8/
/// PT30-retini-core.md, point 4): loops, seeds and the origins of the pattern
/// lines go through the similarity; the angle of the hatch and of every
/// pattern line gains its rotation, or becomes the rotation minus the angle
/// with a reflection, not normalized; the scale and the dashes are multiplied
/// by its factor, and the offset of every pattern line, a vector of the
/// drawing, gets its linear part. A whole-turn arc of an edge loop stays a
/// whole turn, from 0 to 2*pi.
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const Hatch& hatch,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips: none in the 0.8 (docs/adr/0048, point 4).
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Hatch& hatch);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Hatch& hatch, std::size_t index,
                                    const Vec2& target);

}  // namespace tg::core
