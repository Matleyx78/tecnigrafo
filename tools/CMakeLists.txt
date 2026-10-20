# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors

add_executable(fontmetrics fontmetrics/main.cpp)
tg_configure_target(fontmetrics)

set(TG_FONT_METRICS "${CMAKE_CURRENT_BINARY_DIR}/generated/font_metrics.inc")
add_custom_command(OUTPUT "${TG_FONT_METRICS}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND fontmetrics "${PROJECT_SOURCE_DIR}/resources/fonts/LiberationSans-Regular.ttf"
                        "${TG_FONT_METRICS}"
    DEPENDS fontmetrics "${PROJECT_SOURCE_DIR}/resources/fonts/LiberationSans-Regular.ttf"
    VERBATIM)
add_custom_target(tg_font_metrics DEPENDS "${TG_FONT_METRICS}")
