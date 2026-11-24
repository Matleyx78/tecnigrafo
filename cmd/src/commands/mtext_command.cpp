// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/mtext_command.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"

#include <charconv>
#include <cmath>
#include <string_view>
#include <variant>

namespace tg::cmd {
namespace {

constexpr std::string_view kLineSpacingSuffix{"x"};

std::optional<double> lineSpacingFactor(std::string_view text) {
    if (text.ends_with(kLineSpacingSuffix)) {
        text.remove_suffix(kLineSpacingSuffix.size());
    }
    double value = 0.0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !(value > 0.0) ||
        !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

std::string scalar(double value) {
    std::string result = formatLength(value);
    while (result.ends_with('0')) {
        result.pop_back();
    }
    if (result.ends_with('.')) {
        result.pop_back();
    }
    return result;
}

core::MTextAttachment attachment(std::string_view keyword) {
    if (keyword == "TC") {
        return core::MTextAttachment::TopCenter;
    }
    if (keyword == "TR") {
        return core::MTextAttachment::TopRight;
    }
    if (keyword == "ML") {
        return core::MTextAttachment::MiddleLeft;
    }
    if (keyword == "MC") {
        return core::MTextAttachment::MiddleCenter;
    }
    if (keyword == "MR") {
        return core::MTextAttachment::MiddleRight;
    }
    if (keyword == "BL") {
        return core::MTextAttachment::BottomLeft;
    }
    if (keyword == "BC") {
        return core::MTextAttachment::BottomCenter;
    }
    if (keyword == "BR") {
        return core::MTextAttachment::BottomRight;
    }
    return core::MTextAttachment::TopLeft;
}

}  // namespace

CommandDescription MTextCommand::description() {
    return {"MTEXT",
            {"T", "MT"},
            "Creates multiline text from a reference rectangle.",
            false,
            {{"first-corner", "Specify first corner:", ValueType::Point, {}, false},
             {"opposite-corner",
              "Specify opposite corner or [Height/Justify/Line spacing/Rotation/Style/Width]:",
              ValueType::Point,
              {{"Height", "H"},
               {"Justify", "J"},
               {"Line spacing", "L"},
               {"Rotation", "R"},
               {"Style", "S"},
               {"Width", "W"}},
              false},
             {"height", "Specify height <{default}>:", ValueType::Distance, {}, true},
             {"justify",
              "Enter justification [TL/TC/TR/ML/MC/MR/BL/BC/BR] <TL>:",
              ValueType::Keyword,
              {{"TL", "TL"},
               {"TC", "TC"},
               {"TR", "TR"},
               {"ML", "ML"},
               {"MC", "MC"},
               {"MR", "MR"},
               {"BL", "BL"},
               {"BC", "BC"},
               {"BR", "BR"}},
              true},
             {"line-spacing-type",
              "Enter line spacing type [At least/Exactly] <{default}>:",
              ValueType::Keyword,
              {{"At least", "A"}, {"Exactly", "E"}},
              true},
             {"line-spacing-factor",
              "Enter line spacing factor or distance <{default}x>:",
              ValueType::Text,
              {},
              true},
             {"rotation", "Specify rotation angle <0>:", ValueType::Angle, {}, true},
             {"style", "Enter style name or [?] <{default}>:", ValueType::Text, {}, true},
             {"width", "Specify width:", ValueType::Distance, {}, false},
             {"text", "MText:", ValueType::Text, {}, true}}};
}

void MTextCommand::start(CommandContext& context) {
    m_stage = Stage::FirstCorner;
    m_mtext = {};
    m_mtext.height = context.defaults.textHeight;
    m_mtext.lineSpacing = context.defaults.mtextLineSpacing;
    m_mtext.exactSpacing = context.defaults.mtextExactSpacing;
    useStyle(context.editor.document(), context.editor.document().textStyles().current());
    m_first = {};
    m_hasLines = false;
    m_preview.clear();
    context.message("Current text style:  \"" + m_mtext.style +
                    "\"  Text height:  " + scalar(m_mtext.height) + "  Annotative:  No");
    context.editor.beginTransaction("Mtext");
}

Status MTextCommand::handle(CommandContext& context, const Input& input) {
    const auto* point = std::get_if<PointInput>(&input);
    const auto* value = std::get_if<ValueInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const auto* text = std::get_if<TextInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);

