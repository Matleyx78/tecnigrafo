// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

namespace tg::io {
namespace {

struct DirectionGroups {
    std::optional<double> x;
    std::optional<double> y;
};

DirectionGroups directionOf(const core::RawGroups& groups) {
    DirectionGroups result;
    for (const auto& group : groups) {
        if (group.code == 11) {
            result.x = parseNumber(trimDxf(group.value));
        } else if (group.code == 21) {
            result.y = parseNumber(trimDxf(group.value));
        }
    }
    return result;
}

bool isColumnData(const core::RawGroup& group) {
    if (group.code == 101 || group.code == 76 || group.code == 78 || group.code == 79) {
        return true;
    }
    if (group.code != 75) {
        return false;
    }
    const auto columns = parseNumber(trimDxf(group.value));
    return !columns || *columns > 0;
}

std::optional<core::EntityRecord> readMText(const core::RawGroups& groups,
                                            EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    core::MText mtext;
    bool hasX = false;
    bool hasY = false;
    bool hasHeight = false;
    bool hasValue = false;
    bool hasDirectionX = false;
    bool hasDirectionY = false;
    double directionX = 0.0;
    double directionY = 0.0;
    std::string value;
    bool marker = false;
    for (const auto& group : parts.geometry) {
        if (group.code == 100 && !marker && trimDxf(group.value) == "AcDbMText") {
            marker = true;
            continue;
        }
        if (isColumnData(group)) {
            return std::nullopt;
        }
        if (group.code == 3) {
            value += group.value;
            continue;
        }
        if (group.code == 1) {
            value += group.value;
            hasValue = true;
            continue;
        }
        if (group.code == 7) {
            mtext.style = group.value;
            continue;
        }
        const auto number = parseNumber(trimDxf(group.value));
        switch (group.code) {
            case 10:
            case 20:
            case 40:
            case 41:
            case 44:
            case 50:
            case 71:
            case 72:
            case 73:
            case 11:
            case 21:
                if (!number) {
                    return std::nullopt;
                }
                break;
            default:
                parts.extras.specific.push_back(group);
                continue;
        }
        switch (group.code) {
            case 10:
                mtext.insertion.x = *number;
                hasX = true;
                break;
            case 20:
                mtext.insertion.y = *number;
                hasY = true;
                break;
            case 40:
                mtext.height = *number;
                hasHeight = true;
                break;
            case 41:
                mtext.width = *number;
                break;
            case 44:
                mtext.lineSpacing = *number;
                break;
            case 50:
                mtext.rotation = core::degreesToRadians(*number);
                parts.extras.specific.push_back(group);
                break;
            case 71:
                if (*number < 1 || *number > 9) {
                    return std::nullopt;
                }
                mtext.attachment = static_cast<core::MTextAttachment>(static_cast<int>(*number));
                break;
            case 72:
                if (*number != 1 && *number != 5) {
                    return std::nullopt;
                }
                mtext.directionByStyle = *number == 5;
                break;
            case 73:
                if (*number != 1 && *number != 2) {
                    return std::nullopt;
                }
                mtext.exactSpacing = *number == 2;
                break;
            case 11:
                directionX = *number;
                hasDirectionX = true;
                parts.extras.specific.push_back(group);
                break;
            case 21:
                directionY = *number;
                hasDirectionY = true;
                parts.extras.specific.push_back(group);
                break;
            default:
                break;
        }
    }
    if (!hasX || !hasY || !hasHeight || !hasValue || mtext.height <= 0 || mtext.width < 0 ||
        hasDirectionX != hasDirectionY ||
        (hasDirectionX && directionX == 0.0 && directionY == 0.0)) {
        return std::nullopt;
    }
    mtext.value = std::move(value);
    if (hasDirectionX) {
        mtext.rotation = core::atan2(directionY, directionX);
    }
    if (extrusion == Extrusion::Down) {
        mtext.insertion = flipX(mtext.insertion);
        mtext.rotation = negate(mtext.rotation);
        parts.extras.flippedExtrusion = true;
    }
    return core::EntityRecord{context.id, std::move(mtext), std::move(parts.extras), context.order};
}

std::size_t utf8CharactersEnd(std::string_view text, std::size_t from, std::size_t count) {
    std::size_t position = from;
    while (position < text.size() && count != 0) {
        ++position;
        while (position < text.size() &&
               (static_cast<unsigned char>(text[position]) & 0xc0U) == 0x80U) {
            ++position;
        }
        --count;
    }
    return position;
}

void writeValue(std::string_view value, core::RawGroups& out) {
    constexpr std::size_t kGroupCharacters = 250;
    std::size_t from = 0;
    while (utf8CharactersEnd(value, from, kGroupCharacters) < value.size()) {
        const std::size_t end = utf8CharactersEnd(value, from, kGroupCharacters);
        out.push_back({3, std::string{value.substr(from, end - from)}});
        from = end;
    }
    out.push_back({1, std::string{value.substr(from)}});
}

void writeMText(const core::EntityRecord& record, EntityWriteContext& context,
                core::RawGroups& out) {
    auto mtext = std::get<core::MText>(record.entity);
    if (record.extras.flippedExtrusion) {
        mtext.insertion = flipX(mtext.insertion);
        mtext.rotation = negate(mtext.rotation);
    }
    core::EntityExtras tailExtras = record.extras;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    out.push_back({100, "AcDbMText"});
    out.push_back({10, formatNumber(mtext.insertion.x)});
    out.push_back({20, formatNumber(mtext.insertion.y)});
    constexpr std::array kInsertionZ{30};
    writePlacement(record.extras, kInsertionZ, out);
    out.push_back({40, formatNumber(mtext.height)});
    out.push_back({41, formatNumber(mtext.width)});
    for (const auto& group : record.extras.specific) {
        if (group.code == 46) {
            out.push_back(group);
        }
    }
    tailExtras.specific.erase(
        std::remove_if(tailExtras.specific.begin(), tailExtras.specific.end(),
                       [](const core::RawGroup& group) { return group.code == 46; }),
        tailExtras.specific.end());
    out.push_back({71, std::to_string(static_cast<int>(mtext.attachment))});
    out.push_back({72, mtext.directionByStyle ? "5" : "1"});
    writeValue(mtext.value, out);
    out.push_back({7, mtext.style});

    const DirectionGroups direction = directionOf(tailExtras.specific);
    const auto rotationGroup = std::find_if(
        tailExtras.specific.begin(), tailExtras.specific.end(),
        [](const core::RawGroup& group) { return group.code == 50; });
    const std::optional<core::RawGroup> originalRotation =
        rotationGroup == tailExtras.specific.end() ? std::nullopt
                                                   : std::optional{*rotationGroup};
    const auto originalDegrees = originalRotation
                                     ? parseNumber(trimDxf(originalRotation->value))
                                     : std::nullopt;
    const bool preservesDirection = direction.x && direction.y &&
                                    core::atan2(*direction.y, *direction.x) == mtext.rotation;
    tailExtras.specific.erase(
        std::remove_if(tailExtras.specific.begin(), tailExtras.specific.end(),
                       [](const core::RawGroup& group) {
                           return group.code == 11 || group.code == 21 || group.code == 31 ||
                                  group.code == 50;
                       }),
        tailExtras.specific.end());
    if (preservesDirection) {
        for (const auto& group : record.extras.specific) {
            if (group.code == 11 || group.code == 21 || group.code == 31) {
                out.push_back(group);
            }
        }
    } else if (direction.x || direction.y) {
        const core::SinCos vector = core::sinCos(mtext.rotation);
        out.push_back({11, formatNumber(vector.cosine)});
        out.push_back({21, formatNumber(vector.sine)});
    } else if (originalDegrees && core::degreesToRadians(*originalDegrees) == mtext.rotation) {
        out.push_back(originalRotation.value_or(core::RawGroup{}));
    } else if (mtext.rotation != 0.0) {
        out.push_back({50, formatNumber(core::radiansToDegrees(mtext.rotation))});
    }
    out.push_back({73, mtext.exactSpacing ? "2" : "1"});
    out.push_back({44, formatNumber(mtext.lineSpacing)});
    constexpr std::array kExtrusionAndUnusedPlacement{210, 220, 230, 38, 39};
    writePlacement(record.extras, kExtrusionAndUnusedPlacement, out);
    writeEntityTail(tailExtras, out);
    restorePlacement(record.extras, from, out);
}

}  // namespace

const EntityCodec kMTextCodec{"MTEXT", readMText, writeMText};

}  // namespace tg::io
