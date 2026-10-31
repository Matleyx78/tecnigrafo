// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_registry.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_description.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

/// A command that is never run: the registry only creates it.
class IdleCommand final : public Command {
public:
    void start(CommandContext& /*context*/) override {}

    Status handle(CommandContext& /*context*/, const Input& /*input*/) override {
        return Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }
};

CommandRegistry::Factory idleFactory() {
    return [] {
        return std::make_unique<IdleCommand>();
    };
}

CommandDescription description(std::string name, std::vector<std::string> aliases = {}) {
    return {std::move(name),
            std::move(aliases),
            "Does nothing.",
            false,
            {{"next-point",
              "Specify next point or [Close/Undo]:",
              ValueType::Point,
              {{"Close", "C"}, {"Undo", "U"}},
              true}}};
}

PromptDescription described(std::string text, std::vector<Keyword> keywords = {},
                            ValueType value = ValueType::Point) {
    return {"id", std::move(text), value, std::move(keywords), false};
}

Prompt shown(std::string text, std::vector<Keyword> keywords = {},
             PromptKind kind = PromptKind::Point) {
    return {std::move(text), std::move(keywords), kind};
}

// --- matches() ----------------------------------------------------------------

TEST(PromptMatches, EqualTextWithoutDefault) {
    EXPECT_TRUE(matches(described("Specify first point:"), shown("Specify first point:")));
    EXPECT_FALSE(matches(described("Specify first point:"), shown("Specify first point")));
    EXPECT_FALSE(matches(described("Specify first point:"), shown("specify first point:")));
    EXPECT_FALSE(matches(described("Specify first point:"), shown("Specify first point: ")));
}

TEST(PromptMatches, DefaultAtTheStartInTheMiddleAndTwice) {
    EXPECT_TRUE(matches(described("{default} is the name:"), shown("Drawing1.cadx is the name:")));
    EXPECT_TRUE(matches(described("Specify height <{default}>:"), shown("Specify height <2.5>:")));
    EXPECT_TRUE(
        matches(described("From <{default}> to <{default}>:"), shown("From <1,2> to <3,4>:")));
    EXPECT_TRUE(matches(described("Value {default}"), shown("Value 12")));
    // A value may contain the text that follows "{default}".
    EXPECT_TRUE(matches(described("Save changes to {default}?"), shown("Save changes to a?b?")));
    EXPECT_FALSE(matches(described("From <{default}> to <{default}>:"), shown("From <1,2> to:")));
}

TEST(PromptMatches, DefaultNeedsANonEmptyValueWithoutAngleBrackets) {
    EXPECT_FALSE(matches(described("Specify height <{default}>:"), shown("Specify height <>:")));
    EXPECT_FALSE(matches(described("Specify height <{default}>:"), shown("Specify height <<2>:")));
    EXPECT_FALSE(matches(described("Specify height <{default}>:"), shown("Specify height <2<>:")));
    EXPECT_FALSE(matches(described("Specify height <{default}>:"), shown("Specify height:")));
}

TEST(PromptMatches, KeywordsMustBeEqualAndInTheSameOrder) {
    const std::vector<Keyword> keywords{{"Close", "C"}, {"Undo", "U"}};
    const PromptDescription description = described("Next or [Close/Undo]:", keywords);
    EXPECT_TRUE(matches(description, shown("Next or [Close/Undo]:", keywords)));
    EXPECT_FALSE(
        matches(description, shown("Next or [Close/Undo]:", {{"Undo", "U"}, {"Close", "C"}})));
    EXPECT_FALSE(
        matches(description, shown("Next or [Close/Undo]:", {{"Close", "CL"}, {"Undo", "U"}})));
    EXPECT_FALSE(matches(description, shown("Next or [Close/Undo]:", {{"Close", "C"}})));
    EXPECT_FALSE(matches(description, shown("Next or [Close/Undo]:")));
}

TEST(PromptMatches, KindsAndValueTypes) {
    const auto compatible = [](PromptKind kind, ValueType value) {
        return matches(described("Q:", {}, value), shown("Q:", {}, kind));
    };
    const std::vector<ValueType> all{ValueType::Point,    ValueType::Distance, ValueType::Angle,
                                     ValueType::Integer,  ValueType::Real,     ValueType::Text,
                                     ValueType::Name,     ValueType::Color,    ValueType::Keyword,
                                     ValueType::FileOpen, ValueType::FileSave};
    for (const ValueType value : all) {
        const auto index = static_cast<int>(value);
        EXPECT_EQ(compatible(PromptKind::Point, value),
                  value == ValueType::Point || value == ValueType::Keyword)
            << index;
        EXPECT_EQ(compatible(PromptKind::Distance, value),
                  value == ValueType::Distance || value == ValueType::Keyword)
            << index;
        EXPECT_EQ(compatible(PromptKind::Angle, value),
                  value == ValueType::Angle || value == ValueType::Keyword)
            << index;
        EXPECT_EQ(compatible(PromptKind::Text, value), value != ValueType::Point &&
                                                           value != ValueType::Distance &&
                                                           value != ValueType::Angle)
            << index;
    }
}

// --- Validity -----------------------------------------------------------------

