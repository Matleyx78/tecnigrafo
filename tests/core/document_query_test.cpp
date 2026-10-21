// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document_query.hpp"

#include "tg/core/proximity.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <random>
#include <tuple>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

/// A two-point polyline from `from` to `to`, optionally an arc.
Polyline segment(const Vec2& from, const Vec2& to, double bulge = 0.0) {
    Polyline polyline;
    polyline.addVertex(from, bulge);
    polyline.addVertex(to);
    return polyline;
}

/// The closed axis-aligned rectangle with corners `a` and `b`.
Polyline rectangle(const Vec2& a, const Vec2& b) {
    return Polyline{{{a}, {{b.x, a.y}}, {b}, {{a.x, b.y}}}, true};
}

std::vector<std::uint64_t> values(const std::vector<EntityId>& ids) {
    std::vector<std::uint64_t> result;
    result.reserve(ids.size());
    for (const EntityId id : ids) {
        result.push_back(id.value);
    }
    return result;
}

std::optional<std::uint64_t> value(const std::optional<EntityId>& id) {
    return id ? std::optional<std::uint64_t>{id->value} : std::nullopt;
}

// --- pickEntity ---------------------------------------------------------

TEST(DocumentQuery, PickOnAnEntity) {
    Document document;
    const EntityId line = document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(value(pickEntity(document, {5, 0}, 0.5)), line.value);
    EXPECT_EQ(value(pickEntity(document, {5, 0}, 0.0)), line.value);
    EXPECT_EQ(value(pickEntity(document, {5, 0.4}, 0.5)), line.value);
}

TEST(DocumentQuery, PickToleranceIsInclusive) {
    Document document;
    const EntityId line = document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(value(pickEntity(document, {5, 2}, 2.0)), line.value);
    EXPECT_EQ(pickEntity(document, {5, 2.5}, 2.0), std::nullopt);
}

TEST(DocumentQuery, PickBetweenTwoEntitiesTakesTheNearer) {
    Document document;
    const EntityId lower = document.add(segment({0, 0}, {10, 0}));
    const EntityId upper = document.add(segment({0, 2}, {10, 2}));
    EXPECT_EQ(value(pickEntity(document, {5, 0.75}, 3.0)), lower.value);
    EXPECT_EQ(value(pickEntity(document, {5, 1.25}, 3.0)), upper.value);
}

TEST(DocumentQuery, PickAtEqualDistanceTakesTheHighestId) {
    Document document;
    const EntityId upper = document.add(segment({0, 2}, {10, 2}));
    const EntityId lower = document.add(segment({0, 0}, {10, 0}));
    ASSERT_GT(lower.value, upper.value);
    EXPECT_EQ(value(pickEntity(document, {5, 1}, 3.0)), lower.value);
}

TEST(DocumentQuery, PickInTheVoidFindsNothing) {
    Document document;
    EXPECT_EQ(pickEntity(document, {0, 0}, 5.0), std::nullopt);
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(pickEntity(document, {5, 30}, 5.0), std::nullopt);
}

TEST(DocumentQuery, PickNeverTakesAnEmptyEntity) {
    Document document;
    document.add(Polyline{});
    EXPECT_EQ(pickEntity(document, {0, 0}, 1e6), std::nullopt);
}

TEST(DocumentQuery, PickInsideAClosedPolylineFindsNothing) {
    Document document;
    document.add(rectangle({0, 0}, {10, 10}));
    EXPECT_EQ(pickEntity(document, {5, 5}, 1.0), std::nullopt);
}

TEST(DocumentQuery, PickIgnoresRemovedEntities) {
    Document document;
    const EntityId first = document.add(segment({0, 0}, {10, 0}));
    const EntityId second = document.add(segment({0, 0.5}, {10, 0.5}));
    ASSERT_TRUE(document.remove(second).has_value());
    EXPECT_EQ(value(pickEntity(document, {5, 0.5}, 1.0)), first.value);
}

// --- selectInRectangle --------------------------------------------------

/// Three entities: one inside [0,10]x[0,10], one across its right border,
/// one outside.
struct RectangleScene {
    Document document;
    EntityId outside;
    EntityId across;
    EntityId inside;

    RectangleScene()
        : outside(document.add(segment({20, 20}, {30, 20}))),
          across(document.add(segment({8, 5}, {15, 5}))),
          inside(document.add(rectangle({2, 2}, {4, 4}))) {}
};

