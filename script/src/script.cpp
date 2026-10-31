// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/script.hpp"

#include "text.hpp"
#include "tg/script/number_format.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>

namespace tg::script {
namespace {

// The size of a fixed view, in pixels: from one pixel to a size no screen
// has, so that a typing error ("80000x600") is caught.
constexpr int kMaxViewSide = 16384;

// Image tolerances (docs/adr/0011, point 6): a channel goes from 0 to 255,
// the share of different pixels is a percentage.
constexpr int kMaxChannel = 255;
constexpr double kMaxPercent = 100.0;

using Words = std::vector<std::string_view>;

// The directives that the catalogue describes from stage 0.7 (docs/verifica/
// script.md) and the program does not have yet. This list is read only after
// every directive the program has (parseDirective), so an entry left here
// when its package realizes it (P3 `scr`, P8 `polar`) does nothing; remove
// it to keep the list short.
constexpr std::array<std::string_view, 1> kAnnouncedDirectives{"polar"};

/// An abbreviation of AutoCAD for a snap mode, and the member of
/// core::SnapModes that it turns on (docs/adr/0033, point 7).
struct OsnapAbbreviation {
    std::string_view name;
    bool core::SnapModes::*member;
};

constexpr std::array kOsnapAbbreviations{OsnapAbbreviation{"end", &core::SnapModes::endpoint},
                                         OsnapAbbreviation{"mid", &core::SnapModes::midpoint},
                                         OsnapAbbreviation{"cen", &core::SnapModes::center},
                                         OsnapAbbreviation{"ins", &core::SnapModes::insertion},
                                         OsnapAbbreviation{"int", &core::SnapModes::intersection},
                                         OsnapAbbreviation{"per", &core::SnapModes::perpendicular},
                                         OsnapAbbreviation{"tan", &core::SnapModes::tangent},
                                         OsnapAbbreviation{"qua", &core::SnapModes::quadrant}};

// A new member of core::SnapModes needs its abbreviation above, or !osnap
// could never turn it on.
static_assert(sizeof(core::SnapModes) == kOsnapAbbreviations.size() * sizeof(bool),
              "every snap mode needs an abbreviation in kOsnapAbbreviations");

/// A directive of the catalogue that the program does not have yet: a script
/// that uses it is not passed, it is not invalid.
struct AnnouncedDirective {
    std::string message;
};

/// A directive read from its words, or the error message.
using DirectiveResult = std::variant<Action, std::string, AnnouncedDirective>;

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

std::optional<int> parseCount(std::string_view text, int maxValue) {
    if (text.empty() || text.size() > 6 ||
        !std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; })) {
        return std::nullopt;
    }
    int value = 0;
    for (const char c : text) {
        value = (value * 10) + (c - '0');
    }
    if (value > maxValue) {
        return std::nullopt;
    }
    return value;
}

std::optional<bool> parseOnOff(std::string_view text) {
    if (text == "on") {
        return true;
    }
    if (text == "off") {
        return false;
    }
    return std::nullopt;
}

bool isName(std::string_view text) {
    return !text.empty() && std::ranges::all_of(text, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '-';
    });
}

std::string wrongArguments(std::string_view directive, std::string_view usage) {
    return "wrong arguments for !" + std::string{directive} + ", expected: " + std::string{usage};
}

DirectiveResult parseView(const Words& arguments) {
    constexpr std::string_view kUsage = "!view cx,cy scale WxH";
    if (arguments.size() != 3) {
        return wrongArguments("view", kUsage);
    }
    const std::optional<core::Vec2> center = parsePoint(arguments[0]);
    const std::optional<double> scale = parseNumber(arguments[1]);
    const std::size_t times = arguments[2].find('x');
    if (!center || !scale || !(*scale > 0.0) || times == std::string_view::npos) {
        return wrongArguments("view", kUsage);
    }
    const std::optional<int> width = parseCount(arguments[2].substr(0, times), kMaxViewSide);
    const std::optional<int> height = parseCount(arguments[2].substr(times + 1), kMaxViewSide);
    if (!width || !height || *width == 0 || *height == 0) {
        return wrongArguments("view", kUsage);
    }
    return Action{ViewDirective{*center, *scale, *width, *height}};
}

