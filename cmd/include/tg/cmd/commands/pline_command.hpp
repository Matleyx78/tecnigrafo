// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

#include <optional>
#include <vector>

namespace tg::cmd {

/// PLINE with straight segments and tangent arcs. Geometry remains a preview until confirmation;
/// the completed polyline is added in a single undoable transaction.
class PlineCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;

    /// The last vertex; none before the first one, or once Undo has removed
    /// them all.
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    Status finish(CommandContext& context, bool closed);
    void updatePreview();
    [[nodiscard]] std::optional<double> segmentBulge(const core::Vec2& end) const;

    bool m_arcMode{false};
    std::vector<core::PolylineVertex> m_vertices;
    std::optional<core::Vec2> m_pointer;
    std::optional<core::Vec2> m_previousPoint;
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
