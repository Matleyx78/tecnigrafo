// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/pedit_command.hpp"

#include "../text.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

namespace tg::cmd {
namespace {

constexpr std::string_view kOpenOptions =
    "Enter an option [Close/Join/Ltype gen/Reverse/Undo]:";
constexpr std::string_view kClosedOptions =
    "Enter an option [Open/Join/Ltype gen/Reverse/Undo]:";
constexpr std::string_view kMultipleOptions =
    "Enter an option [Close/Open/Join/Ltype gen/Reverse/Undo]:";

std::vector<Keyword> optionKeywords(bool multiple, bool closed) {
    std::vector<Keyword> result;
    if (multiple || !closed) {
        result.push_back({"Close", "C"});
    }
    if (multiple || closed) {
        result.push_back({"Open", "O"});
    }
    result.insert(result.end(),
                  {{"Join", "J"}, {"Ltype gen", "L"}, {"Reverse", "R"}, {"Undo", "U"}});
    return result;
}

bool convertible(const core::Entity& entity) {
    return std::holds_alternative<core::Line>(entity) || std::holds_alternative<core::Arc>(entity);
}

bool editable(const core::Entity& entity) {
    return std::holds_alternative<core::Polyline>(entity) || convertible(entity);
}

std::string_view joinTypeName(core::JoinType type) {
    switch (type) {
        case core::JoinType::Extend:
            return "Extend";
        case core::JoinType::Add:
            return "Add";
        case core::JoinType::Both:
            return "Both";
    }
    return "Extend";
}

void reportJoinType(CommandContext& context, core::JoinType type) {
    std::string name{joinTypeName(type)};
    if (type == core::JoinType::Add) {
        name = "Add segment";
    } else if (type == core::JoinType::Both) {
        name = "Both (Extend or Add)";
    }
    context.message("Join Type = " + name);
}

// Elevation is kept as DXF group 38, outside the 2D geometry model. Compare
// the numeric property itself: PT33 requires equal elevations, not a planar
// coincidence test. Invalid retained values cannot safely take part in Join.
std::optional<double> elevation(const core::EntityRecord& record) {
    for (const auto& group : record.extras.placement) {
        if (group.code == 38) {
            auto text = detail::trim(group.value);
            if (text.starts_with('+')) {
                text.remove_prefix(1);
            }
            double value = 0.0;
            const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
            if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() ||
                !std::isfinite(value)) {
                return std::nullopt;
            }
            return value;
        }
    }
    return 0.0;
}

}  // namespace

CommandDescription PeditCommand::description() {
    return {"PEDIT",
            {"PE"},
            "Edits polylines and converts lines and arcs to polylines.",
            false,
            {{"source",
              "Select polyline or [Multiple]:",
              ValueType::Object,
              {{"Multiple", "M"}},
              true},
             {"selection", "Select objects:", ValueType::Selection, {}, true},
             {"conversion", "Do you want to turn it into one? <Y>", ValueType::Text, {}, true},
             {"multiple-conversion",
              "Convert Lines, Arcs and Splines to polylines [Yes/No]? <Y>",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true},
             {"open-options",
              std::string{kOpenOptions},
              ValueType::Keyword,
              optionKeywords(false, false),
              true},
             {"closed-options",
              std::string{kClosedOptions},
              ValueType::Keyword,
              optionKeywords(false, true),
              true},
             {"multiple-options",
              std::string{kMultipleOptions},
              ValueType::Keyword,
              optionKeywords(true, false),
              true},
             {"fuzz",
              "Enter fuzz distance or [Jointype] <0.0000>:",
              ValueType::Distance,
              {{"Jointype", "J"}},
              true},
             {"type",
              "Enter join type [Extend/Add/Both] <{default}>:",
              ValueType::Keyword,
              {{"Extend", "E"}, {"Add", "A"}, {"Both", "B"}},
              true},
             {"ltype",
              "Enter polyline linetype generation option [ON/OFF] <{default}>:",
              ValueType::Keyword,
              {{"ON", "ON"}, {"OFF", "OFF"}},
              true}}};
}

