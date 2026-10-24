// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace tg::io {

extern const EntityCodec kCircleCodec;
extern const EntityCodec kArcCodec;

namespace {

/// The values of CIRCLE and ARC as read: centre in world coordinates,
/// radius, and the angles 50/51 in degrees as in the file (0 for CIRCLE).
struct Curve {
    core::EntityExtras extras;
    core::Vec2 center;
    double radius{0.0};
    double startDegrees{0.0};
    double endDegrees{0.0};
    bool flipped{false};
};

/// Reads the groups of a CIRCLE (`arc` false) or of an ARC, each value once;
/// std::nullopt for an extrusion other than (0,0,±1), a value missing or not
/// a number, or a radius <= 0. The subclass markers are the codec's:
/// AcDbCircle, then AcDbArc.
std::optional<Curve> readCurve(const core::RawGroups& groups, bool arc) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    // Centre x, y, radius, start and end angle, in this order.
    std::array<std::optional<double>, 5> values;
    bool circleMarker = false;
    bool arcMarker = false;
    for (const auto& group : parts.geometry) {
        const auto value = trimDxf(group.value);
        if (group.code == 100 && !circleMarker && value == "AcDbCircle") {
            circleMarker = true;
            continue;
        }
        if (group.code == 100 && arc && !arcMarker && value == "AcDbArc") {
            arcMarker = true;
            continue;
        }
        std::size_t index = values.size();
        switch (group.code) {
            case 10:
                index = 0;
                break;
            case 20:
                index = 1;
                break;
            case 40:
                index = 2;
                break;
            case 50:
                index = arc ? 3 : index;
                break;
            case 51:
                index = arc ? 4 : index;
                break;
            default:
                break;
        }
        if (index == values.size()) {
            parts.extras.specific.push_back(group);
            continue;
        }
        const auto number = parseNumber(value);
        if (!number || values.at(index).has_value()) {
            return std::nullopt;
        }
        values.at(index) = number;
    }
    const std::size_t required = arc ? values.size() : 3;
    for (std::size_t i = 0; i < required; ++i) {
        if (!values.at(i).has_value()) {
            return std::nullopt;
        }
    }
    Curve curve{std::move(parts.extras), {values[0].value_or(0), values[1].value_or(0)},
                values[2].value_or(0),   values[3].value_or(0),
                values[4].value_or(0),   extrusion == Extrusion::Down};
    // A radius <= 0 is degenerate (docs/adr/0018, point 3).
    if (!(curve.radius > 0.0)) {
        return std::nullopt;
    }
    if (curve.flipped) {
        curve.center = flipX(curve.center);  // docs/adr/0018, point 5
    }
    curve.extras.flippedExtrusion = curve.flipped;
    return curve;
}

/// Writes 39, the centre with 30, the radius: the AcDbCircle part shared by
/// CIRCLE and ARC, in the order of the DXF reference.
void writeCircleGroups(const core::Vec2& center, double radius, const core::EntityExtras& extras,
                       core::RawGroups& out) {
    const core::Vec2 file = extras.flippedExtrusion ? flipX(center) : center;
    out.push_back({100, "AcDbCircle"});
    constexpr std::array kThickness{39};
    writePlacement(extras, kThickness, out);
    out.push_back({10, formatNumber(file.x)});
    out.push_back({20, formatNumber(file.y)});
    constexpr std::array kZ{30};
    writePlacement(extras, kZ, out);
    out.push_back({40, formatNumber(radius)});
}

/// Writes the extrusion and the placement groups that CIRCLE and ARC do not
/// use (31, 38) but that were read, then the rest of the entity.
void writeCurveTail(const core::EntityExtras& extras, std::size_t from, core::RawGroups& out) {
    constexpr std::array kExtrusion{210, 220, 230, 31, 38};
    writePlacement(extras, kExtrusion, out);
    writeEntityTail(extras, out);
    restorePlacement(extras, from, out);
}

std::optional<core::EntityRecord> readCircle(const core::RawGroups& groups,
                                             EntityReadContext& context) {
    auto values = readCurve(groups, false);
    if (!values) {
        return std::nullopt;
    }
    core::Circle circle{values->center, values->radius};
    return core::EntityRecord{context.id, circle, std::move(values->extras), context.order};
}

void writeCircle(const core::EntityRecord& record, EntityWriteContext& context,
                 core::RawGroups& out) {
    const auto& circle = std::get<core::Circle>(record.entity);
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    writeCircleGroups(circle.center, circle.radius, record.extras, out);
    writeCurveTail(record.extras, from, out);
}

std::optional<core::EntityRecord> readArc(const core::RawGroups& groups,
                                          EntityReadContext& context) {
    auto values = readCurve(groups, true);
    if (!values) {
        return std::nullopt;
    }
    // Degrees in the file, radians in [0, 2π) in the model (docs/adr/0012).
    // The angles are brought into [0, 360) first, so that 30 and 390 give
    // the same radians. With x -> -x a counter-clockwise arc turns
    // clockwise: start and end swap, each mirrored to 180 - angle.
    double start = normalizeDegrees(values->startDegrees);
    double end = normalizeDegrees(values->endDegrees);
    if (values->flipped) {
        start = flipDirectionDegrees(values->endDegrees);
        end = flipDirectionDegrees(values->startDegrees);
    }
    const double startRadians = core::normalizeAngle(core::degreesToRadians(start));
    const double endRadians = core::normalizeAngle(core::degreesToRadians(end));
    // Exact on purpose: equal angles after normalization are the degenerate
    // arc of docs/adr/0018, point 3; the invariant of core::Arc.
    if (startRadians == endRadians) {
        return std::nullopt;
    }
    core::Arc arc{values->center, values->radius, startRadians, endRadians};
    return core::EntityRecord{context.id, arc, std::move(values->extras), context.order};
}

void writeArc(const core::EntityRecord& record, EntityWriteContext& context, core::RawGroups& out) {
    const auto& arc = std::get<core::Arc>(record.entity);
    const auto& extras = record.extras;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    writeCircleGroups(arc.center, arc.radius, extras, out);
    double start = core::radiansToDegrees(arc.startAngle);
    double end = core::radiansToDegrees(arc.endAngle);
    if (extras.flippedExtrusion) {
        // The inverse of the reading is the same formula.
        const double modelStart = start;
        start = flipDirectionDegrees(end);
        end = flipDirectionDegrees(modelStart);
    }
    out.push_back({100, "AcDbArc"});
    out.push_back({50, formatNumber(start)});
    out.push_back({51, formatNumber(end)});
    writeCurveTail(extras, from, out);
}

}  // namespace

const EntityCodec kCircleCodec{"CIRCLE", readCircle, writeCircle};
const EntityCodec kArcCodec{"ARC", readArc, writeArc};

}  // namespace tg::io
