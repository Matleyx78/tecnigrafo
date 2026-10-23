// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"

#include <array>
#include <limits>
#include <utility>

namespace tg::io {
namespace {

class SkeletonBuilder {
public:
    explicit SkeletonBuilder(std::uint64_t first) : m_next(first) {}

    std::string handle() { return handleText(m_next++); }

    std::string table(std::string_view name, int count) {
        const auto id = handle();
        append(m_tables, {{0, "TABLE"},
                          {2, std::string{name}},
                          {5, id},
                          {330, "0"},
                          {100, "AcDbSymbolTable"},
                          {70, std::to_string(count)}});
        if (name == "DIMSTYLE") {
            append(m_tables, {{100, "AcDbDimStyleTable"}, {71, "1"}});
        }
        return id;
    }

    void endTable() { m_tables.push_back({0, "ENDTAB"}); }

    std::string record(std::string_view type, std::string_view subclass, std::string_view name,
                       const std::string& owner, const core::RawGroups& extra = {}) {
        const auto id = handle();
        const std::int16_t handleCode = type == "DIMSTYLE" ? 105 : 5;
        append(m_tables, {{0, std::string{type}},
                          {handleCode, id},
                          {330, owner},
                          {100, "AcDbSymbolTableRecord"},
                          {100, std::string{subclass}},
                          {2, std::string{name}},
                          {70, "0"}});
        append(m_tables, extra);
        return id;
    }

    void emptyTable(std::string_view name) {
        table(name, 0);
        endTable();
    }

    void block(const std::string& owner, std::string_view name, bool paper) {
        append(m_blocks, {{0, "BLOCK"}, {5, handle()}, {330, owner}, {100, "AcDbEntity"}});
        if (paper) {
            m_blocks.push_back({67, "1"});
        }
        append(m_blocks, {{8, "0"},
                          {100, "AcDbBlockBegin"},
                          {2, std::string{name}},
                          {70, "0"},
                          {10, "0.0"},
                          {20, "0.0"},
                          {30, "0.0"},
                          {3, std::string{name}},
                          {1, ""},
                          {0, "ENDBLK"},
                          {5, handle()},
                          {330, owner},
                          {100, "AcDbEntity"}});
        if (paper) {
            m_blocks.push_back({67, "1"});
        }
        append(m_blocks, {{8, "0"}, {100, "AcDbBlockEnd"}});
    }

    void dictionary(const std::string& id, const std::string& owner,
                    const core::RawGroups& entries) {
        append(m_objects,
               {{0, "DICTIONARY"}, {5, id}, {330, owner}, {100, "AcDbDictionary"}, {281, "1"}});
        append(m_objects, entries);
    }

    void layout(const std::string& id, const std::string& owner, const std::string& blockId,
                std::string_view name, bool paper) {
        append(m_objects, {{0, "LAYOUT"},
                           {5, id},
                           {102, "{ACAD_REACTORS"},
                           {330, owner},
                           {102, "}"},
                           {330, owner},
                           {100, "AcDbPlotSettings"},
                           {1, ""},
                           {2, ""},
                           {4, ""},
                           {6, ""},
                           {70, paper ? "0" : "1024"},
                           {100, "AcDbLayout"},
                           {1, std::string{name}},
                           {70, "1"},
                           {71, paper ? "1" : "0"},
                           {10, "0.0"},
                           {20, "0.0"},
                           {11, "420.0"},
                           {21, "297.0"},
                           {12, "0.0"},
                           {22, "0.0"},
                           {32, "0.0"},
                           {14, "0.0"},
                           {24, "0.0"},
                           {34, "0.0"},
                           {15, "0.0"},
                           {25, "0.0"},
                           {35, "0.0"},
                           {146, "0.0"},
                           {13, "0.0"},
                           {23, "0.0"},
                           {33, "0.0"},
                           {16, "1.0"},
                           {26, "0.0"},
                           {36, "0.0"},
                           {17, "0.0"},
                           {27, "1.0"},
                           {37, "0.0"},
                           {76, "0"},
                           {330, blockId}});
    }

