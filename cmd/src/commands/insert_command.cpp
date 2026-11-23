// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/insert_command.hpp"

#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/edit/blocks.hpp"
#include "tg/io/block_import.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr std::string_view kNonzero = "Value must be nonzero.";
constexpr std::string_view kExtension = ".dxf";

/// A path of a DXF file rather than a block name (PT21, point 2): it ends
/// with .dxf, in any case, or names a folder.
bool isPath(std::string_view text) {
    const bool dxf =
        text.size() >= kExtension.size() &&
        detail::equalsIgnoringCase(text.substr(text.size() - kExtension.size()), kExtension);
    return dxf || text.find_first_of("/\\") != std::string_view::npos;
}

/// The block a file gives: its name without folders and extension, as
/// written, as io::importBlock() names it.
std::string fileBlockName(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    std::string_view name = slash == std::string_view::npos ? path : path.substr(slash + 1);
    const auto dot = name.find_last_of('.');
    if (dot != std::string_view::npos && dot > 0) {
        name = name.substr(0, dot);
    }
    return std::string{name};
}

/// The ratio of the units of a definition to those of the drawing, as
/// io::importBlock() computes it for a file: 1 when either is 0 (no unit) or
/// outside the table of $INSUNITS, such as 25, or when they are equal.
double unitFactor(int from, int to) {
    const auto known = [](int units) {
        return units > 0 && units <= core::Document::kMaxUnits;
    };
    if (!known(from) || !known(to) || from == to) {
        return 1.0;
    }
    return core::unitInMeters(from) / core::unitInMeters(to);
}

/// Up to 4 decimals without trailing zeros: "0", "12.5".
std::string shortNumber(double value) {
    std::string text = formatLength(value);
    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') {
            text.pop_back();
        }
        if (text.back() == '.') {
            text.pop_back();
        }
    }
    return text == "-0" ? "0" : text;
}

/// The shortest text that reads back as `value`, for a DXF group.
std::string groupNumber(double value) {
    std::array<char, 32> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return error == std::errc{} ? std::string(buffer.data(), end) : std::string{"1"};
}

core::Vec2 rotated(const core::Vec2& vector, double angle) {
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {(vector.x * c) - (vector.y * s), (vector.x * s) + (vector.y * c)};
}

}  // namespace

void InsertCommand::start(CommandContext& context) {
    m_last = context.defaults.insertBlock.value_or(std::string{});
    context.editor.beginTransaction("Insert");
}

Status InsertCommand::handle(CommandContext& context, const Input& input) {
    switch (m_stage) {
        case Stage::Name:
            return handleName(context, input);
        case Stage::Point:
        case Stage::Base:
            return handlePoint(context, input);
        case Stage::Rotation:
            return handleRotation(context, input);
        case Stage::ScaleX:
        case Stage::Corner:
        case Stage::XyzX:
        case Stage::XyzY:
        case Stage::XyzZ:
        case Stage::ScaleY:
            return handleScale(context, input);
    }
    return Status::Continue;
}

Status InsertCommand::handleName(CommandContext& context, const Input& input) {
    std::string text;
    if (const auto* typed = std::get_if<TextInput>(&input)) {
        text = std::string(detail::trim(typed->text));
    } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        text = keyword->keyword;
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        text = m_last;
    }
    if (text.empty()) {
        return Status::Continue;
    }
    if (text == "?") {
        for (const core::BlockDefinition& block :
             context.editor.document().blocks().definitions()) {
            context.message(block.name);
        }
        return Status::Continue;
    }
    const core::BlockTable& blocks = context.editor.document().blocks();
    if (const auto equal = text.find('='); equal != std::string::npos) {
        // The first '=' always separates the name from the path, as AutoCAD
        // does: a second one is an invalid file name (decision of 2026-11-13).
        const std::string name{detail::trim(std::string_view{text}.substr(0, equal))};
        const std::string path{detail::trim(std::string_view{text}.substr(equal + 1))};
        if (path.empty() || path.find('=') != std::string::npos) {
            context.message("Invalid file name.");
            return Status::Continue;
        }
        if (!core::isValidTableName(name)) {
            context.message("Invalid block name.");
            return Status::Continue;
        }
        const bool exists = blocks.find(name) != nullptr;
        if (!import(context, name, path, true)) {
            return Status::Continue;
        }
        m_keepOnCancel = true;
        context.message("Block \"" + m_block + "\" " + (exists ? "redefined" : "defined"));
        return Status::Continue;
    }
    if (isPath(text)) {
        const std::string name = fileBlockName(text);
        if (blocks.find(name) != nullptr) {
            // The definition that is there, without reading the file.
            choose(context, name);
            return Status::Continue;
        }
        if (!core::isValidTableName(name)) {
            context.message("Invalid block name.");
            return Status::Continue;
        }
        import(context, name, text, false);
        return Status::Continue;
    }
    if (blocks.find(text) == nullptr) {
        context.message("Unknown block name: \"" + text + "\".");
        return Status::Continue;
    }
    choose(context, text);
    return Status::Continue;
}

