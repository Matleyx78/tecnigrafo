// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/math.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

namespace tg::io {

namespace {

/// The largest coordinate that the tolerance rule judges (solid-space's
/// domain, core::ToleranceRule::coincident()).
constexpr double kRuleDomain = 0x1p50;

/// The groups of the model, in the order of the DXF reference: centre x, y,
/// end of the major axis x, y, ratio, start and end parameter.
constexpr std::array kModelCodes{10, 20, 11, 21, 40, 41, 42};

/// The index of `code` in kModelCodes, or std::nullopt.
std::optional<std::size_t> modelIndex(int code) {
    for (std::size_t i = 0; i < kModelCodes.size(); ++i) {
        if (kModelCodes.at(i) == code) {
            return i;
        }
    }
    return std::nullopt;
}

bool withinRule(const core::Vec2& point) {
    return std::abs(point.x) <= kRuleDomain && std::abs(point.y) <= kRuleDomain;
}

/// The parameters of `ellipse` as the model keeps them, or false when it is
/// degenerate (docs/adr/0018, point 3). The span from 41 to 42, brought into
/// (0, 2π], is the full ellipse 0 and 2π when it is 2π, or when it is more
/// than π and its two end points coincide under the tolerance rule of the
/// drawing (decision of 2026-11-15 in #679, solid-space ADR 0009): an ellipse
/// exploded from a scaled block comes with a gap of about 1e-15 rad. Any other
/// arc keeps its parameters, brought into [0, 2π); equal there, it is
/// degenerate.
bool settleParameters(core::Ellipse& ellipse, const core::ToleranceRule& rule) {
    const double span = core::parameterSpan(ellipse);
    bool whole = span == core::kTwoPi;
    if (!whole && span > core::kPi) {
        const core::Vec2 start = core::pointAt(ellipse, ellipse.startParameter);
        const core::Vec2 end = core::pointAt(ellipse, ellipse.endParameter);
        whole = withinRule(start) && withinRule(end) && rule.coincident(start, end);
    }
    if (whole) {
        ellipse.startParameter = 0.0;
        ellipse.endParameter = core::kTwoPi;
        return true;
    }
    const double start = core::normalizeAngle(ellipse.startParameter);
    const double end = core::normalizeAngle(ellipse.endParameter);
    if (start == end) {
        return false;
    }
    ellipse.startParameter = start;
    ellipse.endParameter = end;
    return true;
}

/// Reads an ELLIPSE, each value once; std::nullopt for an extrusion other
/// than (0,0,±1), a value missing or not a number, an ellipse that is not
/// core::supported() (a major axis 0, a ratio outside (0, 1]) or a degenerate
/// arc. The subclass marker AcDbEllipse is the codec's.
std::optional<core::EntityRecord> readEllipse(const core::RawGroups& groups,
                                              EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    std::array<std::optional<double>, kModelCodes.size()> values;
    bool marker = false;
    for (const auto& group : parts.geometry) {
        const auto value = trimDxf(group.value);
        if (group.code == 100 && !marker && value == "AcDbEllipse") {
            marker = true;
            continue;
        }
        const std::optional<std::size_t> index = modelIndex(group.code);
        if (!index) {
            parts.extras.specific.push_back(group);
            continue;
        }
        const auto number = parseNumber(value);
        if (!number || values.at(*index).has_value()) {
            return std::nullopt;
        }
        values.at(*index) = number;
    }
    for (const auto& value : values) {
        if (!value.has_value()) {
            return std::nullopt;
        }
    }
    core::Ellipse ellipse{{values[0].value_or(0), values[1].value_or(0)},
                          {values[2].value_or(0), values[3].value_or(0)},
                          values[4].value_or(0),
                          values[5].value_or(0),
                          values[6].value_or(0)};
    if (!core::supported(ellipse)) {
        return std::nullopt;
    }
    const bool flipped = extrusion == Extrusion::Down;
    if (flipped) {
        // docs/adr/0018, point 5: x -> -x for the centre and the axis. The
        // minor axis turns the other way, so the point of parameter t has -t
        // in the world: start and end swap, each with its sign changed.
        ellipse.center = flipX(ellipse.center);
        ellipse.majorAxis = flipX(ellipse.majorAxis);
        const double start = ellipse.startParameter;
        ellipse.startParameter = negate(ellipse.endParameter);
        ellipse.endParameter = negate(start);
    }
    if (!settleParameters(ellipse, core::ToleranceRule{context.units})) {
        return std::nullopt;
    }
    parts.extras.flippedExtrusion = flipped;
    return core::EntityRecord{context.id, ellipse, std::move(parts.extras), context.order};
}

/// Writes the groups of the ELLIPSE in the order of the DXF reference, the
/// placement groups as they were read, then the rest of the entity. An arc is
/// written with 41 in [0, 2π) and 42 = 41 + its span, as AutoCAD writes it; a
/// full ellipse with 0 and 2π.
void writeEllipse(const core::EntityRecord& record, EntityWriteContext& context,
                  core::RawGroups& out) {
    const auto& ellipse = std::get<core::Ellipse>(record.entity);
    const auto& extras = record.extras;
    const bool flipped = extras.flippedExtrusion;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    const core::Vec2 center = flipped ? flipX(ellipse.center) : ellipse.center;
    const core::Vec2 axis = flipped ? flipX(ellipse.majorAxis) : ellipse.majorAxis;
    const double span = core::parameterSpan(ellipse);
    double start = 0.0;
    double end = core::kTwoPi;
    if (span != core::kTwoPi) {
        // The inverse of the reading: the start in the file is the end in
        // the model, with its sign changed.
        start =
            core::normalizeAngle(flipped ? negate(ellipse.endParameter) : ellipse.startParameter);
        end = start + span;
    }
    out.push_back({100, "AcDbEllipse"});
    out.push_back({10, formatNumber(center.x)});
    out.push_back({20, formatNumber(center.y)});
    constexpr std::array kCenterZ{30};
    writePlacement(extras, kCenterZ, out);
    out.push_back({11, formatNumber(axis.x)});
    out.push_back({21, formatNumber(axis.y)});
    // The Z of the axis end, the extrusion, then the placement groups that
    // ELLIPSE does not use but that were read.
    constexpr std::array kAxisZAndExtrusion{31, 210, 220, 230, 38, 39};
    writePlacement(extras, kAxisZAndExtrusion, out);
    out.push_back({40, formatNumber(ellipse.ratio)});
    out.push_back({41, formatNumber(start)});
    out.push_back({42, formatNumber(end)});
    writeEntityTail(extras, out);
    restorePlacement(extras, from, out);
}

}  // namespace

const EntityCodec kEllipseCodec{"ELLIPSE", readEllipse, writeEllipse};

}  // namespace tg::io
