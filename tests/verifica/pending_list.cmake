# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# The pending list of the verification cases (docs/adr/0011, point 7): the
# folder tests/verifica/in-attesa/, one file <X.Y>-R<N>.txt per block of
# references (for example 0.7-R16.txt). The package of references adds its
# file and the package of work deletes it, so two packages running in parallel
# never touch the same path and the list needs no merge.
#
# A file has one line per case, "<area>/<name> #<issue>" or
# "<area>/<name> #<issue> esente" (docs/adr/0037, point 9); empty lines and
# lines starting with # are ignored, and so are the names starting with "."
# (the .gitkeep that keeps the folder in git). A case is listed in one file
# only, and a file with no case is a leftover to delete. The old single file
# in-attesa.txt is rejected.

# tg_read_pending_list(<directory> <known cases> <prefix> [CONFIGURE_DEPENDS])
#
# Reads the pending folder and sets, in the caller's scope:
#   <prefix>_NAMES   the listed cases, "<area>/<name>"
#   <prefix>_EXEMPT  the listed cases marked "esente"
#   <prefix>_FILES   the pending files that were read
#   <prefix>_ERRORS  one message per problem; empty when the list is valid
# <known cases> is the list of the cases that exist under tests/verifica/casi.
# CONFIGURE_DEPENDS makes the folder listing part of the build system, so a
# file added or deleted configures again.
function(tg_read_pending_list directory known_cases prefix)
    set(display "tests/verifica/in-attesa")
    set(names "")
    set(exempt "")
    set(files "")
    set(errors "")

    if(EXISTS "${directory}.txt")
        list(APPEND errors
            "${display}.txt: the single list was replaced by the folder ${display}/, one file <X.Y>-R<N>.txt per block of references (docs/adr/0011, point 7): move its lines into the file of their block and delete it")
    endif()

    if(IS_DIRECTORY "${directory}")
        if("CONFIGURE_DEPENDS" IN_LIST ARGN)
            file(GLOB entries LIST_DIRECTORIES true CONFIGURE_DEPENDS "${directory}/*")
        else()
            file(GLOB entries LIST_DIRECTORIES true "${directory}/*")
        endif()
    else()
        set(entries "")
    endif()
    list(SORT entries)

    foreach(entry IN LISTS entries)
        get_filename_component(file_name "${entry}" NAME)
        if(file_name MATCHES "^\\.")
            continue()
        endif()
        set(shown "${display}/${file_name}")
        if(IS_DIRECTORY "${entry}" OR NOT file_name MATCHES "^[0-9]+\\.[0-9]+-R[0-9]+\\.txt$")
            list(APPEND errors
                "${shown}: expected a file named <X.Y>-R<N>.txt, after the tag of its block of references")
            continue()
        endif()
        list(APPEND files "${entry}")

        # Semicolons and brackets would split or merge the lines of the list.
        file(READ "${entry}" content)
        string(REPLACE ";" "<SEMICOLON>" content "${content}")
        string(REPLACE "[" "<LBRACKET>" content "${content}")
        string(REPLACE "]" "<RBRACKET>" content "${content}")
        string(REPLACE "\r" "" content "${content}")
        string(REPLACE "\n" ";" lines "${content}")

        set(listed 0)
        set(in_this_file "")
        list(LENGTH errors errors_before)
        foreach(line IN LISTS lines)
            string(STRIP "${line}" line)
            if(line STREQUAL "" OR line MATCHES "^#")
                continue()
            endif()
            if(NOT line MATCHES "^([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+)[ \t]+#[0-9]+([ \t]+esente)?$")
                list(APPEND errors
                    "${shown}: expected \"<area>/<name> #<issue>\" or \"<area>/<name> #<issue> esente\", found \"${line}\"")
                continue()
            endif()
            set(name "${CMAKE_MATCH_1}")
            set(is_exempt "${CMAKE_MATCH_2}")
            math(EXPR listed "${listed} + 1")
            if(NOT name IN_LIST known_cases)
                list(APPEND errors
                    "${shown} lists ${name}, which is not a case of tests/verifica/casi")
            endif()
            if(name IN_LIST in_this_file)
                list(APPEND errors "${shown} lists ${name} twice")
                continue()
            endif()
            list(APPEND in_this_file "${name}")
            if(DEFINED "TG_PENDING_OWNER_${name}")
                list(APPEND errors
                    "${shown} lists ${name}, which is also listed in ${TG_PENDING_OWNER_${name}}")
                continue()
            endif()
            set("TG_PENDING_OWNER_${name}" "${shown}")
            list(APPEND names "${name}")
            # Quoted: an optional group that does not match leaves the match
            # variable undefined, and an unquoted if() would compare the name.
            if(NOT "${is_exempt}" STREQUAL "")
                list(APPEND exempt "${name}")
            endif()
        endforeach()

        # A file whose lines were all wrong is reported for them, not as empty.
        list(LENGTH errors errors_after)
        if(listed EQUAL 0 AND errors_after EQUAL errors_before)
            list(APPEND errors
                "${shown} lists no case: delete the file when its cases become mandatory")
        endif()
    endforeach()

    set(${prefix}_NAMES "${names}" PARENT_SCOPE)
    set(${prefix}_EXEMPT "${exempt}" PARENT_SCOPE)
    set(${prefix}_FILES "${files}" PARENT_SCOPE)
    set(${prefix}_ERRORS "${errors}" PARENT_SCOPE)
endfunction()
