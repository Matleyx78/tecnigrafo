#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Wait for a CI run to finish, in ONE command that costs no tokens while it
# waits: the pauses happen inside the script, so an agent has nothing to poll.
#
#     scripts/ci-attendi.sh              # the run of the commit you are on
#     scripts/ci-attendi.sh <sha>        # the run of that commit
#     scripts/ci-attendi.sh <pr number>  # the run of the last commit of that pull request
#     scripts/ci-attendi.sh <run id>     # that run
#
# A number of up to 6 digits is a pull request, a longer one a run id (run ids
# have 10 digits and more).
#
# Exit status:
#   0  the run succeeded
#   1  a check failed (read why with scripts/ci-errori.sh). The script does not
#      wait for the slow jobs (Windows, macOS) once one job has already failed
#   2  the GitHub API did not answer (the last error is printed)
#   3  the run is still going when the time ran out: run the same command again,
#      it resumes where it was
#   4  the run was cancelled: nothing failed, a newer push on the same branch
#      replaced it, so wait for that one
#   5  the time ran out TG_ATTESA_RILANCI times in a row on the same run (about
#      90 minutes in all, longer than any healthy run should take): do not
#      wait again, report it in the pull request (AGENTS.md, "Quando fermarsi")
#   6  the run waits for an approval (action_required) for longer than
#      TG_ATTESA_APPROVAZIONE: nothing failed and there is nothing to decide.
#      The script has written once "CI da approvare" in the pull request; stop
#      without the label decisione (docs/agenti/catena.md, docs/DECISIONI.md,
#      2026-11-19)
#   7  there is no run because the pull request is in conflict with main
#      (mergeable_state "dirty"): GitHub does not start pull_request workflows
#      on it. Nothing is broken: update the branch with main (merge commit,
#      never rebase), resolve the conflicts, push and wait again
#      (docs/agenti/comandi.md; docs/DECISIONI.md, 2026-11-28)
#
# A run in action_required is not finished: the script writes the note in the
# pull request once and goes on waiting, so an approval within the time lets
# the run go on as usual.
#
# TG_ATTESA_MAX     seconds to wait before giving up (default 540: the Bash
#                   tool of an agent stops a foreground command at 600 s)
# TG_ATTESA_PAUSA   seconds between two checks (default 90: every check costs
#                   two requests and the anonymous limit of the GitHub API is 60
#                   an hour; GITHUB_TOKEN raises it)
# TG_ATTESA_RILANCI how many timeouts (exit 3) are allowed on one run (default 10: about 90 minutes in all)
# TG_ATTESA_WORKFLOW workflow file or id to wait for (default ci.yml); ignored
#                   when an explicit run id is given
# TG_ATTESA_APPROVAZIONE seconds a run may wait for an approval, counted from
#                   its creation (default 1200: 20 minutes)
#
# Needs jq, and either gh or curl (the repository is public: no token needed).
set -euo pipefail

repo=${TG_REPO:-Matleyx78/tecnigrafo}
workflow=${TG_ATTESA_WORKFLOW:-ci.yml}
max=${TG_ATTESA_MAX:-540}
pause=${TG_ATTESA_PAUSA:-90}
max_timeouts=${TG_ATTESA_RILANCI:-10}
approval_wait=${TG_ATTESA_APPROVAZIONE:-1200}

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
    echo "usage: ci-attendi.sh [commit sha | pull request number | run id]" >&2
    exit 2
fi
ref=${1:-$(git rev-parse HEAD)}

err=$(mktemp)
trap 'rm -f "$err"' EXIT

start=$SECONDS
errors=0
run=
status=
conclusion=

if [[ $ref =~ ^[0-9]{1,6}$ ]]; then
    # A pull request number: wait for the run of its last commit.
    pr=$ref
    ref=$(api "repos/$repo/pulls/$pr" | jq -r '.head.sha // empty') || ref=
    if [[ -z $ref ]]; then
        echo "No pull request #$pr in $repo." >&2
        exit 2
    fi
    echo "pull request #$pr: commit $ref"
elif [[ $ref =~ ^[0-9]{7,}$ ]]; then
    run=$ref
fi

# Counts how many times in a row the wait ran out on this run.
timeouts_file() {
    printf '%s/tg-ci-attendi-%s.timeouts' "${TMPDIR:-/tmp}" "$run"
}

# Names of the jobs of the run that have already failed, even if the run goes on.
failed_jobs() {
    api "repos/$repo/actions/runs/$1/jobs?per_page=100" 2>> "$err" \
        | jq -r '.jobs[] | select(.conclusion == "failure" or .conclusion == "timed_out") | .name'
}

# Writes once in the pull request of the run that the run waits for an
# approval. Needs gh, which can write; without it the note is only printed.
note_approval() {
    local reply=$1 sha number marker
    sha=$(jq -r '.head_sha // empty' <<< "$reply")
    [[ -z $sha ]] && sha=$ref
    marker="CI da approvare: commit ${sha:0:12}"
    number=${pr:-}
    if [[ -z $number ]]; then
        number=$(api "repos/$repo/commits/$sha/pulls" 2>> "$err" | jq -r '.[0].number // empty') || number=
    fi
    if [[ -z $number ]] || ! command -v gh > /dev/null 2>&1; then
        echo "$marker: approve the run in Actions (no pull request to write to)."
        return 0
    fi
    if api "repos/$repo/issues/$number/comments?per_page=100" 2>> "$err" \
        | jq -e --arg m "$marker" 'any(.[]; .body | contains($m))' > /dev/null; then
        return 0
    fi
    gh api "repos/$repo/issues/$number/comments" \
        -f body="$marker. L'esecuzione $run della CI aspetta un'approvazione (action_required): approvala da Actions. scripts/ci-attendi.sh aspetta fino a $((approval_wait / 60)) minuti dalla sua creazione, poi l'agente si ferma senza decisione (docs/agenti/catena.md)." \
        > /dev/null 2>> "$err" || true
    echo "$marker: written in pull request #$number."
}

