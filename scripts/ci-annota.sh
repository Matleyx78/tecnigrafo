#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Pass merged stdout/stderr through unchanged, then emit at most ten GitHub
# error annotations. Use with pipefail so a failing producer still fails CI.
# Only the first difference of each failed verification case is annotated.
set -euo pipefail

count=0
annotations=()
case_name=
case_message=
case_line=
expected=
workspace=${GITHUB_WORKSPACE:-$PWD}

escape_data() {
    REPLY=${1//'%'/'%25'}
    REPLY=${REPLY//$'\r'/'%0D'}
    REPLY=${REPLY//$'\n'/'%0A'}
}

escape_property() {
    escape_data "$1"
    REPLY=${REPLY//':'/'%3A'}
    REPLY=${REPLY//','/'%2C'}
}

annotate() {
    count=$((count + 1))
    if (( count <= 9 )); then
        local file=$1 line=$2 title=$3 message=$4 properties=
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

diagnostic='^(.+):([0-9]+):([0-9]+): (warning|error|fatal error): (.*)$'
failed='^[[:space:]]*FAILED: (.+: .+)$'
report_line='^[[:space:]]+line ([0-9]+)$'
expected_line='^[[:space:]]+expected: (.*)$'
obtained_line='^[[:space:]]+obtained: (.*)$'

while IFS= read -r line || [[ -n $line ]]; do
    printf '%s\n' "$line"
    # CTest output can carry CRs even on Linux.
    parsed=${line%$'\r'}
    if [[ $parsed =~ $diagnostic ]]; then
        annotate "${BASH_REMATCH[1]}" "${BASH_REMATCH[2]}" "" \
            "${BASH_REMATCH[4]}: ${BASH_REMATCH[5]}" "${BASH_REMATCH[3]}"
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
finish_case

if (( ${#annotations[@]} > 0 )); then
    printf '%s\n' "${annotations[@]}"
fi
if (( count > 9 )); then
    printf '::error title=Additional diagnostics::%s more errors omitted; see the full log.\n' "$((count - 9))"
fi
