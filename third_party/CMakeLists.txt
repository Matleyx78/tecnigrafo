# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Code of other projects, copied unmodified (docs/adr/0019). It does not call
# tg_configure_target: our warnings and clang-tidy would be noise we cannot
# fix. The sanitizer and fuzzing options are global, so they apply here too.
#
# solid-space is the exception: it is the kernel of Tecnigrafo, a git submodule
# of ours, and its targets follow our warning rules (docs/adr/0030, points 5-7).

# solid-space, the geometric kernel (docs/adr/0030). The submodule is
# initialised without --recursive: the 2D components need neither OCCT nor
# SolveSpace, which are its own submodules.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/solid-space/CMakeLists.txt")
    message(FATAL_ERROR
        "The solid-space submodule is missing. Run:\n"
        "    git submodule update --init third_party/solid-space\n"
        "(without --recursive) and configure again.")
endif()

# Only what core links is built: without the kernel and features components,
# which are the only ones that need OCCT. These are normal variables, so they
# also win over a value in the cache.
set(SOLID_SPACE_BUILD_KERNEL OFF)
# The same warning rules as our own code (docs/adr/0030, point 7). Nothing here
# passes -ffast-math or /fp:fast to its targets: robust and geom2d must give
# the same results, bit for bit, on every system.
set(SOLID_SPACE_WARNINGS_AS_ERRORS ${TG_WARNINGS_AS_ERRORS})
# Not SYSTEM: the headers are ours, with our warnings. EXCLUDE_FROM_ALL: a
# component is built only when a target of ours links it. Its tests are built
# only when it is the top-level project, so they are not built here.
add_subdirectory(solid-space EXCLUDE_FROM_ALL)

# The fuzz preset names only the C++ compiler. The C compiler must be the
# matching Clang, otherwise the global -fsanitize=fuzzer-no-link options reach
# a compiler that does not know them, and two sanitizer runtimes get mixed.
if(NOT CMAKE_C_COMPILER AND NOT DEFINED ENV{CC} AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_filename_component(tg_cxx_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(tg_cxx_name "${CMAKE_CXX_COMPILER}" NAME)
    string(REPLACE "clang++" "clang" tg_c_name "${tg_cxx_name}")
    if(NOT tg_c_name STREQUAL tg_cxx_name AND EXISTS "${tg_cxx_dir}/${tg_c_name}")
        set(CMAKE_C_COMPILER "${tg_cxx_dir}/${tg_c_name}" CACHE FILEPATH "C compiler")
    endif()
endif()
enable_language(C)

# zlib: deflate, inflate and CRC-32 only (third_party/zlib/README.md).
add_library(tg_zlib STATIC
    zlib/adler32.c
    zlib/crc32.c
    zlib/deflate.c
    zlib/inffast.c
    zlib/inflate.c
    zlib/inftrees.c
    zlib/trees.c
    zlib/zutil.c
)
# SYSTEM: the macros of zlib.h (inflateInit2, deflateInit2) expand C casts
# inside our files; they are zlib's, not ours to warn about.
target_include_directories(tg_zlib SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/zlib>)
# Z_PREFIX: every zlib symbol takes the prefix z_, so that ours never clash
# with the system zlib that Qt loads on Linux. ZLIB_CONST: input buffers are
# const, so that callers need no const_cast. Both are options of zlib.h.
target_compile_definitions(tg_zlib PUBLIC Z_PREFIX ZLIB_CONST)
if(MSVC)
    # The standard C functions zutil.c uses are "deprecated" only by MSVC.
    target_compile_definitions(tg_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
endif()
set_target_properties(tg_zlib PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS ON
    POSITION_INDEPENDENT_CODE ON)

# SARibbon: the GUI-only dependency approved by ADR 0009 (0.6-P3).
# Keep Qt optional for the script, sanitizer and fuzz builds without app.
if(TG_BUILD_APP)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)
    add_library(tg_saribbon STATIC saribbon/SARibbon.cpp saribbon/SARibbon.h)
    target_include_directories(tg_saribbon SYSTEM PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/saribbon>)
    target_link_libraries(tg_saribbon PUBLIC Qt6::Widgets)
    set_target_properties(tg_saribbon PROPERTIES
        AUTOMOC ON
        POSITION_INDEPENDENT_CODE ON)
    if(MSVC)
        target_compile_options(tg_saribbon PRIVATE /utf-8)
    endif()
endif()

# nlohmann/json: the JSON library of mcp only (docs/adr/0027, point 2;
# third_party/nlohmann_json/README.md). Header-only; included as
# "nlohmann_json/json.hpp" and as SYSTEM, so our warnings stay out of it.
add_library(tg_nlohmann_json INTERFACE)
target_include_directories(tg_nlohmann_json SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