    DxfSkeleton finish() {
        core::RawDrawing raw;
        raw.version = "AC1032";
        raw.codePage = "ANSI_1252";
        raw.sections = {{"HEADER", {}},
                        {"CLASSES", {}},
                        {"TABLES", std::move(m_tables)},
                        {"BLOCKS", std::move(m_blocks)},
                        {"ENTITIES", {}},
                        {"OBJECTS", std::move(m_objects)}};
        return {std::move(raw), m_next};
    }

private:
    static void append(core::RawGroups& target, const core::RawGroups& groups) {
        target.insert(target.end(), groups.begin(), groups.end());
    }

    std::uint64_t m_next;
    core::RawGroups m_tables;
    core::RawGroups m_blocks;
    core::RawGroups m_objects;
};

}  // namespace

Result<DxfSkeleton> makeSkeleton(std::uint64_t firstId) {
    // The skeleton uses fewer than 64 handles; leave space for subsequent edits.
    constexpr std::uint64_t kHandleBudget = 64;
    if (firstId == 0 || firstId > std::numeric_limits<std::uint64_t>::max() - kHandleBudget) {
        return Error{Error::Kind::Unsupported, 0, "not enough handles for the DXF skeleton"};
    }
    SkeletonBuilder builder{firstId};
    const auto root = builder.handle();
    const auto groups = builder.handle();
    const auto layouts = builder.handle();
    const auto modelLayout = builder.handle();
    const auto paperLayout = builder.handle();

    auto owner = builder.table("VPORT", 1);
    builder.record("VPORT", "AcDbViewportTableRecord", "*Active", owner,
                   {{10, "0.0"},
                    {20, "0.0"},
                    {11, "1.0"},
                    {21, "1.0"},
                    {12, "0.0"},
                    {22, "0.0"},
                    {16, "0.0"},
                    {26, "0.0"},
                    {36, "1.0"},
                    {40, "100.0"},
                    {41, "1.0"},
                    {42, "50.0"}});
    builder.endTable();
    owner = builder.table("LTYPE", 3);
    constexpr std::array<std::string_view, 3> kLineTypes{"ByBlock", "ByLayer", "Continuous"};
    for (const auto name : kLineTypes) {
        builder.record("LTYPE", "AcDbLinetypeTableRecord", name, owner,
                       {{3, ""}, {72, "65"}, {73, "0"}, {40, "0.0"}});
    }
    builder.endTable();
    owner = builder.table("LAYER", 1);
    builder.record("LAYER", "AcDbLayerTableRecord", "0", owner, {{62, "7"}, {6, "Continuous"}});
    builder.endTable();
    owner = builder.table("STYLE", 1);
    const auto style = builder.record(
        "STYLE", "AcDbTextStyleTableRecord", "Standard", owner,
        {{40, "0.0"}, {41, "1.0"}, {50, "0.0"}, {71, "0"}, {42, "2.5"}, {3, "txt"}, {4, ""}});
    builder.endTable();
    builder.emptyTable("VIEW");
    builder.emptyTable("UCS");
    owner = builder.table("APPID", 1);
    builder.record("APPID", "AcDbRegAppTableRecord", "ACAD", owner);
    builder.endTable();
    owner = builder.table("DIMSTYLE", 1);
    builder.record("DIMSTYLE", "AcDbDimStyleTableRecord", "Standard", owner,
                   {{40, "1.0"}, {41, "2.5"}, {140, "2.5"}, {340, style}});
    builder.endTable();
    owner = builder.table("BLOCK_RECORD", 2);
    const auto model = builder.record("BLOCK_RECORD", "AcDbBlockTableRecord", "*Model_Space", owner,
                                      {{340, modelLayout}, {280, "1"}, {281, "0"}});
    const auto paper = builder.record("BLOCK_RECORD", "AcDbBlockTableRecord", "*Paper_Space", owner,
                                      {{340, paperLayout}, {280, "1"}, {281, "0"}});
    builder.endTable();
    builder.block(model, "*Model_Space", false);
    builder.block(paper, "*Paper_Space", true);
    builder.dictionary(root, "0",
                       {{3, "ACAD_GROUP"}, {350, groups}, {3, "ACAD_LAYOUT"}, {350, layouts}});
    builder.dictionary(groups, root, {});
    builder.dictionary(layouts, root,
                       {{3, "Model"}, {350, modelLayout}, {3, "Layout1"}, {350, paperLayout}});
    builder.layout(modelLayout, layouts, model, "Model", false);
    builder.layout(paperLayout, layouts, paper, "Layout1", true);
    return builder.finish();
}

}  // namespace tg::io