void PeditCommand::start(CommandContext& context) {
    m_stage = Stage::Source;
    m_multiple = false;
    m_closed = false;
    m_ltype = false;
    m_joinType = core::JoinType::Extend;
    m_ids.clear();
    m_others.clear();
    m_options.clear();
    context.editor.beginTransaction("Pedit");
}

Status PeditCommand::finish(CommandContext& context) {
    context.editor.commit();
    return Status::Finished;
}

void PeditCommand::cancel(CommandContext& context) {
    // Conversion and completed options survive Esc (docs/adr/0015).
    context.editor.commit();
}

void PeditCommand::updateOptions(const CommandContext& context) {
    m_stage = Stage::Options;
    if (!m_ids.empty()) {
        const auto* record = context.editor.document().find(m_ids.front());
        if (record != nullptr) {
            if (const auto* polyline = std::get_if<core::Polyline>(&record->entity)) {
                m_closed = polyline->isClosed();
                m_ltype = polyline->continuousLinetype();
            }
        }
    }
}

Status PeditCommand::selected(CommandContext& context) {
    const auto& document = context.editor.document();
    bool needsConversion = false;
    std::erase_if(m_ids, [&document, &needsConversion](core::EntityId id) {
        const auto* record = document.find(id);
        if (record == nullptr || !editable(record->entity)) {
            return true;
        }
        needsConversion = needsConversion || convertible(record->entity);
        return false;
    });
    if (m_ids.empty()) {
        return finish(context);
    }
    if (needsConversion && !context.defaults.peditAccept) {
        if (!m_multiple) {
            context.message("Object selected is not a polyline");
        }
        m_stage = Stage::Conversion;
        return Status::Continue;
    }
    return convert(context, true);
}

Status PeditCommand::convert(CommandContext& context, bool accept) {
    std::vector<core::EntityId> polylines;
    for (const auto id : m_ids) {
        const auto* record = context.editor.document().find(id);
        if (record == nullptr) {
            continue;
        }
        if (std::holds_alternative<core::Polyline>(record->entity)) {
            polylines.push_back(id);
        } else if (accept) {
            core::Polyline polyline;
            if (const auto* line = std::get_if<core::Line>(&record->entity)) {
                polyline = core::toPolyline(*line);
            } else {
                polyline = core::toPolyline(std::get<core::Arc>(record->entity));
            }
            if (const auto copy = context.editor.addCopy(id, std::move(polyline))) {
                context.editor.remove(id);
                polylines.push_back(*copy);
            }
        }
    }
    m_ids = std::move(polylines);
    if (m_ids.empty()) {
        return finish(context);
    }
    updateOptions(context);
    return Status::Continue;
}

void PeditCommand::remember(CommandContext& context) {
    m_options.push_back({context.editor.savepoint(), m_ids});
}

void PeditCommand::joinSelected(CommandContext& context, double fuzz) {
    remember(context);
    const auto& document = context.editor.document();
    const auto rule = core::ToleranceRule::of(document);
    const auto sources = m_ids;
    std::size_t added = 0;
    for (const auto source : sources) {
        const auto* record = document.find(source);
        if (record == nullptr) {
            continue;
        }
        const auto sourceElevation = elevation(*record);
        if (!sourceElevation) {
            continue;
        }
        const auto polyline = std::get<core::Polyline>(record->entity);
        std::vector<core::Entity> others;
        std::vector<core::EntityId> otherIds;
        others.reserve(m_others.size());
        otherIds.reserve(m_others.size());
        for (const auto id : m_others) {
            const auto* other = document.find(id);
            if (id == source || other == nullptr || !editable(other->entity)) {
                continue;
            }
            const auto otherElevation = elevation(*other);
            if (!otherElevation || otherElevation != sourceElevation) {
                continue;
            }
            others.push_back(other->entity);
            otherIds.push_back(id);
        }
        auto result = core::joinWithGaps(polyline, others, fuzz, m_joinType, rule);
        context.editor.replace(source, std::move(result.polyline));
        for (const auto index : result.consumed) {
            context.editor.remove(otherIds[index]);
        }
        added += result.added;
    }
    std::erase_if(m_ids, [&document](core::EntityId id) { return document.find(id) == nullptr; });
    context.message(std::to_string(added) + " segments added to polyline");
    updateOptions(context);
}

