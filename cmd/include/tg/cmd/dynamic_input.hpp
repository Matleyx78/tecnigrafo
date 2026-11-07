// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_runner.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tg::cmd {

/// One field of the dynamic input (docs/adr/0042, point 2).
struct DynamicField {
    enum class Kind : std::uint8_t { X, Y, Length, Angle, Value };
    Kind kind{Kind::X};
    std::string typed;  ///< what the user typed; empty: follows the pointer

    friend bool operator==(const DynamicField&, const DynamicField&) = default;
};

/// What the dynamic input shows for the current prompt of a runner.
///
/// The dynamic input is not a second interpreter (docs/adr/0042, point 1):
/// it composes the text the user would type on the command line, which the
/// application hands to CommandRunner::submitText(). Tab, which changes
/// `active`, and the keys that are not part of a number, which go to the
/// command line, belong to the application (docs/adr/0042, point 3).
struct DynamicInput {
    std::string prompt;                ///< Prompt::text, as on the command line
    std::vector<DynamicField> fields;  ///< empty for Text, Object, Selection
    std::size_t active{0};             ///< the field that receives the keys

    friend bool operator==(const DynamicInput&, const DynamicInput&) = default;
};

/// The fields for the runner's current prompt: X and Y without a base point,
/// Length and Angle with one, Value for Distance, Angle and Number prompts.
///
/// `prompt` is CommandRunner::promptText(). There are no fields with no
/// command running ("Command:"), while the runner builds a selection
/// ("Select objects:", the corners of a rectangle, a Selection prompt) and at
/// Text and Object prompts. The base point is CommandRunner::basePoint().
[[nodiscard]] DynamicInput dynamicInputFor(const CommandRunner& runner);

/// The values shown in the fields that the user has not typed, from
/// `pointer`, the point the runner would use for a click there (after ortho,
/// polar and snap), formatted as the command line formats them.
///
/// One string per field of `input`, in order; a typed field shows what was
/// typed. X and Y are the coordinates of that point with formatLength();
/// Length is its distance from the base point with formatLength(), and Angle
/// the direction from the base point with formatAngle(), in [0, 360)
/// degrees, 0 when the point is the base point. Value is the distance
/// (Distance and Number prompts) or the angle (Angle prompts) from the base
/// point, or empty without one.
[[nodiscard]] std::vector<std::string> dynamicInputShown(const DynamicInput& input,
                                                         const CommandRunner& runner,
                                                         core::Vec2 pointer);

/// The line Enter sends with submitText: empty when no field is typed;
/// otherwise "<x>,<y>", "@<length><<angle>", "@<dx>,<dy>" (a comma typed in
/// the Length field) or the value, taking the fields not typed from
/// `pointer` as dynamicInputShown does.
///
/// Typed fields enter the line as typed, so the runner reads and diagnoses
/// them as it reads the command line. With a comma in the Length field the
/// Angle field is not used.
///
/// The line is what Enter guarantees, not the point the fields measure: the
/// runner reads "@" from the last point (docs/adr/0005), while Length and
/// Angle are shown from the base point (docs/adr/0042, point 2). The two
/// differ where a command keeps its base point after other points, as COPY
/// does for the copies after the first.
[[nodiscard]] std::string dynamicInputText(const DynamicInput& input, const CommandRunner& runner,
                                           core::Vec2 pointer);

}  // namespace tg::cmd
