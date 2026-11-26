// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document_query.hpp"

#include "dimension_graphics.hpp"
#include "insert_snap.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/fillet.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/solid.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <deque>
#include <limits>
#include <type_traits>
#include <utility>
#include <variant>

namespace tg::core {
namespace {

// proximity.cpp accepts contacts within a roundoff allowance relative to the
// coordinates, while the spatial index compares bounding boxes exactly.
// Growing the search area by the same relative allowance keeps such
// borderline entities among the candidates; the exact geometry still decides.
constexpr double kRoundoff = 32.0 * std::numeric_limits<double>::epsilon();

/// `area` grown by the roundoff allowance. An empty area stays empty.
BBox searchArea(const BBox& area) {
    if (area.isEmpty()) {
        return area;
    }
    const double scale = std::max({std::abs(area.min().x), std::abs(area.min().y),
                                   std::abs(area.max().x), std::abs(area.max().y)});
    const double margin = kRoundoff * scale;
    const Vec2 grow{margin, margin};
    return {area.min() - grow, area.max() + grow};
}

/// The square of half side `halfSide` centred on `center`: it contains every
/// point within `halfSide` of `center`.
BBox squareAround(const Vec2& center, double halfSide) {
    const Vec2 half{halfSide, halfSide};
    return {center - half, center + half};
}

std::vector<EntityId> candidates(const Document& document, const BBox& area) {
    std::vector<EntityId> ids;
    document.query(searchArea(area), ids);
    std::erase_if(ids, [&](EntityId id) {
        const EntityRecord* record = document.find(id);
        assert(record != nullptr && "the index only holds document entities");
        return !isVisible(document, *record);
    });
    return ids;
}

const EntityRecord& recordOf(const Document& document, EntityId id) {
    const EntityRecord* record = document.find(id);
    assert(record != nullptr && "the spatial index only holds entities of the document");
    return *record;
}

/// True for the five entities of before the 0.8, which are their own shape.
bool isOwnShape(const Entity& entity) {
    return std::holds_alternative<Polyline>(entity) || std::holds_alternative<Line>(entity) ||
           std::holds_alternative<Circle>(entity) || std::holds_alternative<Arc>(entity) ||
           std::holds_alternative<Text>(entity);
}

/// True for the shapes that take part in tangents and intersections: curves,
/// not texts. Solids wait for their package (docs/tappe/0.8, PT10).
bool isSnapCurve(const Entity& shape) {
    return std::holds_alternative<Polyline>(shape) || std::holds_alternative<Line>(shape) ||
           std::holds_alternative<Circle>(shape) || std::holds_alternative<Arc>(shape);
}

/// True for the shapes that take part in intersections: the curves of
/// isSnapCurve() and ellipses, which give no tangent (docs/adr/0052, point 3;
/// docs/tappe/0.8/PT50-ellisse-selezione-snap.md, point 4).
bool meetsOthers(const Entity& shape) {
    return isSnapCurve(shape) || std::holds_alternative<Ellipse>(shape);
}

/// A shape of an entity for the object snap, with the entity's id.
struct SnapShape {
    std::uint64_t id{0};
    const Entity* shape{nullptr};
};

/// Rank of a snap kind on equal distance (docs/adr/0033, point 5): endpoint,
/// intersection, midpoint, quadrant, perpendicular, tangent. The centre, the
/// insertion point and the nearest point never compete with them, and
/// extension and parallel are guides of the tracking, not snap points, so
/// their rank is not used.
int tieRank(SnapKind kind) {
    switch (kind) {
        case SnapKind::Endpoint:
            return 0;
        case SnapKind::Intersection:
            return 1;
        case SnapKind::Midpoint:
            return 2;
        case SnapKind::Quadrant:
            return 3;
        case SnapKind::Perpendicular:
            return 4;
        case SnapKind::Tangent:
            return 5;
        case SnapKind::Center:
        case SnapKind::Insertion:
        case SnapKind::Nearest:
        case SnapKind::Extension:
        case SnapKind::Parallel:
            break;
    }
    return 6;
}

/// Extension and parallel give nothing here: they are guides of the
/// tracking (docs/adr/0057, point 3).
bool anyModeEnabled(const SnapModes& modes) {
    return modes.endpoint || modes.midpoint || modes.center || modes.insertion ||
           modes.intersection || modes.perpendicular || modes.tangent || modes.quadrant ||
           modes.nearest;
}

/// A candidate answer, ordered by distance, then rank, then highest id.
/// Ties are exact: any difference in distance decides. On a complete tie the
/// candidate found first stays, so within an entity the earliest segment wins.
struct Candidate {
    double distance{std::numeric_limits<double>::infinity()};
    int rank{0};
    std::uint64_t id{0};

