# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors

# New readers only add a fuzz_*.cpp and, optionally, corpus/<reader>/ seeds.
file(GLOB fuzz_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fuzz_*.cpp")
add_custom_target(tg_fuzzers)
foreach(source IN LISTS fuzz_sources)
    get_filename_component(target "${source}" NAME_WE)
    add_executable(${target} "${source}")
    target_link_libraries(${target} PRIVATE Tecnigrafo::io)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer,address,undefined)
    tg_configure_target(${target})
    add_dependencies(tg_fuzzers ${target})
endforeach()
