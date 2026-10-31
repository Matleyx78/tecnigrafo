// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_descriptions.hpp"

#include "tg/cmd/commands/aid_commands.hpp"
#include "tg/cmd/commands/chprop_command.hpp"
#include "tg/cmd/commands/explode_command.hpp"
#include "tg/cmd/commands/layer_commands.hpp"
#include "tg/cmd/commands/mirror_commands.hpp"
#include "tg/cmd/commands/move_copy_commands.hpp"
#include "tg/cmd/commands/property_commands.hpp"
#include "tg/cmd/commands/rotate_scale_commands.hpp"

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

// The prompts that MOVE and COPY share (docs/adr/0032, point 9: every prompt of
// both sequences is described).

PromptDescription displacementBasePrompt() {
    return {"base-point",
            "Specify base point or [Displacement] <Displacement>:",
            ValueType::Point,
            {{"Displacement", "D"}},
            true};
}

PromptDescription displacementSecondPrompt() {
    return {"second-point",
            "Specify second point or <use first point as displacement>:",
            ValueType::Point,
            {},
            true};
}

PromptDescription displacementPrompt() {
    return {"displacement", "Specify displacement <{default}>:", ValueType::Point, {}, true};
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
                {"first-point", "Specify first point:", ValueType::Point, {}, true},
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

namespace tg::cmd {

CommandDescription LayerCommand::description() {
    return {
        "-LAYER",
        {"-LA"},
        "Creates and renames layers, sets the current layer and changes layer properties.",
        false,
        {
            {"option",
             "Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/Plot/Freeze/Thaw/"
             "LOck/Unlock]:",
             ValueType::Keyword,
             {
                 {"?", "?"},
                 {"Make", "M"},
                 {"Set", "S"},
                 {"New", "N"},
                 {"Rename", "R"},
                 {"ON", "ON"},
                 {"OFF", "OFF"},
                 {"Color", "C"},
                 {"Ltype", "LT"},
                 {"LWeight", "LW"},
                 {"Plot", "P"},
                 {"Freeze", "F"},
                 {"Thaw", "T"},
                 {"LOck", "LO"},
                 {"Unlock", "U"},
             },
             true},
            {"list", "Enter layer name(s) to list <*>:", ValueType::Name, {}, true},
            {"make",
             "Enter name for new layer (becomes the current layer) <{default}>:",
             ValueType::Name,
             {},
             true},
            {"set", "Enter layer name to make current:", ValueType::Name, {}, true},
            {"new", "Enter name list for new layer(s):", ValueType::Name, {}, true},
            {"rename-old", "Enter old layer name:", ValueType::Name, {}, true},
            {"rename-new", "Enter new layer name:", ValueType::Name, {}, true},
            {"on", "Enter name list of layer(s) to turn on:", ValueType::Name, {}, true},
            {"off", "Enter name list of layer(s) to turn off:", ValueType::Name, {}, true},
            {"confirm-off",
             "Really want layer {default} (the CURRENT layer) off? [Yes/No] <No>:",
             ValueType::Keyword,
             {{"Yes", "Y"}, {"No", "N"}},
             true},
            {"color", "New color [Truecolor]:", ValueType::Color, {{"Truecolor", "T"}}, false},
            {"rgb", "Red, Green, Blue:", ValueType::Color, {}, false},
            {"color-layers",
             "Enter name list of layer(s) for color {default} <{default}>:",
             ValueType::Name,
             {},
             true},
            {"linetype", "Enter a loaded linetype name <Continuous>:", ValueType::Name, {}, true},
            {"linetype-layers",
             "Enter name list of layer(s) for linetype \"{default}\" <{default}>:",
             ValueType::Name,
             {},
             true},
            {"lineweight", "Enter lineweight (0.0mm - 2.11mm):", ValueType::Real, {}, false},
            {"lineweight-layers",
             "Enter name list of layer(s) for lineweight {default} <{default}>:",
             ValueType::Name,
             {},
             true},
            {"plot",
             "Enter a plotting preference [Plot/No plot] <Plot>:",
             ValueType::Keyword,
             {{"Plot", "P"}, {"No plot", "N"}},
             true},
            {"plot-layers",
             "Enter name list of layer(s) for this plot preference <{default}>:",
             ValueType::Name,
             {},
             true},
            {"freeze", "Enter name list of layer(s) to freeze:", ValueType::Name, {}, true},
            {"thaw", "Enter name list of layer(s) to thaw:", ValueType::Name, {}, true},
            {"lock", "Enter name list of layer(s) to lock:", ValueType::Name, {}, true},
            {"unlock", "Enter name list of layer(s) to unlock:", ValueType::Name, {}, true},
        }};
}

CommandDescription ClayerCommand::description() {
    return {"CLAYER",
            {},
            "Sets an existing, thawed layer as the current layer.",
            false,
            {{"layer", "Enter new value for CLAYER <\"{default}\">:", ValueType::Name, {}, true}}};
}

CommandDescription ChpropCommand::description() {
    return {
        "CHPROP",
        {},
        "Changes the color, layer, linetype, linetype scale or lineweight of the selected "
        "objects.",
        true,
        {{"property",
          "Enter property to change [Color/LAyer/LType/ltScale/LWeight]:",
          ValueType::Keyword,
          {{"Color", "C"}, {"LAyer", "LA"}, {"LType", "LT"}, {"ltScale", "LTS"}, {"LWeight", "LW"}},
          true},
         {"color",
          "New color [Truecolor] <{default}>:",
          ValueType::Color,
          {{"Truecolor", "T"}},
          true},
         {"rgb", "Red, Green, Blue:", ValueType::Color, {}, true},
         {"layer", "Enter new layer name <{default}>:", ValueType::Name, {}, true},
         {"linetype", "Enter new linetype name <{default}>:", ValueType::Name, {}, true},
         {"linetype-scale", "Specify new linetype scale <{default}>:", ValueType::Real, {}, true},
         {"lineweight", "Enter new lineweight <{default}>:", ValueType::Real, {}, true}}};
}

CommandDescription ColorCommand::description() {
    return {"COLOR",
            {"COL", "COLOUR", "-COLOR"},
            "Sets the color of new objects.",
            false,
            {{"color",
              "Enter default object color [Truecolor] <{default}>:",
              ValueType::Color,
              {{"Truecolor", "T"}},
              true},
             {"rgb", "Red, Green, Blue:", ValueType::Color, {}, true}}};
}

CommandDescription CopyCommand::description() {
    return {"COPY",
            {"CO", "CP"},
            "Copies the selected objects by a displacement, as many times as asked.",
            true,
            {detail::displacementBasePrompt(),
             detail::displacementSecondPrompt(),
             detail::displacementPrompt(),
             {"next-copy",
              "Specify second point or [Exit/Undo] <Exit>:",
              ValueType::Point,
              {{"Exit", "E"}, {"Undo", "U"}},
              true}}};
}

CommandDescription ExplodeCommand::description() {
    return {"EXPLODE", {"X"}, "Breaks the selected polylines into lines and arcs.", true, {}};
}

CommandDescription MoveCommand::description() {
    return {"MOVE",
            {"M"},
            "Moves the selected objects by a displacement.",
            true,
            {detail::displacementBasePrompt(), detail::displacementSecondPrompt(),
             detail::displacementPrompt()}};
}

CommandDescription RotateCommand::description() {
    return {"ROTATE",
            {"RO"},
            "Rotates the selected objects about a base point.",
            true,
            {{"base-point", "Specify base point:", ValueType::Point, {}, false},
             {"rotation-angle",
              "Specify rotation angle or [Copy/Reference] <{default}>:",
              ValueType::Angle,
              {{"Copy", "C"}, {"Reference", "R"}},
              true},
             {"reference-angle", "Specify the reference angle <0>:", ValueType::Angle, {}, true},
             // Also the second point of the Points option of the new angle.
             {"second-point", "Specify second point:", ValueType::Point, {}, false},
             {"new-angle",
              "Specify the new angle or [Points] <0>:",
              ValueType::Angle,
              {{"Points", "P"}},
              true},
             {"first-point", "Specify first point:", ValueType::Point, {}, false}}};
}

CommandDescription ScaleObjectsCommand::description() {
    return {
        "SCALE",
        {"SC"},
        "Scales the selected objects about a base point.",
        true,
        {{"base-point", "Specify base point:", ValueType::Point, {}, false},
         {"scale-factor",
          "Specify scale factor or [Copy/Reference]:",
          ValueType::Real,
          {{"Copy", "C"}, {"Reference", "R"}},
          false},
         {"reference-length", "Specify reference length <1.0000>:", ValueType::Distance, {}, true},
         // Also the second point of the Points option of the new length.
         {"second-point", "Specify second point:", ValueType::Point, {}, false},
         {"new-length",
          "Specify new length or [Points] <1.0000>:",
          ValueType::Distance,
          {{"Points", "P"}},
          true},
         {"first-point", "Specify first point:", ValueType::Point, {}, false}}};
}

CommandDescription MirrorCommand::description() {
    return {"MIRROR",
            {"MI"},
            "Mirrors the selected objects about a line, keeping or replacing the source.",
            true,
            {{"first-point", "Specify first point of mirror line:", ValueType::Point, {}, false},
             {"second-point", "Specify second point of mirror line:", ValueType::Point, {}, false},
             {"erase",
              "Erase source objects? [Yes/No] <No>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true}}};
}

CommandDescription MirrtextCommand::description() {
    return {"MIRRTEXT",
            {},
            "Sets whether MIRROR reflects text (1) or keeps it readable (0).",
            false,
            {{"value", "Enter new value for MIRRTEXT <{default}>:", ValueType::Integer, {}, true}}};
}

CommandDescription PolarangCommand::description() {
    return {"POLARANG",
            {},
            "Sets the angle increment of polar tracking, in degrees.",
            false,
            {{"value", "Enter new value for POLARANG <{default}>:", ValueType::Real, {}, true}}};
}

CommandDescription LinetypeCommand::description() {
    return {"-LINETYPE",
            {"LINETYPE", "LT", "LTYPE", "-LT"},
            "Loads standard linetypes and sets the linetype of new objects.",
            false,
            {{"option",
              "Enter an option [?/Load/Set]:",
              ValueType::Keyword,
              {{"?", "?"}, {"Load", "L"}, {"Set", "S"}},
              true},
             {"load", "Enter linetype(s) to load:", ValueType::Name, {}, true},
             {"set",
              "Specify linetype name or [?] <{default}>:",
              ValueType::Name,
              {{"?", "?"}},
              true}}};
}

CommandDescription LwdisplayCommand::description() {
    return {"LWDISPLAY",
            {},
            "Turns the display of lineweights on or off.",
            false,
            {{"display", "Enter new value for LWDISPLAY <{default}>:", ValueType::Text, {}, true}}};
}

CommandDescription LweightCommand::description() {
    return {"LWEIGHT",
            {"LW", "LINEWEIGHT", "-LWEIGHT"},
            "Sets the lineweight of new objects.",
            false,
            {{"weight",
              "Enter default lineweight for new objects or [?]:",
              ValueType::Real,
              {{"?", "?"}},
              true}}};
}

CommandDescription ScaleCommand::description(Variable variable) {
    if (variable == Variable::Global) {
        CommandDescription result;
        result.name = "LTSCALE";
        result.aliases = {"LTS"};
        result.summary = "Sets the global linetype scale factor.";
        result.prompts.push_back(
            {"scale", "Enter new linetype scale factor <{default}>:", ValueType::Real, {}, true});
        return result;
    }
    return {"CELTSCALE",
            {},
            "Sets the linetype scale factor of new objects.",
            false,
            {{"scale", "Enter new value for CELTSCALE <{default}>:", ValueType::Real, {}, true}}};
}

}  // namespace tg::cmd
