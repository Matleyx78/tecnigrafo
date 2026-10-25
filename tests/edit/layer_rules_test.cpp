// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/layer_rules.hpp"

#include <gtest/gtest.h>

namespace tg::edit {
namespace {

core::LayerId addNamed(core::Document& doc, const std::string& name) {
    core::Layer layer;
    layer.name = name;
    layer.linetype = doc.continuousLinetype();
    return doc.addLayer(layer);
}

TEST(LayerRules, NamesAndProtectedLayers) {
    core::Document doc = core::Document::newDrawing();
    const auto first = addNamed(doc, "Walls");
    const auto second = addNamed(doc, "Doors");
    EXPECT_EQ(checkNewLayer(doc, ""), LayerProblem::InvalidName);
    EXPECT_EQ(checkNewLayer(doc, "bad/name"), LayerProblem::InvalidName);
    EXPECT_EQ(checkNewLayer(doc, "wALLS"), LayerProblem::NameInUse);
    EXPECT_EQ(checkNewLayer(doc, "New"), LayerProblem::None);
    EXPECT_EQ(checkRename(doc, {}, "New"), LayerProblem::NotFound);
    EXPECT_EQ(checkRename(doc, doc.layerZero(), "New"), LayerProblem::LayerZero);
    EXPECT_EQ(checkRename(doc, first, " bad"), LayerProblem::InvalidName);
    EXPECT_EQ(checkRename(doc, first, "DOORS"), LayerProblem::NameInUse);
    EXPECT_EQ(checkRename(doc, first, "wALLS"), LayerProblem::None);
    EXPECT_EQ(checkRename(doc, second, "New"), LayerProblem::None);
    EXPECT_EQ(checkRemove(doc, {}), LayerProblem::NotFound);
    EXPECT_EQ(checkRemove(doc, doc.layerZero()), LayerProblem::LayerZero);
    EXPECT_EQ(checkRemove(doc, first), LayerProblem::None);
    EXPECT_EQ(checkFreeze(doc, {}), LayerProblem::NotFound);
    EXPECT_EQ(checkFreeze(doc, doc.layerZero()), LayerProblem::Current);
    EXPECT_EQ(checkFreeze(doc, first), LayerProblem::None);
    auto current = doc.current();
    current.layer = first;
    doc.setCurrent(current);
    EXPECT_EQ(checkRemove(doc, first), LayerProblem::Current);
    EXPECT_EQ(checkFreeze(doc, first), LayerProblem::Current);
    EXPECT_EQ(checkFreeze(doc, doc.layerZero()), LayerProblem::None);
}

TEST(LayerRules, HandledAndUnhandledEntitiesAndBlocksPreventRemoval) {
    core::Document doc = core::Document::newDrawing();
    const auto handled = addNamed(doc, "Handled");
    const auto raw = addNamed(doc, "Raw");
    const auto block = addNamed(doc, "Block");
    const auto other = addNamed(doc, "Other");
    const auto id = doc.add(core::Line{{0, 0}, {1, 0}});
    auto properties = doc.current();
    properties.layer = handled;
    doc.setProperties(id, properties);
    doc.insertRaw({core::EntityId{1000}, 2, "HATCH", {{8, "rAW"}}});
    core::RawDrawing drawing;
    drawing.sections = {{"BLOCKS", {{0, "LINE"}, {8, "bLOCK"}}}, {"OBJECTS", {{8, "Other"}}}};
    doc.setRaw(drawing);
    EXPECT_EQ(checkRemove(doc, handled), LayerProblem::InUse);
    EXPECT_EQ(checkRemove(doc, raw), LayerProblem::InUse);
    EXPECT_EQ(checkRemove(doc, block), LayerProblem::InUse);
    EXPECT_EQ(checkRemove(doc, other), LayerProblem::None);
    EXPECT_EQ(checkRename(doc, handled, "Renamed"), LayerProblem::None);
}

TEST(LayerRules, LayerGroupsAreUpdatedButOtherDataNeedsAWarning) {
    core::Document doc = core::Document::newDrawing();
    const auto id = addNamed(doc, "Walls");
    EXPECT_FALSE(namedInOtherUnhandledData(doc, {}));
    EXPECT_FALSE(namedInOtherUnhandledData(doc, id));
    doc.insertRaw({core::EntityId{1000}, 1, "HATCH", {{8, "wALLS"}}});
    core::RawDrawing raw;
    raw.sections = {{"BLOCKS", {{0, "LINE"}, {8, "WALLS"}}}};
    doc.setRaw(raw);
    EXPECT_FALSE(namedInOtherUnhandledData(doc, id));
    raw.sections.push_back({"OBJECTS", {{1, "walls"}}});
    doc.setRaw(raw);
    EXPECT_TRUE(namedInOtherUnhandledData(doc, id));
    raw.sections.pop_back();
    raw.sections[0].groups.push_back({1001, "APP"});
    raw.sections[0].groups.push_back({1000, "walls"});
    doc.setRaw(raw);
    EXPECT_TRUE(namedInOtherUnhandledData(doc, id));
}

TEST(LayerRules, FindsXdataAndEveryKindOfEntityAndTableExtra) {
    core::Document doc = core::Document::newDrawing();
    const auto id = addNamed(doc, "Walls");
    const auto entity = doc.add(core::Line{{0, 0}, {1, 0}});
    const auto original = *doc.find(entity);
    for (auto member : {&core::EntityExtras::beforeEntity, &core::EntityExtras::common,
                        &core::EntityExtras::specific, &core::EntityExtras::placement,
                        &core::EntityExtras::extended}) {
        auto record = doc.remove(entity);
        *record = original;
        record->extras.*member = {{1000, "wALLS"}};
        doc.restore(*record);
        EXPECT_TRUE(namedInOtherUnhandledData(doc, id));
    }
    doc.remove(entity);
    auto layer = *doc.layer(id);
    layer.extras = {{1000, "WALLS"}};
    doc.replaceLayer(layer);
    EXPECT_TRUE(namedInOtherUnhandledData(doc, id));
    layer.extras.clear();
    doc.replaceLayer(layer);
    core::Linetype linetype;
    linetype.name = "Custom";
    linetype.extras = {{1000, "Walls"}};
    doc.addLinetype(linetype);
    EXPECT_TRUE(namedInOtherUnhandledData(doc, id));
}

TEST(LayerRules, RawXdataReferencesDoNotCountAsEntityLayerGroups) {
    core::Document doc = core::Document::newDrawing();
    const auto id = addNamed(doc, "Walls");
    doc.insertRaw(
        {core::EntityId{1000}, 1, "HATCH", {{8, "0"}, {1001, "APP"}, {1000, "WALLS"}}});
    EXPECT_EQ(checkRemove(doc, id), LayerProblem::None);
    EXPECT_TRUE(namedInOtherUnhandledData(doc, id));
}

}  // namespace
}  // namespace tg::edit
