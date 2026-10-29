#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Open a request to solid-space ("richiesta") from a Markdown file: the request
# lives only in solid-space (ADR 0030, point 11; docs/RICHIESTE-A-SOLID-SPACE.md).
#
#     scripts/richiesta.sh <file> [--tappa ss-X.Y] [--prova | --si] [--senza-project]
#
# The file holds the title as "# Richiesta: ..." and the six sections as "## ..."
# headings, named as in the issue form of solid-space
# (.github/ISSUE_TEMPLATE/richiesta.yml): Da dove nasce, Il bisogno, Casi
# concreti, Quando è soddisfatta, Vincoli, Che cosa blocca. The script:
#
#   1. checks what the solid-space workflow controlla-richiesta.yml checks: the
#      title, the six sections not empty, a Matleyx78/tecnigrafo#N reference in
#      "Da dove nasce", no relative #N reference; it stops on the first problems;
#   2. refuses to open a second request with the same title;
#   3. shows the issue and asks for confirmation;
#   4. opens it in solid-space with the label "richiesta";
#   5. adds it to the Project "Tecnigrafo" with Tipo = richiesta and, with
#      --tappa, Tappa = that value (for example ss-0.2);
#   6. prints the reference to cite in the plan: Matleyx78/solid-space#N.
#
# --prova          stop after showing the issue: nothing is written
# --si             do not ask for confirmation. An agent uses it only when the
#                  maintainer asked in the chat to open this very request
# --senza-project  skip step 5
#
# Exit status: 0 done (a failed Project step only prints a warning), 1 the file
# or the options are wrong, 2 GitHub refused or did not answer, 3 not confirmed.
#
# Needs bash 4, jq and gh, logged in with write access to solid-space. Step 5
# also needs the "project" scope, once: gh auth refresh -s project.
#
# TG_SOLID_SPACE     repository of the request (default Matleyx78/solid-space)
# TG_PROJECT_OWNER   user that owns the Project (default Matleyx78)
# TG_PROJECT_TITLE   title of the Project (default Tecnigrafo)
# TG_PROJECT_NUMBER  number of the Project, instead of looking it up by title
set -euo pipefail

repo=${TG_SOLID_SPACE:-Matleyx78/solid-space}
project_owner=${TG_PROJECT_OWNER:-Matleyx78}
project_title=${TG_PROJECT_TITLE:-Tecnigrafo}
project_number=${TG_PROJECT_NUMBER:-}
project_fields=''

# Section names of the issue form, in form order. Rename them together with
# richiesta.yml and controlla-richiesta.js of solid-space.
sections=(
    'Da dove nasce'
    'Il bisogno'
    'Casi concreti'
    'Quando è soddisfatta'
    'Vincoli'
    'Che cosa blocca'
)

usage() {
    echo "usage: richiesta.sh <file> [--tappa ss-X.Y] [--prova | --si] [--senza-project]" >&2
    exit 1
}

warn() { echo "WARNING: $*" >&2; }

# --- Project "Tecnigrafo" (REST API for user projects) ----------------------

# Loads number and fields of the Project; returns 1 with a warning if it cannot.
project_load() {
    local out
    if [[ -z $project_number ]]; then
        if ! out=$(gh api "users/$project_owner/projectsV2?per_page=100" 2>&1); then
            warn "cannot read the projects of $project_owner: $out"
            warn "if it speaks of scopes or permissions, run once: gh auth refresh -s project"
            return 1
        fi
        project_number=$(jq -r --arg t "$project_title" \
            '[.[] | select(.title == $t)][0].number // empty' <<< "$out")
        if [[ -z $project_number ]]; then
            warn "no project titled \"$project_title\" for $project_owner"
            return 1
        fi
    fi
    if ! out=$(gh api --paginate \
        "users/$project_owner/projectsV2/$project_number/fields?per_page=100" 2>&1); then
        warn "cannot read the fields of project $project_number: $out"
        return 1
    fi
    project_fields=$(jq -s 'add' <<< "$out")
}

