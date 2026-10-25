// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/file_commands.hpp"

#include "../text.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"

#include <charconv>
#include <string_view>
#include <system_error>
#include <variant>

namespace tg::cmd {
namespace {

std::string_view extension(std::string_view name) {
    const std::size_t dot = name.find_last_of('.');
    const std::size_t slash = name.find_last_of("/\\");
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
        return {};
    }
    return name.substr(dot);
}

io::DxfVersion originalVersion(const core::Document& document) {
    const std::string& version = document.raw().version;
    if (version == "AC1015") {
        return io::DxfVersion::R2000;
    }
    if (version == "AC1018") {
        return io::DxfVersion::R2004;
    }
    if (version == "AC1021") {
        return io::DxfVersion::R2007;
    }
    if (version == "AC1024") {
        return io::DxfVersion::R2010;
    }
    if (version == "AC1027") {
        return io::DxfVersion::R2013;
    }
    return io::DxfVersion::R2018;
}

}  // namespace

void SaveCommand::start(CommandContext& context) {
    m_defaultName = context.editor.fileName();
    m_stage = Stage::Name;
    if (context.files == nullptr) {
        context.message("File access is not available.");
        m_stage = Stage::Done;
    } else if (m_mode == Mode::Qsave && context.editor.hasFile()) {
        chooseName(context, m_defaultName);
    }
}

Status SaveCommand::handle(CommandContext& context, const Input& input) {
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    if (m_stage == Stage::Replace) {
        if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            if (keyword->keyword == "Yes") {
                return save(context);
            }
            if (keyword->keyword == "No") {
                m_stage = Stage::Name;
            }
        } else if (confirm) {
            m_stage = Stage::Name;
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Done) {
        return Status::Finished;
    }
    if (const auto* text = std::get_if<TextInput>(&input)) {
        return chooseName(context, text->text);
    }
    return confirm ? chooseName(context, m_defaultName) : Status::Continue;
}

Status SaveCommand::chooseName(CommandContext& context, std::string name) {
    m_name = std::move(name);
    if (extension(m_name).empty()) {
        m_name += ".cadx";
    }
    m_dxf = detail::equalsIgnoringCase(extension(m_name), ".dxf");
    if (!m_dxf && !detail::equalsIgnoringCase(extension(m_name), ".cadx")) {
        return fail(context, "only .cadx and .dxf files are supported.");
    }
    const bool quickCurrent =
        m_mode == Mode::Qsave && context.editor.hasFile() && m_name == context.editor.fileName();
    if (!quickCurrent && context.files->exists(m_name)) {
        m_stage = Stage::Replace;
        return Status::Continue;
    }
    return save(context);
}

Status SaveCommand::save(CommandContext& context) {
    const bool current = context.editor.hasFile() && m_name == context.editor.fileName();
    const io::DxfVersion version = current && m_mode != Mode::Saveas
                                       ? originalVersion(context.editor.document())
                                       : io::DxfVersion::R2018;
    const io::Result<std::string> bytes = m_dxf ? io::writeDxf(context.editor.document(), version)
                                                : io::writeCadx(context.editor.document());
    if (!bytes.ok()) {
        return fail(context, bytes.error().message);
    }
    if (const auto error = context.files->write(m_name, bytes.value())) {
        return fail(context, error->message);
    }
    // Saving a copy must not move the undo stack's saved position.
    if (m_mode != Mode::Save || m_name == context.editor.fileName()) {
        context.editor.markSaved(m_name);
    }
    m_stage = Stage::Done;
    return Status::Finished;
}

Status SaveCommand::fail(CommandContext& context, const std::string& reason) {
    context.message("Cannot save " + m_name + ": " + reason);
    m_stage = Stage::Done;
    return Status::Finished;
}

Prompt SaveCommand::prompt() const {
    switch (m_stage) {
        case Stage::Name:
            return {
                "Save drawing as <" + m_defaultName + ">:", {}, PromptKind::Text, FilePrompt::Save};
        case Stage::Replace:
            return {
                "A drawing with this name already exists. Do you want to replace it? "
                "[Yes/No] <No>:",
                {{"Yes", "Y"}, {"No", "N"}}};
        case Stage::Done:
            return {};
    }
    return {};
}

void NewCommand::start(CommandContext& context) {
    m_currentName = context.editor.fileName();
    if (context.editor.isModified()) {
        m_stage = Stage::Changes;
    } else {
        proceed(context);
    }
}

Status NewCommand::proceed(CommandContext& context) {
    if (m_open) {
        if (context.files == nullptr) {
            context.message("File access is not available.");
            m_stage = Stage::Done;
            return Status::Finished;
        }
        m_stage = Stage::Name;
        return Status::Continue;
    }
    context.editor.replaceDocument(core::Document::newDrawing(), "");
    context.documentReplaced = true;
    m_stage = Stage::Done;
    return Status::Finished;
}

