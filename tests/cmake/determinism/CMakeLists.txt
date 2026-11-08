# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# A small project configured by tests/cmake/determinism_test.cmake to check
# cmake/TecnigrafoDeterminism.cmake (docs/adr/0038, point 4). It is only
# configured, never built. The probe variables put a forbidden option where
# the real project could have one:
#   TG_PROBE_FLAGS_VARIABLE / TG_PROBE_FLAGS_VALUE  a CMAKE_CXX_FLAGS* variable
#   TG_PROBE_TARGET_OPTION                          an option of probe_library
#   TG_PROBE_DIRECTORY_OPTION                       a directory option added
#                                                   after probe_program exists
# Without them, the configuration ends with "determinism probe: ok" only if
# the deterministic option reached every target of ours and no target of
# third_party/.

cmake_minimum_required(VERSION 3.24)
project(TgDeterminismProbe LANGUAGES CXX)

include("${TG_DETERMINISM_MODULE}")

if(TG_PROBE_FLAGS_VARIABLE)
    set(${TG_PROBE_FLAGS_VARIABLE} "${TG_PROBE_FLAGS_VALUE}")
endif()
tg_check_floating_point_flags()

add_library(probe_library STATIC probe.cpp)
if(TG_PROBE_TARGET_OPTION)
    target_compile_options(probe_library PRIVATE "${TG_PROBE_TARGET_OPTION}")
endif()
add_subdirectory(programma)
add_subdirectory(third_party)

tg_configure_determinism_in("${PROJECT_SOURCE_DIR}")

if(MSVC)
    set(expected /fp:precise)
else()
    set(expected -ffp-contract=off)
endif()
foreach(target IN ITEMS probe_library probe_program)
    get_target_property(options ${target} COMPILE_OPTIONS)
    if(NOT expected IN_LIST options)
        message(FATAL_ERROR "determinism probe: ${target} lacks ${expected}: ${options}")
    endif()
endforeach()
get_target_property(options probe_third_party COMPILE_OPTIONS)
if(expected IN_LIST options)
    message(FATAL_ERROR "determinism probe: third_party/ got ${expected}")
endif()
message(STATUS "determinism probe: ok")
