#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Pass merged stdout/stderr (or the log file given as the only argument)
# through unchanged, then emit at most ten GitHub error annotations, so that
# an agent without access to the CI logs can still read why a step failed.
# Use with pipefail so a failing producer still fails CI.
#
# Recognised: GCC/Clang/clang-format diagnostics, MSVC diagnostics, linker
# errors, GoogleTest failures, sanitizer reports and failed verification cases
# (only the first difference of each). A ctest failure that produced none of
# these (a crash, a timeout) is reported by test name.
#
# Keep it compatible with bash 3.2 (macOS): no associative arrays, no quotes
# in the replacement part of ${var//pattern/replacement}.
set -euo pipefail

if (( $# > 1 )); then
    echo "usage: ci-annota.sh [log file]" >&2
    exit 2
fi
if (( $# == 1 )); then
    exec < "$1"
fi

count=0
annotations=()
ctest_failures=()
case_name=
case_message=
case_line=
expected=
gtest_file=
gtest_line=
gtest_message=
gtest_lines=0
workspace=${GITHUB_WORKSPACE:-$PWD}
workspace=${workspace//\\//}
nl=$'\n'

escape_data() {
    REPLY=${1//'%'/%25}
    REPLY=${REPLY//$'\r'/%0D}
    REPLY=${REPLY//$'\n'/%0A}
}

escape_property() {
    escape_data "$1"
    REPLY=${REPLY//':'/%3A}
    REPLY=${REPLY//','/%2C}
}

annotate() {
    count=$((count + 1))
    if (( count <= 9 )); then
        local file=$1 line=$2 title=$3 message=$4 properties=
        # MSVC prints Windows paths; annotations want repository-relative ones.
        file=${file//\\//}
        file=${file#"$workspace/"}
        if [[ -n $file ]]; then
            escape_property "$file"
            properties=" file=$REPLY,line=$line"
            if [[ -n $5 ]]; then
                properties+=",col=$5"
            fi
        fi
        if [[ -n $title ]]; then
            escape_property "$title"
            properties+="${properties:+,}title=$REPLY"
            [[ $properties == ' '* ]] || properties=" $properties"
        fi
        escape_data "$message"
        annotations+=("::error$properties::$REPLY")
    fi
}

finish_case() {
    if [[ -n $case_name ]]; then
        # Image failures and execution errors have no report line to attach to.
        annotate "" "" "$case_name" "$case_message" ""
    fi
    case_name=
    case_line=
    expected=
}

finish_gtest() {
    if [[ -n $gtest_file ]]; then
        annotate "$gtest_file" "$gtest_line" "GoogleTest" "$gtest_message" ""
    fi
    gtest_file=
    gtest_line=
    gtest_message=
    gtest_lines=0
}

diagnostic='^(.+):([0-9]+):([0-9]+): (warning|error|fatal error|runtime error): (.*)$'
msvc_diagnostic='^(.+)\(([0-9]+)(,([0-9]+))?\) ?: (warning|error|fatal error) ([A-Z]+[0-9]+: .*)$'
linker='(undefined reference to|error LNK[0-9]+:|Undefined symbols for architecture|ld: error:)'
sanitizer='^[[:space:]]*SUMMARY: ([A-Za-z]+Sanitizer): (.*)$'
gtest_failure='^(.+):([0-9]+): Failure$'
gtest_msvc_failure='^(.+)\(([0-9]+)\): error: (.*)$'
ctest_failed='Test +#[0-9]+: ([^ ]+) [ .]*\*\*\*(.*[^ ])[[:space:]]+[0-9.]+ sec'
failed='^[[:space:]]*FAILED: (.+: .+)$'
report_line='^[[:space:]]+line ([0-9]+)$'
expected_line='^[[:space:]]+expected: (.*)$'
obtained_line='^[[:space:]]+obtained: (.*)$'

while IFS= read -r line || [[ -n $line ]]; do
    printf '%s\n' "$line"
    # CTest output can carry CRs even on Linux; PowerShell logs always do.
    parsed=${line%$'\r'}

    # A GoogleTest failure message runs until a blank line or a "[" line.
    if [[ -n $gtest_file ]]; then
        if [[ -z $parsed || $parsed == '['* || $parsed =~ $gtest_failure \
              || $parsed =~ $gtest_msvc_failure ]] || (( gtest_lines >= 8 )); then
            finish_gtest
        else
            gtest_message+="${gtest_message:+$nl}$parsed"
            gtest_lines=$((gtest_lines + 1))
            continue
        fi
    fi

    if [[ $parsed =~ $diagnostic ]]; then
        annotate "${BASH_REMATCH[1]}" "${BASH_REMATCH[2]}" "" \
            "${BASH_REMATCH[4]}: ${BASH_REMATCH[5]}" "${BASH_REMATCH[3]}"
    elif [[ $parsed =~ $msvc_diagnostic ]]; then
        annotate "${BASH_REMATCH[1]}" "${BASH_REMATCH[2]}" "" \
            "${BASH_REMATCH[5]} ${BASH_REMATCH[6]}" "${BASH_REMATCH[4]}"
    elif [[ $parsed =~ $gtest_failure ]]; then
        gtest_file=${BASH_REMATCH[1]}
        gtest_line=${BASH_REMATCH[2]}
    elif [[ $parsed =~ $gtest_msvc_failure ]]; then
        gtest_file=${BASH_REMATCH[1]}
        gtest_line=${BASH_REMATCH[2]}
        gtest_message=${BASH_REMATCH[3]}
    elif [[ $parsed =~ $sanitizer ]]; then
        annotate "" "" "${BASH_REMATCH[1]}" "${BASH_REMATCH[2]}" ""
    elif [[ $parsed =~ $linker ]]; then
        annotate "" "" "Linker" "$parsed" ""
    elif [[ $parsed =~ $ctest_failed ]]; then
        ctest_failures+=("${BASH_REMATCH[1]}: ${BASH_REMATCH[2]}")
    elif [[ $parsed =~ $failed ]]; then
        finish_case
        case_message=${BASH_REMATCH[1]}
        case_name=${case_message%%: *}
    elif [[ $parsed == pending:* || $parsed == passed:* || $parsed == INVALID:* ]]; then
        finish_case
    elif [[ -n $case_name && $parsed =~ $report_line ]]; then
        case_line=${BASH_REMATCH[1]}
    elif [[ -n $case_line && $parsed =~ $expected_line ]]; then
        expected=${BASH_REMATCH[1]}
    elif [[ -n $case_line && $parsed =~ $obtained_line ]]; then
        obtained=${BASH_REMATCH[1]}
        # Both verification levels compare this repository-relative report.
        case_path=${case_name% (level 2)}
        case_path=${case_path#"$workspace/"}
        annotate "$case_path/atteso.txt" "$case_line" "$case_name" \
            "line $case_line: expected: $expected; obtained: $obtained" ""
        case_name=
        case_line=
    fi
done
finish_gtest
finish_case

# Name the failed tests only when nothing more precise explains them.
if (( count == 0 && ${#ctest_failures[@]} > 0 )); then
    for failure in "${ctest_failures[@]}"; do
        annotate "" "" "ctest" "$failure (no diagnostic recognised; see the log)" ""
    done
fi

if (( ${#annotations[@]} > 0 )); then
    printf '%s\n' "${annotations[@]}"
fi
if (( count > 9 )); then
    printf '::error title=Additional diagnostics::%s more errors omitted; see the full log.\n' "$((count - 9))"
fi
