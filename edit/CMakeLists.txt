# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_edit: the only way to modify a document (Editor, transactions, undo).
# May depend on tg_core only, never on Qt (docs/adr/0001, docs/adr/0005).

add_library(tg_edit
    src/editor.cpp
)
add_library(Tecnigrafo::edit ALIAS tg_edit)

target_include_directories(tg_edit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_edit PUBLIC Tecnigrafo::core)
tg_configure_target(tg_edit)