Status PeditCommand::handle(CommandContext& context, const Input& input) {
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);
    switch (m_stage) {
        case Stage::Source:
            if (keyword != nullptr && keyword->keyword == "Multiple") {
                m_multiple = true;
                m_stage = Stage::Selection;
            } else if (const auto* object = std::get_if<ObjectInput>(&input)) {
                const auto* record = context.editor.document().find(object->id);
                if (record != nullptr && editable(record->entity)) {
                    m_ids = {object->id};
                    return selected(context);
                }
                context.message("Object selected is not a polyline");
            } else if (confirmed) {
                return finish(context);
            }
            break;
        case Stage::Selection:
            if (const auto* selection = std::get_if<SelectionInput>(&input)) {
                m_ids = selection->ids;
                return selected(context);
            }
            break;
        case Stage::Conversion: {
            if (confirmed || (keyword != nullptr && keyword->keyword == "Yes")) {
                return convert(context, true);
            }
            if (keyword != nullptr && keyword->keyword == "No") {
                return convert(context, false);
            }
            // AutoCAD's single-object question has no bracketed keywords.
            if (const auto* text = std::get_if<TextInput>(&input)) {
                auto answer = detail::trim(text->text);
                if (answer.starts_with('_')) {
                    answer.remove_prefix(1);
                }
                if (matchesKeyword({"Yes", "Y"}, answer)) {
                    return convert(context, true);
                }
                if (matchesKeyword({"No", "N"}, answer)) {
                    return convert(context, false);
                }
                context.message("Invalid option keyword.");
            }
            break;
        }
        case Stage::Options:
            if (confirmed) {
                return finish(context);
            }
            if (keyword == nullptr) {
                break;
            }
            if (keyword->keyword == "Undo") {
                if (m_options.empty()) {
                    context.message("Nothing to undo.");
                } else {
                    auto option = std::move(m_options.back());
                    m_options.pop_back();
                    context.editor.rollbackTo(option.mark);
                    m_ids = std::move(option.ids);
                    updateOptions(context);
                }
            } else if (keyword->keyword == "Join") {
                if (m_multiple) {
                    m_others = m_ids;
                    reportJoinType(context, m_joinType);
                    m_stage = Stage::Fuzz;
                } else {
                    m_stage = Stage::Join;
                }
            } else if (keyword->keyword == "Ltype gen") {
                m_stage = Stage::Ltype;
            } else {
                remember(context);
                for (const auto id : m_ids) {
                    auto polyline =
                        std::get<core::Polyline>(context.editor.document().find(id)->entity);
                    if (keyword->keyword == "Reverse") {
                        polyline = core::reversed(polyline);
                    } else if (keyword->keyword == "Open") {
                        polyline.setClosed(false);
                    } else if (keyword->keyword == "Close") {
                        // Close adds a straight segment, even if the unused
                        // final vertex of an open polyline carries a bulge.
                        if (!polyline.isClosed() && polyline.vertexCount() != 0) {
                            std::vector<core::PolylineVertex> vertices(polyline.vertices().begin(),
                                                                      polyline.vertices().end());
                            vertices.back().bulge = 0.0;
                            core::Polyline closed{std::move(vertices), true};
                            closed.setConstantWidth(polyline.constantWidth());
                            closed.setContinuousLinetype(polyline.continuousLinetype());
                            polyline = std::move(closed);
                        }
                    }
                    context.editor.replace(id, std::move(polyline));
                }
                updateOptions(context);
            }
            break;
        case Stage::Join:
            if (const auto* selection = std::get_if<SelectionInput>(&input)) {
                m_others = selection->ids;
                joinSelected(context, 0.0);
            }
            break;
        case Stage::Fuzz:
            if (keyword != nullptr && keyword->keyword == "Jointype") {
                m_stage = Stage::Type;
            } else if (confirmed) {
                joinSelected(context, 0.0);
            } else if (const auto* value = std::get_if<ValueInput>(&input)) {
                if (!std::isfinite(value->value) || value->value < 0.0) {
                    context.message("Requires a non-negative distance.");
                } else {
                    joinSelected(context, value->value);
                }
            }
            break;
        case Stage::Type:
            if (keyword != nullptr || confirmed) {
                if (keyword != nullptr) {
                    if (keyword->keyword == "Extend") {
                        m_joinType = core::JoinType::Extend;
                    } else if (keyword->keyword == "Add") {
                        m_joinType = core::JoinType::Add;
                    } else if (keyword->keyword == "Both") {
                        m_joinType = core::JoinType::Both;
                    }
                }
                reportJoinType(context, m_joinType);
                m_stage = Stage::Fuzz;
            }
            break;
        case Stage::Ltype:
            if (keyword != nullptr || confirmed) {
                remember(context);
                const bool continuous = keyword == nullptr ? m_ltype : keyword->keyword == "ON";
                for (const auto id : m_ids) {
                    auto polyline =
                        std::get<core::Polyline>(context.editor.document().find(id)->entity);
                    polyline.setContinuousLinetype(continuous);
                    context.editor.replace(id, std::move(polyline));
                }
                updateOptions(context);
            }
            break;
    }
    return Status::Continue;
}

