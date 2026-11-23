// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/document.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tg::cmd {

/// -INSERT, with the alias INSERT (docs/adr/0045, points 5-7;
/// docs/tappe/0.8/PT21-comando-insert.md): inserts a block of the drawing,
/// or a DXF file as a block, with its scale and rotation.
///
/// Every scale factor, typed, proposed or given by the opposite corner, is
/// multiplied by the unit factor of the block: the scale of the insert is the
/// factor given times the unit factor, as AutoCAD does. With `name=path`,
/// Esc keeps the definition read from the file and the tables it brought, in
/// one `Insert` step, without a new insert.
class InsertCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return m_preview; }

    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Name,
        Point,
        Base,
        ScaleX,
        Corner,
        XyzX,
        XyzY,
        XyzZ,
        ScaleY,
        Rotation,
        Attribute,
        Verify
    };

    Status handleName(CommandContext& context, const Input& input);
    Status handlePoint(CommandContext& context, const Input& input);
    Status handleScale(CommandContext& context, const Input& input);
    Status handleRotation(CommandContext& context, const Input& input);
    Status handleAttribute(CommandContext& context, const Input& input);
    /// Accepts the block `name` of the drawing, with the unit factor of its
    /// definition.
    void choose(CommandContext& context, const std::string& name);
    /// Reads `path` as the block `name`; with `redefine`, a block with that
    /// name takes the definition of the file. False after a message.
    bool import(CommandContext& context, const std::string& name, const std::string& path,
                bool redefine);
    /// The scales from the diagonal from the insertion point `point` to
    /// `corner`, times the unit factor; false after a message if one of them
    /// is zero.
    bool cornerScales(CommandContext& context, const core::Vec2& point, const core::Vec2& corner);
    /// Goes on after the insertion point or a scale or rotation given before
    /// it: back to the insertion point, to the next question, or inserts.
    Status advance(CommandContext& context);
    /// Inserts at the insertion point `point` and ends the command.
    Status finish(CommandContext& context, const core::Vec2& point);
    /// Starts the attribute questions, or inserts immediately when ATTREQ is
    /// zero or the definition has no values to ask for.
    Status askAttributes(CommandContext& context);
    /// The insert as it would be added now, at `point`.
    [[nodiscard]] core::Insert insertAt(const core::Vec2& point) const;
    void updatePreview();

    Stage m_stage{Stage::Name};
    std::string m_last;  ///< the block proposed at the name prompt
    std::string m_block;
    core::Vec2 m_definitionBase;
    std::optional<core::Vec2> m_base;  ///< Basepoint, for this insert only
    double m_unitFactor{1.0};
    std::optional<core::Vec2> m_point;   ///< the insertion point
    std::optional<core::Vec2> m_corner;  ///< an opposite corner given before the point
    bool m_scaleGiven{false};
    bool m_rotationGiven{false};
    double m_scaleX{1.0};
    double m_scaleY{1.0};
    std::optional<double> m_scaleZ;  ///< given with XYZ, kept as group 43
    double m_rotation{0.0};
    bool m_keepOnCancel{false};  ///< name=path: Esc keeps the definition

    struct AttributeQuestion {
        std::size_t index;
        std::string prompt;
        bool verify;
    };

    std::vector<core::Attribute> m_attributes;
    std::vector<AttributeQuestion> m_questions;
    std::size_t m_question{0};
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

/// ATTREQ: whether -INSERT asks for attribute values. It is a session value,
/// one at start-up, and opens no undo step (docs/adr/0056, point 3).
class AttreqCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2&) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    bool m_value{true};
};

}  // namespace tg::cmd
