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
#   scripts/controlla.sh --tidy          # the same, plus clang-tidy on the touched files
#   scripts/controlla.sh --tidy-tutto    # the same, plus clang-tidy on the whole project
#   scripts/controlla.sh --solo test     # one step only (repeatable)
#   scripts/controlla.sh --app-off       # build without the Qt application
#   scripts/controlla.sh --righe 80      # lines of log shown on failure
#
# Steps: formato, compila, test, verifica, tidy, tidy-tutto.
#
# tidy runs clang-tidy only on the .cpp files that differ from main (committed,
# modified or new) and on the .cpp files that mention a touched header, using
# the compile commands of the ci preset: a few minutes at most. tidy-tutto
# rebuilds the whole project under clang-tidy, which takes tens of minutes: it
# is the CI's job (AGENTS.md), not a local check.
# TG_TIDY_BASE   the revision to compare with (default origin/main, then main,
#                then only the uncommitted changes, as in a shallow clone)
# TG_TIDY_MAX    most files tidy accepts (default 25); above that the step
#                fails with a message and the check is left to the CI

set -euo pipefail

cd "$(dirname "$0")/.."

all_steps=(formato compila test verifica tidy tidy-tutto)
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
    --tidy-tutto)
        steps+=(tidy-tutto)
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
clang_tidy=clang-tidy-18
command -v "$clang_tidy" >/dev/null 2>&1 || clang_tidy=clang-tidy

# clang-tidy on the touched .cpp files only (see the header of this file).
tidy_touched() {
    local build="build${TG_BUILD_SUFFIX:-}/ci" max=${TG_TIDY_MAX:-25} base= name file
    local -a changed=() candidates=() files=()

    if [[ ! -f $build/compile_commands.json ]]; then
        cmake --preset ci "${app_flag[@]}"
    fi

    base=${TG_TIDY_BASE:-}
    if [[ -z $base ]]; then
        for ref in origin/main main; do
            if base=$(git merge-base HEAD "$ref" 2>/dev/null); then
                break
            fi
            base=
        done
    fi
    if [[ -z $base ]]; then
        echo "No main to compare with (shallow clone?): using only the uncommitted changes."
        base=HEAD
    fi
    echo "Comparing with ${base:0:12}"

    mapfile -t changed < <({ git diff --name-only "$base"; git ls-files --others --exclude-standard; } | sort -u)
    for file in "${changed[@]}"; do
        [[ -f $file ]] || continue
        case $file in
        *.cpp)
            candidates+=("$file")
            ;;
        *.hpp)
            name=$(basename "$file")
            while IFS= read -r user; do
                candidates+=("$user")
            done < <(git grep -l -F "$name" -- '*.cpp')
            ;;
        esac
    done

    # Only the files the ci preset compiles (the Qt application is not there
    # with --app-off), once each.
    local compiled
    compiled=$(jq -r '.[].file' "$build/compile_commands.json")
    if [[ ${#candidates[@]} -gt 0 ]]; then
        while IFS= read -r file; do
            if grep -qxF "$PWD/$file" <<<"$compiled"; then
                files+=("$file")
            fi
        done < <(printf '%s\n' "${candidates[@]}" | sort -u)
    fi

    if [[ ${#files[@]} -eq 0 ]]; then
        echo "No touched .cpp file to check."
        return 0
    fi
    if [[ ${#files[@]} -gt $max ]]; then
        echo "TOO MANY FILES: ${#files[@]} touched .cpp files (limit $max, TG_TIDY_MAX)."
        echo "Do not run clang-tidy on the whole project here: leave it to the CI and"
        echo "write in the pull request that tidy was not run locally."
        return 3
    fi
    echo "clang-tidy on ${#files[@]} files"
    printf '%s\n' "${files[@]}" | xargs -P "$(nproc)" -n 1 "$clang_tidy" -p "$build" --quiet
}

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
        tidy_touched
        ;;
    tidy-tutto)
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
