// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The own properties of HATCH and DIMENSION
// (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md, points 2 and 3).

#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_properties.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/hatch_patterns.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

PropertyChange refused(std::string_view reason) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{reason}};
}

PropertyChange changed(Entity entity, std::optional<EntityExtras> extras = std::nullopt) {
    return PropertyChange{std::move(entity), std::move(extras), {}};
}

// --- HATCH -------------------------------------------------------------------

/// The names of the island styles, in the order of IslandStyle.
constexpr std::array<std::string_view, 3> kIslandNames{"normal", "outer", "ignore"};

std::string_view trimmed(std::string_view text) {
    const std::size_t first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

/// $MEASUREMENT of the drawing, as -HATCH reads it: kMetricMeasurement when
/// the header does not give it (a new drawing, a file without it).
int measurementOf(const Document& document) {
    for (const RawSection& section : document.raw().sections) {
        if (section.name != "HEADER") {
            continue;
        }
        for (std::size_t i = 0; i + 1 < section.groups.size(); ++i) {
            if (section.groups[i].code != 9 || trimmed(section.groups[i].value) != "$MEASUREMENT") {
                continue;
            }
            int value = kMetricMeasurement;
            const std::string_view text = trimmed(section.groups[i + 1].value);
            const auto [end, error] =
                std::from_chars(text.data(), text.data() + text.size(), value);
            if (error == std::errc{} && end == text.data() + text.size()) {
                return value;
            }
        }
    }
    return kMetricMeasurement;
}

bool sameAsciiName(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    const auto upper = [](char c) {
        return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
    };
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (upper(a[i]) != upper(b[i])) {
            return false;
        }
    }
    return true;
}

/// withPattern() as a property change: the refusal of a name, scale or angle
/// it does not accept.
PropertyChange patternChange(const Document& document, const EntityRecord& record,
                             const Hatch& hatch, std::string_view name, double scale,
                             double angle) {
    std::optional<Hatch> result = withPattern(hatch, patternOrigin(record.extras), name, scale,
                                              angle, measurementOf(document));
    if (!result) {
        return refused(kInvalidPropertyValue);
    }
    return changed(Entity{std::move(*result)});
}

// --- DIMENSION ---------------------------------------------------------------

/// The dimension regenerated with the tables of `document`, or the refusal of
/// docs/adr/0047, point 4, the entity left as it was.
PropertyChange regeneratedChange(const Document& document, Dimension dimension) {
    std::optional<Dimension> result = regenerated(document, std::move(dimension));
    if (!result) {
        return refused(kDimensionNotRegenerable);
    }
    return changed(Entity{std::move(*result)});
}

}  // namespace

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/, const EntityRecord& record,
                                         const Hatch& hatch) {
    std::vector<std::string> islands(kIslandNames.begin(), kIslandNames.end());
    return {
        {"pattern", PropertyKind::Text, hatch.pattern, {}, false},
        {"scale", PropertyKind::Number, hatch.scale, {}, false},
        {"angle", PropertyKind::Angle, hatch.angle, {}, false},
        {"origin", PropertyKind::Point, patternOrigin(record.extras), {}, false},
        {"islands", PropertyKind::Choice,
         std::string{kIslandNames.at(static_cast<std::size_t>(hatch.islands))}, std::move(islands),
         false},
        {"~area", PropertyKind::Number, filledArea(hatch), {}, true},
    };
}

PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& record,
                                 const Hatch& hatch, std::string_view name,
                                 const PropertyValue& value) {
    const auto* text = std::get_if<std::string>(&value);
    const auto* number = std::get_if<double>(&value);
    if (name == "pattern") {
        if (text == nullptr) {
            return refused(kInvalidPropertyValue);
        }
        if (sameAsciiName(*text, hatch.pattern)) {
            return changed(Entity{hatch});
        }
        return patternChange(document, record, hatch, *text, hatch.scale, hatch.angle);
    }
    if (name == "scale" || name == "angle") {
        if (number == nullptr) {
            return refused(kInvalidPropertyValue);
        }
        const bool scale = name == "scale";
        if (*number == (scale ? hatch.scale : hatch.angle)) {
            return changed(Entity{hatch});
        }
        return patternChange(document, record, hatch, hatch.pattern, scale ? *number : hatch.scale,
                             scale ? hatch.angle : *number);
    }
    if (name == "origin") {
        const auto* point = std::get_if<Vec2>(&value);
        if (point == nullptr || !std::isfinite(point->x) || !std::isfinite(point->y)) {
            return refused(kInvalidPropertyValue);
        }
        const Vec2 from = patternOrigin(record.extras);
        if (*point == from) {
            return changed(Entity{hatch});
        }
        return changed(Entity{withPatternOrigin(hatch, from, *point)},
                       withPatternOriginXdata(record.extras, *point));
    }
    if (name == "islands") {
        if (text == nullptr) {
            return refused(kInvalidPropertyValue);
        }
        for (std::size_t i = 0; i < kIslandNames.size(); ++i) {
            if (*text == kIslandNames.at(i)) {
                Hatch result = hatch;
                result.islands = static_cast<IslandStyle>(i);
                return changed(Entity{std::move(result)});
            }
        }
        return refused(kInvalidPropertyValue);
    }
    return refused(kUnknownProperty);  // unreachable: withOwnProperty() checks the name
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/,
                                         const Dimension& dimension) {
    const DimensionPresentation& presentation = dimension.presentation;
    return {
        {"style", PropertyKind::Text, presentation.style, {}, false},
        {"text", PropertyKind::Text, presentation.textOverride, {}, false},
        {"text-rotation", PropertyKind::Angle, presentation.textRotation.value_or(0.0), {}, false},
        {"~measurement", PropertyKind::Number, dimension.value.measurement, {}, true},
    };
}

PropertyChange withOwnPropertyOf(const Document& document, const EntityRecord& /*record*/,
                                 const Dimension& dimension, std::string_view name,
                                 const PropertyValue& value) {
    const auto* text = std::get_if<std::string>(&value);
    Dimension result = dimension;
    DimensionPresentation& presentation = result.presentation;
    if (name == "style") {
        const DimStyle* style = text != nullptr ? document.dimStyles().find(*text) : nullptr;
        if (style == nullptr) {
            return refused(kInvalidPropertyValue);
        }
        if (style->name == presentation.style) {
            return changed(Entity{dimension});
        }
        presentation.style = style->name;
        return regeneratedChange(document, std::move(result));
    }
    if (name == "text") {
        if (text == nullptr) {
            return refused(kInvalidPropertyValue);
        }
        if (*text == presentation.textOverride) {
            return changed(Entity{dimension});
        }
        presentation.textOverride = *text;
        return regeneratedChange(document, std::move(result));
    }
    if (name == "text-rotation") {
        const auto* angle = std::get_if<double>(&value);
        if (angle == nullptr || !std::isfinite(*angle)) {
            return refused(kInvalidPropertyValue);
        }
        // The value shown is 0 for the automatic angle too: the value shown
        // leaves the dimension as it is, so 0 keeps an automatic angle
        // automatic; any other value becomes the explicit angle, which holds
        // for every position (docs/tappe/0.8/PT23-quote-lineari.md, point 4).
        if (*angle == presentation.textRotation.value_or(0.0)) {
            return changed(Entity{dimension});
        }
        presentation.textRotation = *angle;
        return regeneratedChange(document, std::move(result));
    }
    return refused(kUnknownProperty);  // unreachable: withOwnProperty() checks the name
}

}  // namespace tg::core