DirectiveResult parsePointDirective(std::string_view name, const Words& arguments) {
    const bool click = name == "click";
    const std::string_view usage = click ? "!click x,y [remove]" : "!move x,y";
    const bool remove = click && arguments.size() == 2 && arguments[1] == "remove";
    if (arguments.empty() || arguments.size() > (remove ? 2U : 1U)) {
        return wrongArguments(name, usage);
    }
    const std::optional<core::Vec2> point = parsePoint(arguments[0]);
    if (!point) {
        return wrongArguments(name, usage);
    }
    if (click) {
        return Action{ClickDirective{*point, remove}};
    }
    return Action{MoveDirective{*point}};
}

/// "!osnap end,mid", or "!osnap none": the modes written are on, the others
/// off.
DirectiveResult parseOsnap(const Words& arguments) {
    constexpr std::string_view kUsage = "!osnap end,mid,cen,ins,int,per,tan,qua|none";
    if (arguments.size() != 1) {
        return wrongArguments("osnap", kUsage);
    }
    core::SnapModes modes;
    for (const OsnapAbbreviation& abbreviation : kOsnapAbbreviations) {
        modes.*abbreviation.member = false;
    }
    if (arguments[0] == "none") {
        return Action{OsnapDirective{modes}};
    }
    std::string_view rest = arguments[0];
    while (true) {
        const std::size_t comma = rest.find(',');
        const std::string_view abbreviation = rest.substr(0, comma);
        // A loop, not std::ranges::find: the iterator of std::array is a
        // pointer with some standard libraries only.
        const OsnapAbbreviation* found = nullptr;
        for (const OsnapAbbreviation& candidate : kOsnapAbbreviations) {
            if (candidate.name == abbreviation) {
                found = &candidate;
                break;
            }
        }
        if (found == nullptr) {
            return "unknown snap mode " + quoted(abbreviation) +
                   " in !osnap, expected: end, mid, cen, ins, int, per, tan, qua or none";
        }
        modes.*found->member = true;
        if (comma == std::string_view::npos) {
            return Action{OsnapDirective{modes}};
        }
        rest.remove_prefix(comma + 1);
    }
}

DirectiveResult parseDirective(std::string_view name, const Words& arguments) {
    if (name == "view") {
        return parseView(arguments);
    }
    if (name == "move" || name == "click") {
        return parsePointDirective(name, arguments);
    }
    if (name == "rightclick" || name == "esc" || name == "report") {
        if (!arguments.empty()) {
            return "!" + std::string{name} + " takes no arguments";
        }
        if (name == "rightclick") {
            return Action{RightClickDirective{}};
        }
        return name == "esc" ? Action{EscDirective{}} : Action{ReportDirective{}};
    }
    if (name == "snap" || name == "ortho" || name == "scr") {
        const std::optional<bool> on =
            arguments.size() == 1 ? parseOnOff(arguments[0]) : std::nullopt;
        if (!on) {
            return wrongArguments(name, "!" + std::string{name} + " on|off");
        }
        if (name == "scr") {
            return Action{ScriptSequencesDirective{*on}};
        }
        const auto aid = name == "snap" ? AidDirective::Aid::Snap : AidDirective::Aid::Ortho;
        return Action{AidDirective{aid, *on}};
    }
    if (name == "osnap") {
        return parseOsnap(arguments);
    }
    if (name == "key") {
        if (arguments.size() != 1) {
            return wrongArguments(name, "!key <key>");
        }
        return Action{KeyDirective{std::string{arguments[0]}}};
    }
    if (name == "screenshot") {
        if (arguments.size() != 1 || !isName(arguments[0])) {
            return wrongArguments(name, "!screenshot <name>, a name of letters, digits, _ and -");
        }
        return Action{ScreenshotDirective{std::string{arguments[0]}}};
    }
    if (name == "level") {
        const std::optional<int> level =
            arguments.size() == 1 ? parseCount(arguments[0], 2) : std::nullopt;
        if (!level || *level == 0) {
            return wrongArguments(name, "!level 1|2");
        }
        return Action{LevelDirective{*level}};
    }
    if (name == "tolerance") {
        std::variant<ToleranceDirective, std::string> tolerance = parseTolerance(arguments);
        if (auto* error = std::get_if<std::string>(&tolerance)) {
            return std::move(*error);
        }
        return Action{std::get<ToleranceDirective>(tolerance)};
    }
    if (std::ranges::find(kAnnouncedDirectives, name) != kAnnouncedDirectives.end()) {
        return AnnouncedDirective{"the directive " + quoted("!" + std::string{name}) +
                                  " is not implemented yet"};
    }
    return "unknown directive " + quoted("!" + std::string{name});
}

bool needsLevel2(const Action& action) {
    return std::holds_alternative<KeyDirective>(action) ||
           std::holds_alternative<ScreenshotDirective>(action);
}

}  // namespace