Prompt PeditCommand::prompt() const {
    switch (m_stage) {
        case Stage::Source:
            return {"Select polyline or [Multiple]:", {{"Multiple", "M"}}, PromptKind::Object};
        case Stage::Selection:
        case Stage::Join:
            return {"Select objects:", {}, PromptKind::Selection};
        case Stage::Conversion:
            if (m_multiple) {
                return {"Convert Lines, Arcs and Splines to polylines [Yes/No]? <Y>",
                        {{"Yes", "Y"}, {"No", "N"}}};
            }
            return {"Do you want to turn it into one? <Y>", {}, PromptKind::Text};
        case Stage::Options:
            return {std::string{m_multiple ? kMultipleOptions
                                           : (m_closed ? kClosedOptions : kOpenOptions)},
                    optionKeywords(m_multiple, m_closed)};
        case Stage::Fuzz:
            return {"Enter fuzz distance or [Jointype] <0.0000>:",
                    {{"Jointype", "J"}},
                    PromptKind::Distance};
        case Stage::Type:
            return {"Enter join type [Extend/Add/Both] <" +
                        std::string{joinTypeName(m_joinType)} + ">:",
                    {{"Extend", "E"}, {"Add", "A"}, {"Both", "B"}}};
        case Stage::Ltype:
            return {"Enter polyline linetype generation option [ON/OFF] <" +
                        std::string{m_ltype ? "On" : "Off"} + ">:",
                    {{"ON", "ON"}, {"OFF", "OFF"}}};
    }
    return {};
}

CommandDescription PeditacceptCommand::description() {
    return {"PEDITACCEPT",
            {},
            "Sets whether PEDIT converts lines and arcs without asking.",
            false,
            {{"value",
              "Enter new value for PEDITACCEPT <{default}>:",
              ValueType::Integer,
              {},
              true}}};
}

void PeditacceptCommand::start(CommandContext& context) {
    m_value = context.defaults.peditAccept;
}

Status PeditacceptCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        auto value = detail::trim(text->text);
        if (value.starts_with('+')) {
            value.remove_prefix(1);
        }
        int number = 0;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || number < 0 ||
            number > 1) {
            context.message("Requires an integer between 0 and 1.");
            return Status::Continue;
        }
        context.defaults.peditAccept = number == 1;
        return Status::Finished;
    }
    return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
}

Prompt PeditacceptCommand::prompt() const {
    return {"Enter new value for PEDITACCEPT <" + std::string{m_value ? "1" : "0"} + ">:",
            {},
            PromptKind::Text};
}

}  // namespace tg::cmd
