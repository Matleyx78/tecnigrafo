// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_protocol.hpp"
#include "tg/cmd/input.hpp"
#include "tg/core/document.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/document_file.hpp"
#include "tg/edit/editor.hpp"

#include <optional>
#include <span>

namespace tg::io {
class FileAccess;
}

namespace tg::cmd {

/// Values commands remember for the whole session (docs/adr/0014).
struct CommandDefaults {
    double textHeight{2.5};
    double textRotation{0.0};            ///< radians
    std::optional<double> circleRadius;  ///< none before the first circle
    double rotation{0.0};                ///< the last rotation of ROTATE, radians
    /// MIRRTEXT: 0 (Readable) at start-up (docs/adr/0031, point 3).
    core::MirrorText mirrorText{core::MirrorText::Readable};
    /// POLARANG, the increment of polar tracking, radians: 90 degrees at
    /// start-up (docs/adr/0033, point 8).
    double polarAngle{core::kPi / 2.0};
    /// TRIMEXTENDMODE: 1 (quick) at start-up, or 0 (standard); the mode
    /// TRIM and EXTEND start in (docs/adr/0035, point 3).
    int trimExtendMode{1};
    std::optional<double> offsetDistance = std::nullopt;  ///< none means Through
    bool offsetEraseSource{false};
    bool offsetCurrentLayer{false};  ///< false means Source
    double filletRadius{0.0};
    double chamferFirstDistance{0.0};
    double chamferSecondDistance{0.0};
    double chamferLength{0.0};
    double chamferAngle{0.0};  ///< radians
    bool chamferUseAngle{false};
    bool trimMode{true};
};

/// What a command of the drawing works with: the protocol's context
/// (docs/adr/0041, point 5) and the drawing's. The runner preserves this
/// state between commands, so `lastPoint` carries over, as the "@" of
/// AutoCAD does. Ortho uses the running command's base point
/// (Command::basePoint()).
/// When adding a field, update CommandRunner::setDocumentFile to preserve it
/// when rebinding the file reference.
struct CommandContext : ProtocolContext {
    edit::Editor& editor;
    edit::DocumentFile& file;
    std::optional<core::Vec2> lastPoint;  ///< updated by the command
    CommandDefaults defaults{};
    io::FileAccess* files{nullptr};
    bool documentReplaced{false};
};

/// An interactive command (docs/adr/0005). Commands never read text: they
/// receive inputs already interpreted by CommandRunner.
class Command {
public:
    Command() = default;
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    Command(Command&&) = delete;
    Command& operator=(Command&&) = delete;
    virtual ~Command() = default;

    /// Called once, before any input. Opens the command's transaction.
    virtual void start(CommandContext& context) = 0;

    /// Handles one input; never a CancelInput, which the runner handles by
    /// calling cancel(). Before returning Finished the command
    /// commits or rolls back its transaction.
    virtual Status handle(CommandContext& context, const Input& input) = 0;

    /// Esc: the command keeps what it has completed and closes its
    /// transaction (docs/adr/0015). The default rolls everything back.
    virtual void cancel(CommandContext& context) {
        if (context.editor.inTransaction()) {
            context.editor.rollback();
        }
    }

    /// The pointer moved to `point`, in drawing coordinates.
    virtual void pointerMoved(const core::Vec2& point) = 0;

    [[nodiscard]] virtual Prompt prompt() const = 0;

    /// Entities drawn as a preview, not part of the document. Valid until the
    /// next call to a non-const function of the command.
    [[nodiscard]] virtual std::span<const core::Entity> preview() const = 0;

    /// The point of this command that the next point is drawn from, which
    /// ortho aligns the mouse to (docs/adr/0008); std::nullopt while the
    /// command asks for its first point, so that a first click lands where
    /// the user clicks, as in AutoCAD. Unlike CommandContext::lastPoint it
    /// never comes from a previous command.
    [[nodiscard]] virtual std::optional<core::Vec2> basePoint() const { return std::nullopt; }

    /// True for commands that work on existing entities. The runner asks
    /// "Select objects:" itself and calls start() only once it has a
    /// selection, then hands it over with one SelectionInput
    /// (docs/adr/0008).
    [[nodiscard]] virtual bool needsSelection() const { return false; }
};

}  // namespace tg::cmd