std::variant<ToleranceDirective, std::string> parseTolerance(const Words& arguments) {
    constexpr std::string_view kUsage = "!tolerance absolute|relative|channel|pixels <value>";
    if (arguments.size() != 2) {
        return wrongArguments("tolerance", kUsage);
    }
    const std::string_view kind = arguments[0];
    if (kind == "channel") {
        const std::optional<int> channel = parseCount(arguments[1], kMaxChannel);
        if (!channel) {
            return "!tolerance channel needs an integer from 0 to 255";
        }
        return ToleranceDirective{ToleranceDirective::Kind::Channel, static_cast<double>(*channel)};
    }
    const std::optional<double> value = parseNumber(arguments[1]);
    if (!value || *value < 0.0) {
        return "!tolerance needs a number not below 0";
    }
    if (kind == "absolute") {
        return ToleranceDirective{ToleranceDirective::Kind::Absolute, *value};
    }
    if (kind == "relative") {
        return ToleranceDirective{ToleranceDirective::Kind::Relative, *value};
    }
    if (kind == "pixels") {
        if (*value > kMaxPercent) {
            return "!tolerance pixels is a percentage, from 0 to 100";
        }
        return ToleranceDirective{ToleranceDirective::Kind::Pixels, *value};
    }
    return wrongArguments("tolerance", kUsage);
}

void applyTolerance(const ToleranceDirective& directive, Tolerances& tolerances) {
    switch (directive.kind) {
        case ToleranceDirective::Kind::Absolute:
            tolerances.absolute = directive.value;
            break;
        case ToleranceDirective::Kind::Relative:
            tolerances.relative = directive.value;
            break;
        case ToleranceDirective::Kind::Channel:
            tolerances.channel = static_cast<int>(directive.value);
            break;
        case ToleranceDirective::Kind::Pixels:
            tolerances.pixels = directive.value;
            break;
    }
}

std::variant<Script, ParseError> parseScript(std::string_view text, std::string_view sourceName) {
    Script script;
    const std::size_t dot = sourceName.find_last_of('.');
    const std::string_view extension =
        dot == std::string_view::npos ? std::string_view{} : sourceName.substr(dot);
    if (extension.size() == 4 && extension[0] == '.' &&
        (extension[1] == 's' || extension[1] == 'S') &&
        (extension[2] == 'c' || extension[2] == 'C') &&
        (extension[3] == 'r' || extension[3] == 'R')) {
        script.actions.emplace_back(ScriptSequencesDirective{true});
    }
    std::optional<int> firstLevel2Line;
    std::optional<int> levelOneLine;
    const std::vector<std::string_view> lines = splitLines(text);
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const int lineNumber = static_cast<int>(index) + 1;
        const std::string_view line = lines[index];
        if (line.starts_with(';')) {
            continue;
        }
        if (!line.starts_with('!')) {
            script.actions.emplace_back(TypedLine{std::string{line}});
            continue;
        }
        const Words words = detail::splitWords(line.substr(1));
        if (words.empty() || line.size() < 2 || line[1] == ' ' || line[1] == '\t') {
            return ParseError{lineNumber, "a directive needs a name right after '!'"};
        }
        const Words arguments(words.begin() + 1, words.end());
        DirectiveResult result = parseDirective(words.front(), arguments);
        if (auto* error = std::get_if<std::string>(&result)) {
            return ParseError{lineNumber, std::move(*error)};
        }
        if (auto* announced = std::get_if<AnnouncedDirective>(&result)) {
            return ParseError{lineNumber, std::move(announced->message), true};
        }
        Action action = std::get<Action>(std::move(result));
        if (needsLevel2(action) && !firstLevel2Line) {
            firstLevel2Line = lineNumber;
        }
        if (const auto* level = std::get_if<LevelDirective>(&action)) {
            if (level->level == 2) {
                script.level = 2;
            } else if (!levelOneLine) {
                levelOneLine = lineNumber;
            }
        }
        if (const auto* tolerance = std::get_if<ToleranceDirective>(&action)) {
            applyTolerance(*tolerance, script.tolerances);
        }
        script.actions.push_back(std::move(action));
    }
    if (script.level == 2 && levelOneLine) {
        return ParseError{*levelOneLine, "the script asks for both !level 1 and !level 2"};
    }
    if (script.level == 1 && firstLevel2Line) {
        return ParseError{*firstLevel2Line,
                          "this directive needs level 2: add \"!level 2\" to the script"};
    }
    return script;
}

}  // namespace tg::script