    if (m_stage == Stage::FirstCorner && point != nullptr) {
        m_first = point->point;
        m_mtext.insertion = m_first;
        context.lastPoint = m_first;
        m_stage = Stage::OppositeCorner;
    } else if (m_stage == Stage::OppositeCorner) {
        if (point != nullptr) {
            const auto direction = core::sinCos(m_mtext.rotation);
            const core::Vec2 delta = point->point - m_first;
            beginText(std::abs(delta.x * direction.cosine + delta.y * direction.sine));
        } else if (keyword != nullptr) {
            if (keyword->keyword == "Height") {
                m_stage = Stage::Height;
            } else if (keyword->keyword == "Justify") {
                m_stage = Stage::Justify;
            } else if (keyword->keyword == "Line spacing") {
                m_stage = Stage::LineSpacingType;
            } else if (keyword->keyword == "Rotation") {
                m_stage = Stage::Rotation;
            } else if (keyword->keyword == "Style") {
                m_stage = Stage::Style;
            } else if (keyword->keyword == "Width") {
                m_stage = Stage::Width;
            }
        }
    } else if (m_stage == Stage::Height && (point != nullptr || value != nullptr || confirm)) {
        double height = m_mtext.height;
        if (point != nullptr) {
            height = core::distance(m_first, point->point);
        } else if (value != nullptr) {
            height = value->value;
        }
        if (!(height > 0.0) || !std::isfinite(height)) {
            context.message("Value must be positive and nonzero.");
            return Status::Continue;
        }
        m_mtext.height = height;
        m_stage = Stage::OppositeCorner;
    } else if (m_stage == Stage::Justify && (keyword != nullptr || confirm)) {
        if (keyword != nullptr) {
            m_mtext.attachment = attachment(keyword->keyword);
        }
        m_stage = Stage::OppositeCorner;
    } else if (m_stage == Stage::LineSpacingType && (keyword != nullptr || confirm)) {
        if (keyword != nullptr) {
            m_mtext.exactSpacing = keyword->keyword == "Exactly";
        }
        context.defaults.mtextExactSpacing = m_mtext.exactSpacing;
        m_stage = Stage::LineSpacingFactor;
    } else if (m_stage == Stage::LineSpacingFactor && (text != nullptr || confirm)) {
        if (text != nullptr) {
            const auto factor = lineSpacingFactor(text->text);
            if (!factor.has_value()) {
                context.message("Value must be positive and nonzero.");
                return Status::Continue;
            }
            m_mtext.lineSpacing = *factor;
        }
        context.defaults.mtextLineSpacing = m_mtext.lineSpacing;
        m_stage = Stage::OppositeCorner;
    } else if (m_stage == Stage::Rotation && (point != nullptr || value != nullptr || confirm)) {
        double rotation = 0.0;
        if (point != nullptr) {
            const auto delta = point->point - m_first;
            rotation = core::atan2(delta.y, delta.x);
        } else if (value != nullptr) {
            rotation = value->value;
        }
        if (std::isfinite(rotation)) {
            m_mtext.rotation = rotation;
        }
        m_stage = Stage::OppositeCorner;
    } else if (m_stage == Stage::Style && (text != nullptr || confirm)) {
        const std::string name = text != nullptr ? text->text : m_mtext.style;
        if (context.editor.document().textStyles().find(name) == nullptr) {
            context.message("Cannot find text style \"" + name + "\".");
            return Status::Continue;
        }
        useStyle(context.editor.document(), name);
        m_stage = Stage::OppositeCorner;
    } else if (m_stage == Stage::Width && (point != nullptr || value != nullptr)) {
        const double width =
            value != nullptr ? value->value : core::distance(m_first, point->point);
        if (!(width > 0.0) || !std::isfinite(width)) {
            context.message("Value must be positive and nonzero.");
            return Status::Continue;
        }
        beginText(width);
    } else if (m_stage == Stage::Text) {
        if (text != nullptr && !text->text.empty()) {
            if (m_hasLines) {
                m_mtext.value += "\\P";
            }
            m_mtext.value += text->text;
            m_hasLines = true;
        } else if (confirm) {
            finish(context);
            return Status::Finished;
        }
    }
    return Status::Continue;
}

void MTextCommand::finish(CommandContext& context) {
    if (m_hasLines) {
        // Defaults describe a created text, not an abandoned setup.
        context.defaults.textHeight = m_mtext.height;
        context.editor.add(m_mtext);
        context.lastPoint = m_mtext.insertion;
        context.editor.commit();
    } else {
        context.editor.rollback();
    }
    m_preview.clear();
}

void MTextCommand::cancel(CommandContext& context) {
    finish(context);
}

void MTextCommand::beginText(double width) {
    m_mtext.width = width;
    m_stage = Stage::Text;
}

void MTextCommand::useStyle(const core::Document& document, std::string style) {
    m_mtext.style = std::move(style);
    if (const auto* record = document.textStyles().find(m_mtext.style);
        record != nullptr && record->fixedHeight > 0.0) {
        m_mtext.height = record->fixedHeight;
    }
}

void MTextCommand::pointerMoved(const core::Vec2& /*point*/) {}

Prompt MTextCommand::prompt() const {
    switch (m_stage) {
        case Stage::FirstCorner:
            return {"Specify first corner:", {}};
        case Stage::OppositeCorner:
            return {
                "Specify opposite corner or [Height/Justify/Line spacing/Rotation/Style/Width]:",
                {{"Height", "H"},
                 {"Justify", "J"},
                 {"Line spacing", "L"},
                 {"Rotation", "R"},
                 {"Style", "S"},
                 {"Width", "W"}}};
        case Stage::Height:
            return {
                "Specify height <" + formatLength(m_mtext.height) + ">:", {}, PromptKind::Distance};
        case Stage::Justify:
            return {"Enter justification [TL/TC/TR/ML/MC/MR/BL/BC/BR] <TL>:",
                    {{"TL", "TL"},
                     {"TC", "TC"},
                     {"TR", "TR"},
                     {"ML", "ML"},
                     {"MC", "MC"},
                     {"MR", "MR"},
                     {"BL", "BL"},
                     {"BC", "BC"},
                     {"BR", "BR"}}};
        case Stage::LineSpacingType:
            return {"Enter line spacing type [At least/Exactly] <" +
                        std::string(m_mtext.exactSpacing ? "Exactly" : "At least") + ">:",
                    {{"At least", "A"}, {"Exactly", "E"}}};
        case Stage::LineSpacingFactor:
            return {"Enter line spacing factor or distance <" + scalar(m_mtext.lineSpacing) + "x>:",
                    {},
                    PromptKind::Text};
        case Stage::Rotation:
            return {"Specify rotation angle <0>:", {}, PromptKind::Angle};
        case Stage::Style:
            return {"Enter style name or [?] <" + m_mtext.style + ">:", {}, PromptKind::Text};
        case Stage::Width:
            return {"Specify width:", {}, PromptKind::Distance};
        case Stage::Text:
            return {"MText:", {}, PromptKind::Text};
    }
    return {};
}

std::span<const core::Entity> MTextCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> MTextCommand::basePoint() const {
    return m_stage == Stage::FirstCorner ? std::nullopt : std::optional<core::Vec2>{m_first};
}

std::optional<TextEditRequest> MTextCommand::textEditing() const {
    if (m_stage != Stage::Text) {
        return std::nullopt;
    }
    return TextEditRequest{std::nullopt, m_mtext, "", std::nullopt};
}

}  // namespace tg::cmd
