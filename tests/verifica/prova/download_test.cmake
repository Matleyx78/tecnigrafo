# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors

cmake_minimum_required(VERSION 3.24)

file(REMOVE_RECURSE "${TG_VERIFICA_OUTPUT}")
file(MAKE_DIRECTORY "${TG_VERIFICA_OUTPUT}")
configure_file("${TG_VERIFICA_PROBE}/source.txt" "${TG_VERIFICA_OUTPUT}/source.txt" COPYONLY)
set(source "${TG_VERIFICA_OUTPUT}/source.txt")
file(SHA256 "${source}" correct_hash)
set(SOURCE_HASH "${correct_hash}")
if(TG_VERIFICA_OUTCOME STREQUAL "wrong-hash")
    string(REPEAT "0" 64 SOURCE_HASH)
endif()
if(WIN32)
    set(SOURCE_PATH "/${source}")
else()
    set(SOURCE_PATH "${source}")
endif()
string(REPLACE " " "%20" SOURCE_PATH "${SOURCE_PATH}")
set(manifest "${TG_VERIFICA_OUTPUT}/sorgenti.json")
configure_file("${TG_VERIFICA_PROBE}/sorgenti.json" "${manifest}" @ONLY)
set(output "${TG_VERIFICA_OUTPUT}/downloads")

execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DTG_VERIFICA_MANIFEST=${manifest}"
    "-DTG_VERIFICA_SOURCES_DIR=${output}"
    -P "${TG_VERIFICA_MODULE}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(TG_VERIFICA_OUTCOME STREQUAL "wrong-hash")
    if(result EQUAL 0 OR NOT "${stdout}${stderr}" MATCHES "[Hh][Aa][Ss][Hh] mismatch")
        message(FATAL_ERROR "A wrong SHA-256 must fail with a hash mismatch: ${stdout}${stderr}")
    endif()
    if(EXISTS "${output}/local-fixture")
        message(FATAL_ERROR "A failed download must not leave a usable file")
    endif()
    return()
endif()
if(NOT result EQUAL 0)
    message(FATAL_ERROR "The correct SHA-256 must download: ${stdout}${stderr}")
endif()
file(SHA256 "${output}/local-fixture" downloaded_hash)
if(NOT downloaded_hash STREQUAL correct_hash)
    message(FATAL_ERROR "Downloaded content differs from the fixture")
endif()

include("${TG_VERIFICA_MODULE}")
# A verified cache must work even when the original URL no longer exists.
file(RENAME "${source}" "${source}.hidden")
tg_download_verifica_sources("${manifest}" "${output}")
file(RENAME "${source}.hidden" "${source}")
# A corrupted cache must be replaced, never accepted as a cache hit.
file(WRITE "${output}/local-fixture" "corrupted cache\n")
tg_download_verifica_sources("${manifest}" "${output}")
file(SHA256 "${output}/local-fixture" repaired_hash)
if(NOT repaired_hash STREQUAL correct_hash)
    message(FATAL_ERROR "A corrupt cached file was not repaired")
endif()
