// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
#include "dxf_tables.hpp"
#include "tg/core/start_handles.hpp"

#include <array>
#include <limits>
#include <unordered_set>
#include <utility>

namespace tg::io {
namespace {

namespace sh = core::start_handles;

/// The handles of the objects of the skeleton, in the order of
/// start_handles.hpp: the four records every drawing has (used only when
/// the document lacks them), then the tables, the other records, the
/// blocks and the objects.
constexpr std::array<std::uint64_t, 28> kFixedHandles{
    sh::kByBlock,          sh::kByLayer,           sh::kContinuous,       sh::kLayerZero,
    sh::kTableVport,       sh::kTableLtype,        sh::kTableLayer,       sh::kTableStyle,
    sh::kTableView,        sh::kTableUcs,          sh::kTableAppid,       sh::kTableDimstyle,
    sh::kTableBlockRecord, sh::kVportActive,       sh::kStyleStandard,    sh::kAppidAcad,
    sh::kDimstyleStandard, sh::kBlockRecordModel,  sh::kBlockRecordPaper, sh::kBlockModel,
    sh::kEndBlockModel,    sh::kBlockPaper,        sh::kEndBlockPaper,    sh::kDictionaryRoot,
    sh::kDictionaryGroups, sh::kDictionaryLayouts, sh::kLayoutModel,      sh::kLayoutPaper};

/// Positions in kFixedHandles.
enum Object : std::uint8_t {
    ByBlock,
    ByLayer,
    Continuous,
    LayerZero,
    TableVport,
    TableLtype,
    TableLayer,
    TableStyle,
    TableView,
    TableUcs,
    TableAppid,
    TableDimstyle,
    TableBlockRecord,
    VportActive,
    StyleStandard,
    AppidAcad,
    DimstyleStandard,
    BlockRecordModel,
    BlockRecordPaper,
    BlockModel,
    EndBlockModel,
    BlockPaper,
    EndBlockPaper,
    DictionaryRoot,
    DictionaryGroups,
    DictionaryLayouts,
    LayoutModel,
    LayoutPaper,
};

using Handles = std::array<std::uint64_t, kFixedHandles.size()>;

/// Which of the four records every drawing has the skeleton writes raw,
/// because the document lacks them.
struct Defaults {
    bool byBlock{true};
    bool byLayer{true};
    bool continuous{true};
    bool layerZero{true};
};

class SkeletonBuilder {
public:
    explicit SkeletonBuilder(const Handles& handles) : m_handles(handles) {}

    [[nodiscard]] std::string handle(Object object) const {
        return handleText(m_handles.at(object));
    }

    std::string table(Object object, std::string_view name, int count) {
        const auto id = handle(object);
        append(m_tables, {{0, "TABLE"},
                          {2, std::string{name}},
                          {5, id},
                          {330, "0"},
                          {100, "AcDbSymbolTable"},
                          {70, std::to_string(count)}});
        if (name == "DIMSTYLE") {
            append(m_tables, {{100, "AcDbDimStyleTableRecord"}, {71, "1"}});
        }
        return id;
    }

    void endTable() { m_tables.push_back({0, "ENDTAB"}); }

