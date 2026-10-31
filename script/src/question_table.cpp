// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"

#include "tg/core/arc_geometry.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <map>
#include <utility>

namespace tg::script::detail {

std::optional<core::EntityId> QuestionModel::find(std::string_view name) const {
    assert(ids.size() == names.size());
    const auto found = std::ranges::find(names, name);
    if (found == names.end()) {
        return std::nullopt;
    }
    return ids[static_cast<std::size_t>(found - names.begin())];
}

std::string_view QuestionModel::nameOf(core::EntityId id) const {
    const auto found = std::ranges::find(ids, id);
    assert(found != ids.end() && "every entity of the document has a name");
    return names[static_cast<std::size_t>(found - ids.begin())];
}

namespace {

using Arguments = std::span<const std::string_view>;

template <typename T>
using Parsed = std::variant<T, ArgumentError>;

// ---------------------------------------------------------------------------
// Arguments

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

ArgumentError usage(std::string_view form) {
    return ArgumentError{"wrong arguments, expected: " + std::string{form}};
}

Parsed<const core::Entity*> entityArgument(const QuestionModel& model, std::string_view name) {
    const std::optional<core::EntityId> id = model.find(name);
    if (!id) {
        return ArgumentError{"no entity named " + quoted(name) + " is defined above"};
    }
    const core::EntityRecord* record = model.document.find(*id);
    assert(record != nullptr && "every named entity is in the document");
    return &record->entity;
}

Parsed<core::Vec2> pointArgument(std::string_view text) {
    const std::optional<core::Vec2> point = parsePoint(text);
    if (!point) {
        return ArgumentError{"expected a point x,y, found " + quoted(text)};
    }
    return *point;
}

Parsed<core::BBox> rectangleArgument(std::string_view first, std::string_view second) {
    const std::optional<core::Vec2> a = parsePoint(first);
    const std::optional<core::Vec2> b = parsePoint(second);
    if (!a || !b) {
        return ArgumentError{"expected a rectangle as two corners x1,y1 x2,y2"};
    }
    return core::BBox{*a, *b};
}

/// The arguments "key=value" after the positional ones. Every key must be
/// one of `allowed`, at most once. A key of `announced` is one that the
/// catalogue describes and the program does not have yet: the case is not
/// passed, it is not invalid. `announced` is read only for a key that is not
/// in `allowed`, so a key left in both when its package realizes it does
/// nothing; the package moves it to `allowed` and can remove it from
/// `announced`.
Parsed<std::map<std::string_view, std::string_view>> keyedArguments(
    Arguments arguments, std::span<const std::string_view> allowed,
    std::span<const std::string_view> announced = {}) {
    std::map<std::string_view, std::string_view> keyed;
    for (const std::string_view argument : arguments) {
        const std::size_t equals = argument.find('=');
        const std::string_view key = argument.substr(0, equals);
        const bool hasValue = equals != std::string_view::npos;
        const bool known = hasValue && std::ranges::find(allowed, key) != allowed.end();
        if (hasValue && !known && std::ranges::find(announced, key) != announced.end()) {
            return ArgumentError{"the argument " + quoted(key) + " is not implemented yet", true};
        }
        if (!known || !keyed.emplace(key, argument.substr(equals + 1)).second) {
            return ArgumentError{"unexpected argument " + quoted(argument)};
        }
    }
    return keyed;
}

Parsed<double> nonNegative(std::string_view key, std::string_view text) {
    const std::optional<double> value = parseNumber(text);
    if (!value || *value < 0.0) {
        return ArgumentError{std::string{key} + " needs a number not below 0"};
    }
    return *value;
}

/// The modes of a question without `modes=`: the four of stage 0.4
/// (docs/verifica/domande.md), not the defaults of core::SnapModes, so that
/// the questions written before stage 0.7 do not change (docs/adr/0033,
/// point 6).
core::SnapModes stageFourModes() {
    core::SnapModes modes;
    modes.intersection = false;
    modes.perpendicular = false;
    modes.tangent = false;
    modes.quadrant = false;
    return modes;
}

/// "modes=endpoint,center", or "modes=none" for none; the four modes of
/// stage 0.4 when the argument is absent.
Parsed<core::SnapModes> modesArgument(std::optional<std::string_view> text) {
    if (!text) {
        return stageFourModes();
    }
    core::SnapModes modes;
    for (const SnapModeName& mode : snapModeNames()) {
        modes.*mode.member = false;
    }
    if (*text == "none") {
        return modes;
    }
    std::string_view rest = *text;
    while (true) {
        const std::size_t comma = rest.find(',');
        const std::string_view name = rest.substr(0, comma);
        const auto found = std::ranges::find(snapModeNames(), name, &SnapModeName::name);
        if (found == snapModeNames().end()) {
            return ArgumentError{"unknown snap mode " + quoted(name)};
        }
        modes.*found->member = true;
        if (comma == std::string_view::npos) {
            return modes;
        }
        rest.remove_prefix(comma + 1);
    }
}

// ---------------------------------------------------------------------------
// Answers

std::string yesNo(bool value) {
    return value ? "yes" : "no";
}

std::string snapText(const core::SnapPoint& snap) {
    return std::string{snapKindName(snap.kind)} + " " + formatPoint(snap.point);
}

std::string namesText(const QuestionModel& model, std::span<const core::EntityId> ids) {
    if (ids.empty()) {
        return "none";
    }
    std::string text;
    for (const core::EntityId id : ids) {
        if (!text.empty()) {
            text += ' ';
        }
        text += model.nameOf(id);
    }
    return text;
}

// Geometry that core answers for one entity type at a time: each new type
// of core::Entity is handled here, and the compiler checks it (ADR 0004).

/// Builds one visitor out of several lambdas, one per entity type.
template <typename... Lambdas>
struct Overloaded : Lambdas... {
    using Lambdas::operator()...;
};

// Explicit deduction guide: aggregate CTAD is missing from Apple Clang 15.
template <typename... Lambdas>
Overloaded(Lambdas...) -> Overloaded<Lambdas...>;

/// The length of the entity's curve; std::nullopt for a text, which is a
/// region (docs/verifica/domande.md).
std::optional<double> lengthOf(const core::Entity& entity) {
    return std::visit(
        Overloaded{
            [](const core::Text& /*text*/) -> std::optional<double> { return std::nullopt; },
            [](const auto& curve) -> std::optional<double> { return curve.length(); },
        },
        entity);
}

std::optional<core::Vec2> closestOnPolyline(const core::Polyline& polyline,
                                            const core::Vec2& point) {
    if (polyline.vertexCount() == 0) {
        return std::nullopt;
    }
    if (polyline.vertexCount() == 1) {
        return polyline.vertices().front().position;
    }
    // On equal distance the earliest segment wins, as in core's queries.
    std::optional<core::Vec2> best;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        const core::Vec2 candidate = core::closestPoint(polyline.segment(i), point);
        const double candidateDistance = core::distance(candidate, point);
        if (!best || candidateDistance < bestDistance) {
            best = candidate;
            bestDistance = candidateDistance;
        }
    }
    return best;
}

std::optional<core::Vec2> closestOn(const core::Entity& entity, const core::Vec2& point) {
    return std::visit(
        Overloaded{
            [&point](const core::Polyline& polyline) { return closestOnPolyline(polyline, point); },
            [&point](const core::Line& line) -> std::optional<core::Vec2> {
                return core::closestPoint(core::PolylineSegment{line.start, line.end}, point);
            },
            [&point](const core::Circle& circle) -> std::optional<core::Vec2> {
                return core::closestPoint(circle.geometry(), point);
            },
            [&point](const core::Arc& arc) -> std::optional<core::Vec2> {
                return core::closestPoint(arc.geometry(), point);
            },
            [&point](const core::Text& text) -> std::optional<core::Vec2> {
                return core::closestPoint(text, point);
            },
        },
        entity);
}

// ---------------------------------------------------------------------------
// Questions about one object

/// Reads "<entity>" followed by `count` more arguments.
Parsed<const core::Entity*> objectArguments(const QuestionModel& model, Arguments arguments,
                                            std::size_t count, std::string_view form) {
    if (arguments.size() != count + 1) {
        return usage(form);
    }
    return entityArgument(model, arguments[0]);
}

QuestionResult askBoundingBox(const QuestionModel& model, Arguments arguments) {
    const Parsed<const core::Entity*> entity =
        objectArguments(model, arguments, 0, "bbox <entity>");
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const core::BBox box = core::boundingBox(*std::get<const core::Entity*>(entity));
    if (box.isEmpty()) {
        return std::string{"empty"};
    }
    return formatPoint(box.min()) + " " + formatPoint(box.max());
}

QuestionResult askLength(const QuestionModel& model, Arguments arguments) {
    const Parsed<const core::Entity*> entity =
        objectArguments(model, arguments, 0, "length <entity>");
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const std::optional<double> length = lengthOf(*std::get<const core::Entity*>(entity));
    return length ? formatNumber(*length) : std::string{"none"};
}

/// The entity and the point of "<question> <entity> x,y".
Parsed<std::pair<const core::Entity*, core::Vec2>> entityAndPoint(const QuestionModel& model,
                                                                  Arguments arguments,
                                                                  std::string_view form) {
    const Parsed<const core::Entity*> entity = objectArguments(model, arguments, 1, form);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const Parsed<core::Vec2> point = pointArgument(arguments[1]);
    if (const auto* error = std::get_if<ArgumentError>(&point)) {
        return *error;
    }
    return std::pair{std::get<const core::Entity*>(entity), std::get<core::Vec2>(point)};
}

QuestionResult askDistance(const QuestionModel& model, Arguments arguments) {
    const auto parsed = entityAndPoint(model, arguments, "distance <entity> x,y");
    if (const auto* error = std::get_if<ArgumentError>(&parsed)) {
        return *error;
    }
    const auto& [entity, point] = std::get<0>(parsed);
    return formatNumber(core::distance(*entity, point));
}

QuestionResult askClosest(const QuestionModel& model, Arguments arguments) {
    const auto parsed = entityAndPoint(model, arguments, "closest <entity> x,y");
    if (const auto* error = std::get_if<ArgumentError>(&parsed)) {
        return *error;
    }
    const auto& [entity, point] = std::get<0>(parsed);
    const std::optional<core::Vec2> closest = closestOn(*entity, point);
    return closest ? formatPoint(*closest) : std::string{"none"};
}

/// "<question> <entity> x1,y1 x2,y2", answered by `test`.
template <typename Test>
QuestionResult askRectangle(const QuestionModel& model, Arguments arguments, std::string_view form,
                            const Test& test) {
    const Parsed<const core::Entity*> entity = objectArguments(model, arguments, 2, form);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const Parsed<core::BBox> rectangle = rectangleArgument(arguments[1], arguments[2]);
    if (const auto* error = std::get_if<ArgumentError>(&rectangle)) {
        return *error;
    }
    return yesNo(test(*std::get<const core::Entity*>(entity), std::get<core::BBox>(rectangle)));
}

QuestionResult askInside(const QuestionModel& model, Arguments arguments) {
    return askRectangle(model, arguments, "inside <entity> x1,y1 x2,y2",
                        [](const core::Entity& entity, const core::BBox& rectangle) {
                            return core::isInside(entity, rectangle);
                        });
}

QuestionResult askCrosses(const QuestionModel& model, Arguments arguments) {
    return askRectangle(model, arguments, "crosses <entity> x1,y1 x2,y2",
                        [](const core::Entity& entity, const core::BBox& rectangle) {
                            return core::intersects(entity, rectangle);
                        });
}

QuestionResult askSnaps(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "snaps <entity> [modes=<mode>,...]";
    if (arguments.empty()) {
        return usage(kForm);
    }
    const Parsed<const core::Entity*> entity = entityArgument(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    constexpr std::array<std::string_view, 1> kKeys{"modes"};
    const auto keyed = keyedArguments(arguments.subspan(1), kKeys);
    if (const auto* error = std::get_if<ArgumentError>(&keyed)) {
        return *error;
    }
    const auto& values = std::get<0>(keyed);
    const auto modesText = values.find("modes");
    const Parsed<core::SnapModes> modes =
        modesArgument(modesText == values.end() ? std::nullopt : std::optional{modesText->second});
    if (const auto* error = std::get_if<ArgumentError>(&modes)) {
        return *error;
    }
    std::vector<core::SnapPoint> points;
    core::appendSnapPoints(*std::get<const core::Entity*>(entity), std::get<core::SnapModes>(modes),
                           points);
    if (points.empty()) {
        return std::string{"none"};
    }
    std::string text;
    for (const core::SnapPoint& point : points) {
        if (!text.empty()) {
            text += "; ";
        }
        text += snapText(point);
    }
    return text;
}

// ---------------------------------------------------------------------------
// Questions about the document

QuestionResult askPick(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "pick x,y tolerance=<t>";
    if (arguments.size() != 2) {
        return usage(kForm);
    }
    const Parsed<core::Vec2> point = pointArgument(arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&point)) {
        return *error;
    }
    constexpr std::array<std::string_view, 1> kKeys{"tolerance"};
    const auto keyed = keyedArguments(arguments.subspan(1), kKeys);
    if (const auto* error = std::get_if<ArgumentError>(&keyed)) {
        return *error;
    }
    const auto& values = std::get<0>(keyed);
    const auto toleranceText = values.find("tolerance");
    if (toleranceText == values.end()) {
        return usage(kForm);
    }
    const Parsed<double> tolerance = nonNegative("tolerance", toleranceText->second);
    if (const auto* error = std::get_if<ArgumentError>(&tolerance)) {
        return *error;
    }
    const std::optional<core::EntityId> picked =
        core::pickEntity(model.document, std::get<core::Vec2>(point), std::get<double>(tolerance));
    return picked ? std::string{model.nameOf(*picked)} : std::string{"none"};
}

QuestionResult askSelection(const QuestionModel& model, Arguments arguments,
                            core::RectangleMode mode, std::string_view form) {
    if (arguments.size() != 2) {
        return usage(form);
    }
    const Parsed<core::BBox> rectangle = rectangleArgument(arguments[0], arguments[1]);
    if (const auto* error = std::get_if<ArgumentError>(&rectangle)) {
        return *error;
    }
    const std::vector<core::EntityId> selected =
        core::selectInRectangle(model.document, std::get<core::BBox>(rectangle), mode);
    return namesText(model, selected);
}

QuestionResult askWindow(const QuestionModel& model, Arguments arguments) {
    return askSelection(model, arguments, core::RectangleMode::Window, "window x1,y1 x2,y2");
}

QuestionResult askCrossing(const QuestionModel& model, Arguments arguments) {
    return askSelection(model, arguments, core::RectangleMode::Crossing, "crossing x1,y1 x2,y2");
}

QuestionResult askSnap(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "snap x,y aperture=<a> [modes=<mode>,...] [from=x,y]";
    if (arguments.size() < 2) {
        return usage(kForm);
    }
    const Parsed<core::Vec2> pointer = pointArgument(arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&pointer)) {
        return *error;
    }
    constexpr std::array<std::string_view, 3> kKeys{"aperture", "modes", "from"};
    const auto keyed = keyedArguments(arguments.subspan(1), kKeys);
    if (const auto* error = std::get_if<ArgumentError>(&keyed)) {
        return *error;
    }
    const auto& values = std::get<0>(keyed);
    const auto apertureText = values.find("aperture");
    if (apertureText == values.end()) {
        return usage(kForm);
    }
    const Parsed<double> aperture = nonNegative("aperture", apertureText->second);
    if (const auto* error = std::get_if<ArgumentError>(&aperture)) {
        return *error;
    }
    const auto modesText = values.find("modes");
    const Parsed<core::SnapModes> modes =
        modesArgument(modesText == values.end() ? std::nullopt : std::optional{modesText->second});
    if (const auto* error = std::get_if<ArgumentError>(&modes)) {
        return *error;
    }
    // The base point of the running command, from which perpendicular starts.
    std::optional<core::Vec2> from;
    if (const auto fromText = values.find("from"); fromText != values.end()) {
        const Parsed<core::Vec2> base = pointArgument(fromText->second);
        if (const auto* error = std::get_if<ArgumentError>(&base)) {
            return *error;
        }
        from = std::get<core::Vec2>(base);
    }
    const std::optional<core::SnapPoint> snap =
        core::findSnap(model.document, std::get<core::Vec2>(pointer), std::get<double>(aperture),
                       std::get<core::SnapModes>(modes), from);
    return snap ? snapText(*snap) : std::string{"none"};
}

}  // namespace