void InsertCommand::choose(CommandContext& context, const std::string& name) {
    const core::Document& document = context.editor.document();
    const core::BlockDefinition* definition = document.blocks().find(name);
    m_block = definition->name;
    m_definitionBase = definition->base;
    m_unitFactor = unitFactor(definition->units, document.units());
    m_scaleX = m_unitFactor;
    m_scaleY = m_unitFactor;
    context.defaults.insertBlock = m_block;
    m_stage = Stage::Point;
}

bool InsertCommand::import(CommandContext& context, const std::string& name,
                           const std::string& path, bool redefine) {
    if (context.files == nullptr) {
        context.message("File access is not available.");
        return false;
    }
    const std::filesystem::path source{std::u8string{path.begin(), path.end()}};
    io::Result<io::BlockImport> result =
        io::importBlock(source, context.editor.document(), *context.files);
    if (!result.ok()) {
        context.message(result.error().message);
        return false;
    }
    io::BlockImport& read = result.value();
    edit::BlockImport block{std::move(read.definition),
                            std::move(read.nested),
                            std::move(read.layers),
                            std::move(read.linetypes),
                            std::move(read.textStyles),
                            std::move(read.dimStyles),
                            read.nextId};
    block.definition.name = name;
    if (!edit::applyImport(context.editor, std::move(block), redefine)) {
        context.message("Block \"" + name + "\" references itself.");
        return false;
    }
    choose(context, name);
    // The factor of the file, as PT19 gives it, not that of the definition.
    m_unitFactor = read.unitFactor;
    m_scaleX = m_unitFactor;
    m_scaleY = m_unitFactor;
    return true;
}

Status InsertCommand::handlePoint(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    if (m_stage == Stage::Base) {
        if (point != nullptr) {
            m_base = point->point;
            m_stage = Stage::Point;
        } else if (std::holds_alternative<ConfirmInput>(input)) {
            m_stage = Stage::Point;
        }
        updatePreview();
        return Status::Continue;
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "Basepoint") {
            m_stage = Stage::Base;
        } else if (keyword->keyword == "Scale") {
            m_stage = Stage::ScaleX;
        } else if (keyword->keyword == "Rotate") {
            m_stage = Stage::Rotation;
        }
        updatePreview();
        return Status::Continue;
    }
    if (point == nullptr) {
        return Status::Continue;
    }
    m_point = point->point;
    context.lastPoint = point->point;
    if (m_corner && !cornerScales(context, point->point, *m_corner)) {
        // The diagonal starts from the insertion point: another one is asked.
        m_point.reset();
        return Status::Continue;
    }
    return advance(context);
}

bool InsertCommand::cornerScales(CommandContext& context, const core::Vec2& point,
                                 const core::Vec2& corner) {
    const core::Vec2 diagonal = corner - point;
    const double x = diagonal.x * m_unitFactor;
    const double y = diagonal.y * m_unitFactor;
    if (x == 0.0 || y == 0.0 || !std::isfinite(x) || !std::isfinite(y)) {
        context.message(std::string{kNonzero});
        return false;
    }
    m_scaleX = x;
    m_scaleY = y;
    return true;
}

