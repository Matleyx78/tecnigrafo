// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::cmd {

/// -ATTEDIT (alias ATE, also ATTEDIT): edits the attributes of the inserts
/// of the drawing one at a time or globally, as AutoCAD (docs/adr/0056,
/// point 4; docs/tappe/0.8.1/PT19-attedit.md).
///
/// The three filters, block name, tag and value, take the wildcards of
/// AutoCAD without case (detail::matchesName()). "Select Attributes:" takes
/// one selection and closes by itself: a click, through core::pickAttribute()
/// with the filters as its `accept`, or a Window or Crossing rectangle, which
/// takes the visible attributes whose text it selects (core::isInside(),
/// core::intersects()). Only the ATTRIBs of the inserts of the drawing, never
/// those of a nested insert, nor those of an insert on a locked layer.
///
/// One at a time, the attributes go in the order of the inserts and of their
/// attributes, the current one highlighted. Position, Height and Angle are
/// refused for an attribute with its position locked. Globally, the string
/// to change is replaced at every occurrence of each value; with No to the
/// visible attributes, the invisible ones are taken as well, and no selection
/// is asked.
///
/// The whole command is one undo step, "Attedit", also when it changes
/// nothing (docs/adr/0056, point 4). Esc keeps the changes already made.
class AtteditCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] std::span<const core::EntityId> highlighted() const override;

private:
    enum class Stage : std::uint8_t {
        OneAtATime,
        Visible,
        BlockSpecification,
        TagSpecification,
        ValueSpecification,
        Select,
        FirstCorner,
        OppositeCorner,
        Option,
        ValueModification,
        StringToChange,
        NewString,
        NewValue,
        Position,
        Height,
        Angle,
        Style,
        Layer,
        Color,
        TrueColor,
        GlobalStringToChange,
        GlobalNewString
    };

    /// An attribute chosen: the insert and the index of the attribute.
    struct Chosen {
        core::EntityId insert;
        std::size_t index{0};
    };

    Status afterFilters(CommandContext& context);
    Status chosen(CommandContext& context, std::vector<Chosen> attributes);
    Status nextAttribute(CommandContext& context);
    Status chooseOption(CommandContext& context, std::string_view text);
    Status finish(CommandContext& context);
    Status selectInRectangle(CommandContext& context, const core::Vec2& corner);
    Status pick(CommandContext& context, const core::Vec2& point);
    [[nodiscard]] bool passesFilters(const core::Insert& insert,
                                     const core::Attribute& attribute) const;
    [[nodiscard]] const core::Attribute* current(const core::Document& document) const;
    /// Applies `change` to a copy of the current attribute and puts the
    /// insert back into the drawing.
    template <typename Change>
    void changeCurrent(CommandContext& context, Change change);

    Stage m_stage{Stage::OneAtATime};
    bool m_oneAtATime{true};
    bool m_visibleOnly{true};
    bool m_crossing{false};
    std::string m_blockSpecification{"*"};
    std::string m_tagSpecification{"*"};
    std::string m_valueSpecification{"*"};
    std::string m_stringToChange;
    std::string m_default;  ///< what the question shows between < >
    core::Vec2 m_firstCorner;
    std::vector<Chosen> m_attributes;
    std::size_t m_current{0};
    std::vector<core::EntityId> m_highlighted;
    std::optional<core::Vec2> m_basePoint;
};

}  // namespace tg::cmd