    std::string record(Object object, std::string_view type, std::string_view subclass,
                       std::string_view name, const std::string& owner,
                       const core::RawGroups& extra = {}) {
        const auto id = handle(object);
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

    void emptyTable(Object object, std::string_view name) {
        table(object, name, 0);
        endTable();
    }

    void block(Object begin, Object end, const std::string& owner, std::string_view name,
               bool paper) {
        append(m_blocks, {{0, "BLOCK"}, {5, handle(begin)}, {330, owner}, {100, "AcDbEntity"}});
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
                          {5, handle(end)},
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

    DxfSkeleton finish(std::uint64_t nextId) {
        core::RawDrawing raw;
        raw.version = "AC1032";
        raw.codePage = "ANSI_1252";
        raw.sections = {{"HEADER", {}},
                        {"CLASSES", {}},
                        {"TABLES", std::move(m_tables)},
                        {"BLOCKS", std::move(m_blocks)},
                        {"ENTITIES", {}},
                        {"OBJECTS", std::move(m_objects)}};
        return {std::move(raw), nextId};
    }

private:
    static void append(core::RawGroups& target, const core::RawGroups& groups) {
        target.insert(target.end(), groups.begin(), groups.end());
    }

    Handles m_handles;
    core::RawGroups m_tables;
    core::RawGroups m_blocks;
    core::RawGroups m_objects;
};

/// The skeleton with `handles`; the LTYPE and LAYER tables hold only the
/// `defaults` records, the writer adds those of the document.
DxfSkeleton buildSkeleton(const Handles& handles, const Defaults& defaults, std::uint64_t nextId) {
    SkeletonBuilder builder{handles};
    const auto root = builder.handle(DictionaryRoot);
    const auto groups = builder.handle(DictionaryGroups);
    const auto layouts = builder.handle(DictionaryLayouts);
    const auto modelLayout = builder.handle(LayoutModel);
    const auto paperLayout = builder.handle(LayoutPaper);

    auto owner = builder.table(TableVport, "VPORT", 1);
    builder.record(VportActive, "VPORT", "AcDbViewportTableRecord", "*Active", owner,
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
    const std::array<std::pair<Object, bool>, 3> linetypes{{{ByBlock, defaults.byBlock},
                                                            {ByLayer, defaults.byLayer},
                                                            {Continuous, defaults.continuous}}};
    const int linetypeCount = static_cast<int>(defaults.byBlock) +
                              static_cast<int>(defaults.byLayer) +
                              static_cast<int>(defaults.continuous);
    owner = builder.table(TableLtype, "LTYPE", linetypeCount);
    constexpr std::array<std::string_view, 3> kLinetypeNames{"ByBlock", "ByLayer", "Continuous"};
    for (std::size_t i = 0; i < linetypes.size(); ++i) {
        if (linetypes.at(i).second) {
            builder.record(linetypes.at(i).first, "LTYPE", "AcDbLinetypeTableRecord",
                           kLinetypeNames.at(i), owner,
                           {{3, ""}, {72, "65"}, {73, "0"}, {40, "0.0"}});
        }
    }
    builder.endTable();
    owner = builder.table(TableLayer, "LAYER", defaults.layerZero ? 1 : 0);
    if (defaults.layerZero) {
        builder.record(LayerZero, "LAYER", "AcDbLayerTableRecord", "0", owner,
                       {{62, "7"}, {6, "Continuous"}});
    }
    builder.endTable();
    owner = builder.table(TableStyle, "STYLE", 1);
    const auto style = builder.record(
        StyleStandard, "STYLE", "AcDbTextStyleTableRecord", "Standard", owner,
        {{40, "0.0"}, {41, "1.0"}, {50, "0.0"}, {71, "0"}, {42, "2.5"}, {3, "txt"}, {4, ""}});
    builder.endTable();
    builder.emptyTable(TableView, "VIEW");
    builder.emptyTable(TableUcs, "UCS");
    owner = builder.table(TableAppid, "APPID", 1);
    builder.record(AppidAcad, "APPID", "AcDbRegAppTableRecord", "ACAD", owner);
    builder.endTable();
    owner = builder.table(TableDimstyle, "DIMSTYLE", 1);
    builder.record(DimstyleStandard, "DIMSTYLE", "AcDbDimStyleTableRecord", "Standard", owner,
                   {{40, "1.0"}, {41, "2.5"}, {140, "2.5"}, {340, style}});
    builder.endTable();
    owner = builder.table(TableBlockRecord, "BLOCK_RECORD", 2);
    const auto model =
        builder.record(BlockRecordModel, "BLOCK_RECORD", "AcDbBlockTableRecord", "*Model_Space",
                       owner, {{340, modelLayout}, {280, "1"}, {281, "0"}});
    const auto paper =
        builder.record(BlockRecordPaper, "BLOCK_RECORD", "AcDbBlockTableRecord", "*Paper_Space",
                       owner, {{340, paperLayout}, {280, "1"}, {281, "0"}});
    builder.endTable();
    builder.block(BlockModel, EndBlockModel, model, "*Model_Space", false);
    builder.block(BlockPaper, EndBlockPaper, paper, "*Paper_Space", true);
    builder.dictionary(root, "0",
                       {{3, "ACAD_GROUP"}, {350, groups}, {3, "ACAD_LAYOUT"}, {350, layouts}});
    builder.dictionary(groups, root, {});
    builder.dictionary(layouts, root,
                       {{3, "Model"}, {350, modelLayout}, {3, "Layout1"}, {350, paperLayout}});
    builder.layout(modelLayout, layouts, model, "Model", false);
    builder.layout(paperLayout, layouts, paper, "Layout1", true);
    return builder.finish(nextId);
}

/// Handles from `first` up, one per object; std::nullopt if they run out.
std::optional<Handles> sequentialHandles(std::uint64_t first) {
    // Leave space for the handles the writer and later edits give.
    constexpr std::uint64_t kHandleBudget = 64;
    if (first == 0 || first > std::numeric_limits<std::uint64_t>::max() - kHandleBudget) {
        return std::nullopt;
    }
    Handles handles{};
    for (auto& handle : handles) {
        handle = first++;
    }
    return handles;
}

}  // namespace

Result<DxfSkeleton> makeSkeleton(std::uint64_t firstId) {
    const auto handles = sequentialHandles(firstId);
    if (!handles) {
        return Error{Error::Kind::Unsupported, 0, "not enough handles for the DXF skeleton"};
    }
    return buildSkeleton(*handles, {}, handles->back() + 1);
}

Result<DxfSkeleton> makeStartSkeleton(const core::Document& document) {
    Defaults defaults;
    defaults.byBlock = document.findLinetype("ByBlock") == nullptr;
    defaults.byLayer = document.findLinetype("ByLayer") == nullptr;
    defaults.continuous = document.findLinetype("Continuous") == nullptr;
    defaults.layerZero = document.findLayer("0") == nullptr;

    // The fixed handles when nothing of the document holds one of them, as
    // in a new drawing (docs/adr/0023, point 1).
    std::unordered_set<std::uint64_t> used;
    for (const auto& entity : document.entities()) {
        used.insert(entity.id.value);
    }
    for (const auto& entity : document.rawEntities()) {
        used.insert(entity.id.value);
    }
    for (const auto& layer : document.layers()) {
        used.insert(layer.id.value);
    }
    for (const auto& linetype : document.linetypes()) {
        used.insert(linetype.id.value);
    }
    const std::array<bool, 4> needed{defaults.byBlock, defaults.byLayer, defaults.continuous,
                                     defaults.layerZero};
    bool fixed = true;
    for (std::size_t i = 0; i < kFixedHandles.size(); ++i) {
        const bool wanted = i >= needed.size() || needed.at(i);
        fixed = fixed && !(wanted && used.contains(kFixedHandles.at(i)));
    }
    if (fixed) {
        return buildSkeleton(kFixedHandles, defaults,
                             std::max(document.nextId(), sh::kFirstEntity));
    }
    const auto handles = sequentialHandles(document.nextId());
    if (!handles) {
        return Error{Error::Kind::Unsupported, 0, "not enough handles for the DXF skeleton"};
    }
    return buildSkeleton(*handles, defaults, handles->back() + 1);
}

}  // namespace tg::io
