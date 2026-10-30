// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// list_commands and describe_command: the command descriptions of
// docs/adr/0026 as tools of the MCP server (docs/adr/0027, point 5).

#include "nlohmann_json/json.hpp"
#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/mcp/tool.hpp"

#include <cassert>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::mcp {

using nlohmann::json;

ToolResult ToolResult::success(std::string text) {
    return ToolResult{.text = std::move(text), .structured = std::nullopt, .isError = false};
}

ToolResult ToolResult::object(json value) {
    assert(value.is_object());
    std::string text = value.dump(-1, ' ', false, json::error_handler_t::replace);
    return ToolResult{.text = std::move(text), .structured = std::move(value), .isError = false};
}

ToolResult ToolResult::error(std::string message) {
    return ToolResult{.text = std::move(message), .structured = std::nullopt, .isError = true};
}

namespace {

/// The same names as `tecnigrafo-script describe` (script/tool/main.cpp).
std::string_view valueName(cmd::ValueType value) {
    using cmd::ValueType;
    switch (value) {
        case ValueType::Point:
            return "point";
        case ValueType::Distance:
            return "distance";
        case ValueType::Angle:
            return "angle";
        case ValueType::Integer:
            return "integer";
        case ValueType::Real:
            return "real";
        case ValueType::Text:
            return "text";
        case ValueType::Name:
            return "name";
        case ValueType::Color:
            return "color";
        case ValueType::Keyword:
            return "keyword";
        case ValueType::Object:
            return "object";
        case ValueType::Selection:
            return "selection";
        case ValueType::FileOpen:
            return "file-open";
        case ValueType::FileSave:
            return "file-save";
    }
    return "unknown";
}

/// A CommandDescription with the same field names.
json describeJson(const cmd::CommandDescription& description) {
    json prompts = json::array();
    for (const cmd::PromptDescription& prompt : description.prompts) {
        json keywords = json::array();
        for (const cmd::Keyword& keyword : prompt.keywords) {
            keywords.push_back(json{{"name", keyword.name}, {"shortcut", keyword.shortcut}});
        }
        prompts.push_back(json{{"id", prompt.id},
                               {"text", prompt.text},
                               {"value", valueName(prompt.value)},
                               {"keywords", std::move(keywords)},
                               {"acceptsEmpty", prompt.acceptsEmpty}});
    }
    return json{{"name", description.name},
                {"aliases", description.aliases},
                {"summary", description.summary},
                {"selectsFirst", description.selectsFirst},
                {"prompts", std::move(prompts)}};
}

/// The first argument not in `allowed`, or an empty string.
std::string unknownArgument(const json& arguments,
                            std::initializer_list<std::string_view> allowed) {
    for (const auto& [key, value] : arguments.items()) {
        bool known = false;
        for (const std::string_view name : allowed) {
            known = known || key == name;
        }
        if (!known) {
            return key;
        }
    }
    return {};
}

ToolResult unknownArgumentError(const std::string& tool, const std::string& argument) {
    return ToolResult::error("Unknown argument \"" + argument + "\" of " + tool + ".");
}

class ListCommands final : public Tool {
public:
    explicit ListCommands(std::shared_ptr<const cmd::CommandRegistry> registry)
        : m_registry(std::move(registry)) {}

    [[nodiscard]] std::string name() const override { return "list_commands"; }

    [[nodiscard]] std::string description() const override {
        return "Lists every command of Tecnigrafo with its name, aliases and summary, in "
               "alphabetical order of name.";
    }

    [[nodiscard]] json inputSchema() const override {
        return json{
            {"type", "object"}, {"properties", json::object()}, {"additionalProperties", false}};
    }

    [[nodiscard]] ToolResult call(const json& arguments) override {
        if (const std::string extra = unknownArgument(arguments, {}); !extra.empty()) {
            return unknownArgumentError(name(), extra);
        }
        json commands = json::array();
        for (const cmd::CommandDescription* description : m_registry->descriptions()) {
            commands.push_back(json{{"name", description->name},
                                    {"aliases", description->aliases},
                                    {"summary", description->summary}});
        }
        return ToolResult::object(json{{"commands", std::move(commands)}});
    }

private:
    std::shared_ptr<const cmd::CommandRegistry> m_registry;
};

class DescribeCommand final : public Tool {
public:
    explicit DescribeCommand(std::shared_ptr<const cmd::CommandRegistry> registry)
        : m_registry(std::move(registry)) {}

    [[nodiscard]] std::string name() const override { return "describe_command"; }

    [[nodiscard]] std::string description() const override {
        return "Describes one command, named by name or alias: its aliases, summary, whether it "
               "selects objects first, and every prompt it can show with the value type, the "
               "keywords and whether Enter alone is an answer.";
    }

    [[nodiscard]] json inputSchema() const override {
        return json{
            {"type", "object"},
            {"properties", json{{"name", json{{"type", "string"},
                                              {"description",
                                               "The name or an alias of the command, in any "
                                               "case: LINE, line, L."}}}}},
            {"required", json::array({"name"})},
            {"additionalProperties", false}};
    }

    [[nodiscard]] ToolResult call(const json& arguments) override {
        if (const std::string extra = unknownArgument(arguments, {"name"}); !extra.empty()) {
            return unknownArgumentError(name(), extra);
        }
        const auto command = arguments.find("name");
        if (command == arguments.end() || !command->is_string() ||
            command->get_ref<const std::string&>().empty()) {
            return ToolResult::error(
                "describe_command needs \"name\", the name or an alias of a command.");
        }
        const auto& requested = command->get_ref<const std::string&>();
        const cmd::CommandDescription* description = m_registry->describe(requested);
        if (description == nullptr) {
            return ToolResult::error("Unknown command \"" + requested + "\".");
        }
        return ToolResult::object(describeJson(*description));
    }

private:
    std::shared_ptr<const cmd::CommandRegistry> m_registry;
};

}  // namespace

std::vector<std::unique_ptr<Tool>> commandTools(
    std::shared_ptr<const cmd::CommandRegistry> registry) {
    assert(registry != nullptr);
    std::vector<std::unique_ptr<Tool>> tools;
    tools.push_back(std::make_unique<ListCommands>(registry));
    tools.push_back(std::make_unique<DescribeCommand>(std::move(registry)));
    return tools;
}

}  // namespace tg::mcp
