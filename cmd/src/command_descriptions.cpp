// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_descriptions.hpp"

namespace tg::cmd::detail {
namespace {

// The prompts of SaveCommand, shown by every command that saves.

PromptDescription saveNamePrompt() {
    return {"save-name", "Save drawing as <{default}>:", ValueType::FileSave, {}, true};
}

PromptDescription replacePrompt() {
    return {"replace",
            "A drawing with this name already exists. Do you want to replace it? [Yes/No] <No>:",
            ValueType::Keyword,
            {{"Yes", "Y"}, {"No", "N"}},
            true};
}

// The first prompt of NewCommand, when the drawing has unsaved changes.
PromptDescription saveChangesPrompt() {
    return {"save-changes",
            "Save changes to {default}? [Yes/No/Cancel] <Yes>:",
            ValueType::Keyword,
            {{"Yes", "Y"}, {"No", "N"}, {"Cancel", "C"}},
            true};
}

}  // namespace

CommandDescription arcDescription() {
    return {"ARC",
            {"A"},
            "Draws an arc through three points, or from a start point, a center and an end "
            "point.",
            false,
            {
                {"start-point", "Specify start point of arc:", ValueType::Point, {}, false},
                {"second-point",
                 "Specify second point of arc or [Center]:",
                 ValueType::Point,
                 {{"Center", "C"}},
                 false},
                {"center-point", "Specify center point of arc:", ValueType::Point, {}, false},
                {"end-point", "Specify end point of arc:", ValueType::Point, {}, false},
            }};
}

CommandDescription circleDescription() {
    return {
        "CIRCLE",
        {"C"},
        "Draws a circle from its center and radius or diameter, or through two or three points.",
        false,
        {
            {"center-point",
             "Specify center point for circle or [3P/2P]:",
             ValueType::Point,
             {{"3P", "3P"}, {"2P", "2P"}},
             false},
            {"radius",
             "Specify radius of circle or [Diameter]:",
             ValueType::Distance,
             {{"Diameter", "D"}},
             false},
            {"radius-default",
             "Specify radius of circle or [Diameter] <{default}>:",
             ValueType::Distance,
             {{"Diameter", "D"}},
             true},
            {"diameter", "Specify diameter of circle:", ValueType::Distance, {}, false},
            {"diameter-default",
             "Specify diameter of circle <{default}>:",
             ValueType::Distance,
             {},
             true},
            {"three-point-first", "Specify first point on circle:", ValueType::Point, {}, false},
            {"three-point-second", "Specify second point on circle:", ValueType::Point, {}, false},
            {"three-point-third", "Specify third point on circle:", ValueType::Point, {}, false},
            {"two-point-first",
             "Specify first end point of circle's diameter:",
             ValueType::Point,
             {},
             false},
            {"two-point-second",
             "Specify second end point of circle's diameter:",
             ValueType::Point,
             {},
             false},
        }};
}

CommandDescription eraseDescription() {
    return {"ERASE", {"E"}, "Removes the selected objects from the drawing.", true, {}};
}

CommandDescription insunitsDescription() {
    return {
        "INSUNITS",
        {},
        "Sets the units of the drawing, as an INSUNITS code from 0 to 24.",
        false,
        {
            {"units", "Enter new value for INSUNITS <{default}>:", ValueType::Integer, {}, true},
        }};
}

CommandDescription lineDescription() {
    return {"LINE",
            {"L"},
            "Draws a straight line segment, or a chain of them.",
            false,
            {
                {"first-point", "Specify first point:", ValueType::Point, {}, false},
                {"next-point",
                 "Specify next point or [Undo]:",
                 ValueType::Point,
                 {{"Undo", "U"}},
                 true},
                {"next-point-close",
                 "Specify next point or [Close/Undo]:",
                 ValueType::Point,
                 {{"Close", "C"}, {"Undo", "U"}},
                 true},
            }};
}

CommandDescription newDescription() {
    return {"NEW",
            {},
            "Starts a new empty drawing, offering to save the changes to the current one.",
            false,
            {saveChangesPrompt(), saveNamePrompt(), replacePrompt()}};
}

CommandDescription openDescription() {
    return {"OPEN",
            {},
            "Opens a drawing from a .cadx or .dxf file, offering to save the changes to the "
            "current one.",
            false,
            {
                saveChangesPrompt(),
                saveNamePrompt(),
                replacePrompt(),
                {"name", "Enter name of drawing to open:", ValueType::FileOpen, {}, false},
            }};
}

CommandDescription plineDescription() {
    return {"PLINE",
            {"PL"},
            "Draws a polyline of line and arc segments.",
            false,
            {
                {"start-point", "Specify start point:", ValueType::Point, {}, false},
                {"next-point",
                 "Specify next point or [Arc/Undo]:",
                 ValueType::Point,
                 {{"Arc", "A"}, {"Undo", "U"}},
                 true},
                {"next-point-close",
                 "Specify next point or [Arc/Close/Undo]:",
                 ValueType::Point,
                 {{"Arc", "A"}, {"Close", "C"}, {"Undo", "U"}},
                 true},
                {"arc-end-point",
                 "Specify end point of arc or [Line/Undo]:",
                 ValueType::Point,
                 {{"Line", "L"}, {"Undo", "U"}},
                 true},
                {"arc-end-point-close",
                 "Specify end point of arc or [CLose/Line/Undo]:",
                 ValueType::Point,
                 {{"CLose", "CL"}, {"Line", "L"}, {"Undo", "U"}},
                 true},
            }};
}

CommandDescription qsaveDescription() {
    return {"QSAVE",
            {},
            "Saves the drawing to its file, asking for a name if it has none.",
            false,
            {saveNamePrompt(), replacePrompt()}};
}

CommandDescription saveDescription() {
    return {"SAVE",
            {},
            "Saves the drawing to a file; another name than the current one saves a copy.",
            false,
            {saveNamePrompt(), replacePrompt()}};
}

CommandDescription saveasDescription() {
    return {"SAVEAS",
            {},
            "Saves the drawing to a file, whose name becomes the name of the drawing.",
            false,
            {saveNamePrompt(), replacePrompt()}};
}

CommandDescription textDescription() {
    return {
        "TEXT",
        {"DT", "DTEXT"},
        "Writes single-line texts, one for each line typed.",
        false,
        {
            {"start-point", "Specify start point of text:", ValueType::Point, {}, false},
            {"height", "Specify height <{default}>:", ValueType::Distance, {}, true},
            {"rotation", "Specify rotation angle of text <{default}>:", ValueType::Angle, {}, true},
            {"text", "Enter text:", ValueType::Text, {}, true},
        }};
}

}  // namespace tg::cmd::detail
