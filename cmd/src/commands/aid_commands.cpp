// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/aid_commands.hpp"

#include "tg/cmd/coordinate_input.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"

#include <optional>
#include <string>
#include <variant>

namespace tg::cmd {
namespace {

// The range of POLARANG, in degrees (docs/adr/0033, point 8): the lower
// bound is excluded, the upper one included.
constexpr double kMinPolarAngle = 0.0;
constexpr double kMaxPolarAngle = 180.0;

}  // namespace

void PolarangCommand::start(CommandContext& context) {
    m_value = context.defaults.polarAngle;
}

Status PolarangCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        // A number alone, with the syntax of a typed distance: no unit, a
        // decimal point, an optional exponent. The comparisons are written so
        // that NaN fails them.
        const std::optional<PointSpec> spec = parseCoordinate(text->text);
        if (!spec || spec->kind != PointSpec::Kind::Distance || !(spec->a > kMinPolarAngle) ||
            !(spec->a <= kMaxPolarAngle)) {
            context.message("Value must be between 0 and 180.");
            return Status::Continue;
        }
        context.defaults.polarAngle = core::degreesToRadians(spec->a);
        return Status::Finished;
    }
    // Enter keeps the current value; no other input answers this question.
    return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
}

Prompt PolarangCommand::prompt() const {
    return {"Enter new value for POLARANG <" + formatAngle(m_value) + ">:", {}, PromptKind::Text};
}

}  // namespace tg::cmd
