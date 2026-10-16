# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_edit: the only way to modify a document (Editor, transactions, undo).
# May depend on tg_core only, never on Qt (docs/adr/0001, docs/adr/0005).
#
# The module starts empty: it is an INTERFACE library until package 0.2-P3
# adds the first sources, then it becomes a regular library like tg_core.

add_library(tg_edit INTERFACE)
add_library(Tecnigrafo::edit ALIAS tg_edit)

target_include_directories(tg_edit INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_edit INTERFACE Tecnigrafo::core)
