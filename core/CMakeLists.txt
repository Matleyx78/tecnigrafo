# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_core: geometry and drawing data model. Pure C++20, no third-party
# dependencies (docs/adr/0001).

add_library(tg_core
    src/arc.cpp
    src/arc_geometry.cpp
    src/circle.cpp
    src/color.cpp
    src/dashes.cpp
    src/lineweight.cpp
    src/linetype_pattern.cpp
    src/document.cpp
    src/document_query.cpp
    src/effective_properties.cpp
    src/layer.cpp
    src/line.cpp
    src/polyline.cpp
    src/proximity.cpp
    src/snap_points.cpp
    src/spatial_index.cpp
    src/text.cpp
    src/text_metrics.cpp
    src/transform2d.cpp
)
add_library(Tecnigrafo::core ALIAS tg_core)

target_include_directories(tg_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(tg_core PUBLIC cxx_std_20)
tg_configure_target(tg_core)

add_dependencies(tg_core tg_font_metrics)
target_include_directories(tg_core PRIVATE "${PROJECT_BINARY_DIR}/tools/generated")
