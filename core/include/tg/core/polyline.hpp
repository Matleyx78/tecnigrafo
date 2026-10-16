// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace tg::core {

/// A polyline vertex. `bulge` describes the segment that starts at this
/// vertex, exactly as in the DXF LWPOLYLINE entity:
///
///   bulge = tan(theta / 4)
///
/// where theta is the included angle of the arc. 0 means a straight segment,
/// a positive value an arc travelled counter-clockwise, a negative value a
/// clockwise arc. A bulge of 1 is a half circle.
struct PolylineVertex {
    Vec2 position;
    double bulge{0.0};

    friend constexpr bool operator==(const PolylineVertex&, const PolylineVertex&) = default;
};

/// Geometry of a single polyline segment, resolved from two vertices.
struct PolylineSegment {
    Vec2 start;
    Vec2 end;
    double bulge{0.0};

    [[nodiscard]] bool isArc() const;
    [[nodiscard]] double length() const;
    [[nodiscard]] BBox boundingBox() const;

    /// Appends the points approximating this segment to `out`, excluding the
    /// start point and including the end point. `maxChordError` is the
    /// maximum allowed distance between the true arc and its approximation.
    void tessellate(double maxChordError, std::vector<Vec2>& out) const;
};

/// Arc parameters derived from a bulged segment.
struct ArcGeometry {
    Vec2 center;
    double radius{0.0};
    double startAngle{0.0};  ///< radians, counter-clockwise from +X
    double sweep{0.0};       ///< radians, signed: > 0 counter-clockwise
};

/// Returns the arc described by a bulged segment. Precondition: the
/// segment is an arc (non-zero bulge, distinct end points).
[[nodiscard]] ArcGeometry arcFromBulge(const Vec2& start, const Vec2& end, double bulge);

/// A 2D polyline made of straight and arc segments (DXF LWPOLYLINE model).
class Polyline {
public:
    Polyline() = default;
    explicit Polyline(std::vector<PolylineVertex> vertices, bool closed = false);

    void addVertex(const Vec2& position, double bulge = 0.0);

    [[nodiscard]] std::span<const PolylineVertex> vertices() const { return m_vertices; }

    [[nodiscard]] std::size_t vertexCount() const { return m_vertices.size(); }

    [[nodiscard]] bool isClosed() const { return m_closed; }

    void setClosed(bool closed) { m_closed = closed; }

    /// Number of drawable segments (a closed polyline has one more).
    [[nodiscard]] std::size_t segmentCount() const;

    /// Segment `index`, with 0 <= index < segmentCount().
    [[nodiscard]] PolylineSegment segment(std::size_t index) const;

    [[nodiscard]] double length() const;
    [[nodiscard]] BBox boundingBox() const;

    /// Point list approximating the whole polyline. For a closed polyline the
    /// first point is not repeated at the end.
    [[nodiscard]] std::vector<Vec2> tessellate(double maxChordError) const;

    friend bool operator==(const Polyline&, const Polyline&) = default;

private:
    std::vector<PolylineVertex> m_vertices;
    bool m_closed{false};
};

}  // namespace tg::core
