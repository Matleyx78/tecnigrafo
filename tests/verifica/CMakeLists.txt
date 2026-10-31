# SPDX-License-Identifier: GPL-3.0-or-later
#
# Verification cases of the control mode (docs/adr/0011, docs/verifica/casi.md).
# Every folder tests/verifica/casi/<area>/<name> with a script.tgs or a
# domande.tgq becomes the test verifica/<area>/<name>, labelled "verifica":
#
#   ctest --preset ci -L verifica
#
# The cases listed in in-attesa.txt run with --pending: their work is not
# merged yet, so failing is expected and passing is an error.

set(TG_VERIFICA_CASES "${CMAKE_CURRENT_SOURCE_DIR}/casi")
set(TG_VERIFICA_PENDING "${CMAKE_CURRENT_SOURCE_DIR}/in-attesa.txt")

include(TecnigrafoVerifica)
# Shared by ci, tidy and asan within this agent's build directory.
set(TG_VERIFICA_SOURCES_DIR "${CMAKE_BINARY_DIR}/../verifica-sorgenti" CACHE PATH
    "Pinned public files used by verification cases")
set(manifest "${CMAKE_CURRENT_SOURCE_DIR}/sorgenti.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${manifest}")
tg_download_verifica_sources("${manifest}" "${TG_VERIFICA_SOURCES_DIR}")

foreach(outcome IN ITEMS correct-hash wrong-hash)
    add_test(NAME "verifica-download/${outcome}"
        COMMAND "${CMAKE_COMMAND}"
            "-DTG_VERIFICA_MODULE=${PROJECT_SOURCE_DIR}/cmake/TecnigrafoVerifica.cmake"
            "-DTG_VERIFICA_PROBE=${CMAKE_CURRENT_SOURCE_DIR}/prova"
            "-DTG_VERIFICA_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/prova/${outcome}"
            "-DTG_VERIFICA_OUTCOME=${outcome}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/prova/download_test.cmake")
    set_tests_properties("verifica-download/${outcome}" PROPERTIES TIMEOUT 60)
endforeach()

# CONFIGURE_DEPENDS: a new case is registered at the next build, without
# configuring again by hand.
file(GLOB TG_VERIFICA_FILES CONFIGURE_DEPENDS
    "${TG_VERIFICA_CASES}/*/*/script.tgs"
    "${TG_VERIFICA_CASES}/*/*/domande.tgq")

set(TG_VERIFICA_NAMES "")
foreach(file IN LISTS TG_VERIFICA_FILES)
    get_filename_component(folder "${file}" DIRECTORY)
    file(RELATIVE_PATH name "${TG_VERIFICA_CASES}" "${folder}")
    list(APPEND TG_VERIFICA_NAMES "${name}")
endforeach()
list(REMOVE_DUPLICATES TG_VERIFICA_NAMES)
list(SORT TG_VERIFICA_NAMES)

# in-attesa.txt: one line "<area>/<name> #<issue>", or
# "<area>/<name> #<issue> esente" for a pending case that runs without the
# early checks (docs/adr/0037, point 9); lines starting with # are comments.
# Changing it configures again.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TG_VERIFICA_PENDING}")
set(TG_VERIFICA_PENDING_NAMES "")
set(TG_VERIFICA_EXEMPT_NAMES "")
if(EXISTS "${TG_VERIFICA_PENDING}")
    file(STRINGS "${TG_VERIFICA_PENDING}" pending_lines ENCODING UTF-8)
    foreach(line IN LISTS pending_lines)
        string(STRIP "${line}" line)
        if(line STREQUAL "" OR line MATCHES "^#")
            continue()
        endif()
        if(NOT line MATCHES "^([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+)[ \t]+#[0-9]+([ \t]+esente)?$")
            message(SEND_ERROR
                "tests/verifica/in-attesa.txt: expected \"<area>/<name> #<issue>\" or \"<area>/<name> #<issue> esente\", found \"${line}\"")
            continue()
        endif()
        set(name "${CMAKE_MATCH_1}")
        if(NOT name IN_LIST TG_VERIFICA_NAMES)
            message(SEND_ERROR
                "tests/verifica/in-attesa.txt lists ${name}, which is not a case of tests/verifica/casi")
        endif()
        list(APPEND TG_VERIFICA_PENDING_NAMES "${name}")
        if(NOT CMAKE_MATCH_2 STREQUAL "")
            list(APPEND TG_VERIFICA_EXEMPT_NAMES "${name}")
        endif()
    endforeach()
