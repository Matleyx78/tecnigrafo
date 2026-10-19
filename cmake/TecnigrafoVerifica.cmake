# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    cmake_minimum_required(VERSION 3.24)
endif()

include_guard(GLOBAL)

# Download at configure time only. Tests consume these pinned files offline.
# Each source is stored as <output>/<name>, the identifier in sorgenti.json.
function(tg_download_verifica_sources manifest output)
    if(NOT EXISTS "${manifest}")
        return()
    endif()

    file(READ "${manifest}" json)
    string(JSON format ERROR_VARIABLE error GET "${json}" format)
    if(error OR NOT format STREQUAL "1")
        message(FATAL_ERROR "${manifest}: expected source manifest format 1")
    endif()
    string(JSON sources_type ERROR_VARIABLE error TYPE "${json}" sources)
    if(error OR NOT sources_type STREQUAL "ARRAY")
        message(FATAL_ERROR "${manifest}: sources must be an array")
    endif()
    string(JSON count LENGTH "${json}" sources)
    if(count EQUAL 0)
        return()
    endif()

    file(MAKE_DIRECTORY "${output}")
    file(LOCK "${output}/.download.lock" GUARD FUNCTION TIMEOUT 120)
    set(names "")
    math(EXPR last "${count} - 1")
    foreach(index RANGE 0 ${last})
        foreach(field IN ITEMS name url version sha256 license license-url)
            string(JSON type ERROR_VARIABLE error TYPE "${json}" sources ${index} "${field}")
            if(error OR NOT type STREQUAL "STRING")
                message(FATAL_ERROR "${manifest}: source ${index}: ${field} must be a string")
            endif()
            string(JSON value GET "${json}" sources ${index} "${field}")
            if(value STREQUAL "")
                message(FATAL_ERROR "${manifest}: source ${index}: ${field} must not be empty")
            endif()
            set("source_${field}" "${value}")
        endforeach()
        if(NOT source_name MATCHES "^[a-z0-9-]+$" OR source_name IN_LIST names)
            message(FATAL_ERROR "${manifest}: invalid or duplicate source name: ${source_name}")
        endif()
        list(APPEND names "${source_name}")
        string(LENGTH "${source_sha256}" hash_length)
        if(NOT hash_length EQUAL 64 OR NOT source_sha256 MATCHES "^[0-9a-f]+$")
            message(FATAL_ERROR "${manifest}: ${source_name}: invalid SHA-256")
        endif()

        set(destination "${output}/${source_name}")
        # A cache hit is trusted only after checking the content, not its name.
        if(EXISTS "${destination}")
            file(SHA256 "${destination}" actual_hash)
            if(actual_hash STREQUAL source_sha256)
                continue()
            endif()
            file(REMOVE "${destination}")
        endif()
        set(partial "${destination}.part")
        # EXPECTED_HASH aborts CMake on a mismatch. A partial file can then
        # remain, but is never published and is removed before the next try.
        file(REMOVE "${partial}")
        file(DOWNLOAD "${source_url}" "${partial}"
            EXPECTED_HASH "SHA256=${source_sha256}"
            TLS_VERIFY ON TIMEOUT 120 INACTIVITY_TIMEOUT 30
            STATUS status)
        list(GET status 0 code)
        list(GET status 1 detail)
        if(NOT code EQUAL 0)
            file(REMOVE "${partial}")
            message(FATAL_ERROR "${manifest}: ${source_name}: download failed: ${detail}")
        endif()
        file(RENAME "${partial}" "${destination}")
    endforeach()
endfunction()

# Also usable from a standalone configure-time probe or a CTest subprocess.
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    if(NOT DEFINED TG_VERIFICA_MANIFEST OR NOT DEFINED TG_VERIFICA_SOURCES_DIR)
        message(FATAL_ERROR "Set TG_VERIFICA_MANIFEST and TG_VERIFICA_SOURCES_DIR")
    endif()
    tg_download_verifica_sources("${TG_VERIFICA_MANIFEST}" "${TG_VERIFICA_SOURCES_DIR}")
endif()
