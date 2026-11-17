// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/math.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>

namespace tg::io {
namespace {

class Cursor {
public:
    explicit Cursor(const core::RawGroups& groups) : m_groups(groups) {}

    [[nodiscard]] bool at(int code) const {
        return m_pos < m_groups.size() && m_groups[m_pos].code == code;
    }

    [[nodiscard]] std::optional<double> number(int code) {
        if (!at(code)) {
            return std::nullopt;
        }
        return parseNumber(trimDxf(m_groups[m_pos++].value));
    }

    [[nodiscard]] std::optional<std::string> text(int code) {
        if (!at(code)) {
            return std::nullopt;
        }
        return std::string{trimDxf(m_groups[m_pos++].value)};
    }

    [[nodiscard]] const core::RawGroup* take(int code) {
        return at(code) ? &m_groups[m_pos++] : nullptr;
    }

    [[nodiscard]] bool done() const { return m_pos == m_groups.size(); }

    [[nodiscard]] const core::RawGroup& current() const { return m_groups[m_pos]; }

    void advance() { ++m_pos; }

private:
    const core::RawGroups& m_groups;
    std::size_t m_pos{0};
};

std::optional<core::Vec2> point(Cursor& cursor, int xCode = 10, int yCode = 20) {
    const auto x = cursor.number(xCode);
    const auto y = cursor.number(yCode);
    if (!x || !y) {
        return std::nullopt;
    }
    return core::Vec2{*x, *y};
}

std::optional<core::HatchLoop> readLoop(Cursor& cursor, bool flipped) {
    const auto flags = cursor.number(92);
    if (!flags) {
        return std::nullopt;
    }
    core::HatchLoop loop;
    loop.flags = static_cast<std::uint32_t>(*flags);
    constexpr std::uint32_t kPolyline = 2;
    if ((loop.flags & kPolyline) != 0) {
        const auto hasBulge = cursor.number(72);
        const auto closed = cursor.number(73);
        const auto count = cursor.number(93);
        if (!hasBulge || !closed || !count || *closed == 0 || *count < 1) {
            return std::nullopt;
        }
        std::vector<core::PolylineVertex> vertices;
        for (int i = 0; i < static_cast<int>(*count); ++i) {
            auto position = point(cursor);
            if (!position) {
                return std::nullopt;
            }
            double bulge = cursor.number(42).value_or(0.0);
            if (flipped) {
                *position = flipX(*position);
                bulge = negate(bulge);
            }
            vertices.push_back({*position, bulge});
        }
        loop.polyline = core::Polyline{std::move(vertices), true};
    } else {
        const auto count = cursor.number(93);
        if (!count || *count < 1) {
            return std::nullopt;
        }
        for (int i = 0; i < static_cast<int>(*count); ++i) {
            const auto type = cursor.number(72);
            if (!type) {
                return std::nullopt;
            }
            if (*type == 1) {
                auto start = point(cursor);
                auto end = point(cursor, 11, 21);
                if (!start || !end) {
                    return std::nullopt;
                }
                if (flipped) {
                    *start = flipX(*start);
                    *end = flipX(*end);
                }
                loop.edges.push_back({core::Line{*start, *end}});
            } else if (*type == 2) {
                auto center = point(cursor);
                const auto radius = cursor.number(40);
                const auto start = cursor.number(50);
                const auto end = cursor.number(51);
                const auto ccw = cursor.number(73);
                if (!center || !radius || !start || !end || !ccw || !(*radius > 0.0)) {
                    return std::nullopt;
                }
                double first = *start;
                double last = *end;
                if (*ccw == 0) {
                    std::swap(first, last);
                }
                if (flipped) {
                    *center = flipX(*center);
                    const double oldFirst = first;
                    first = flipDirectionDegrees(last);
                    last = flipDirectionDegrees(oldFirst);
                }
                const double firstRad = core::degreesToRadians(normalizeDegrees(first));
                double lastRad = core::degreesToRadians(normalizeDegrees(last));
                if (std::abs(last - first) >= 360.0) {
                    lastRad = firstRad + core::kTwoPi;
                }
                loop.edges.push_back({core::Arc{*center, *radius, firstRad, lastRad}});
            } else {
                return std::nullopt;  // Ellipse and spline boundary edges stay raw.
            }
        }
    }
    const auto* const associated = cursor.take(97);
    if (associated == nullptr) {
        return std::nullopt;
    }
    loop.extras.push_back(*associated);
    const auto count = parseNumber(trimDxf(associated->value));
    if (!count || *count < 0) {
        return std::nullopt;
    }
    const int associatedCount = static_cast<int>(count.value_or(0.0));
    for (int i = 0; i < associatedCount; ++i) {
        const auto* const handle = cursor.take(330);
        if (handle == nullptr) {
            return std::nullopt;
        }
        loop.extras.push_back(*handle);
    }
    return loop;
}

std::optional<core::EntityRecord> readHatch(const core::RawGroups& groups,
                                            EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    Cursor cursor(parts.geometry);
    if (!cursor.text(100) || !point(cursor) || !cursor.text(2)) {
        return std::nullopt;
    }
    // Re-read the fixed prefix into the model. The first point is elevation
    // (its Z component is retained by splitEntityGroups()).
    Cursor read(parts.geometry);
    if (read.text(100).value_or("") != "AcDbHatch") {
        return std::nullopt;
    }
    if (!point(read)) {
        return std::nullopt;
    }
    core::Hatch hatch;
    hatch.pattern = read.text(2).value_or("");
    const auto solid = read.number(70);
    const core::RawGroup* associative = read.take(71);
    const auto loopCount = read.number(91);
    if (!solid || associative == nullptr || !loopCount || *loopCount < 0) {
        return std::nullopt;
    }
    hatch.solid = *solid != 0;
    parts.extras.specific.push_back(*associative);
    for (int i = 0; i < static_cast<int>(*loopCount); ++i) {
        auto loop = readLoop(read, extrusion == Extrusion::Down);
        if (!loop) {
            return std::nullopt;
        }
        hatch.loops.push_back(std::move(*loop));
    }
    const auto islands = read.number(75);
    const auto patternType = read.number(76);
    if (!islands || !patternType) {
        return std::nullopt;
    }
    const double angle = read.number(52).value_or(0.0);
    const double scale = read.number(41).value_or(1.0);
    const double doubled = read.number(77).value_or(0.0);
    const double lineCount = read.number(78).value_or(0.0);
    hatch.islands = static_cast<core::IslandStyle>(static_cast<int>(*islands));
    hatch.patternType = static_cast<int>(*patternType);
    hatch.angle = core::reducedDegreesToRadians(angle);
    hatch.scale = scale;
    hatch.doubled = doubled != 0;
    for (int i = 0; i < static_cast<int>(lineCount); ++i) {
        const auto lineAngle = read.number(53);
        auto origin = point(read, 43, 44);
        auto offset = point(read, 45, 46);
        const auto dashCount = read.number(79);
        if (!lineAngle || !origin || !offset || !dashCount) {
            return std::nullopt;
        }
        core::PatternLine line{core::reducedDegreesToRadians(*lineAngle), *origin, *offset, {}};
        for (int dash = 0; dash < static_cast<int>(*dashCount); ++dash) {
            const auto value = read.number(49);
            if (!value) {
                return std::nullopt;
            }
            line.dashes.push_back(*value);
        }
        hatch.patternLines.push_back(std::move(line));
    }
    if (read.at(47)) {
        parts.extras.specific.push_back(*read.take(47));
    }
    const auto seedCount = read.number(98);
    if (!seedCount) {
        return std::nullopt;
    }
    for (int i = 0; i < static_cast<int>(*seedCount); ++i) {
        auto seed = point(read);
        if (!seed) {
            return std::nullopt;
        }
        hatch.seeds.push_back(extrusion == Extrusion::Down ? flipX(*seed) : *seed);
    }
    while (!read.done()) {
        if (read.current().code == 450 && parseNumber(read.current().value).value_or(0) != 0) {
            return std::nullopt;
        }
        parts.extras.specific.push_back(read.current());
        read.advance();
    }
    parts.extras.flippedExtrusion = extrusion == Extrusion::Down;
    if (!core::supported(hatch)) {
        return std::nullopt;
    }
    const core::ToleranceRule rule{context.units};
    for (const auto& loop : hatch.loops) {
        if (!core::isClosed(loop, rule)) {
            return std::nullopt;
        }
    }
    return core::EntityRecord{context.id, std::move(hatch), std::move(parts.extras), context.order};
}

void appendSpecific(const core::EntityExtras& extras, int code, core::RawGroups& out) {
    for (const auto& group : extras.specific) {
        if (group.code == code) {
            out.push_back(group);
        }
    }
}

std::string integerText(std::size_t value) {
    return std::to_string(value);
}

void writeHatch(const core::EntityRecord& record, EntityWriteContext& context,
                core::RawGroups& out) {
    const auto& hatch = std::get<core::Hatch>(record.entity);
    const auto& extras = record.extras;
    const bool flipped = extras.flippedExtrusion;
    writeEntityCommon(record, context, out);
    out.push_back({100, "AcDbHatch"});
    out.push_back({10, "0"});
    out.push_back({20, "0"});
    writePlacement(extras, std::array{30}, out);
    writePlacement(extras, std::array{210, 220, 230}, out);
    out.push_back({2, hatch.pattern});
    out.push_back({70, hatch.solid ? "1" : "0"});
    appendSpecific(extras, 71, out);
    out.push_back({91, integerText(hatch.loops.size())});
    for (const auto& loop : hatch.loops) {
        out.push_back({92, std::to_string(loop.flags)});
        if (loop.polyline) {
            const bool hasBulge = std::ranges::any_of(
                loop.polyline->vertices(),
                [](const core::PolylineVertex& vertex) { return vertex.bulge != 0.0; });
            out.push_back({72, hasBulge ? "1" : "0"});
            out.push_back({73, "1"});
            out.push_back({93, integerText(loop.polyline->vertexCount())});
            for (const auto& vertex : loop.polyline->vertices()) {
                const core::Vec2 position = flipped ? flipX(vertex.position) : vertex.position;
                out.push_back({10, formatNumber(position.x)});
                out.push_back({20, formatNumber(position.y)});
                const double bulge = flipped ? negate(vertex.bulge) : vertex.bulge;
                if (bulge != 0.0) {
                    out.push_back({42, formatNumber(bulge)});
                }
            }
        } else {
            out.push_back({93, integerText(loop.edges.size())});
            for (const auto& edge : loop.edges) {
                if (const auto* line = std::get_if<core::Line>(&edge.curve)) {
                    const auto start = flipped ? flipX(line->start) : line->start;
                    const auto end = flipped ? flipX(line->end) : line->end;
                    out.insert(out.end(), {{72, "1"},
                                           {10, formatNumber(start.x)},
                                           {20, formatNumber(start.y)},
                                           {11, formatNumber(end.x)},
                                           {21, formatNumber(end.y)}});
                } else {
                    const auto& arc = std::get<core::Arc>(edge.curve);
                    const auto center = flipped ? flipX(arc.center) : arc.center;
                    double start = core::radiansToDegrees(arc.startAngle);
                    double end = core::radiansToDegrees(arc.endAngle);
                    if (flipped) {
                        const double old = start;
                        start = flipDirectionDegrees(end);
                        end = flipDirectionDegrees(old);
                    }
                    out.insert(out.end(), {{72, "2"},
                                           {10, formatNumber(center.x)},
                                           {20, formatNumber(center.y)},
                                           {40, formatNumber(arc.radius)},
                                           {50, formatNumber(start)},
                                           {51, formatNumber(end)},
                                           {73, "1"}});
                }
            }
        }
        out.insert(out.end(), loop.extras.begin(), loop.extras.end());
    }
    out.push_back({75, std::to_string(static_cast<int>(hatch.islands))});
    out.push_back({76, std::to_string(hatch.patternType)});
    if (!hatch.solid) {
        out.push_back({52, formatNumber(core::radiansToDegrees(hatch.angle))});
        out.push_back({41, formatNumber(hatch.scale)});
        out.push_back({77, hatch.doubled ? "1" : "0"});
        out.push_back({78, integerText(hatch.patternLines.size())});
        for (const auto& line : hatch.patternLines) {
            out.insert(out.end(), {{53, formatNumber(core::radiansToDegrees(line.angle))},
                                   {43, formatNumber(line.origin.x)},
                                   {44, formatNumber(line.origin.y)},
                                   {45, formatNumber(line.offset.x)},
                                   {46, formatNumber(line.offset.y)},
                                   {79, integerText(line.dashes.size())}});
            for (const double dash : line.dashes) {
                out.push_back({49, formatNumber(dash)});
            }
        }
    }
    appendSpecific(extras, 47, out);
    out.push_back({98, integerText(hatch.seeds.size())});
    for (const auto& modelSeed : hatch.seeds) {
        const auto seed = flipped ? flipX(modelSeed) : modelSeed;
        out.push_back({10, formatNumber(seed.x)});
        out.push_back({20, formatNumber(seed.y)});
    }
    for (const auto& group : extras.specific) {
        if (group.code != 71 && group.code != 47) {
            out.push_back(group);
        }
    }
    out.insert(out.end(), extras.extended.begin(), extras.extended.end());
}

}  // namespace

const EntityCodec kHatchCodec{"HATCH", readHatch, writeHatch};

}  // namespace tg::io
