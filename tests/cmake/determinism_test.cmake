# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Test of cmake/TecnigrafoDeterminism.cmake (docs/adr/0038, point 4): the
# probe project tests/cmake/determinism is configured once without forbidden
# options, which must succeed with the deterministic option on every target of
# ours, and then once for every forbidden option in every place the module
# checks, which must stop the configuration with the message of the rule.
#
# The message names where the option is: the variable, the target or the
# directory.
#
# Inputs: TG_DETERMINISM_MODULE, TG_DETERMINISM_PROBE (the probe project),
# TG_DETERMINISM_OUTPUT (a build folder of its own), TG_DETERMINISM_GENERATOR
# and TG_DETERMINISM_COMPILER (those of the real build).

cmake_minimum_required(VERSION 3.24)

file(REMOVE_RECURSE "${TG_DETERMINISM_OUTPUT}")

# Configures the probe in one build folder, so the compiler is detected once.
# Every probe variable is passed every time, empty when unused, so that no
# value stays in the cache from the previous configuration.
function(configure_probe flags_variable flags_value target_option directory_option)
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -S "${TG_DETERMINISM_PROBE}" -B "${TG_DETERMINISM_OUTPUT}"
            -G "${TG_DETERMINISM_GENERATOR}"
            "-DCMAKE_CXX_COMPILER=${TG_DETERMINISM_COMPILER}"
            "-DTG_DETERMINISM_MODULE=${TG_DETERMINISM_MODULE}"
            "-DTG_PROBE_FLAGS_VARIABLE=${flags_variable}"
            "-DTG_PROBE_FLAGS_VALUE=${flags_value}"
            "-DTG_PROBE_TARGET_OPTION=${target_option}"
            "-DTG_PROBE_DIRECTORY_OPTION=${directory_option}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output)
    set(RESULT "${result}" PARENT_SCOPE)
    # CMake wraps long messages: compare the words, not the lines.
    string(REGEX REPLACE "[ \t\r\n]+" " " output "${output}")
    set(OUTPUT "${output}" PARENT_SCOPE)
endfunction()

function(expect_stop label place option)
    if(RESULT EQUAL 0)
        message(FATAL_ERROR "${label}: the configuration did not stop:\n${OUTPUT}")
    endif()
    string(FIND "${OUTPUT}" "${place} has forbidden floating-point options (${option})" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "${label}: expected the message of docs/adr/0038 for ${option}, got:\n${OUTPUT}")
    endif()
endfunction()

configure_probe("" "" "" "")
if(NOT RESULT EQUAL 0 OR NOT OUTPUT MATCHES "determinism probe: ok")
    message(FATAL_ERROR "clean probe: expected success, got ${RESULT}:\n${OUTPUT}")
endif()

foreach(option IN ITEMS -ffast-math -Ofast -funsafe-math-optimizations /fp:fast /fp:contract)
    configure_probe(CMAKE_CXX_FLAGS "-O2 ${option}" "" "")
    expect_stop("CMAKE_CXX_FLAGS ${option}" CMAKE_CXX_FLAGS "${option}")
endforeach()

configure_probe(CMAKE_CXX_FLAGS_RELEASE "-Ofast" "" "")
expect_stop("CMAKE_CXX_FLAGS_RELEASE" CMAKE_CXX_FLAGS_RELEASE "-Ofast")
# A custom configuration is checked too.
configure_probe(CMAKE_CXX_FLAGS_FAST "-ffast-math" "" "")
expect_stop("CMAKE_CXX_FLAGS_FAST" CMAKE_CXX_FLAGS_FAST "-ffast-math")

configure_probe("" "" "-funsafe-math-optimizations" "")
expect_stop("target option" probe_library "-funsafe-math-optimizations")

configure_probe("" "" "" "/fp:fast")
expect_stop("directory option" "/programma" "/fp:fast")

# The clean configuration works again after the failures.
configure_probe("" "" "" "")
if(NOT RESULT EQUAL 0)
    message(FATAL_ERROR "clean probe again: expected success, got ${RESULT}:\n${OUTPUT}")
endif()
message(STATUS "TecnigrafoDeterminism: all checks passed")