# The pull request of the commit, for the conflict check: the one given, or
# the first open one that has the commit at its head.
pr_of_ref() {
    if [[ -n ${pr:-} ]]; then
        echo "$pr"
        return 0
    fi
    api "repos/$repo/commits/$ref/pulls" 2>> "$err" \
        | jq -r --arg sha "$ref" '[.[] | select(.state == "open" and (.head.sha | startswith($sha)))][0].number // empty'
}

# True when GitHub has computed that the pull request is in conflict with its
# base. Right after a push the state is "unknown": then it is not a conflict
# yet, and the next check asks again.
in_conflict() {
    local number state
    number=$(pr_of_ref) || number=
    [[ -n $number ]] || return 1
    state=$(api "repos/$repo/pulls/$number" 2>> "$err" | jq -r '.mergeable_state // "unknown"') || return 1
    [[ $state == dirty ]] || return 1
    conflict_pr=$number
}

while :; do
    if reply=$( { if [[ -z $run ]]; then
                      api "repos/$repo/actions/workflows/$workflow/runs?head_sha=$ref&per_page=1" \
                          | jq -c '.workflow_runs[0] // empty'
                  else
                      api "repos/$repo/actions/runs/$run" | jq -c '.'
                  fi; } 2> "$err" ); then
        errors=0
        if [[ -z $reply && -z $run ]] && in_conflict; then
            echo "No CI run for $ref: pull request #$conflict_pr is in conflict with main, and GitHub"
            echo "does not start the CI on it. Update the branch with main (merge commit, never rebase),"
            echo "resolve the conflicts, push and run scripts/ci-attendi.sh again (docs/agenti/comandi.md)."
            exit 7
        fi
        if [[ -n $reply ]]; then
            run=$(jq -r '.id' <<< "$reply")
            status=$(jq -r '.status' <<< "$reply")
            conclusion=$(jq -r '.conclusion // "-"' <<< "$reply")
            if [[ $status == action_required || $conclusion == action_required ]]; then
                [[ -n ${noted:-} ]] || { note_approval "$reply"; noted=1; }
                age=$(jq -r '(now - (.created_at | fromdateiso8601)) | floor' <<< "$reply" 2> /dev/null) || age=0
                if (( ${age:-0} >= approval_wait )); then
                    rm -f "$(timeouts_file)"
                    echo "run $run: waiting for an approval for $age s. Nothing failed and there is"
                    echo "nothing to decide: stop without the label decisione (docs/agenti/catena.md)."
                    exit 6
                fi
                echo "run $run: waiting for an approval ($age s of $approval_wait)."
            elif [[ $status == completed ]]; then
                rm -f "$(timeouts_file)"
                echo "run $run: completed $conclusion"
                [[ $conclusion == success ]] && exit 0
                if [[ $conclusion == cancelled ]]; then
                    echo "Nothing failed: a newer push on the same branch replaced this run."
                    echo "Run scripts/ci-attendi.sh again, on the newest commit."
                    exit 4
                fi
                exit 1
            fi
            # Still running: a job that already failed is enough to stop waiting.
            failed=$(failed_jobs "$run") || failed=
            if [[ -n $failed ]]; then
                rm -f "$(timeouts_file)"
                echo "run $run: still $status, but a check has already failed:"
                sed 's/^/  - /' <<< "$failed"
                echo "Do not wait for the other jobs: read why with scripts/ci-errori.sh $run"
                exit 1
            fi
        fi
    else
        # A network hiccup or the anonymous limit: try again a few times.
        errors=$((errors + 1))
        if (( errors >= 5 )); then
            echo "The GitHub API did not answer five times in a row. Last error:" >&2
            tail -n 3 "$err" >&2
            if grep -qE '403|429|rate limit' "$err"; then
                echo "It looks like the anonymous request limit: set GITHUB_TOKEN, or wait." >&2
            fi
            exit 2
        fi
    fi
    if (( SECONDS - start + pause > max )); then
        if [[ -z $run ]]; then
            echo "No CI run for $ref after $((SECONDS - start)) s: it has not started, or the commit was not pushed." >&2
            exit 1
        fi
        count=$(( $(cat "$(timeouts_file)" 2> /dev/null || echo 0) + 1 ))
        echo "$count" > "$(timeouts_file)"
        if (( count >= max_timeouts )); then
            echo "run $run: still ${status:-unknown} after $count waits in a row. Do not wait again:"
            echo "write in the pull request what you saw (AGENTS.md, \"Quando fermarsi\"), remove the label, end the session."
            exit 5
        fi
        echo "run $run: still ${status:-unknown} after $((SECONDS - start)) s (wait $count of $max_timeouts). Run the same command again."
        exit 3
    fi
    sleep "$pause"
done
