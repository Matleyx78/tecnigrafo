# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_render: view transforms and toolkit-independent scene drawing.
# May depend on tg_core only (docs/adr/0001).

add_library(tg_render
    src/linework.cpp
    src/scene_renderer.cpp
    src/viewport.cpp
)
add_library(Tecnigrafo::render ALIAS tg_render)

target_include_directories(tg_render PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_render PUBLIC Tecnigrafo::core)
tg_configure_target(tg_render)
