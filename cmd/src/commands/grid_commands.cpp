// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/grid_commands.hpp"

#include "../text.hpp"
#include "tg/cmd/coordinate_input.hpp"
#include "tg/cmd/value_format.hpp"

#include <charconv>
#include <cmath>
#include <optional>
#include <string>
#include <system_error>
#include <variant>

namespace tg::cmd {
namespace {

// The bits of GRIDDISPLAY that Tecnigrafo uses (docs/adr/0055, point 1).
constexpr int kAdaptiveBit = 2;
constexpr int kSubdivisionBit = 4;
// GRIDDISPLAY holds four bits in AutoCAD: 0 to 15.
constexpr int kMaxGridDisplay = 15;

// UCSICON as AutoCAD 2021 sets it (docs/tappe/0.8.1/dxf-di-prova.md,
// "Seconda seduta in AutoCAD"): bit 1 on, bit 2 at the origin.
constexpr int kUcsIconOn = 3;
constexpr int kUcsIconOff = 2;
constexpr int kUcsIconNoOrigin = 1;
constexpr int kUcsIconOnBit = 1;

constexpr std::string_view kInvalidOption = "Invalid option keyword.";
constexpr std::string_view kNumericDistance = "Requires numeric distance.";
constexpr std::string_view kGridPositive = "Value must be positive.";
constexpr std::string_view kSnapPositive = "Value must be positive and nonzero.";
constexpr std::string_view kMajorRange = "Requires an integer between 1 and 100.";

/// What the answer of a spacing question is: a number, or a multiple of the
/// snap spacing ("2x").
struct Spacing {
    double value{0.0};
    bool ofSnap{false};  ///< "2x": value times the snap spacing
};

/// A number alone, with the syntax of a typed distance (no unit, a decimal
/// point, an optional exponent).
std::optional<double> parseNumber(std::string_view text) {
    const std::optional<PointSpec> spec = parseCoordinate(text);
    if (!spec || spec->kind != PointSpec::Kind::Distance) {
        return std::nullopt;
    }
    return spec->a;
}

/// A number, or a number followed by x or X (AutoCAD's "2x").
std::optional<Spacing> parseSpacing(std::string_view text) {
    if (text.ends_with('x') || text.ends_with('X')) {
        text.remove_suffix(1);
        if (const std::optional<double> value = parseNumber(text)) {
            return Spacing{*value, true};
        }
        return std::nullopt;
    }
    if (const std::optional<double> value = parseNumber(text)) {
        return Spacing{*value, false};
    }
    return std::nullopt;
}

/// "x,y", absolute: the only point a variable takes.
std::optional<core::Vec2> parsePoint(std::string_view text) {
    const std::optional<PointSpec> spec = parseCoordinate(text);
    if (!spec || spec->kind != PointSpec::Kind::Absolute) {
        return std::nullopt;
    }
    return core::Vec2{spec->a, spec->b};
}

/// An integer, with an optional sign, as MIRRTEXT reads it.
std::optional<int> parseInteger(std::string_view text) {
    // from_chars accepts a minus sign but not an explicit plus sign.
    if (text.starts_with('+')) {
        text.remove_prefix(1);
    }
    int number = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return number;
}

/// The name of the option of `prompt` that `text` names, if any: a copy,
/// since prompt() returns a temporary.
std::optional<std::string> optionOf(const Prompt& prompt, std::string_view text) {
    const Keyword* keyword = findKeyword(prompt, text);
    if (keyword == nullptr) {
        return std::nullopt;
    }
    return keyword->name;
}

std::string formatPoint(const core::Vec2& point) {
    return formatLength(point.x) + "," + formatLength(point.y);
}

/// The proposal of a question for one spacing per axis: the spacing, or
/// "A" when the two differ (Aspect).
std::string spacingProposal(const core::Vec2& spacing) {
    return spacing.x == spacing.y ? formatLength(spacing.x) : std::string{"A"};
}

Prompt textPrompt(std::string text, std::vector<Keyword> keywords = {}) {
    return {std::move(text), std::move(keywords), PromptKind::Text};
}

const std::vector<Keyword>& yesNo() {
    static const std::vector<Keyword> kYesNo{{"Yes", "Y"}, {"No", "N"}};
    return kYesNo;
}

std::string yesNoProposal(bool yes) {
    return yes ? "Yes" : "No";
}

/// The text of an input at a text question: std::nullopt for Enter, the
/// trimmed text otherwise; `entered` false for any other input.
struct Answer {
    bool entered{false};
    std::optional<std::string_view> text;
};

Answer answerOf(const Input& input) {
    if (std::holds_alternative<ConfirmInput>(input)) {
        return {true, std::nullopt};
    }
    if (const auto* text = std::get_if<TextInput>(&input)) {
        const std::string_view trimmed = detail::trim(text->text);
        if (trimmed.empty()) {
            return {true, std::nullopt};
        }
        return {true, trimmed};
    }
    return {};
}

}  // namespace

core::Vec2 gridNode(const core::GridSettings& grid, const core::Vec2& point) {
    const core::Vec2& base = grid.snapBase;
    const core::Vec2& unit = grid.snapUnit;
    return {base.x + (std::round((point.x - base.x) / unit.x) * unit.x),
            base.y + (std::round((point.y - base.y) / unit.y) * unit.y)};
}

// GridCommand

void GridCommand::start(CommandContext& context) {
    m_grid = context.editor.document().grid();
    m_stage = Stage::Spacing;
}

Status GridCommand::handle(CommandContext& context, const Input& input) {
    const Answer answer = answerOf(input);
    if (!answer.entered) {
        return Status::Continue;
    }
    if (!answer.text) {
        // Enter takes the proposal.
        switch (m_stage) {
            case Stage::Spacing:
                m_grid.gridOn = true;
                return finish(context);
            case Stage::HorizontalSpacing:
                m_stage = Stage::VerticalSpacing;
                return Status::Continue;
            case Stage::VerticalSpacing:
                m_grid.gridOn = true;
                return finish(context);
            case Stage::Major:
                return finish(context);
            case Stage::Adaptive:
                return adaptive(context, yesNoProposal((m_grid.gridDisplay & kAdaptiveBit) != 0));
            case Stage::Subdivision:
                return finish(context);
        }
        return Status::Continue;
    }
    switch (m_stage) {
        case Stage::Spacing:
            return spacing(context, *answer.text);
        case Stage::HorizontalSpacing:
        case Stage::VerticalSpacing:
            return aspect(context, *answer.text);
        case Stage::Major:
            return major(context, *answer.text);
        case Stage::Adaptive:
        case Stage::Subdivision:
            return adaptive(context, *answer.text);
    }
    return Status::Continue;
}

Status GridCommand::spacing(CommandContext& context, std::string_view text) {
    if (const std::optional<std::string> keyword = optionOf(prompt(), text)) {
        if (*keyword == "ON" || *keyword == "OFF") {
            m_grid.gridOn = *keyword == "ON";
            return finish(context);
        }
        if (*keyword == "Snap") {
            m_grid.gridUnit = {0.0, 0.0};  // both axes follow the snap
            return finish(context);
        }
        if (*keyword == "Major") {
            m_stage = Stage::Major;
        } else if (*keyword == "aDaptive") {
            m_stage = Stage::Adaptive;
        } else {
            m_stage = Stage::HorizontalSpacing;
        }
        return Status::Continue;
    }
    const std::optional<Spacing> spacing = parseSpacing(text);
    if (!spacing) {
        context.message(std::string{kInvalidOption});
        return Status::Continue;
    }
    // Written so that NaN fails too.
    if (!(spacing->value >= 0.0)) {
        context.message(std::string{kGridPositive});
        return Status::Continue;
    }
    m_grid.gridUnit = spacing->ofSnap ? m_grid.snapUnit * spacing->value
                                      : core::Vec2{spacing->value, spacing->value};
    m_grid.gridOn = true;
    return finish(context);
}

Status GridCommand::aspect(CommandContext& context, std::string_view text) {
    const std::optional<Spacing> spacing = parseSpacing(text);
    if (!spacing) {
        context.message(std::string{kNumericDistance});
        return Status::Continue;
    }
    if (!(spacing->value >= 0.0)) {
        context.message(std::string{kGridPositive});
        return Status::Continue;
    }
    // A spacing of 0 is kept for its axis, which then follows the snap
    // (docs/adr/0055, point 4).
    if (m_stage == Stage::HorizontalSpacing) {
        m_grid.gridUnit.x = spacing->ofSnap ? m_grid.snapUnit.x * spacing->value : spacing->value;
        m_stage = Stage::VerticalSpacing;
        return Status::Continue;
    }
    m_grid.gridUnit.y = spacing->ofSnap ? m_grid.snapUnit.y * spacing->value : spacing->value;
    m_grid.gridOn = true;
    return finish(context);
}

Status GridCommand::major(CommandContext& context, std::string_view text) {
    const std::optional<int> value = parseInteger(text);
    if (!value || *value < 1 || *value > core::GridSettings::kMaxGridMajor) {
        context.message(std::string{kMajorRange});
        return Status::Continue;
    }
    m_grid.gridMajor = *value;
    return finish(context);
}

Status GridCommand::adaptive(CommandContext& context, std::string_view text) {
    const std::optional<std::string> keyword = optionOf(prompt(), text);
    if (!keyword) {
        context.message(std::string{kInvalidOption});
        return Status::Continue;
    }
    const bool yes = *keyword == "Yes";
    if (m_stage == Stage::Subdivision) {
        m_grid.gridDisplay =
            yes ? (m_grid.gridDisplay | kSubdivisionBit) : (m_grid.gridDisplay & ~kSubdivisionBit);
        return finish(context);
    }
    if (yes) {
        m_grid.gridDisplay |= kAdaptiveBit;
        m_stage = Stage::Subdivision;
        return Status::Continue;
    }
    // The subdivision works only with the adaptive grid (docs/adr/0055,
    // point 5): No clears both bits.
    m_grid.gridDisplay &= ~(kAdaptiveBit | kSubdivisionBit);
    return finish(context);
}

Status GridCommand::finish(CommandContext& context) {
    context.editor.tables().setGrid(m_grid);
    return Status::Finished;
}

Prompt GridCommand::prompt() const {
    switch (m_stage) {
        case Stage::Spacing:
            return textPrompt("Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect] <" +
                                  spacingProposal(m_grid.gridUnit) + ">:",
                              {{"ON", "ON"},
                               {"OFF", "OFF"},
                               {"Snap", "S"},
                               {"Major", "M"},
                               {"aDaptive", "D"},
                               {"Aspect", "A"}});
        case Stage::HorizontalSpacing:
            return textPrompt("Specify the horizontal spacing(X) <" +
                              formatLength(m_grid.gridUnit.x) + ">:");
        case Stage::VerticalSpacing:
            return textPrompt("Specify the vertical spacing(Y) <" +
                              formatLength(m_grid.gridUnit.y) + ">:");
        case Stage::Major:
            return textPrompt("Enter number of grid divisions per major grid line <" +
                              std::to_string(m_grid.gridMajor) + ">:");
        case Stage::Adaptive:
            return textPrompt("Turn adaptive behavior on [Yes/No] <" +
                                  yesNoProposal((m_grid.gridDisplay & kAdaptiveBit) != 0) + ">:",
                              yesNo());
        case Stage::Subdivision:
            return textPrompt("Allow subdivision below grid spacing [Yes/No] <" +
                                  yesNoProposal((m_grid.gridDisplay & kSubdivisionBit) != 0) + ">:",
                              yesNo());
    }
    return {};
}

CommandDescription GridCommand::description() {
    return {"GRID",
            {},
            "Sets the grid of the drawing.",
            false,
            {{"spacing",
              "Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect] <{default}>:",
              ValueType::Real,
              {{"ON", "ON"},
               {"OFF", "OFF"},
               {"Snap", "S"},
               {"Major", "M"},
               {"aDaptive", "D"},
               {"Aspect", "A"}},
              true},
             {"horizontal-spacing",
              "Specify the horizontal spacing(X) <{default}>:",
              ValueType::Real,
              {},
              true},
             {"vertical-spacing",
              "Specify the vertical spacing(Y) <{default}>:",
              ValueType::Real,
              {},
              true},
             {"major",
              "Enter number of grid divisions per major grid line <{default}>:",
              ValueType::Integer,
              {},
              true},
             {"adaptive",
              "Turn adaptive behavior on [Yes/No] <{default}>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true},
             {"subdivision",
              "Allow subdivision below grid spacing [Yes/No] <{default}>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true}}};
}

// SnapCommand

void SnapCommand::start(CommandContext& context) {
    m_grid = context.editor.document().grid();
    m_stage = Stage::Spacing;
}

Status SnapCommand::handle(CommandContext& context, const Input& input) {
    const Answer answer = answerOf(input);
    if (!answer.entered) {
        return Status::Continue;
    }
    if (!answer.text) {
        // Enter takes the proposal.
        if (m_stage == Stage::HorizontalSpacing) {
            m_stage = Stage::VerticalSpacing;
            return Status::Continue;
        }
        m_grid.snapOn = true;
        return finish(context);
    }
    const std::string_view text = *answer.text;
    if (m_stage == Stage::Spacing) {
        if (const std::optional<std::string> keyword = optionOf(prompt(), text)) {
            if (*keyword == "Aspect") {
                m_stage = Stage::HorizontalSpacing;
                return Status::Continue;
            }
            m_grid.snapOn = *keyword == "ON";
            return finish(context);
        }
    }
    const std::optional<double> value = parseNumber(text);
    if (!value) {
        context.message(std::string{m_stage == Stage::Spacing ? kInvalidOption : kNumericDistance});
        return Status::Continue;
    }
    // Written so that NaN fails too.
    if (!(*value > 0.0)) {
        context.message(std::string{kSnapPositive});
        return Status::Continue;
    }
    switch (m_stage) {
        case Stage::Spacing:
            m_grid.snapUnit = {*value, *value};
            break;
        case Stage::HorizontalSpacing:
            m_grid.snapUnit.x = *value;
            m_stage = Stage::VerticalSpacing;
            return Status::Continue;
        case Stage::VerticalSpacing:
            m_grid.snapUnit.y = *value;
            break;
    }
    m_grid.snapOn = true;
    return finish(context);
}

Status SnapCommand::finish(CommandContext& context) {
    context.editor.tables().setGrid(m_grid);
    return Status::Finished;
}

Prompt SnapCommand::prompt() const {
    switch (m_stage) {
        case Stage::Spacing:
            return textPrompt("Specify snap spacing or [ON/OFF/Aspect] <" +
                                  spacingProposal(m_grid.snapUnit) + ">:",
                              {{"ON", "ON"}, {"OFF", "OFF"}, {"Aspect", "A"}});
        case Stage::HorizontalSpacing:
            return textPrompt("Specify horizontal spacing <" + formatLength(m_grid.snapUnit.x) +
                              ">:");
        case Stage::VerticalSpacing:
            return textPrompt("Specify vertical spacing <" + formatLength(m_grid.snapUnit.y) +
                              ">:");
    }
    return {};
}

CommandDescription SnapCommand::description() {
    return {
        "SNAP",
        {},
        "Sets the snap to the grid of the drawing.",
        false,
        {{"spacing",
          "Specify snap spacing or [ON/OFF/Aspect] <{default}>:",
          ValueType::Real,
          {{"ON", "ON"}, {"OFF", "OFF"}, {"Aspect", "A"}},
          true},
         {"horizontal-spacing",
          "Specify horizontal spacing <{default}>:",
          ValueType::Real,
          {},
          true},
         {"vertical-spacing", "Specify vertical spacing <{default}>:", ValueType::Real, {}, true}}};
}

// UcsiconCommand

void UcsiconCommand::start(CommandContext& context) {
    m_value = context.editor.document().grid().ucsIcon;
}

Status UcsiconCommand::handle(CommandContext& context, const Input& input) {
    const Answer answer = answerOf(input);
    if (!answer.entered) {
        return Status::Continue;
    }
    if (!answer.text) {
        return Status::Finished;  // Enter keeps the state
    }
    const std::optional<std::string> keyword = optionOf(prompt(), *answer.text);
    if (!keyword) {
        context.message(std::string{kInvalidOption});
        return Status::Continue;
    }
    core::GridSettings grid = context.editor.document().grid();
    if (*keyword == "OFF") {
        grid.ucsIcon = kUcsIconOff;
    } else if (*keyword == "Noorigin") {
        grid.ucsIcon = kUcsIconNoOrigin;
    } else {
        grid.ucsIcon = kUcsIconOn;  // ON and ORigin
    }
    context.editor.tables().setGrid(grid);
    return Status::Finished;
}

Prompt UcsiconCommand::prompt() const {
    return textPrompt(std::string{"Enter an option [ON/OFF/Noorigin/ORigin] <"} +
                          ((m_value & kUcsIconOnBit) != 0 ? "ON" : "OFF") + ">:",
                      {{"ON", "ON"}, {"OFF", "OFF"}, {"Noorigin", "N"}, {"ORigin", "OR"}});
}

CommandDescription UcsiconCommand::description() {
    return {"UCSICON",
            {},
            "Shows or hides the UCS icon of the drawing.",
            false,
            {{"option",
              "Enter an option [ON/OFF/Noorigin/ORigin] <{default}>:",
              ValueType::Keyword,
              {{"ON", "ON"}, {"OFF", "OFF"}, {"Noorigin", "N"}, {"ORigin", "OR"}},
              true}}};
}

// GridVariableCommand

std::string_view GridVariableCommand::name(Variable variable) {
    switch (variable) {
        case Variable::GridMode:
            return "GRIDMODE";
        case Variable::GridUnit:
            return "GRIDUNIT";
        case Variable::SnapMode:
            return "SNAPMODE";
        case Variable::SnapUnit:
            return "SNAPUNIT";
        case Variable::SnapBase:
            return "SNAPBASE";
        case Variable::GridDisplay:
            return "GRIDDISPLAY";
        case Variable::GridMajor:
            return "GRIDMAJOR";
    }
    return {};
}

void GridVariableCommand::start(CommandContext& context) {
    m_grid = context.editor.document().grid();
}

Status GridVariableCommand::handle(CommandContext& context, const Input& input) {
    const Answer answer = answerOf(input);
    if (!answer.entered) {
        return Status::Continue;
    }
    if (!answer.text) {
        return Status::Finished;  // Enter keeps the value
    }
    const std::string_view text = *answer.text;
    core::GridSettings grid = m_grid;
    switch (m_variable) {
        case Variable::GridMode:
        case Variable::SnapMode: {
            const std::optional<int> value = parseInteger(text);
            if (!value || (*value != 0 && *value != 1)) {
                context.message("Requires an integer between 0 and 1.");
                return Status::Continue;
            }
            (m_variable == Variable::GridMode ? grid.gridOn : grid.snapOn) = *value == 1;
            break;
        }
        case Variable::GridDisplay: {
            const std::optional<int> value = parseInteger(text);
            if (!value || *value < 0 || *value > kMaxGridDisplay) {
                context.message("Requires an integer between 0 and 15.");
                return Status::Continue;
            }
            grid.gridDisplay = *value;
            break;
        }
        case Variable::GridMajor: {
            const std::optional<int> value = parseInteger(text);
            if (!value || *value < 1 || *value > core::GridSettings::kMaxGridMajor) {
                context.message(std::string{kMajorRange});
                return Status::Continue;
            }
            grid.gridMajor = *value;
            break;
        }
        case Variable::GridUnit:
        case Variable::SnapUnit:
        case Variable::SnapBase: {
            const std::optional<core::Vec2> point = parsePoint(text);
            if (point) {
                if (m_variable == Variable::GridUnit) {
                    grid.gridUnit = *point;
                } else if (m_variable == Variable::SnapUnit) {
                    grid.snapUnit = *point;
                } else {
                    grid.snapBase = *point;
                }
            }
            // core::isValid() rejects a negative grid spacing, a snap
            // spacing that is not > 0 and numbers that are not finite.
            if (!point || !core::isValid(grid)) {
                context.message("Cannot set " + std::string{name(m_variable)} + " to that value.");
                context.message("*Invalid*");
                return Status::Finished;
            }
            break;
        }
    }
    context.editor.tables().setGrid(grid);
    return Status::Finished;
}

Prompt GridVariableCommand::prompt() const {
    std::string value;
    switch (m_variable) {
        case Variable::GridMode:
            value = m_grid.gridOn ? "1" : "0";
            break;
        case Variable::SnapMode:
            value = m_grid.snapOn ? "1" : "0";
            break;
        case Variable::GridUnit:
            value = formatPoint(m_grid.gridUnit);
            break;
        case Variable::SnapUnit:
            value = formatPoint(m_grid.snapUnit);
            break;
        case Variable::SnapBase:
            value = formatPoint(m_grid.snapBase);
            break;
        case Variable::GridDisplay:
            value = std::to_string(m_grid.gridDisplay);
            break;
        case Variable::GridMajor:
            value = std::to_string(m_grid.gridMajor);
            break;
    }
    return textPrompt("Enter new value for " + std::string{name(m_variable)} + " <" + value + ">:");
}

CommandDescription GridVariableCommand::description(Variable variable) {
    const std::string variableName{name(variable)};
    ValueType type = ValueType::Text;  // the points, x,y
    if (variable == Variable::GridMode || variable == Variable::SnapMode ||
        variable == Variable::GridDisplay || variable == Variable::GridMajor) {
        type = ValueType::Integer;
    }
    return {variableName,
            {},
            "Sets " + variableName + " of the drawing.",
            false,
            {{"value", "Enter new value for " + variableName + " <{default}>:", type, {}, true}}};
}

}  // namespace tg::cmd