Status NewCommand::afterSave(CommandContext& context) {
    // A failed QSAVE leaves the modified drawing in place.
    if (context.editor.isModified()) {
        m_stage = Stage::Done;
        return Status::Finished;
    }
    return proceed(context);
}

Status NewCommand::handle(CommandContext& context, const Input& input) {
    if (m_stage == Stage::Saving) {
        return m_save.handle(context, input) == Status::Finished ? afterSave(context)
                                                                 : Status::Continue;
    }
    if (m_stage == Stage::Name) {
        if (const auto* text = std::get_if<TextInput>(&input);
            text != nullptr && !text->text.empty()) {
            return open(context, text->text);
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Done) {
        return Status::Finished;
    }
    const auto* keyword = std::get_if<KeywordInput>(&input);
    if (keyword != nullptr && keyword->keyword == "Cancel") {
        context.message("*Cancel*");
        m_stage = Stage::Done;
        return Status::Finished;
    }
    if (keyword != nullptr && keyword->keyword == "No") {
        return proceed(context);
    }
    if (std::holds_alternative<ConfirmInput>(input) ||
        (keyword != nullptr && keyword->keyword == "Yes")) {
        m_stage = Stage::Saving;
        m_save.start(context);
        if (m_save.prompt().text.empty()) {
            return afterSave(context);
        }
    }
    return Status::Continue;
}

Status NewCommand::open(CommandContext& context, const std::string& name) {
    m_stage = Stage::Done;
    const auto fail = [&](const io::Error& error) {
        context.message("Cannot open " + name + ": " + error.message);
        return Status::Finished;
    };
    // Public source identifiers deliberately have no extension (ADR 0021, point 4).
    const bool dxf =
        name.starts_with("@sources/") || detail::equalsIgnoringCase(extension(name), ".dxf");
    if (!dxf && !detail::equalsIgnoringCase(extension(name), ".cadx")) {
        return fail({io::Error::Kind::Unsupported, 0, "only .cadx and .dxf files are supported."});
    }
    const auto bytes = context.files->read(name);
    if (!bytes.ok()) {
        return fail(bytes.error());
    }
    auto result = dxf ? io::readDxf(bytes.value()) : io::readCadx(bytes.value());
    if (!result.ok()) {
        return fail(result.error());
    }
    context.editor.replaceDocument(std::move(result.value().document), name);
    context.documentReplaced = true;
    const auto& report = result.value().report;
    int total = 0;
    std::string types;
    for (const auto& [type, count] : report.unhandled) {
        total += count;
        if (!types.empty()) {
            types += ", ";
        }
        types += type + " " + std::to_string(count);
    }
    if (total > 0) {
        context.message(std::to_string(total) +
                        (total == 1 ? " entity not supported: " : " entities not supported: ") +
                        types);
    }
    for (const auto& warning : report.warnings) {
        context.message(warning);
    }
    return Status::Finished;
}

Prompt NewCommand::prompt() const {
    switch (m_stage) {
        case Stage::Changes:
            return {"Save changes to " + m_currentName + "? [Yes/No/Cancel] <Yes>:",
                    {{"Yes", "Y"}, {"No", "N"}, {"Cancel", "C"}}};
        case Stage::Saving:
            return m_save.prompt();
        case Stage::Name:
            return {"Enter name of drawing to open:", {}, PromptKind::Text, FilePrompt::Open};
        case Stage::Done:
            return {};
    }
    return {};
}

void InsunitsCommand::start(CommandContext& context) {
    m_units = context.editor.document().units();
}

Status InsunitsCommand::handle(CommandContext& context, const Input& input) {
    int units = m_units;
    if (const auto* text = std::get_if<TextInput>(&input)) {
        std::string_view value = detail::trim(text->text);
        // from_chars accepts a minus sign but not an explicit plus sign.
        if (value.starts_with('+')) {
            value.remove_prefix(1);
        }
        const auto result = std::from_chars(value.data(), value.data() + value.size(), units);
        constexpr int kMinUnits = 0;
        constexpr int kMaxUnits = 24;
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
            units < kMinUnits || units > kMaxUnits) {
            context.message("Requires an integer between 0 and 24.");
            return Status::Continue;
        }
    } else if (!std::holds_alternative<ConfirmInput>(input)) {
        return Status::Continue;
    }
    context.editor.beginTransaction("INSUNITS");
    context.editor.setUnits(units);
    context.editor.commit();
    return Status::Finished;
}

Prompt InsunitsCommand::prompt() const {
    return {
        "Enter new value for INSUNITS <" + std::to_string(m_units) + ">:", {}, PromptKind::Text};
}

}  // namespace tg::cmd
