#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Runs locally the same checks as the CI, in the same order, with one command:
# formatting, build, tests, verification cases and, on request, clang-tidy.
# It stops at the first failing step. The full output of every step is saved
# in build$TG_BUILD_SUFFIX/controlla/<step>.log; on the terminal it prints one
# line per step and, for a failing step, only the tail of its log.
#
# Usage:
#   scripts/controlla.sh                 # formato, compila, test, verifica
#   scripts/controlla.sh --tidy          # the same, plus clang-tidy
#   scripts/controlla.sh --solo test     # one step only (repeatable)
#   scripts/controlla.sh --app-off       # build without the Qt application
#   scripts/controlla.sh --righe 80      # lines of log shown on failure
#
# Steps: formato, compila, test, verifica, tidy.

set -euo pipefail

cd "$(dirname "$0")/.."

all_steps=(formato compila test verifica tidy)
steps=(formato compila test verifica)
selected=()
app_flag=()
tail_lines=40

usage() {
    sed -n '/^# Usage:/,/^# Steps:/p' "$0" | sed 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
    --tidy)
        steps+=(tidy)
        ;;
    --solo)
        [[ $# -ge 2 ]] || { usage >&2; exit 2; }
        selected+=("$2")
        shift
        ;;
    --app-off)
        app_flag=(-DTG_BUILD_APP=OFF)
        ;;
    --righe)
        [[ $# -ge 2 ]] || { usage >&2; exit 2; }
        tail_lines="$2"
        shift
        ;;
    -h | --help)
        usage
        exit 0
        ;;
    *)
        echo "Unknown option: $1" >&2
        usage >&2
        exit 2
        ;;
    esac
    shift
done

if [[ ${#selected[@]} -gt 0 ]]; then
    for step in "${selected[@]}"; do
        if [[ ! " ${all_steps[*]} " =~ " ${step} " ]]; then
            echo "Unknown step: ${step} (steps: ${all_steps[*]})" >&2
            exit 2
        fi
    done
    steps=("${selected[@]}")
fi

log_dir="build${TG_BUILD_SUFFIX:-}/controlla"
mkdir -p "$log_dir"

clang_format=clang-format-18
command -v "$clang_format" >/dev/null 2>&1 || clang_format=clang-format

run_step() {
    case "$1" in
    formato)
        git ls-files '*.cpp' '*.hpp' | xargs "$clang_format" --dry-run --Werror
        ;;
    compila)
        cmake --preset ci "${app_flag[@]}" && cmake --build --preset ci
        ;;
    test)
        ctest --preset ci -LE verifica
        ;;
    verifica)
        ctest --preset ci -L verifica --no-tests=error
        ;;
    tidy)
        cmake --preset tidy "${app_flag[@]}" && cmake --build --preset tidy
        ;;
    esac
}

total=${#steps[@]}
index=0
for step in "${steps[@]}"; do
    index=$((index + 1))
    log="$log_dir/$step.log"
    start=$SECONDS
    if run_step "$step" >"$log" 2>&1; then
        echo "[$index/$total] $step: OK ($((SECONDS - start)) s)"
    else
        echo "[$index/$total] $step: FAILED ($((SECONDS - start)) s), full log: $log"
        echo "--- last $tail_lines lines of $log ---"
        tail -n "$tail_lines" "$log"
        exit 1
    fi
done

echo "All steps passed. Logs in $log_dir/"
