# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors

add_executable(fontmetrics fontmetrics/main.cpp)
tg_configure_target(fontmetrics)

# One generated table per face, each in units of its own file's cap height.
set(TG_FONT_METRICS "")
foreach(face Regular Bold Italic BoldItalic)
    string(TOLOWER "${face}" name)
    set(font "${PROJECT_SOURCE_DIR}/resources/fonts/LiberationSans-${face}.ttf")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/generated/font_metrics_${name}.inc")
    add_custom_command(OUTPUT "${output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
        COMMAND fontmetrics "${font}" "${output}"
        DEPENDS fontmetrics "${font}"
        VERBATIM)
    list(APPEND TG_FONT_METRICS "${output}")
endforeach()
add_custom_target(tg_font_metrics DEPENDS ${TG_FONT_METRICS})