    [[nodiscard]] bool beats(const Candidate& other) const {
        if (distance != other.distance) {
            return distance < other.distance;
        }
        if (rank != other.rank) {
            return rank < other.rank;
        }
        return id > other.id;
    }
};

/// The segments that findSnap() considers: a single vertex counts as a
/// zero-length segment.
template <typename Visit>
void forEachSnapSegment(const Polyline& polyline, const Visit& visit) {
    if (polyline.vertexCount() == 1) {
        const Vec2 point = polyline.vertices().front().position;
        visit(PolylineSegment{point, point});
        return;
    }
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        visit(polyline.segment(i));
    }
}

/// A point of a curve and its distance from the pointer.
struct NearPoint {
    Vec2 point;
    double distance{std::numeric_limits<double>::infinity()};
};

/// The point of the segments nearest to `pointer`; on equal distance the
/// earliest segment, as the question `closest` (docs/verifica/domande.md).
template <typename Segments>
std::optional<NearPoint> nearestOnSegments(const Segments& forEachSegment, const Vec2& pointer) {
    std::optional<NearPoint> best;
    forEachSegment([&](const PolylineSegment& segment) {
        const Vec2 point = closestPoint(segment, pointer);
        const double pointDistance = distance(point, pointer);
        if (!best || pointDistance < best->distance) {
            best = NearPoint{point, pointDistance};
        }
    });
    return best;
}

/// The point of a shape that Nearest takes (docs/adr/0057, point 2): the
/// projection of closestPoint() on lines, circles, arcs and polylines,
/// nearest() of solid-space on an ellipse, with its distance. A solid is
/// measured from its sides, in the order v0, v1, v3, v2 in which it is
/// filled, not from the filled region its click picks. Texts give none, as
/// in AutoCAD; the types of the 0.8 that are not shapes reach here only
/// through their shapes.
std::optional<NearPoint> nearestOnShape(const Entity& shape, const Vec2& pointer) {
    if (const auto* polyline = std::get_if<Polyline>(&shape)) {
        return nearestOnSegments([&](const auto& visit) { forEachSnapSegment(*polyline, visit); },
                                 pointer);
    }
    if (const auto* line = std::get_if<Line>(&shape)) {
        const Vec2 point = closestPoint(PolylineSegment{line->start, line->end}, pointer);
        return NearPoint{point, distance(point, pointer)};
    }
    if (const auto* circle = std::get_if<Circle>(&shape)) {
        const Vec2 point = closestPoint(circle->geometry(), pointer);
        return NearPoint{point, distance(point, pointer)};
    }
    if (const auto* arc = std::get_if<Arc>(&shape)) {
        const Vec2 point = closestPoint(arc->geometry(), pointer);
        return NearPoint{point, distance(point, pointer)};
    }
    if (const auto* ellipse = std::get_if<Ellipse>(&shape)) {
        // Point and distance both from nearest(), within its declared band
        // (docs/adr/0052, point 4); data solid-space refuses give none.
        const std::optional<Vec2> point = closestPoint(*ellipse, pointer);
        if (!point) {
            return std::nullopt;
        }
        return NearPoint{*point, distance(*ellipse, pointer)};
    }
    if (const auto* solid = std::get_if<Solid>(&shape)) {
        const auto& v = solid->vertices;
        const std::array corners{v[0], v[1], v[3], v[2]};
        return nearestOnSegments(
            [&](const auto& visit) {
                for (std::size_t i = 0; i < corners.size(); ++i) {
                    visit(PolylineSegment{corners.at(i), corners.at((i + 1) % corners.size())});
                }
            },
            pointer);
    }
    return std::nullopt;
}

/// Where a snap point comes from (findSnapHit()): the entity and, for a
/// shape that is the entity itself, the segment of a polyline and the start
/// of the curve, which tells which end an Endpoint is.
struct Source {
    std::uint64_t id{0};
    std::optional<std::size_t> segment;
    std::optional<Vec2> start;
};

/// A point of the generated geometry, of an intersection or of a text: only
/// the entity.
Source entityOnly(std::uint64_t id) {
    return Source{id, std::nullopt, std::nullopt};
}

SnapHit hitOf(const SnapPoint& snap, const Source& source) {
    const bool atStart =
        snap.kind == SnapKind::Endpoint && source.start && snap.point == *source.start;
    return SnapHit{snap, EntityId{source.id}, source.segment, atStart};
}

/// The first point of a line or an arc, as appendSnapPoints() gives it.
std::optional<Vec2> curveStart(const Entity& entity) {
    if (const auto* line = std::get_if<Line>(&entity)) {
        return line->start;
    }
    if (const auto* arc = std::get_if<Arc>(&entity)) {
        return bridge::arcEndpoints(*arc).first;
    }
    return std::nullopt;
}

}  // namespace

