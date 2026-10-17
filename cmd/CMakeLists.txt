# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_cmd: interactive commands, command runner, typed coordinates.
# May depend on tg_core and tg_edit only, never on Qt (docs/adr/0001,
# docs/adr/0005).

add_library(tg_cmd
    src/builtin_commands.cpp
    src/commands/pline_command.cpp
    src/command_registry.cpp
    src/command_runner.cpp
    src/coordinate_input.cpp
    src/selection_set.cpp
)
add_library(Tecnigrafo::cmd ALIAS tg_cmd)

target_include_directories(tg_cmd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_cmd PUBLIC Tecnigrafo::edit)
tg_configure_target(tg_cmd)
