# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Test of tg_read_pending_list (tests/verifica/pending_list.cmake) on temporary
# folders: the valid shapes and every way the pending folder can be wrong.

cmake_minimum_required(VERSION 3.24)
include("${TG_VERIFICA_PENDING_MODULE}")

set(known a/one a/two b/three)

# Starts a fresh folder <output>/<label>/in-attesa and sets `dir` to it.
macro(fresh label)
    file(REMOVE_RECURSE "${TG_VERIFICA_OUTPUT}/${label}")
    set(dir "${TG_VERIFICA_OUTPUT}/${label}/in-attesa")
    file(MAKE_DIRECTORY "${dir}")
endmacro()

function(expect_error label needle)
    # The message is searched for in the whole list of errors.
    string(FIND "${ERRORS}" "${needle}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "${label}: expected an error containing \"${needle}\", got: ${ERRORS}")
    endif()
endfunction()

function(expect_one_error label needle)
    list(LENGTH ERRORS count)
    if(NOT count EQUAL 1)
        message(FATAL_ERROR "${label}: expected one error, got ${count}: ${ERRORS}")
    endif()
    expect_error("${label}" "${needle}")
endfunction()

macro(read)
    tg_read_pending_list("${dir}" "${known}" R)
    set(ERRORS "${R_ERRORS}")
endmacro()

# --- valid shapes -----------------------------------------------------------
fresh(valid)
file(WRITE "${dir}/0.7-R16.txt"
    "# 0.7-R16: a comment; with a semicolon and [brackets]\n"
    "\n"
    "a/one #361\n"
    "  a/two   #361   esente  \n")
file(WRITE "${dir}/0.7-R20.txt" "b/three #369\r\n")
file(WRITE "${dir}/.gitkeep" "")
read()
if(NOT "${ERRORS}" STREQUAL "")
    message(FATAL_ERROR "valid: unexpected errors: ${ERRORS}")
endif()
if(NOT "${R_NAMES}" STREQUAL "a/one;a/two;b/three")
    message(FATAL_ERROR "valid: wrong cases: ${R_NAMES}")
endif()
if(NOT "${R_EXEMPT}" STREQUAL "a/two")
    message(FATAL_ERROR "valid: wrong exempt cases: ${R_EXEMPT}")
endif()
list(LENGTH R_FILES file_count)
if(NOT file_count EQUAL 2)
    message(FATAL_ERROR "valid: the .gitkeep must be ignored, files: ${R_FILES}")
endif()

# A folder with only the .gitkeep, and a folder that does not exist, are empty lists.
fresh(only-gitkeep)
file(WRITE "${dir}/.gitkeep" "")
read()
if(NOT "${ERRORS}" STREQUAL "" OR NOT "${R_NAMES}" STREQUAL "")
    message(FATAL_ERROR "only-gitkeep: expected an empty valid list: ${ERRORS} ${R_NAMES}")
endif()
file(REMOVE_RECURSE "${TG_VERIFICA_OUTPUT}/only-gitkeep")
read()
if(NOT "${ERRORS}" STREQUAL "" OR NOT "${R_NAMES}" STREQUAL "")
    message(FATAL_ERROR "missing folder: expected an empty valid list: ${ERRORS} ${R_NAMES}")
endif()

# --- the old single file ----------------------------------------------------
fresh(old-file)
file(WRITE "${dir}.txt" "a/one #361\n")
read()
expect_one_error("old-file" "in-attesa.txt: the single list was replaced by the folder")

# --- a file with no case ----------------------------------------------------
fresh(empty-file)
file(WRITE "${dir}/0.7-R16.txt" "")
read()
expect_one_error("empty-file" "0.7-R16.txt lists no case")
file(WRITE "${dir}/0.7-R16.txt" "# 0.7-R16: only a comment\n\n")
read()
expect_one_error("comment-only-file" "0.7-R16.txt lists no case")

# --- a wrong name ------------------------------------------------------------
foreach(wrong IN ITEMS R16.txt 0.7-R16 0.7-R16.md 0.7-r16.txt 0.7-R16a.txt in-attesa.txt)
    fresh(wrong-name)
    file(WRITE "${dir}/${wrong}" "a/one #361\n")
    read()
    expect_one_error("wrong name ${wrong}" "${wrong}: expected a file named <X.Y>-R<N>.txt")
endforeach()
fresh(subfolder)
file(MAKE_DIRECTORY "${dir}/0.7-R16.txt")
read()
expect_one_error("subfolder" "0.7-R16.txt: expected a file named")

# --- a case in two files, twice in one file ----------------------------------
fresh(two-files)
file(WRITE "${dir}/0.7-R16.txt" "a/one #361\n")
file(WRITE "${dir}/0.7-R20.txt" "a/one #369\na/two #369\n")
read()
expect_one_error("two-files" "0.7-R20.txt lists a/one, which is also listed in tests/verifica/in-attesa/0.7-R16.txt")
fresh(twice)
file(WRITE "${dir}/0.7-R16.txt" "a/one #361\na/one #361\n")
read()
expect_one_error("twice" "0.7-R16.txt lists a/one twice")

# --- a case that does not exist, a malformed line -----------------------------
fresh(unknown-case)
file(WRITE "${dir}/0.7-R16.txt" "a/missing #361\n")
read()
expect_one_error("unknown-case" "lists a/missing, which is not a case of tests/verifica/casi")
fresh(bad-line)
file(WRITE "${dir}/0.7-R16.txt" "a/one\na/two #x\na/one #361 exempt\n")
read()
list(LENGTH ERRORS bad_count)
if(NOT bad_count EQUAL 3)
    message(FATAL_ERROR "bad-line: expected three errors, got ${bad_count}: ${ERRORS}")
endif()
expect_error("bad-line" "expected \"<area>/<name> #<issue>\"")

message(STATUS "tg_read_pending_list: every case behaves as expected")
