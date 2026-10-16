# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_core: geometry and drawing data model. Pure C++20, no third-party
# dependencies (docs/adr/0001).

add_library(tg_core
    src/document.cpp
    src/polyline.cpp
    src/proximity.cpp
    src/spatial_index.cpp
    src/transform2d.cpp
)
add_library(Tecnigrafo::core ALIAS tg_core)

target_include_directories(tg_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(tg_core PUBLIC cxx_std_20)
tg_configure_target(tg_core)
