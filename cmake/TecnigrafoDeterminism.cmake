# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Floating-point determinism (docs/adr/0038, point 4). The verification cases
# compare numbers, and core computes on top of the results of solid-space: the
# same computation must give the same numbers on Linux, Windows and macOS. Every
# target of Tecnigrafo therefore compiles without contracting multiplications
# and additions (-ffp-contract=off with GCC and Clang, /fp:precise with MSVC),
# and the configuration stops when it finds an option that lets the compiler
# reorder or approximate floating-point expressions. It is the rule of
# solid-space (its ADR 0008, cmake/SolidSpaceDeterminism.cmake) with the same
# forbidden options, written again for Tecnigrafo.
#
#   tg_check_floating_point_flags()
#       Stops if CMAKE_CXX_FLAGS or any CMAKE_CXX_FLAGS_<CONFIG> (custom
#       configurations included) has a forbidden option.
#   tg_configure_determinism(<target>)
#       Stops if <target> or the global flags have a forbidden option, then
#       adds the deterministic option of the compiler to <target>.
#   tg_configure_determinism_in(<directory>)
#       Applies tg_configure_determinism to every compiled target defined in
#       <directory> and in its subdirectories, except third_party/ (code that
#       is not ours, as for the warnings) and anything inside the build tree
#       (dependencies fetched at configure time, such as GoogleTest).

include_guard(GLOBAL)

# The forbidden options, as in solid-space.
set(TG_FORBIDDEN_FLOATING_POINT_OPTIONS
    "-ffast-math|-Ofast|-funsafe-math-optimizations|/fp:fast|/fp:contract")

function(_tg_reject_floating_point_options what options)
    if(options MATCHES "${TG_FORBIDDEN_FLOATING_POINT_OPTIONS}")
        message(FATAL_ERROR
            "docs/adr/0038, point 4: ${what} has forbidden floating-point options "
            "(${CMAKE_MATCH_0}). Tecnigrafo refuses -ffast-math, -Ofast, "
            "-funsafe-math-optimizations, /fp:fast and /fp:contract.")
    endif()
endfunction()

function(tg_check_floating_point_flags)
    # Every configuration, the custom ones too.
    get_cmake_property(variables VARIABLES)
    foreach(variable IN ITEMS CMAKE_CXX_FLAGS LISTS variables)
        if(variable MATCHES "^CMAKE_CXX_FLAGS(_|$)")
            _tg_reject_floating_point_options("${variable}" "${${variable}}")
        endif()
    endforeach()
endfunction()

function(tg_configure_determinism target)
    tg_check_floating_point_flags()
    # Directory options are copied into COMPILE_OPTIONS when the target is
    # created; COMPILE_FLAGS is the old string form of the same thing.
    get_target_property(options ${target} COMPILE_OPTIONS)
    get_target_property(flags ${target} COMPILE_FLAGS)
    foreach(value IN ITEMS options flags)
        if(NOT ${value})
            set(${value} "")
        endif()
    endforeach()
    _tg_reject_floating_point_options("${target}" "${options};${flags}")

    if(MSVC)
        target_compile_options(${target} PRIVATE /fp:precise)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -ffp-contract=off)
    else()
        message(FATAL_ERROR
            "docs/adr/0038, point 4: no deterministic floating-point option is known "
            "for the compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

function(tg_configure_determinism_in directory)
    cmake_path(IS_PREFIX PROJECT_SOURCE_DIR "${directory}" NORMALIZE in_sources)
    cmake_path(APPEND PROJECT_SOURCE_DIR "third_party" OUTPUT_VARIABLE third_party)
    cmake_path(IS_PREFIX third_party "${directory}" NORMALIZE in_third_party)
    # The presets build inside the source tree (build<suffix>/<preset>), so a
    # dependency fetched there is "in the sources" too.
    set(in_build_tree OFF)
    if(NOT PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
        cmake_path(IS_PREFIX PROJECT_BINARY_DIR "${directory}" NORMALIZE in_build_tree)
    endif()
    if(DEFINED FETCHCONTENT_BASE_DIR)
        cmake_path(IS_PREFIX FETCHCONTENT_BASE_DIR "${directory}" NORMALIZE in_fetched)
        if(in_fetched)
            set(in_build_tree ON)
        endif()
    endif()
    if(NOT in_sources OR in_third_party OR in_build_tree)
        return()
    endif()

    get_property(directory_options DIRECTORY "${directory}" PROPERTY COMPILE_OPTIONS)
    _tg_reject_floating_point_options("the directory ${directory}" "${directory_options}")

    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        if(type MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
            tg_configure_determinism(${target})
        endif()
    endforeach()

    get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(subdirectory IN LISTS subdirectories)
        tg_configure_determinism_in("${subdirectory}")
    endforeach()
endfunction()
