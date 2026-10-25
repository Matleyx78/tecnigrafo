# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Code of other projects, copied unmodified (docs/adr/0019). It does not call
# tg_configure_target: our warnings and clang-tidy would be noise we cannot
# fix. The sanitizer and fuzzing options are global, so they apply here too.

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