TEST(DocumentQuery, WindowTakesOnlyEntitiesFullyInside) {
    const RectangleScene scene;
    EXPECT_EQ(values(selectInRectangle(scene.document, {{0, 0}, {10, 10}}, RectangleMode::Window)),
              (std::vector<std::uint64_t>{scene.inside.value}));
}

TEST(DocumentQuery, CrossingAlsoTakesEntitiesAcrossTheBorder) {
    const RectangleScene scene;
    EXPECT_EQ(
        values(selectInRectangle(scene.document, {{0, 0}, {10, 10}}, RectangleMode::Crossing)),
        (std::vector<std::uint64_t>{scene.across.value, scene.inside.value}));
}

TEST(DocumentQuery, RectangleResultsAreSortedById) {
    Document document;
    std::vector<std::uint64_t> expected;
    for (int i = 0; i < 20; ++i) {
        const double x = 2.0 * (19 - i);  // added right to left
        expected.push_back(document.add(segment({x, 0}, {x + 1, 1})).value);
    }
    std::ranges::sort(expected);
    EXPECT_EQ(values(selectInRectangle(document, {{-1, -1}, {50, 50}}, RectangleMode::Window)),
              expected);
}

TEST(DocumentQuery, RectangleOutsideOrEmptySelectsNothing) {
    const RectangleScene scene;
    EXPECT_TRUE(
        selectInRectangle(scene.document, {{40, 40}, {50, 50}}, RectangleMode::Crossing).empty());
    EXPECT_TRUE(selectInRectangle(scene.document, BBox{}, RectangleMode::Crossing).empty());
}

TEST(DocumentQuery, CrossingInsideAClosedPolylineWithoutTouchingIt) {
    Document document;
    document.add(rectangle({0, 0}, {10, 10}));
    EXPECT_TRUE(selectInRectangle(document, {{4, 4}, {6, 6}}, RectangleMode::Crossing).empty());
}

// --- findSnap -----------------------------------------------------------

constexpr SnapModes kAllModes{};

TEST(DocumentQuery, SnapToEndpoint) {
    Document document;
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(findSnap(document, {9.5, 0.5}, 1.0, kAllModes),
              (SnapPoint{{10, 0}, SnapKind::Endpoint}));
}

TEST(DocumentQuery, SnapToMidpoint) {
    Document document;
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(findSnap(document, {5.5, 0.5}, 1.0, kAllModes),
              (SnapPoint{{5, 0}, SnapKind::Midpoint}));
}