# Adds an issue to the Project and prints the id of its item.
project_add() { # owner/repo number
    gh api -X POST "users/$project_owner/projectsV2/$project_number/items" \
        -f type=Issue -f owner="${1%%/*}" -f repo="${1#*/}" -F number="$2" --jq '.id'
}

# Sets a single-select field of an item; returns 1 with a warning if it cannot.
project_set() { # item field option
    local field option
    field=$(jq -r --arg f "$2" '[.[] | select(.name == $f)][0].id // empty' <<< "$project_fields")
    option=$(jq -r --arg f "$2" --arg o "$3" '[.[] | select(.name == $f) | .options[]?
        | select((.name | if type == "object" then .raw else . end) == $o)][0].id // empty' \
        <<< "$project_fields")
    if [[ -z $field || -z $option ]]; then
        warn "the Project has no field \"$2\" with the value \"$3\": add the value, then set it by hand"
        return 1
    fi
    if ! jq -n --argjson f "$field" --arg o "$option" '{fields: [{id: $f, value: $o}]}' |
        gh api -X PATCH "users/$project_owner/projectsV2/$project_number/items/$1" \
            --input - > /dev/null; then
        warn "cannot set \"$2\" = \"$3\""
        return 1
    fi
}

# --- Options -----------------------------------------------------------------

file=''
tappa=''
mode=ask
with_project=1
while [[ $# -gt 0 ]]; do
    case $1 in
        --tappa)
            [[ $# -ge 2 ]] || usage
            tappa=$2
            shift 2
            ;;
        --prova) mode=show; shift ;;
        --si) mode=yes; shift ;;
        --senza-project) with_project=0; shift ;;
        -h | --help) usage ;;
        -*)
            echo "Unknown option: $1" >&2
            usage
            ;;
        *)
            [[ -z $file ]] || usage
            file=$1
            shift
            ;;
    esac
done
[[ -n $file ]] || usage
if [[ ! -r $file ]]; then
    echo "Cannot read $file" >&2
    exit 1
fi
for tool in gh jq; do
    if ! command -v "$tool" > /dev/null 2>&1; then
        echo "richiesta.sh needs $tool." >&2
        exit 1
    fi
done

# --- Read the file -----------------------------------------------------------