std::optional<EntityId> pickEntity(const Document& document, const Vec2& point, double tolerance) {
    assert(tolerance >= 0.0);
    std::optional<EntityId> picked;
    Candidate best;
    for (const EntityId id : candidates(document, squareAround(point, tolerance))) {
        const Candidate candidate{distance(document, recordOf(document, id), point), 0, id.value};
        if (candidate.distance <= tolerance && (!picked || candidate.beats(best))) {
            picked = id;
            best = candidate;
        }
    }
    return picked;
}

std::vector<EntityId> selectInRectangle(const Document& document, const BBox& rectangle,
                                        RectangleMode mode) {
    std::vector<EntityId> selected;
    if (rectangle.isEmpty()) {
        return selected;
    }
    for (const EntityId id : candidates(document, rectangle)) {
        const EntityRecord& record = recordOf(document, id);
        if (isInside(document, record, rectangle) ||
            (mode == RectangleMode::Crossing && intersects(document, record, rectangle))) {
            selected.push_back(id);
        }
    }
    std::ranges::sort(selected, {}, &EntityId::value);
    return selected;
}

std::optional<SnapHit> findSnapHit(const Document& document, const Vec2& pointer, double aperture,
                                   const SnapModes& modes, std::optional<Vec2> from) {
    assert(aperture >= 0.0);
    if (!anyModeEnabled(modes)) {
        return std::nullopt;
    }
    std::optional<SnapHit> point;
    Candidate bestPoint;
    std::optional<SnapHit> center;
    Candidate bestCenter;
    std::optional<SnapHit> nearest;
    Candidate bestNearest;
    std::vector<SnapPoint> curvePoints;

    // One curve within the aperture: a polyline segment, or a whole line,
    // circle, arc or text box, at `curveDistance` from the pointer, with
    // where its points come from.
    const auto consider = [&](double curveDistance, const Source& source) {
        for (const SnapPoint& snap : curvePoints) {
            if (snap.kind == SnapKind::Center || snap.kind == SnapKind::Insertion) {
                // The centre is reached by passing over its curve, the
                // insertion point by passing over the text box.
                const Candidate candidate{curveDistance, 0, source.id};
                if (!center || candidate.beats(bestCenter)) {
                    center = hitOf(snap, source);
                    bestCenter = candidate;
                }
                continue;
            }
            const Candidate candidate{distance(snap.point, pointer), tieRank(snap.kind), source.id};
            if (candidate.distance <= aperture && (!point || candidate.beats(bestPoint))) {
                point = hitOf(snap, source);
                bestPoint = candidate;
            }
        }
    };

    // The tangent points from the base point to a curve within the aperture
    // (docs/adr/0033, points 4 and 5): those of a polyline lie on its
    // segments, so each one within the aperture is on a segment within it.
    const auto considerTangents = [&](const Entity& entity, std::uint64_t id) {
        if (!modes.tangent || !from || !isSnapCurve(entity)) {
            return;
        }
        const double curveDistance = distance(entity, pointer);
        if (curveDistance > aperture) {
            return;
        }
        curvePoints.clear();
        for (const Vec2& tangent : tangentPoints(*from, entity)) {
            curvePoints.push_back({tangent, SnapKind::Tangent});
        }
        consider(curveDistance, entityOnly(id));
    };

    // The nearest point of a curve within the aperture (docs/adr/0057,
    // point 2). On equal distance the highest id wins, and between shapes of
    // one entity (an insert, the sides of a solid) the first generated stays.
    const auto considerNearest = [&](const Entity& entity, std::uint64_t id) {
        if (!modes.nearest) {
            return;
        }
        const std::optional<NearPoint> found = nearestOnShape(entity, pointer);
        if (!found) {
            return;
        }
        const Candidate candidate{found->distance, 0, id};
        if (candidate.distance <= aperture && (!nearest || candidate.beats(bestNearest))) {
            nearest = hitOf(SnapPoint{found->point, SnapKind::Nearest}, entityOnly(id));
            bestNearest = candidate;
        }
    };

    // The snap points of one shape of an entity with identifier `id`; `own`
    // when the shape is the entity itself, whose segments and ends a hit
    // tells (findSnapHit()).
    std::vector<SnapShape> nearbyCurves;
    const auto considerShape = [&](const Entity& entity, std::uint64_t id, bool own) {
        considerNearest(entity, id);
        if (modes.intersection && meetsOthers(entity) && distance(entity, pointer) <= aperture) {
            nearbyCurves.push_back({id, &entity});
        }
        considerTangents(entity, id);
        if (const auto* polyline = std::get_if<Polyline>(&entity)) {
            std::size_t index = 0;
            forEachSnapSegment(*polyline, [&](const PolylineSegment& segment) {
                const std::size_t segmentIndex = index++;
                const double segmentDistance = distance(segment, pointer);
                if (segmentDistance > aperture) {
                    return;
                }
                curvePoints.clear();
                appendSnapPoints(segment, modes, curvePoints, from);
                consider(segmentDistance,
                         own ? Source{id, segmentIndex, segment.start} : entityOnly(id));
            });
            return;
        }
        // Lines, circles, arcs and text boxes are a single curve each.
        const double curveDistance = distance(entity, pointer);
        if (curveDistance > aperture) {
            return;
        }
        curvePoints.clear();
        appendSnapPoints(entity, modes, curvePoints, from);
        consider(curveDistance,
                 own ? Source{id, std::nullopt, curveStart(entity)} : entityOnly(id));
    };

    // A piece of an MTEXT gives no snap point of its own: within the aperture
    // it makes the insertion point of the MTEXT a candidate, at the distance
    // from the piece's box, as the box of a text does with its insertion
    // point (docs/tappe/0.8/PT13-mtext-core.md, point 6;
    // docs/tappe/0.8.1/PT8-impaginazione-formati.md, point 2): a Text with
    // its box, a FormattedText with formattedTextBox(); a TextLine makes
    // nothing a candidate. The nearest piece decides. `mtext` is drawn with
    // the properties of `owner`, the MTEXT itself or the one in the graphics
    // of a dimension; `id` is the entity of the document.
    const auto considerRows = [&](const std::vector<GeneratedItem>& rows, const EntityRecord& owner,
                                  const MText& mtext, std::uint64_t id) {
        for (const GeneratedItem& item : rows) {
            if (!std::holds_alternative<Text>(item.shape) &&
                !std::holds_alternative<FormattedText>(item.shape)) {
                continue;
            }
            const double rowDistance = shapeDistance(item.shape, pointer);
            if (rowDistance > aperture) {
                continue;
            }
            curvePoints.clear();
            appendOwnSnapPointsOf(document, owner, mtext, modes, curvePoints, from);
            consider(rowDistance, entityOnly(id));
        }
    };

    // The types of the 0.8 snap through their drawn shapes, the same that
    // appendSnapPoints(document, …) visits, kept here while they are compared.
    std::deque<Entity> generatedShapes;
    for (const EntityId id : candidates(document, squareAround(pointer, aperture))) {
        const EntityRecord& record = recordOf(document, id);
        if (std::holds_alternative<Hatch>(record.entity)) {
            // The lines of a hatch give no snap points, not even where they
            // cross other curves: OSNAPHATCH 0 of AutoCAD (docs/adr/0048,
            // point 4).
            continue;
        }
        if (isOwnShape(record.entity)) {
            considerShape(record.entity, id.value, true);
            continue;
        }
        if (const auto* mtext = std::get_if<MText>(&record.entity)) {
            considerRows(generate(document, record), record, *mtext, id.value);
        } else {
            // Through an insert the hatches of its blocks give nothing either;
            // through a dimension the rows of its MTEXT give no snap point of
            // their own but lead to the points of the MTEXT, as outside a
            // dimension (docs/tappe/0.8/PT23-quote-lineari.md, point 7).
            std::vector<GeneratedItem> items;
            if (const auto* insert = std::get_if<Insert>(&record.entity)) {
                items = snapItemsOf(document, record, *insert);
            } else if (const auto* dimension = std::get_if<Dimension>(&record.entity)) {
                items = snapItemsOf(document, record, *dimension);
                for (const DrawnMText& text : drawnMTextsOf(document, record, *dimension)) {
                    considerRows(text.rows, text.record, std::get<MText>(text.record.entity),
                                 id.value);
                }
            } else {
                items = generate(document, record);
            }
            for (const GeneratedItem& item : items) {
                // The pieces of a formatted MTEXT in a block give nothing,
                // not even the insertion point of their text, nor are their
                // lines curves (docs/tappe/0.8.1/PT8-impaginazione-formati.md,
                // point 1).
                if (std::holds_alternative<FormattedText>(item.shape) ||
                    std::holds_alternative<TextLine>(item.shape)) {
                    continue;
                }
                if (std::optional<Entity> shape = shapeEntity(item.shape)) {
                    generatedShapes.push_back(std::move(*shape));
                    considerShape(generatedShapes.back(), id.value, false);
                }
            }
        }
        // The points no shape gives (an insertion point) compete by their
        // own distance from the pointer.
        curvePoints.clear();
        std::visit(
            [&](const auto& value) {
                if constexpr (!std::is_same_v<std::decay_t<decltype(value)>, Polyline> &&
                              !std::is_same_v<std::decay_t<decltype(value)>, Line> &&
                              !std::is_same_v<std::decay_t<decltype(value)>, Circle> &&
                              !std::is_same_v<std::decay_t<decltype(value)>, Arc> &&
                              !std::is_same_v<std::decay_t<decltype(value)>, Text>) {
                    appendOwnSnapPointsOf(document, record, value, modes, curvePoints, from);
                }
            },
            record.entity);
        for (const SnapPoint& snap : curvePoints) {
            const Candidate candidate{distance(snap.point, pointer), tieRank(snap.kind), id.value};
            if (candidate.distance <= aperture && (!point || candidate.beats(bestPoint))) {
                point = hitOf(snap, entityOnly(id.value));
                bestPoint = candidate;
            }
        }
    }
    if (modes.intersection) {
        // Stable discovery order is only a deterministic tie-break. Different
        // pairs meeting at one point may differ by an ulp, without a visible
        // effect: snap still picks the point nearest the pointer. solid-space
        // promises identical bits when swapping a pair, not between pairs.
        // Shared stretches deliberately supply no points. Two shapes of the
        // same entity (an insert) meet too (docs/DECISIONI.md, 2026-11-14,
        // "Revisione delle interfacce PT4", P4).
        std::ranges::stable_sort(nearbyCurves, {}, &SnapShape::id);
        const auto considerCrossings = [&](const Intersections& found, std::uint64_t id) {
            curvePoints.clear();
            for (const Crossing& crossing : found.crossings) {
                curvePoints.push_back({crossing.point, SnapKind::Intersection});
            }
            consider(0.0, entityOnly(id));
        };
        for (std::size_t i = 0; i < nearbyCurves.size(); ++i) {
            const SnapShape first = nearbyCurves[i];
            if (const auto* polyline = std::get_if<Polyline>(first.shape)) {
                considerCrossings(selfIntersections(*polyline), first.id);
            }
            for (std::size_t j = i + 1; j < nearbyCurves.size(); ++j) {
                const SnapShape second = nearbyCurves[j];
                const auto found =
                    intersect(*first.shape, Extension::None, *second.shape, Extension::None);
                considerCrossings(found, second.id);
            }
        }
    }
    // A point within the aperture, then Nearest, then centre and insertion
    // (docs/REGOLE-NUMERICHE.md, section 4, "Selezione e snap a oggetto",
    // point 2).
    if (point) {
        return point;
    }
    return nearest ? nearest : center;
}

std::optional<SnapPoint> findSnap(const Document& document, const Vec2& pointer, double aperture,
                                  const SnapModes& modes, std::optional<Vec2> from) {
    const std::optional<SnapHit> hit = findSnapHit(document, pointer, aperture, modes, from);
    if (!hit) {
        return std::nullopt;
    }
    return hit->snap;
}

}  // namespace tg::core
