#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Print why a CI run failed, in a few lines: for every failed job its name, the
# failed step and the error annotations (see scripts/ci-annota.sh). It reads
# the REST API only, so it works where the raw logs cannot be downloaded, and
# it leaves out warnings, which are not the cause of a failure.
#
#     scripts/ci-errori.sh              # the run of the commit you are on
#     scripts/ci-errori.sh <sha>        # the run of that commit
#     scripts/ci-errori.sh <run id>     # that run
#
# Needs gh and jq. Wait for the run to finish first (AGENTS.md).
set -euo pipefail

repo=${TG_REPO:-Matleyx78/tecnigrafo}

if (( $# > 1 )); then
    echo "usage: ci-errori.sh [commit sha | run id]" >&2
    exit 2
fi
ref=${1:-$(git rev-parse HEAD)}

if [[ $ref =~ ^[0-9]{1,12}$ ]]; then
    run=$ref
else
    # The newest run of the commit; several appear after a re-run.
    run=$(gh api "repos/$repo/actions/runs?head_sha=$ref&per_page=1" \
        --jq '.workflow_runs[0].id // empty')
    if [[ -z $run ]]; then
        echo "No CI run for $ref yet: it has not started, or the commit was not pushed." >&2
        exit 1
    fi
fi

gh api "repos/$repo/actions/runs/$run" \
    --jq '"run \(.id): \(.status) \(.conclusion // "-")  \(.html_url)"'

jobs=$(gh api "repos/$repo/actions/runs/$run/jobs?per_page=100" --jq '
    .jobs[] | select(.conclusion == "failure" or .conclusion == "timed_out")
    | "\(.id)\t\(.name)\t\([.steps[] | select(.conclusion == "failure" or .conclusion == "timed_out") | .name] | join(", "))"')

if [[ -z $jobs ]]; then
    echo "No failed job (yet)."
    exit 0
fi

while IFS=$'\t' read -r id name step; do
    echo
    echo "== $name (job $id): failed step: ${step:-?}"
    gh api "repos/$repo/check-runs/$id/annotations?per_page=50" --jq '
        .[] | select(.annotation_level == "failure")
        | select(.message | test("^Process completed with exit code") | not)
        | "\(if .path == ".github" then "" else "\(.path):\(.start_line) " end)\(if (.title // "") == "" then "" else "[\(.title)] " end)\(.message | gsub("\n"; " | ") | .[0:400])"' \
        || true
done <<< "$jobs"
