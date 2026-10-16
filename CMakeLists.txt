# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)

project(Tecnigrafo
    VERSION 0.1.0
    DESCRIPTION "Free 2D/3D CAD"
    HOMEPAGE_URL "https://github.com/tecnigrafo/tecnigrafo"
    LANGUAGES CXX)

option(TG_BUILD_APP "Build the Qt desktop application" ON)
option(TG_BUILD_TESTS "Build the unit tests" ON)
option(TG_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TG_ENABLE_CLANG_TIDY "Run clang-tidy while compiling" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(TecnigrafoWarnings)

if(TG_ENABLE_CLANG_TIDY)
    find_program(TG_CLANG_TIDY_EXE NAMES clang-tidy-18 clang-tidy REQUIRED)
    set(TG_CLANG_TIDY_COMMAND "${TG_CLANG_TIDY_EXE}")
endif()

if(TG_BUILD_TESTS)
    enable_testing()
endif()

# Layering (docs/adr/0001, docs/adr/0005):
#   core <- render <- app
#   core <- edit <- cmd <- app
# Only app may depend on Qt; the CMake targets enforce it.
add_subdirectory(core)
add_subdirectory(render)
add_subdirectory(edit)
add_subdirectory(cmd)

if(TG_BUILD_APP)
    add_subdirectory(app)
endif()

if(TG_BUILD_TESTS)
    add_subdirectory(tests)
endif()