TEST(DocumentQuery, SnapTakesTheNearestPoint) {
    Document document;
    document.add(segment({0, 0}, {2, 0}));  // endpoints 0 and 2, midpoint 1
    EXPECT_EQ(findSnap(document, {1.6, 0}, 5.0, kAllModes),
              (SnapPoint{{2, 0}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(document, {1.4, 0}, 5.0, kAllModes),
              (SnapPoint{{1, 0}, SnapKind::Midpoint}));
}

TEST(DocumentQuery, SnapPrefersEndpointToMidpointAtEqualDistance) {
    Document document;
    // Entity 1 offers the endpoint (5, 2); entity 2 the midpoint (5, 0).
    // The pointer is 1 away from both: the endpoint wins despite the lower id.
    const EntityId vertical = document.add(segment({5, 2}, {5, 10}));
    const EntityId horizontal = document.add(segment({0, 0}, {10, 0}));
    ASSERT_LT(vertical.value, horizontal.value);
    EXPECT_EQ(findSnap(document, {5, 1}, 2.0, kAllModes), (SnapPoint{{5, 2}, SnapKind::Endpoint}));
}

TEST(DocumentQuery, SnapAtEqualDistanceAndKindTakesTheHighestId) {
    Document document;
    document.add(segment({0, 2}, {-10, 2}));  // endpoint (0, 2)
    document.add(segment({0, 0}, {-10, 0}));  // endpoint (0, 0), higher id
    EXPECT_EQ(findSnap(document, {0, 1}, 2.0, kAllModes), (SnapPoint{{0, 0}, SnapKind::Endpoint}));
}

/// A half circle of radius 10 around the origin, from (-10, 0) to (10, 0)
/// through (0, -10).
Polyline halfCircle() {
    return segment({-10, 0}, {10, 0}, 1.0);
}

/// A point of halfCircle() at -45 degrees, far from its endpoints and midpoint.
constexpr double kOnArc = 10.0 / std::numbers::sqrt2;

/// Arc centres are computed with trigonometry: exact only up to rounding.
constexpr double kCenterTolerance = 1e-12;

void expectCenter(const std::optional<SnapPoint>& actual, const Vec2& expected) {
    if (!actual) {
        ADD_FAILURE() << "no snap";
        return;
    }
    EXPECT_EQ(actual->kind, SnapKind::Center);
    EXPECT_NEAR(actual->point.x, expected.x, kCenterTolerance);
    EXPECT_NEAR(actual->point.y, expected.y, kCenterTolerance);
}

TEST(DocumentQuery, SnapToCenterWhenPassingOverTheArc) {
    Document document;
    document.add(halfCircle());
    expectCenter(findSnap(document, {kOnArc, -kOnArc}, 1.0, kAllModes), {0, 0});
}

TEST(DocumentQuery, SnapNeverTakesCenterNearTheCenterItself) {
    Document document;
    document.add(halfCircle());
    EXPECT_EQ(findSnap(document, {0, 0.5}, 1.0, kAllModes), std::nullopt);
}

TEST(DocumentQuery, SnapPrefersEndpointOrMidpointToCenter) {
    Document document;
    document.add(halfCircle());
    EXPECT_EQ(findSnap(document, {0, -9.5}, 1.0, kAllModes),
              (SnapPoint{{0, -10}, SnapKind::Midpoint}));
}

TEST(DocumentQuery, SnapCenterOfTheNearestArc) {
    Document document;
    document.add(segment({-10, 0}, {10, 0}, 1.0));      // centre (0, 0), radius 10
    document.add(segment({-10, 0.5}, {10, 0.5}, 1.0));  // centre (0, 0.5), radius 10
    // About 0.28 from the first arc and 0.07 from the second: the second
    // wins, although the pointer is nearer to the first centre.
    expectCenter(findSnap(document, {kOnArc, -kOnArc + 0.4}, 1.0, kAllModes), {0, 0.5});
}

TEST(DocumentQuery, SnapHonorsEachDisabledMode) {
    Document document;
    document.add(segment({0, 0}, {2, 0}));
    document.add(halfCircle());

    // Endpoint off: the pointer on the endpoint falls back to the midpoint.
    EXPECT_EQ(findSnap(document, {1.9, 0}, 1.0, {false, true, true}),
              (SnapPoint{{1, 0}, SnapKind::Midpoint}));
    // Midpoint off: the pointer on the midpoint falls back to an endpoint.
    EXPECT_EQ(findSnap(document, {1.1, 0}, 1.0, {true, false, true}),
              (SnapPoint{{2, 0}, SnapKind::Endpoint}));
    // Center off: passing over the arc finds nothing.
    EXPECT_EQ(findSnap(document, {kOnArc, -kOnArc}, 1.0, {true, true, false}), std::nullopt);
    // Only center: endpoints and midpoints are ignored, the arc still gives its centre.
    EXPECT_EQ(findSnap(document, {1.9, 0}, 1.0, {false, false, true}), std::nullopt);
    expectCenter(findSnap(document, {-10, 0}, 1.0, {false, false, true}), {0, 0});
    // Every mode off: nothing, even exactly on a vertex.
    EXPECT_EQ(findSnap(document, {2, 0}, 1.0, {false, false, false}), std::nullopt);
}

TEST(DocumentQuery, SnapToASingleVertex) {
    Document document;
    Polyline point;
    point.addVertex({3, 4});
    document.add(point);
    EXPECT_EQ(findSnap(document, {3.5, 4}, 1.0, kAllModes),
              (SnapPoint{{3, 4}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(document, {3.5, 4}, 1.0, {false, true, true}), std::nullopt);
}

TEST(DocumentQuery, SnapInTheVoidOrOnAnEmptyDocument) {
    Document document;
    EXPECT_EQ(findSnap(document, {0, 0}, 1.0, kAllModes), std::nullopt);
    document.add(Polyline{});
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(findSnap(document, {5, 5}, 1.0, kAllModes), std::nullopt);
}

// --- Comparison with an exhaustive search -------------------------------

/// Random geometry reproducible on every platform: std::mt19937 is fully
/// specified by the standard, its distributions are not.
class Generator {
public:
    explicit Generator(std::uint32_t seed) : m_engine(seed) {}

    double unit() { return static_cast<double>(m_engine()) / kRange; }

    double between(double low, double high) { return low + ((high - low) * unit()); }

    std::size_t index(std::size_t count) {
        return static_cast<std::size_t>(unit() * static_cast<double>(count));
    }

    /// Zero to four vertices near a random point, some segments arcs.
    Polyline polyline() {
        Polyline result;
        const Vec2 origin{between(0.0, 500.0), between(0.0, 500.0)};
        const auto vertices = index(5);
        for (std::size_t i = 0; i < vertices; ++i) {
            const double bulge = unit() < 0.3 ? between(-1.5, 1.5) : 0.0;
            result.addVertex(origin + Vec2{between(0.0, 40.0), between(0.0, 40.0)}, bulge);
        }
        result.setClosed(unit() < 0.2);
        return result;
    }

    BBox area() {
        const Vec2 corner{between(-20.0, 520.0), between(-20.0, 520.0)};
        return {corner, corner + Vec2{between(0.0, 150.0), between(0.0, 150.0)}};
    }

    /// Half of the points near a vertex of the document, so that queries hit.
    Vec2 point(const Document& document) {
        if (unit() < 0.5) {
            const auto& entity = document.entities()[index(document.entityCount())].entity;
            const auto vertices = std::get<Polyline>(entity).vertices();
            if (!vertices.empty()) {
                return vertices[index(vertices.size())].position +
                       Vec2{between(-3.0, 3.0), between(-3.0, 3.0)};
            }
        }
        return {between(-20.0, 560.0), between(-20.0, 560.0)};
    }

    SnapModes modes() { return {unit() < 0.7, unit() < 0.7, unit() < 0.7}; }

private:
    static constexpr double kRange = 4294967296.0;  // 2^32, mt19937's range

    std::mt19937 m_engine;
};

std::optional<std::uint64_t> exhaustivePick(const Document& document, const Vec2& point,
                                            double tolerance) {
    std::optional<std::tuple<double, std::uint64_t>> best;  // (distance, -id)
    std::optional<std::uint64_t> picked;
    for (const EntityRecord& record : document.entities()) {
        const double d = distance(record.entity, point);
        const std::tuple<double, std::uint64_t> key{
            d, std::numeric_limits<std::uint64_t>::max() - record.id.value};
        if (d <= tolerance && (!best || key < *best)) {
            best = key;
            picked = record.id.value;
        }
    }
    return picked;
}

std::vector<std::uint64_t> exhaustiveSelect(const Document& document, const BBox& rectangle,
                                            RectangleMode mode) {
    std::vector<std::uint64_t> selected;
    for (const EntityRecord& record : document.entities()) {
        const bool inside = isInside(record.entity, rectangle);
        const bool crossing = inside || intersects(record.entity, rectangle);
        if (mode == RectangleMode::Window ? inside : crossing) {
            selected.push_back(record.id.value);
        }
    }
    std::ranges::sort(selected);
    return selected;
}

std::optional<SnapPoint> exhaustiveSnap(const Document& document, const Vec2& pointer,
                                        double aperture, const SnapModes& modes) {
    // (distance, kind rank, -id, order of discovery inside the entity)
    using Key = std::tuple<double, int, std::uint64_t, std::size_t>;
    std::optional<std::pair<Key, SnapPoint>> point;
    std::optional<std::pair<Key, SnapPoint>> center;
    for (const EntityRecord& record : document.entities()) {
        const auto& polyline = std::get<Polyline>(record.entity);
        std::vector<PolylineSegment> segments;
        if (polyline.vertexCount() == 1) {
            const Vec2 only = polyline.vertices().front().position;
            segments.push_back({only, only});
        }
        for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
            segments.push_back(polyline.segment(i));
        }
        const std::uint64_t reversedId =
            std::numeric_limits<std::uint64_t>::max() - record.id.value;
        std::size_t order = 0;
        for (const PolylineSegment& seg : segments) {
            const double segmentDistance = distance(seg, pointer);
            if (segmentDistance > aperture) {
                continue;
            }
            std::vector<SnapPoint> snaps;
            appendSnapPoints(seg, modes, snaps);
            for (const SnapPoint& snap : snaps) {
                ++order;
                if (snap.kind == SnapKind::Center) {
                    const Key key{segmentDistance, 0, reversedId, order};
                    if (!center || key < center->first) {
                        center = {key, snap};
                    }
                    continue;
                }
                const double d = distance(snap.point, pointer);
                const Key key{d, snap.kind == SnapKind::Endpoint ? 0 : 1, reversedId, order};
                if (d <= aperture && (!point || key < point->first)) {
                    point = {key, snap};
                }
            }
        }
    }
    if (point) {
        return point->second;
    }
    if (center) {
        return center->second;
    }
    return std::nullopt;
}

class RandomDocument : public testing::Test {
protected:
    static constexpr int kEntityCount = 2000;
    static constexpr int kQueryCount = 500;

    void SetUp() override {
        for (int i = 0; i < kEntityCount; ++i) {
            m_document.add(m_generator.polyline());
        }
        // Removals exercise the index after updates, not only after insertions;
        // the document is then refilled to kEntityCount polylines.
        for (std::uint64_t id = 7; id <= kEntityCount; id += 13) {
            ASSERT_TRUE(m_document.remove(EntityId{id}).has_value());
        }
        while (m_document.entityCount() < kEntityCount) {
            m_document.add(m_generator.polyline());
        }
    }

    Generator& generator() { return m_generator; }

    [[nodiscard]] const Document& document() const { return m_document; }

private:
    Generator m_generator{20261017U};
    Document m_document;
};

/// Entities of every type, as in the reference case modello/tipi-misti.
struct MixedDocument {
    Document document;
    EntityId polyline{document.add(Polyline{{{{0.0, 0.0}}, {{10.0, 0.0}}, {{10.0, 10.0}}}})};
    EntityId line{document.add(Line{{20.0, 0.0}, {30.0, 0.0}})};
    EntityId circle{document.add(Circle{{50.0, 0.0}, 10.0})};
    EntityId arc{document.add(Arc{{80.0, 0.0}, 10.0, 0.0, std::numbers::pi})};
};

TEST(MixedTypes, PickTakesEachTypeOnItsCurve) {
    const MixedDocument mixed;
    EXPECT_EQ(pickEntity(mixed.document, {5.0, 0.2}, 0.5), mixed.polyline);
    EXPECT_EQ(pickEntity(mixed.document, {25.0, 0.2}, 0.5), mixed.line);
    EXPECT_EQ(pickEntity(mixed.document, {50.0, 10.2}, 0.5), mixed.circle);
    EXPECT_EQ(pickEntity(mixed.document, {80.0, 10.2}, 0.5), mixed.arc);
    // The centre of a circle or of an arc is not on the curve.
    EXPECT_FALSE(pickEntity(mixed.document, {50.0, 0.0}, 2.0));
    EXPECT_FALSE(pickEntity(mixed.document, {80.0, 0.0}, 2.0));
}

TEST(MixedTypes, WindowAndCrossing) {
    const MixedDocument mixed;
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{-5.0, -15.0}, {95.0, 25.0}},
                                       RectangleMode::Window)),
              (std::vector<std::uint64_t>{mixed.polyline.value, mixed.line.value,
                                          mixed.circle.value, mixed.arc.value}));
    // Inside the circle, touching nothing.
    EXPECT_TRUE(
        selectInRectangle(mixed.document, BBox{{45.0, -5.0}, {55.0, 5.0}}, RectangleMode::Crossing)
            .empty());
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{38.0, -12.0}, {62.0, 12.0}},
                                       RectangleMode::Window)),
              (std::vector<std::uint64_t>{mixed.circle.value}));
    // Across the top of the arc, with no end point inside.
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{78.0, 8.0}, {82.0, 12.0}},
                                       RectangleMode::Crossing)),
              (std::vector<std::uint64_t>{mixed.arc.value}));
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{28.0, -2.0}, {82.0, 12.0}},
                                       RectangleMode::Crossing)),
              (std::vector<std::uint64_t>{mixed.line.value, mixed.circle.value, mixed.arc.value}));
}