// Defined in construction_questions.cpp.
QuestionResult askCircleThrough(const QuestionModel& model, Arguments arguments);
QuestionResult askArcThrough(const QuestionModel& model, Arguments arguments);

QuestionResult askFontMetrics(const QuestionModel& model, Arguments arguments);
QuestionResult askTextWidth(const QuestionModel& model, Arguments arguments);
QuestionResult askBox(const QuestionModel& model, Arguments arguments);

// Defined in file_questions.cpp.
QuestionResult askEntity(const QuestionModel& model, Arguments arguments);
QuestionResult askCount(const QuestionModel& model, Arguments arguments);
QuestionResult askUnhandled(const QuestionModel& model, Arguments arguments);
QuestionResult askVersion(const QuestionModel& model, Arguments arguments);
QuestionResult askUnits(const QuestionModel& model, Arguments arguments);
QuestionResult askWarnings(const QuestionModel& model, Arguments arguments);
QuestionResult askRoundtrip(const QuestionModel& model, Arguments arguments);
QuestionResult askOpenError(const QuestionModel& model, Arguments arguments);

// Defined in modify_questions.cpp.
QuestionResult askTransform(const QuestionModel& model, Arguments arguments);

// Defined in property_questions.cpp.
QuestionResult askQuickSelect(const QuestionModel& model, Arguments arguments);
QuestionResult askAci(const QuestionModel& model, Arguments arguments);
QuestionResult askEffective(const QuestionModel& model, Arguments arguments);
QuestionResult askDashes(const QuestionModel& model, Arguments arguments);
QuestionResult askLayers(const QuestionModel& model, Arguments arguments);
QuestionResult askLinetypes(const QuestionModel& model, Arguments arguments);