TEST(CommandDescriptionValidity, ChecksNamesSummaryIdsAndKeywords) {
    EXPECT_TRUE(isValid(description("LINE")));
    EXPECT_TRUE(isValid({"RIBBON", {}, "Shows the ribbon.", false, {}}));

    const CommandDescription noName = description("");
    EXPECT_FALSE(isValid(noName));

    CommandDescription noSummary = description("LINE");
    noSummary.summary.clear();
    EXPECT_FALSE(isValid(noSummary));

    CommandDescription twice = description("LINE");
    twice.prompts.push_back(twice.prompts.front());
    EXPECT_FALSE(isValid(twice));

    CommandDescription noId = description("LINE");
    noId.prompts.front().id.clear();
    EXPECT_FALSE(isValid(noId));

    CommandDescription outsideBrackets = description("LINE");
    outsideBrackets.prompts.front().text = "Specify next point or Close [Undo]:";
    EXPECT_FALSE(isValid(outsideBrackets));

    CommandDescription unknownKeyword = description("LINE");
    unknownKeyword.prompts.front().keywords.push_back({"Arc", "A"});
    EXPECT_FALSE(isValid(unknownKeyword));
}

// --- Registry -----------------------------------------------------------------

TEST(CommandRegistry, FindsNamesAndAliasesIgnoringCase) {
    CommandRegistry registry;
    registry.add(description("PLINE", {"PL"}), idleFactory());

    EXPECT_EQ(registry.canonicalName("PLINE"), "PLINE");
    EXPECT_EQ(registry.canonicalName("pline"), "PLINE");
    EXPECT_EQ(registry.canonicalName("Pl"), "PLINE");
    EXPECT_EQ(registry.canonicalName("PLIN"), std::nullopt);
    EXPECT_EQ(registry.canonicalName(""), std::nullopt);

    EXPECT_NE(registry.create("pl"), nullptr);
    EXPECT_EQ(registry.create("LINE"), nullptr);
}

TEST(CommandRegistry, CreatesANewInstanceEveryTime) {
    CommandRegistry registry;
    registry.add(description("FAKE"), idleFactory());

    const std::unique_ptr<Command> first = registry.create("FAKE");
    const std::unique_ptr<Command> second = registry.create("FAKE");
    ASSERT_NE(first, nullptr);
    EXPECT_NE(first, second);
}

TEST(CommandRegistry, DescribesByNameOrAliasInAnyCase) {
    CommandRegistry registry;
    registry.add(description("PLINE", {"PL"}), idleFactory());
    const CommandDescription* byName = registry.describe("PLINE");
    ASSERT_NE(byName, nullptr);
    EXPECT_EQ(*byName, description("PLINE", {"PL"}));
    EXPECT_EQ(registry.describe("pl"), byName);
    EXPECT_EQ(registry.describe("Pline"), byName);
    EXPECT_EQ(registry.describe("LINE"), nullptr);
    EXPECT_EQ(registry.describe(""), nullptr);
}

TEST(CommandRegistry, ListsDescriptionsInAlphabeticalOrder) {
    CommandRegistry registry;
    EXPECT_TRUE(registry.descriptions().empty());
    registry.add(description("PLINE"), idleFactory());
    registry.add(description("ARC"), idleFactory());
    registry.add(description("LINE"), idleFactory());
    const CommandDescription* arc = registry.describe("ARC");

    std::vector<std::string> names;
    for (const CommandDescription* entry : registry.descriptions()) {
        names.push_back(entry->name);
    }
    EXPECT_EQ(names, (std::vector<std::string>{"ARC", "LINE", "PLINE"}));
    registry.add(description("CIRCLE"), idleFactory());
    EXPECT_EQ(registry.describe("ARC"), arc);  // addresses survive additions
}

TEST(CommandRegistryDeathTest, PreconditionsAreChecked) {
#ifdef NDEBUG
    GTEST_SKIP() << "preconditions are checked with assert, disabled in this build";
#endif
    CommandRegistry registry;
    registry.add(description("PLINE", {"PL"}), idleFactory());
    EXPECT_DEATH(registry.add(description(""), idleFactory()), "invalid command description");
    CommandDescription noSummary = description("LINE");
    noSummary.summary.clear();
    EXPECT_DEATH(registry.add(noSummary, idleFactory()), "invalid command description");
    EXPECT_DEATH(registry.add(description("LINE"), {}), "needs a factory");
    EXPECT_DEATH(registry.add(description("pline"), idleFactory()), "name already registered");
    EXPECT_DEATH(registry.add(description("PL"), idleFactory()), "name already registered");
    EXPECT_DEATH(registry.add(description("LINE", {"PL"}), idleFactory()),
                 "alias empty or already registered");
    EXPECT_DEATH(registry.add(description("LINE", {"PLINE"}), idleFactory()),
                 "alias empty or already registered");
    EXPECT_DEATH(registry.add(description("LINE", {""}), idleFactory()),
                 "alias empty or already registered");
}

// --- Built-in commands --------------------------------------------------------

TEST(BuiltinCommands, EveryCommandHasAValidDescription) {
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    std::string names;
    for (const CommandDescription* entry : registry.descriptions()) {
        if (!names.empty()) {
            names += ' ';
        }
        names += entry->name;
        EXPECT_TRUE(isValid(*entry)) << entry->name;
        EXPECT_TRUE(entry->summary.ends_with('.')) << entry->name;
        EXPECT_NE(registry.create(entry->name), nullptr) << entry->name;
    }
    const std::string expected =
        "-LAYER -LINETYPE ARC CELTSCALE CHPROP CIRCLE CLAYER COLOR COPY ERASE EXPLODE INSUNITS "
        "LINE LTSCALE LWDISPLAY LWEIGHT MOVE NEW OPEN PLINE QSAVE SAVE SAVEAS TEXT";
    EXPECT_EQ(names, expected);
    EXPECT_TRUE(registry.describe("ERASE")->selectsFirst);
    EXPECT_EQ(registry.describe("DTEXT")->name, "TEXT");
}

}  // namespace
}  // namespace tg::cmd
