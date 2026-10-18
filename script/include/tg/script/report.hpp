// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

/// The report of the control mode (docs/adr/0011, point 4;
/// docs/verifica/rapporto.md). The same text at level 1 and level 2.
namespace tg::script {

/// First line of every report: the version of its format.
inline constexpr std::string_view kReportHeader = "tecnigrafo-report 1";

/// Title of the last section, always written at the end of a script.
inline constexpr std::string_view kFinalSection = "final";

/// What one section of the report shows.
struct ReportInput {
    /// "report <n>" for the n-th !report, kFinalSection at the end.
    std::string_view title;
    const edit::Editor& editor;
    const cmd::CommandRunner& runner;
    /// The lines written in the history of the command line since the
    /// previous section (docs/verifica/rapporto.md, "messages").
    std::span<const std::string> messages;
    /// The last point the pointer was moved to, in drawing coordinates;
    /// std::nullopt before the first movement.
    std::optional<core::Vec2> pointer;
};

/// Writes one section of the report: the state of `editor` and `runner`
/// plus the messages written so far. Shared with level 2 (P3).
void writeState(const ReportInput& input, std::string& out);

/// The DXF name of the entity's type ("LWPOLYLINE").
[[nodiscard]] std::string_view dxfType(const core::Entity& entity);

/// Appends the entity in the format of docs/verifica/rapporto.md, on one
/// line and without its end: the type, then its properties
/// ("LWPOLYLINE closed=no vertex=0,0 vertex=10,0"). The same text is read
/// back by readEntity() (entity_reader.hpp) without losses.
void writeEntity(const core::Entity& entity, std::string& out);

/// The name of a snap kind in reports and answers ("Endpoint").
[[nodiscard]] std::string_view snapKindName(core::SnapKind kind);

/// The name of a snap mode in reports and questions ("endpoint") and the
/// member of core::SnapModes that enables it.
struct SnapModeName {
    std::string_view name;
    bool core::SnapModes::*member;
};

/// Every mode of core::SnapModes, in the order of its members.
[[nodiscard]] std::span<const SnapModeName> snapModeNames();

}  // namespace tg::script