Status InsertCommand::handleScale(CommandContext& context, const Input& input) {
    std::optional<double> factor;
    if (const auto* value = std::get_if<ValueInput>(&input)) {
        factor = value->value;
    }
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);
    const auto* point = std::get_if<PointInput>(&input);
    // Every factor, typed or proposed, times the unit factor (decision of
    // 2026-11-19); Enter at a Y or Z scale takes the X scale, already converted.
    const auto scaled = [&](double proposed) -> std::optional<double> {
        const double scale = factor.value_or(proposed) * m_unitFactor;
        if (scale == 0.0 || !std::isfinite(scale)) {
            context.message(std::string{kNonzero});
            return std::nullopt;
        }
        return scale;
    };

    if ((m_stage == Stage::ScaleX || m_stage == Stage::Corner) && point != nullptr) {
        if (!m_point) {
            // Before the insertion point: the diagonal will start from it.
            m_corner = point->point;
            m_scaleGiven = true;
            return advance(context);
        }
        if (!cornerScales(context, *m_point, point->point)) {
            return Status::Continue;
        }
        m_scaleGiven = true;
        return advance(context);
    }
    if (m_stage == Stage::Corner) {
        return Status::Continue;
    }
    if (m_stage == Stage::ScaleX) {
        if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            m_stage = keyword->keyword == "Corner" ? Stage::Corner : Stage::XyzX;
            return Status::Continue;
        }
    }
    if (!factor && !confirmed) {
        return Status::Continue;
    }
    switch (m_stage) {
        case Stage::ScaleX:
        case Stage::XyzX: {
            const auto scale = scaled(1.0);
            if (!scale) {
                return Status::Continue;
            }
            m_scaleX = *scale;
            if (m_stage == Stage::XyzX) {
                m_stage = Stage::XyzY;
                return Status::Continue;
            }
            if (m_point) {
                m_stage = Stage::ScaleY;
                updatePreview();
                return Status::Continue;
            }
            // Scale before the point: one factor for both axes, as the
            // reference cases of RT21 ask it.
            m_scaleY = m_scaleX;
            m_scaleGiven = true;
            return advance(context);
        }
        case Stage::XyzY:
        case Stage::ScaleY: {
            if (factor) {
                const auto scale = scaled(1.0);
                if (!scale) {
                    return Status::Continue;
                }
                m_scaleY = *scale;
            } else {
                m_scaleY = m_scaleX;
            }
            if (m_stage == Stage::XyzY) {
                m_stage = Stage::XyzZ;
                return Status::Continue;
            }
            m_scaleGiven = true;
            return advance(context);
        }
        case Stage::XyzZ: {
            if (factor) {
                const auto scale = scaled(1.0);
                if (!scale) {
                    return Status::Continue;
                }
                m_scaleZ = scale;
            } else {
                m_scaleZ = m_scaleX;
            }
            m_scaleGiven = true;
            return advance(context);
        }
        default:
            return Status::Continue;
    }
}

Status InsertCommand::handleRotation(CommandContext& context, const Input& input) {
    if (const auto* value = std::get_if<ValueInput>(&input)) {
        m_rotation = value->value;
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        m_rotation = 0.0;
    } else if (const auto* point = std::get_if<PointInput>(&input); point != nullptr && m_point) {
        const core::Vec2 direction = point->point - *m_point;
        if (direction.x == 0.0 && direction.y == 0.0) {
            return Status::Continue;
        }
        m_rotation = std::atan2(direction.y, direction.x);
    } else {
        return Status::Continue;
    }
    m_rotationGiven = true;
    return advance(context);
}

Status InsertCommand::advance(CommandContext& context) {
    if (!m_point) {
        m_stage = Stage::Point;
    } else if (!m_scaleGiven) {
        m_stage = Stage::ScaleX;
    } else if (!m_rotationGiven) {
        m_stage = Stage::Rotation;
    } else {
        return finish(context, *m_point);
    }
    updatePreview();
    return Status::Continue;
}