title=''
current=''
fence=''
declare -A content=()
problems=()
fence_re='^(```|~~~)'
while IFS= read -r line || [[ -n $line ]]; do
    line=${line%$'\r'}
    if [[ $line =~ $fence_re ]]; then
        if [[ -z $fence ]]; then
            fence=${BASH_REMATCH[1]}
        elif [[ $fence == "${BASH_REMATCH[1]}" ]]; then
            fence=''
        fi
    elif [[ -z $fence && -z $title && -z $current && $line =~ ^#[[:space:]]+(.*[^[:space:]]) ]]; then
        title=${BASH_REMATCH[1]}
        continue
    elif [[ -z $fence && $line =~ ^\#\#\#?[[:space:]]+([^#]*[^#[:space:]]) ]]; then
        current=${BASH_REMATCH[1]}
        known=0
        for name in "${sections[@]}"; do
            [[ $current == "$name" ]] && known=1
        done
        if [[ $known -eq 0 ]]; then
            problems+=("Unknown section \"$current\" (the sections are: $(printf '"%s" ' "${sections[@]}"))")
        fi
        content[$current]=${content[$current]:-}
        continue
    fi
    if [[ -n $current ]]; then
        content[$current]+="$line"$'\n'
    elif [[ $line =~ [^[:space:]] ]]; then
        problems+=("Text before the first section: \"$line\"")
    fi
done < "$file"

# Removes blank lines at both ends.
trim() {
    sed -e '/[^[:space:]]/,$!d' <<< "$1" | tac | sed -e '/[^[:space:]]/,$!d' | tac
}

# Removes the parts of Markdown where a #N is not a reference.
strip_non_references() {
    awk '
        /^(```|~~~)/ { if (fence == "") { fence = substr($0, 1, 3); next }
                       if (substr($0, 1, 3) == fence) { fence = ""; next } }
        fence == "" { print }
    ' <<< "$1" | sed -E -e 's/<!--([^-]|-[^-])*-->/ /g' -e 's/`[^`]*`/ /g' \
        -e 's#https?://[^[:space:]]*# #g'
}

if [[ ! $title =~ ^Richiesta:[[:space:]]*[^[:space:]] ]]; then
    problems+=("The first line must be \"# Richiesta: \" followed by the need in one sentence")
fi
body=''
for name in "${sections[@]}"; do
    if [[ ! -v content[$name] ]]; then
        problems+=("Missing section \"$name\"")
        continue
    fi
    text=$(trim "$(sed -E 's/<!--([^-]|-[^-])*-->//g' <<< "${content[$name]}")")
    if [[ -z $text || $text =~ ^_[Nn]o\ response_$ ]]; then
        problems+=("Empty section \"$name\"")
        continue
    fi
    if [[ $name == 'Da dove nasce' ]] && ! grep -qiE \
        'Matleyx78/tecnigrafo#[0-9]+|github\.com/Matleyx78/tecnigrafo/(issues|pull|discussions)/[0-9]+' \
        <<< "$text"; then
        problems+=("\"Da dove nasce\" does not cite Tecnigrafo: write Matleyx78/tecnigrafo#N")
    fi
    body+="### $name"$'\n\n'"$text"$'\n\n'
done
relative=$(strip_non_references "$title"$'\n'"$body" |
    grep -oE '(^|[^[:alnum:]_/.#-])[[:alnum:]_.-]*#[0-9]+' |
    sed -E 's/^[^[:alnum:]_.#-]//' | sort -u | paste -sd ' ' || true)
if [[ -n $relative ]]; then
    problems+=("References that are not absolute: $relative. Write Matleyx78/tecnigrafo#N or Matleyx78/solid-space#N")
fi
if [[ ${#problems[@]} -gt 0 ]]; then
    echo "The request in $file is not ready:" >&2
    printf -- '- %s\n' "${problems[@]}" >&2
    exit 1
fi
body=${body%$'\n\n'}$'\n'

# --- Show and confirm --------------------------------------------------------

if ! existing=$(gh api "repos/$repo/issues?labels=richiesta&state=all&per_page=100" 2>&1); then
    echo "GitHub did not answer: $existing" >&2
    exit 2
fi
same=$(jq -r --arg t "$title" '[.[] | select(.title == $t)][0].number // empty' <<< "$existing")
if [[ -n $same ]]; then
    echo "A request with this title already exists: $repo#$same" >&2
    exit 1
fi

echo "Repository: $repo    Label: richiesta"
[[ $with_project -eq 1 ]] && echo "Project:    $project_title (Tipo = richiesta${tappa:+, Tappa = $tappa})"
echo "Title:      $title"
echo "------------------------------------------------------------------------"
printf '%s' "$body"
echo "------------------------------------------------------------------------"
case $mode in
    show)
        echo "Nothing written (--prova)."
        exit 0
        ;;
    ask)
        if ! read -r -p "Open this request in $repo? [s/N] " answer < /dev/tty; then
            echo "No terminal to ask on: run it again with --si, if it was asked for." >&2
            exit 3
        fi
        if [[ ! $answer =~ ^[sSyY] ]]; then
            echo "Nothing written."
            exit 3
        fi
        ;;
esac

# --- Open --------------------------------------------------------------------

if ! created=$(jq -n --arg t "$title" --arg b "$body" '{title: $t, body: $b, labels: ["richiesta"]}' |
    gh api -X POST "repos/$repo/issues" --input - 2>&1); then
    echo "GitHub did not open the issue: $created" >&2
    exit 2
fi
number=$(jq -r '.number' <<< "$created")
echo "Opened $repo#$number: $(jq -r '.html_url' <<< "$created")"

if [[ $with_project -eq 1 ]]; then
    if project_load && item=$(project_add "$repo" "$number"); then
        project_set "$item" Tipo richiesta || true
        [[ -z $tappa ]] || project_set "$item" Tappa "$tappa" || true
        echo "Added to the Project \"$project_title\"."
    else
        warn "add it to the Project by hand: + Add item, the link above, Tipo = richiesta${tappa:+, Tappa = $tappa}"
    fi
fi
echo
echo "Cite it in the plan as: $repo#$number"