namespace {

// ---------------------------------------------------------------------------
// The catalogue: one row per question, in the order of docs/verifica/domande.md.

constexpr std::array kQuestions{
    // About one object.
    QuestionRow{"bbox", AnswerShape::Single, &askBoundingBox},
    QuestionRow{"length", AnswerShape::Single, &askLength},
    QuestionRow{"distance", AnswerShape::Single, &askDistance},
    QuestionRow{"closest", AnswerShape::Single, &askClosest},
    QuestionRow{"inside", AnswerShape::Single, &askInside},
    QuestionRow{"crosses", AnswerShape::Single, &askCrosses},
    QuestionRow{"snaps", AnswerShape::Snaps, &askSnaps},
    // About the document.
    QuestionRow{"pick", AnswerShape::Single, &askPick},
    QuestionRow{"window", AnswerShape::Names, &askWindow},
    QuestionRow{"crossing", AnswerShape::Names, &askCrossing},
    QuestionRow{"snap", AnswerShape::Single, &askSnap},
    // Constructions (construction_questions.cpp).
    QuestionRow{"circle-through", AnswerShape::Single, &askCircleThrough},
    QuestionRow{"arc-through", AnswerShape::Single, &askArcThrough},
    // Text: packages P11 and P13.
    QuestionRow{"text-width", AnswerShape::Single, &askTextWidth},
    QuestionRow{"font-metrics", AnswerShape::Single, &askFontMetrics},
    QuestionRow{"box", AnswerShape::Single, &askBox},
    // Files: package P8 of milestone 0.5 (file_questions.cpp).
    QuestionRow{"entity", AnswerShape::Single, &askEntity},
    QuestionRow{"count", AnswerShape::Items, &askCount},
    QuestionRow{"unhandled", AnswerShape::Items, &askUnhandled},
    QuestionRow{"version", AnswerShape::Single, &askVersion},
    QuestionRow{"units", AnswerShape::Single, &askUnits},
    QuestionRow{"warnings", AnswerShape::Items, &askWarnings},
    QuestionRow{"roundtrip", AnswerShape::Single, &askRoundtrip},
    QuestionRow{"open-error", AnswerShape::Single, &askOpenError},
    // Layers and properties: stage 0.6 (property_questions.cpp).
    QuestionRow{"aci", AnswerShape::Single, &askAci},              // P9
    QuestionRow{"effective", AnswerShape::Single, &askEffective},  // P9
    QuestionRow{"dashes", AnswerShape::Items, &askDashes},         // P11

    // P23.
    QuestionRow{"quickselect", AnswerShape::Names, &askQuickSelect},
    QuestionRow{"layers", AnswerShape::Items, &askLayers},
    QuestionRow{"linetypes", AnswerShape::Items, &askLinetypes},
    // Stage 0.7: announced questions must fail as unimplemented, not as invalid
    // syntax, so their independent references can precede the work packages.
    QuestionRow{"transform", AnswerShape::Single, &askTransform},    // P4
    QuestionRow{"grips", AnswerShape::Items, nullptr},               // P9
    QuestionRow{"grip", AnswerShape::Single, nullptr},               // P9
    QuestionRow{"intersections", AnswerShape::Items, nullptr},       // P14
    QuestionRow{"self-intersections", AnswerShape::Items, nullptr},  // P14
    QuestionRow{"side", AnswerShape::Single, nullptr},               // P14
    QuestionRow{"coincident", AnswerShape::Single, nullptr},         // P14
    QuestionRow{"degenerate", AnswerShape::Single, nullptr},         // P14
    QuestionRow{"offset", AnswerShape::Single, nullptr},             // P17
    QuestionRow{"fillet", AnswerShape::Single, nullptr},             // P19
    QuestionRow{"tangents", AnswerShape::Items, nullptr},            // P19
    QuestionRow{"circle-ttr", AnswerShape::Single, nullptr},         // P19
    QuestionRow{"chamfer", AnswerShape::Single, nullptr},            // P21
    QuestionRow{"join", AnswerShape::Single, nullptr},               // P22
};

}  // namespace

const QuestionRow* findQuestion(std::string_view name) {
    // A loop, not std::ranges::find: the iterator of std::array is a pointer
    // in some standard libraries and a class in others (MSVC).
    for (const QuestionRow& row : kQuestions) {
        if (row.name == name) {
            return &row;
        }
    }
    return nullptr;
}

}  // namespace tg::script::detail
