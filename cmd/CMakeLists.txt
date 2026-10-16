# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_cmd: interactive commands, command runner, typed coordinates.
# May depend on tg_core and tg_edit only, never on Qt (docs/adr/0001,
# docs/adr/0005).
#
# The module starts empty: it is an INTERFACE library until package 0.2-P4
# or 0.2-P5 adds the first sources, then it becomes a regular library.

add_library(tg_cmd INTERFACE)
add_library(Tecnigrafo::cmd ALIAS tg_cmd)

target_include_directories(tg_cmd INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_cmd INTERFACE Tecnigrafo::edit)
