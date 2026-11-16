// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/dynamic_input.hpp"

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"
#include "tg/core/vec2.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace tg::cmd {
namespace {

using Kind = DynamicField::Kind;

/// The direction from `base` to `point`, as formatAngle() writes it, in
/// [0, 360) degrees; "0" when the two points coincide.
std::string formatDirection(const core::Vec2& base, const core::Vec2& point) {
    const core::Vec2 delta = point - base;
    double angle = core::atan2(delta.y, delta.x);  // 0 for a null delta
    if (angle < 0.0) {
        angle += core::kTwoPi;
    }
    std::string text = formatAngle(angle);
    // An angle just below 360 degrees rounds to "360": the same direction.
    if (text == "360") {
        text = "0";
    }
    return text;
}

/// What a field shows when the user has not typed in it.
std::string pointerValue(Kind kind, const CommandRunner& runner, const core::Vec2& point) {
    const std::optional<core::Vec2> base = runner.basePoint();
    switch (kind) {
        case Kind::X:
            return formatLength(point.x);
        case Kind::Y:
            return formatLength(point.y);
        case Kind::Length:
            return base ? formatLength(core::distance(*base, point)) : std::string{};
        case Kind::Angle:
            return base ? formatDirection(*base, point) : std::string{};
        case Kind::Value:
            if (!base) {
                return {};
            }
            return runner.prompt().kind == PromptKind::Angle
                       ? formatDirection(*base, point)
                       : formatLength(core::distance(*base, point));
    }
    return {};
}

/// The text of the field of `kind`: as typed, or from the pointer.
std::string fieldText(const DynamicInput& input, Kind kind, const CommandRunner& runner,
                      const core::Vec2& point) {
    const auto field = std::ranges::find(input.fields, kind, &DynamicField::kind);
    if (field != input.fields.end() && !field->typed.empty()) {
        return field->typed;
    }
    return pointerValue(kind, runner, point);
}

}  // namespace

DynamicInput dynamicInputFor(const CommandRunner& runner) {
    DynamicInput input;
    input.prompt = runner.promptText();
    // The runner's own questions ("Command:", "Select objects:", corners)
    // report a Point kind: they have no fields (docs/adr/0042, point 2).
    if (!runner.isActive() || runner.isSelecting()) {
        return input;
    }
    switch (runner.prompt().kind) {
        case PromptKind::Point:
            if (runner.basePoint()) {
                input.fields = {{Kind::Length, {}}, {Kind::Angle, {}}};
            } else {
                input.fields = {{Kind::X, {}}, {Kind::Y, {}}};
            }
            break;
        case PromptKind::Distance:
        case PromptKind::Angle:
        case PromptKind::Number:
            input.fields = {{Kind::Value, {}}};
            break;
        case PromptKind::Text:
        case PromptKind::Object:
        case PromptKind::Selection:
            break;
    }
    return input;
}

std::vector<std::string> dynamicInputShown(const DynamicInput& input, const CommandRunner& runner,
                                           core::Vec2 pointer) {
    const core::Vec2 point = runner.clickPoint(pointer);
    std::vector<std::string> shown;
    shown.reserve(input.fields.size());
    for (const DynamicField& field : input.fields) {
        shown.push_back(field.typed.empty() ? pointerValue(field.kind, runner, point)
                                            : field.typed);
    }
    return shown;
}

std::string dynamicInputText(const DynamicInput& input, const CommandRunner& runner,
                             core::Vec2 pointer) {
    const bool typed = std::ranges::any_of(
        input.fields, [](const DynamicField& field) { return !field.typed.empty(); });
    if (!typed) {
        return {};  // Enter, as on the command line (docs/adr/0042, point 3)
    }
    const core::Vec2 point = runner.clickPoint(pointer);
    const auto has = [&input](Kind kind) {
        return std::ranges::find(input.fields, kind, &DynamicField::kind) != input.fields.end();
    };
    if (has(Kind::X) || has(Kind::Y)) {
        return fieldText(input, Kind::X, runner, point) + "," +
               fieldText(input, Kind::Y, runner, point);
    }
    if (has(Kind::Length) || has(Kind::Angle)) {
        const std::string length = fieldText(input, Kind::Length, runner, point);
        if (length.find(',') != std::string::npos) {
            return "@" + length;  // relative Cartesian: @<dx>,<dy>
        }
        return "@" + length + "<" + fieldText(input, Kind::Angle, runner, point);
    }
    return fieldText(input, Kind::Value, runner, point);
}

}  // namespace tg::cmd
