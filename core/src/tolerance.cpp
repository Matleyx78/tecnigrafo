// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/tolerance.hpp"

#include "solid_space/geom2d/coincidence.hpp"
#include "solid_space_bridge.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <optional>

namespace tg::core {
namespace {

/// The length in metres of each INSUNITS value, by index (docs/adr/0036,
/// point 1). Each literal is the double nearest the exact definition; the
/// US survey units are quotients of exact integers, rounded once.
constexpr std::array<double, Document::kMaxUnits + 1> kUnitInMeters{
    0.001,                  // 0: no unit, as millimetres
    0.0254,                 // 1: inches
    0.3048,                 // 2: feet
    1609.344,               // 3: miles
    0.001,                  // 4: millimetres
    0.01,                   // 5: centimetres
    1.0,                    // 6: metres
    1000.0,                 // 7: kilometres
    2.54e-8,                // 8: microinches
    2.54e-5,                // 9: mils
    0.9144,                 // 10: yards
    1e-10,                  // 11: angstroms
    1e-9,                   // 12: nanometres
    1e-6,                   // 13: microns
    0.1,                    // 14: decimetres
    10.0,                   // 15: decametres
    100.0,                  // 16: hectometres
    1e9,                    // 17: gigametres
    149597870700.0,         // 18: astronomical units (IAU)
    9460730472580800.0,     // 19: light years (IAU)
    3.0856775814913673e16,  // 20: parsecs, 648000/pi astronomical units
    1200.0 / 3937.0,        // 21: US survey feet
    100.0 / 3937.0,         // 22: US survey inches
    3600.0 / 3937.0,        // 23: US survey yards
    6336000.0 / 3937.0,     // 24: US survey miles
};

bool validUnits(int units) {
    return units >= 0 && units <= Document::kMaxUnits;
}

/// geom2d's answer, or false when it refuses the data (beyond its domain):
/// what cannot be judged is kept, never dropped (docs/adr/0006).
bool degenerate(const bridge::g::Curve& curve, const ToleranceRule& rule) {
    const auto answer = bridge::g::isDegenerate(curve, bridge::policyOf(rule));
    return answer.hasValue() && answer.value();
}

}  // namespace

double unitInMeters(int units) {
    assert(validUnits(units));
    return kUnitInMeters.at(static_cast<std::size_t>(units));
}

ToleranceRule::ToleranceRule(int units) : m_units(units) {
    assert(validUnits(units));
}

ToleranceRule ToleranceRule::of(const Document& document) {
    return ToleranceRule{document.units()};
}

int ToleranceRule::units() const {
    return m_units;
}

double ToleranceRule::epsilon(double magnitude) const {
    return bridge::policyOf(*this).epsilon(magnitude);
}

bool ToleranceRule::coincident(const Vec2& a, const Vec2& b) const {
    return bridge::policyOf(*this).coincident(bridge::toPoint(a), bridge::toPoint(b));
}

bool isDegenerate(const Entity& entity, const ToleranceRule& rule) {
    const std::optional<bridge::g::Curve> curve = bridge::curveOf(entity);
    return curve && degenerate(*curve, rule);
}

bool isDegenerate(const PolylineSegment& segment, const ToleranceRule& rule) {
    return degenerate(bridge::g::Curve{bridge::pieceOf(segment)}, rule);
}

}  // namespace tg::core
