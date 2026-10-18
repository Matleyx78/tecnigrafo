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

# in-attesa.txt: one line "<area>/<name> #<issue>"; lines starting with #
# are comments. Changing it configures again.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TG_VERIFICA_PENDING}")
set(TG_VERIFICA_PENDING_NAMES "")
if(EXISTS "${TG_VERIFICA_PENDING}")
    file(STRINGS "${TG_VERIFICA_PENDING}" pending_lines ENCODING UTF-8)
    foreach(line IN LISTS pending_lines)
        string(STRIP "${line}" line)
        if(line STREQUAL "" OR line MATCHES "^#")
            continue()
        endif()
        if(NOT line MATCHES "^([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+)[ \t]+#[0-9]+$")
            message(SEND_ERROR
                "tests/verifica/in-attesa.txt: expected \"<area>/<name> #<issue>\", found \"${line}\"")
            continue()
        endif()
        set(name "${CMAKE_MATCH_1}")
        if(NOT name IN_LIST TG_VERIFICA_NAMES)
            message(SEND_ERROR
                "tests/verifica/in-attesa.txt lists ${name}, which is not a case of tests/verifica/casi")
        endif()
        list(APPEND TG_VERIFICA_PENDING_NAMES "${name}")
    endforeach()
endif()

foreach(name IN LISTS TG_VERIFICA_NAMES)
    set(arguments check "${TG_VERIFICA_CASES}/${name}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/casi/${name}")
    if(name IN_LIST TG_VERIFICA_PENDING_NAMES)
        list(APPEND arguments --pending)
    endif()
    add_test(NAME "verifica/${name}" COMMAND tecnigrafo-script ${arguments})
    set_tests_properties("verifica/${name}" PROPERTIES
        LABELS verifica
        SKIP_RETURN_CODE 77)
endforeach()