Status InsertCommand::finish(CommandContext& context, const core::Vec2& point) {
    core::RawGroups specific;
    if (m_scaleZ) {
        specific = {{100, "AcDbBlockReference"}, {43, groupNumber(*m_scaleZ)}};
    }
    edit::addInsert(context.editor, insertAt(point), std::move(specific));
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

void InsertCommand::cancel(CommandContext& context) {
    if (context.editor.inTransaction()) {
        // With name=path the definition and the tables read stay, as AutoCAD
        // redefines a block without inserting it (decision of 2026-11-13).
        if (m_keepOnCancel) {
            context.editor.commit();
        } else {
            context.editor.rollback();
        }
    }
    m_preview.clear();
}

core::Insert InsertCommand::insertAt(const core::Vec2& point) const {
    core::Insert insert{m_block, point, m_scaleX, m_scaleY, m_rotation, {}};
    if (m_base) {
        // The chosen base point lands on `point`: the base point of the
        // definition goes where the scaled and rotated offset takes it.
        const core::Vec2 offset = m_definitionBase - *m_base;
        insert.insertion = point + rotated({offset.x * m_scaleX, offset.y * m_scaleY}, m_rotation);
    }
    return insert;
}

void InsertCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

void InsertCommand::updatePreview() {
    m_preview.clear();
    if (m_block.empty() || m_stage == Stage::Name) {
        return;
    }
    if (!m_point) {
        if (m_pointer && m_stage == Stage::Point) {
            m_preview.emplace_back(insertAt(*m_pointer));
        }
        return;
    }
    core::Insert insert = insertAt(*m_point);
    if (m_pointer) {
        const core::Vec2 diagonal = *m_pointer - *m_point;
        if ((m_stage == Stage::ScaleX || m_stage == Stage::Corner) && diagonal.x != 0.0 &&
            diagonal.y != 0.0) {
            insert.scaleX = diagonal.x * m_unitFactor;
            insert.scaleY = diagonal.y * m_unitFactor;
        } else if (m_stage == Stage::Rotation && (diagonal.x != 0.0 || diagonal.y != 0.0)) {
            insert.rotation = std::atan2(diagonal.y, diagonal.x);
        }
    }
    if (core::supported(insert)) {
        m_preview.emplace_back(std::move(insert));
    }
}

Prompt InsertCommand::prompt() const {
    switch (m_stage) {
        case Stage::Name:
            return {m_last.empty() ? "Enter block name or [?]:"
                                   : "Enter block name or [?] <" + m_last + ">:",
                    {{"?", "?"}},
                    PromptKind::Text};
        case Stage::Point:
            return {"Specify insertion point or [Basepoint/Scale/Rotate]:",
                    {{"Basepoint", "B"}, {"Scale", "S"}, {"Rotate", "R"}},
                    PromptKind::Point};
        case Stage::Base: {
            const core::Vec2 base = m_base.value_or(m_definitionBase);
            return {"Specify base point <" + shortNumber(base.x) + "," + shortNumber(base.y) + ">:",
                    {},
                    PromptKind::Point};
        }
        case Stage::ScaleX:
            return {"Enter X scale factor, specify opposite corner, or [Corner/XYZ] <1>:",
                    {{"Corner", "C"}, {"XYZ", "XYZ"}},
                    PromptKind::Number};
        case Stage::Corner:
            return {"Specify opposite corner:", {}, PromptKind::Point};
        case Stage::XyzX:
            return {"Enter X scale factor <1>:", {}, PromptKind::Number};
        case Stage::XyzY:
        case Stage::ScaleY:
            return {"Enter Y scale factor <use X scale factor>:", {}, PromptKind::Number};
        case Stage::XyzZ:
            return {"Enter Z scale factor <use X scale factor>:", {}, PromptKind::Number};
        case Stage::Rotation:
            return {"Specify rotation angle <0>:", {}, PromptKind::Angle};
    }
    return {};
}

std::optional<core::Vec2> InsertCommand::basePoint() const {
    const bool fromPoint =
        m_stage == Stage::ScaleX || m_stage == Stage::Corner || m_stage == Stage::Rotation;
    return fromPoint ? m_point : std::nullopt;
}

CommandDescription InsertCommand::description() {
    return {"-INSERT",
            {"INSERT", "I"},
            "Inserts a block of the drawing, or a DXF file as a block.",
            false,
            {{"block-name", "Enter block name or [?]:", ValueType::Name, {{"?", "?"}}},
             {"block-name-default",
              "Enter block name or [?] <{default}>:",
              ValueType::Name,
              {{"?", "?"}},
              true},
             {"insertion-point",
              "Specify insertion point or [Basepoint/Scale/Rotate]:",
              ValueType::Point,
              {{"Basepoint", "B"}, {"Scale", "S"}, {"Rotate", "R"}},
              false},
             {"base-point", "Specify base point <{default}>:", ValueType::Point, {}, true},
             {"x-scale",
              "Enter X scale factor, specify opposite corner, or [Corner/XYZ] <{default}>:",
              ValueType::Real,
              {{"Corner", "C"}, {"XYZ", "XYZ"}},
              true},
             {"opposite-corner", "Specify opposite corner:", ValueType::Point, {}, false},
             {"xyz-x-scale", "Enter X scale factor <{default}>:", ValueType::Real, {}, true},
             {"y-scale", "Enter Y scale factor <{default}>:", ValueType::Real, {}, true},
             {"z-scale", "Enter Z scale factor <{default}>:", ValueType::Real, {}, true},
             {"rotation", "Specify rotation angle <{default}>:", ValueType::Angle, {}, true}}};
}

}  // namespace tg::cmd