endif()

foreach(name IN LISTS TG_VERIFICA_NAMES)
    set(arguments check "${TG_VERIFICA_CASES}/${name}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/casi/${name}"
        --sources "${TG_VERIFICA_SOURCES_DIR}")
    if(name IN_LIST TG_VERIFICA_PENDING_NAMES)
        list(APPEND arguments --pending)
        if(name IN_LIST TG_VERIFICA_EXEMPT_NAMES)
            list(APPEND arguments --no-early-checks)
        endif()
    endif()
    add_test(NAME "verifica/${name}" COMMAND tecnigrafo-script ${arguments})
    set_tests_properties("verifica/${name}" PROPERTIES
        LABELS verifica
        SKIP_RETURN_CODE 77)
endforeach()

# tecnigrafo-mcp against tecnigrafo-script (docs/adr/0027, point 6): on every
# case, run_script gives the same report as `tecnigrafo-script run` and
# check_case the same outcome as `tecnigrafo-script check`, with the same
# public files. One test per case and tool, labelled "verifica" too; the
# outcome of the case does not matter, so pending cases are compared as well.
add_executable(tg_mcp_equivalence "${PROJECT_SOURCE_DIR}/tests/mcp/equivalence_test.cpp")
target_link_libraries(tg_mcp_equivalence PRIVATE Tecnigrafo::mcp Tecnigrafo::script
    GTest::gtest_main)
target_compile_definitions(tg_mcp_equivalence PRIVATE
    TG_VERIFICA_CASES="${TG_VERIFICA_CASES}"
    TG_VERIFICA_SOURCES_DIR="${TG_VERIFICA_SOURCES_DIR}"
    TG_EQUIVALENCE_OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/equivalenza")
tg_configure_target(tg_mcp_equivalence)
gtest_discover_tests(tg_mcp_equivalence
    DISCOVERY_MODE PRE_TEST
    TEST_PREFIX "mcp-equivalenza/"
    PROPERTIES LABELS verifica)

# Level 2 (docs/adr/0011, points 2, 6 and 8): with the application, every
# case with a script runs also as verifica2/<area>/<name>. The test tool
# confronta_immagini starts `tecnigrafo --script`, compares its report with
# the same atteso.txt and the screenshots with atteso-<name>.png. Images
# depend on fonts and antialiasing: Linux runs all scripts; Windows and
# macOS run only pline/coordinate, a report-only smoke case from P2.
if(TG_BUILD_APP)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Gui)
    add_executable(confronta_immagini confronta_immagini.cpp)
    target_link_libraries(confronta_immagini PRIVATE Tecnigrafo::script Qt6::Gui)
    target_compile_definitions(confronta_immagini PRIVATE QT_DISABLE_DEPRECATED_UP_TO=0x060400)
    tg_configure_target(confronta_immagini)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32 OR APPLE)
        foreach(name IN LISTS TG_VERIFICA_NAMES)
            if(NOT EXISTS "${TG_VERIFICA_CASES}/${name}/script.tgs")
                continue()  # questions to the model exist only at level 1
            endif()
            if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT name STREQUAL "pline/coordinate")
                continue()
            endif()
            set(arguments case "${TG_VERIFICA_CASES}/${name}"
                --program "$<TARGET_FILE:tecnigrafo>"
                --output "${CMAKE_CURRENT_BINARY_DIR}/casi2/${name}"
                --sources "${TG_VERIFICA_SOURCES_DIR}")
            if(name IN_LIST TG_VERIFICA_PENDING_NAMES)
                list(APPEND arguments --pending)
            endif()
            add_test(NAME "verifica2/${name}" COMMAND confronta_immagini ${arguments})
            set_tests_properties("verifica2/${name}" PROPERTIES
                LABELS verifica
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
                TIMEOUT 180)
        endforeach()
    endif()
endif()
