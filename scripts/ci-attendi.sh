#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Wait for a CI run to finish, in ONE command that costs no tokens while it
# waits: the pauses happen inside the script, so an agent has nothing to poll.
#
#     scripts/ci-attendi.sh              # the run of the commit you are on
#     scripts/ci-attendi.sh <sha>        # the run of that commit
#     scripts/ci-attendi.sh <run id>     # that run
#
# Exit status: 0 the run succeeded, 1 it did not (read why with
# scripts/ci-errori.sh), 3 it is still running when the time ran out: run the
# same command again, it resumes where it was.
#
# TG_ATTESA_MAX   seconds to wait before giving up (default 1500)
# TG_ATTESA_PAUSA seconds between two checks (default 60: the anonymous limit
#                 of the GitHub API is 60 requests an hour; GITHUB_TOKEN raises it)
#
# Needs jq, and either gh or curl (the repository is public: no token needed).
set -euo pipefail

repo=${TG_REPO:-Matleyx78/tecnigrafo}
max=${TG_ATTESA_MAX:-1500}
pause=${TG_ATTESA_PAUSA:-60}

if ! command -v jq > /dev/null 2>&1; then
    echo "ci-attendi.sh needs jq." >&2
    exit 2
fi

# Print the JSON the REST API returns for a path such as repos/<repo>/...
api() {
    if command -v gh > /dev/null 2>&1; then
        gh api "$1"
    elif command -v curl > /dev/null 2>&1; then
        curl -fsS -H 'Accept: application/vnd.github+json' \
            ${GITHUB_TOKEN:+-H "Authorization: Bearer $GITHUB_TOKEN"} \
            "https://api.github.com/$1"
    else
        echo "ci-attendi.sh needs gh or curl." >&2
        return 2
    fi
}

if (( $# > 1 )); then
    echo "usage: ci-attendi.sh [commit sha | run id]" >&2
    exit 2
fi
ref=${1:-$(git rev-parse HEAD)}

start=$SECONDS
errors=0
run=
status=
conclusion=

if [[ $ref =~ ^[0-9]{1,12}$ ]]; then
    run=$ref
fi

while :; do
    if reply=$( { if [[ -z $run ]]; then
                      api "repos/$repo/actions/runs?head_sha=$ref&per_page=1" \
                          | jq -c '.workflow_runs[0] // empty'
                  else
                      api "repos/$repo/actions/runs/$run" | jq -c '.'
                  fi; } 2> /dev/null ); then
        errors=0
        if [[ -n $reply ]]; then
            run=$(jq -r '.id' <<< "$reply")
            status=$(jq -r '.status' <<< "$reply")
            conclusion=$(jq -r '.conclusion // "-"' <<< "$reply")
            if [[ $status == completed ]]; then
                echo "run $run: completed $conclusion"
                [[ $conclusion == success ]] && exit 0
                exit 1
            fi
        fi
    else
        # A network hiccup or the anonymous limit: try again a few times.
        errors=$((errors + 1))
        if (( errors >= 5 )); then
            echo "The GitHub API did not answer five times in a row." >&2
            exit 2
        fi
    fi
    if (( SECONDS - start + pause > max )); then
        if [[ -z $run ]]; then
            echo "No CI run for $ref after $((SECONDS - start)) s: it has not started, or the commit was not pushed." >&2
            exit 1
        fi
        echo "run $run: still ${status:-unknown} after $((SECONDS - start)) s. Run the same command again."
        exit 3
    fi
    sleep "$pause"
done
