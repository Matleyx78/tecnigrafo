// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The locked position of an ATTDEF (docs/adr/0056, point 1;
// docs/tappe/0.8.1/PT15-attributi-core.md, point 1).

#include "tg/core/attribute_definition.hpp"

#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/text.hpp"
#include "tg/core/vec2.hpp"

#include <gtest/gtest.h>

#include <variant>

namespace tg::core {
namespace {

AttributeDefinition lockedDefinition() {
    AttributeDefinition definition;
    definition.text.position = {10, 20};
    definition.text.height = 2.5;
    definition.text.value = "S1";
    definition.tag = "SIGLA";
    definition.prompt = "Sigla";
    definition.lockPosition = true;
    return definition;
}

TEST(AttributeDefinition, ANewDefinitionIsNotLocked) {
    EXPECT_FALSE(AttributeDefinition{}.lockPosition);
    AttributeDefinition other = lockedDefinition();
    other.lockPosition = false;
    EXPECT_NE(other, lockedDefinition());
}

TEST(AttributeDefinition, TransformingAndMovingAGripKeepTheLockedPosition) {
    const AttributeDefinition definition = lockedDefinition();
    const TransformOutcome moved = transformedOf(
        Document{}, definition, Similarity::translation({1, 1}), MirrorText::Readable);
    ASSERT_TRUE(moved.entity.has_value());
    const auto result = std::get<AttributeDefinition>(moved.entity.value_or(Entity{}));
    EXPECT_EQ(result.text.position, (Vec2{11, 21}));
    EXPECT_TRUE(result.lockPosition);

    const GripResult grip = moveGripOf(Document{}, definition, 0, {0, 0});
    ASSERT_EQ(grip.outcome, GripOutcome::Moved);
    const auto gripped = std::get<AttributeDefinition>(grip.entity);
    EXPECT_EQ(gripped.text.position, (Vec2{0, 0}));
    EXPECT_TRUE(gripped.lockPosition);
}

}  // namespace
}  // namespace tg::core