TEST(MixedTypes, SnapFollowsThePrioritiesOfAdr0008) {
    const MixedDocument mixed;
    const SnapModes all;
    // The centre of the circle, passing over the circle.
    EXPECT_EQ(findSnap(mixed.document, {60.0, 0.5}, 1.0, all),
              (SnapPoint{{50.0, 0.0}, SnapKind::Center}));
    EXPECT_FALSE(findSnap(mixed.document, {50.0, 0.0}, 1.0, all));
    // On the arc the midpoint beats the centre; away from it, the centre.
    const SnapPoint midpoint =
        findSnap(mixed.document, {80.0, 9.8}, 1.0, all).value_or(SnapPoint{});
    EXPECT_EQ(midpoint.kind, SnapKind::Midpoint);
    EXPECT_NEAR(midpoint.point.x, 80.0, 1e-12);
    EXPECT_NEAR(midpoint.point.y, 10.0, 1e-12);
    EXPECT_EQ(findSnap(mixed.document, {87.071, 7.071}, 1.0, all),
              (SnapPoint{{80.0, 0.0}, SnapKind::Center}));
    // On the line the nearer of end point and midpoint.
    EXPECT_EQ(findSnap(mixed.document, {20.5, 0.5}, 2.0, all),
              (SnapPoint{{20.0, 0.0}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(mixed.document, {25.0, 0.3}, 1.0, all),
              (SnapPoint{{25.0, 0.0}, SnapKind::Midpoint}));
    // Without the centre mode, passing over the circle snaps to nothing.
    EXPECT_FALSE(findSnap(mixed.document, {60.0, 0.5}, 1.0, SnapModes{true, true, false}));
}

TEST(DocumentQuery, SnapAnEndPointBeatsTheCentreOfAnotherType) {
    Document document;
    document.add(segment({0.0, 0.0}, {10.0, 0.0}));
    document.add(Circle{{30.0, 0.0}, 20.0});  // passes through (10,0)
    // The circle's centre is reached over the circle, but the polyline's end
    // point within the aperture wins (docs/adr/0008, point 4).
    EXPECT_EQ(findSnap(document, {10.2, 0.0}, 1.0, SnapModes{}),
              (SnapPoint{{10.0, 0.0}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(document, {10.2, 0.0}, 1.0, SnapModes{false, true, true}),
              (SnapPoint{{30.0, 0.0}, SnapKind::Center}));
}

// --- Texts: regions with an insertion point (docs/adr/0013, point 6) ------

/// "Hello" 2.5 high at `position`: about 8.28 wide, from y - 0.77 to y + 3.29.
Text hello(const Vec2& position) {
    Text text;
    text.position = position;
    text.value = "Hello";
    return text;
}

TEST(TextQueries, PickInsideTheBoxTakesTheText) {
    Document document;
    const EntityId text = document.add(hello({0.0, 0.0}));
    EXPECT_EQ(pickEntity(document, {4.0, 1.0}, 0.0), text);
    EXPECT_EQ(pickEntity(document, {0.0, 0.0}, 0.0), text);
    // Outside, within or beyond the tolerance.
    EXPECT_EQ(pickEntity(document, {-0.4, 1.0}, 0.5), text);
    EXPECT_FALSE(pickEntity(document, {-0.6, 1.0}, 0.5));
}

TEST(TextQueries, PickOnOverlappingTextsTakesTheHighestId) {
    Document document;
    document.add(hello({0.0, 0.0}));
    const EntityId over = document.add(hello({1.0, 0.5}));
    EXPECT_EQ(pickEntity(document, {2.0, 1.0}, 0.0), over);
}

TEST(TextQueries, WindowAndCrossing) {
    Document document;
    const EntityId text = document.add(hello({0.0, 0.0}));
    const EntityId line = document.add(Line{{20.0, 0.0}, {30.0, 0.0}});
    EXPECT_EQ(
        values(selectInRectangle(document, BBox{{-1.0, -1.0}, {9.0, 4.0}}, RectangleMode::Window)),
        (std::vector<std::uint64_t>{text.value}));
    // Inside the text: the window takes nothing, the crossing takes the text.
    EXPECT_TRUE(
        selectInRectangle(document, BBox{{2.0, 0.0}, {6.0, 2.0}}, RectangleMode::Window).empty());
    EXPECT_EQ(
        values(selectInRectangle(document, BBox{{2.0, 0.0}, {6.0, 2.0}}, RectangleMode::Crossing)),
        (std::vector<std::uint64_t>{text.value}));
    EXPECT_EQ(values(selectInRectangle(document, BBox{{5.0, -1.0}, {25.0, 1.0}},
                                       RectangleMode::Crossing)),
              (std::vector<std::uint64_t>{text.value, line.value}));
}

TEST(TextQueries, SnapToTheInsertionPassingOverTheText) {
    Document document;
    document.add(hello({0.0, 0.0}));
    const SnapPoint insertion{{0.0, 0.0}, SnapKind::Insertion};
    EXPECT_EQ(findSnap(document, {4.0, 1.0}, 1.0, kAllModes), insertion);
    // Within the aperture of the box, from outside.
    EXPECT_EQ(findSnap(document, {4.0, 4.0}, 1.0, kAllModes), insertion);
    EXPECT_FALSE(findSnap(document, {4.0, 5.0}, 1.0, kAllModes));
}

TEST(TextQueries, AnEndpointOrMidpointWithinTheApertureBeatsTheInsertion) {
    Document document;
    document.add(hello({0.0, 0.0}));
    document.add(Line{{8.0, 0.0}, {15.0, 0.0}});
    EXPECT_EQ(findSnap(document, {7.9, 0.1}, 1.0, kAllModes),
              (SnapPoint{{8.0, 0.0}, SnapKind::Endpoint}));
    // With the endpoint mode off, the insertion point comes back.
    EXPECT_EQ(findSnap(document, {7.9, 0.1}, 1.0, SnapModes{false, true, true, true}),
              (SnapPoint{{0.0, 0.0}, SnapKind::Insertion}));
}

TEST(TextQueries, InsertionModeOff) {
    Document document;
    document.add(hello({0.0, 0.0}));
    EXPECT_FALSE(findSnap(document, {4.0, 1.0}, 1.0, SnapModes{true, true, true, false}));
    EXPECT_EQ(findSnap(document, {4.0, 1.0}, 1.0, SnapModes{false, false, false, true}),
              (SnapPoint{{0.0, 0.0}, SnapKind::Insertion}));
}

TEST(TextQueries, InsertionAndCentreHaveTheSamePriority) {
    Document document;
    document.add(Circle{{20.0, 1.0}, 10.0});  // passes through (10,1)
    document.add(hello({0.0, 0.0}));          // its box ends at about x = 8.28
    // 0.5 from the circle, 1.22 from the text box: the circle's centre.
    EXPECT_EQ(findSnap(document, {9.5, 1.0}, 2.0, kAllModes),
              (SnapPoint{{20.0, 1.0}, SnapKind::Center}));
    // Inside the text box, 2 from the circle: the insertion point.
    EXPECT_EQ(findSnap(document, {8.0, 1.0}, 2.0, kAllModes),
              (SnapPoint{{0.0, 0.0}, SnapKind::Insertion}));
}

TEST_F(RandomDocument, PickMatchesExhaustiveSearch) {
    int hits = 0;
    for (int i = 0; i < kQueryCount; ++i) {
        SCOPED_TRACE(i);
        const Vec2 point = generator().point(document());
        const double tolerance = generator().between(0.0, 4.0);
        const auto expected = exhaustivePick(document(), point, tolerance);
        EXPECT_EQ(value(pickEntity(document(), point, tolerance)), expected);
        hits += expected.has_value() ? 1 : 0;
    }
    EXPECT_GT(hits, kQueryCount / 10);  // the comparison must not be trivially empty
}

TEST_F(RandomDocument, RectanglesMatchExhaustiveSearch) {
    for (int i = 0; i < kQueryCount; ++i) {
        SCOPED_TRACE(i);
        const BBox area = generator().area();
        for (const RectangleMode mode : {RectangleMode::Window, RectangleMode::Crossing}) {
            EXPECT_EQ(values(selectInRectangle(document(), area, mode)),
                      exhaustiveSelect(document(), area, mode));
        }
    }
}

TEST_F(RandomDocument, SnapMatchesExhaustiveSearch) {
    int hits = 0;
    int centers = 0;
    for (int i = 0; i < kQueryCount; ++i) {
        SCOPED_TRACE(i);
        const Vec2 pointer = generator().point(document());
        const double aperture = generator().between(0.0, 4.0);
        const SnapModes modes = generator().modes();
        const auto expected = exhaustiveSnap(document(), pointer, aperture, modes);
        EXPECT_EQ(findSnap(document(), pointer, aperture, modes), expected);
        hits += expected.has_value() ? 1 : 0;
        centers += expected && expected->kind == SnapKind::Center ? 1 : 0;
    }
    EXPECT_GT(hits, kQueryCount / 10);
    EXPECT_GT(centers, 0);
}

}  // namespace
}  // namespace tg::core
