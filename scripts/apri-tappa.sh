#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Open a stage ("tappa") from the table of its packages: the plan is the only
# source, and this script turns it into GitHub objects and a pull request
# (docs/ROADMAP.md, "Aprire una tappa"; docs/GUIDA-RESPONSABILE.md, B2).
#
#     scripts/apri-tappa.sh <tappa> [--prova | --si] [--senza-project] [--senza-pr]
#
# It reads the table of section 6 of docs/tappe/<tappa>-*.md (format in
# docs/tappe/0000-modello.md), shows what it would do and asks for
# confirmation. Then it:
#
#   1. creates the label tappa-<tappa> and the milestone v<tappa>, if missing;
#   2. creates one issue [<tappa>-PN] or [<tappa>-RN] per row, with the labels
#      of the plan (tappa, pacchetto or riferimenti, difficulty, agente:...) and
#      a body that cites the sheet, the dependencies and the references;
#   3. marks the native "blocked by" dependencies of the new issues, also
#      towards solid-space; where GitHub refuses one, the "Dipende da" line of
#      the body still holds it;
#   4. adds the issues to the Project "Tecnigrafo" with Tappa = tg-<tappa> and
#      Tipo = pacchetto or riferimenti, and Status = Bloccato for new issues that
#      wait for open ones. The Project's own workflow "Item added to project"
#      may move a new item to Da fare after that: the script waits a few
#      seconds, reads the Status again and puts back Bloccato where it moved;
#   5. writes the issue numbers in the table, the state "aperta" and the opening
#      date in the stage document and in docs/ROADMAP.md, on a new branch
#      docs/apertura-<tappa>, and opens a pull request. Merging it stays with
#      the maintainer.
#
# It can run again, for example after adding packages to an open stage: rows
# that already have an issue are left as they are, and only the new ones are
# created. A package without a sheet yet (the Scheda cell is not a link, as for
# the packages that wait for a contract of solid-space) gets no issue, and
# neither do its references nor the rows that depend on it: run the script
# again once the sheets are merged.
#
# --prova          only show what would be done: nothing is written
# --si             do not ask for confirmation
# --senza-project  skip step 4
# --senza-pr       skip step 5 (and the checks on the git working tree)
#
# Exit status: 0 done (failed steps 3 and 4 only print warnings), 1 the plan,
# the options or the working tree are wrong, 2 GitHub refused or did not
# answer, 3 not confirmed.
#
# Needs bash 4, git, jq and gh, logged in with write access to Tecnigrafo and
# read access to the repositories named in "Dipende da". Step 4 also needs the
# "project" scope, once: gh auth refresh -s project. Run it from the main
# branch, up to date and without local changes.
#
# TG_REPO            repository of the stage (default Matleyx78/tecnigrafo)
# TG_PROJECT_OWNER   user that owns the Project (default Matleyx78)
# TG_PROJECT_TITLE   title of the Project (default Tecnigrafo)
# TG_PROJECT_NUMBER  number of the Project, instead of looking it up by title
# TG_PROJECT_PAUSA   seconds to wait before reading the Status again (default 10)
set -euo pipefail

repo=${TG_REPO:-Matleyx78/tecnigrafo}
project_owner=${TG_PROJECT_OWNER:-Matleyx78}
project_title=${TG_PROJECT_TITLE:-Tecnigrafo}
project_number=${TG_PROJECT_NUMBER:-}
project_pause=${TG_PROJECT_PAUSA:-10}
project_fields=''
blob="https://github.com/$repo/blob/main"

usage() {
    echo "usage: apri-tappa.sh <tappa> [--prova | --si] [--senza-project] [--senza-pr]" >&2
    exit 1
}

warn() { echo "WARNING: $*" >&2; }

die() {
    echo "$*" >&2
    exit 1
}

# Calls the GitHub API; on failure prints the error and exits with 2.
api() {
    local out
    if ! out=$(gh api "$@" 2>&1); then
        echo "GitHub refused: gh api $*" >&2
        echo "$out" >&2
        exit 2
    fi
    printf '%s' "$out"
}

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

# Adds an issue to the Project (nothing changes if it is there already) and
# prints the id of its item.
project_add() { # owner/repo number
    gh api -X POST "users/$project_owner/projectsV2/$project_number/items" \
        -f type=Issue -f owner="${1%%/*}" -f repo="${1#*/}" -F number="$2" --jq '.id'
}

# Prints the id of field $1 of the Project, or nothing.
project_field() { # field
    jq -r --arg f "$1" '[.[] | select(.name == $f)][0].id // empty' <<< "$project_fields"
}

# Prints the id of the option $2 of single-select field $1, or nothing.
project_option() { # field option
    jq -r --arg f "$1" --arg o "$2" '[.[] | select(.name == $f) | .options[]?
        | select((.name | if type == "object" then .raw else . end) == $o)][0].id // empty' \
        <<< "$project_fields"
}

# Sets a single-select field of an item; returns 1 with a warning if it cannot.
project_set() { # item field option
    local field option
    field=$(project_field "$2")
    option=$(project_option "$2" "$3")
    if [[ -z $field || -z $option ]]; then
        warn "the Project has no field \"$2\" with the value \"$3\": add the value, then run again"
        return 1
    fi
    if ! jq -n --argjson f "$field" --arg o "$option" '{fields: [{id: $f, value: $o}]}' |
        gh api -X PATCH "users/$project_owner/projectsV2/$project_number/items/$1" \
            --input - > /dev/null; then
        warn "cannot set \"$2\" = \"$3\""
        return 1
    fi
}

# Prints the option id of the current value of single-select field $2 of an
# item, or nothing; returns 1 if GitHub does not answer.
project_value() { # item field
    local out
    out=$(gh api "users/$project_owner/projectsV2/$project_number/items/$1?fields=$(project_field "$2")" \
        2> /dev/null) || return 1
    jq -r --arg f "$2" '[.fields[]? | select(.name == $f)][0].value.id // empty' <<< "$out"
}

# --- Options -----------------------------------------------------------------

tappa=''
mode=ask
with_project=1
with_pr=1
while [[ $# -gt 0 ]]; do
    case $1 in
        --prova) mode=show ;;
        --si) mode=yes ;;
        --senza-project) with_project=0 ;;
        --senza-pr) with_pr=0 ;;
        -h | --help) usage ;;
        -*)
            echo "Unknown option: $1" >&2
            usage
            ;;
        *)
            [[ -z $tappa ]] || usage
            tappa=$1
            ;;
    esac
    shift
done
[[ $tappa =~ ^[0-9]+\.[0-9]+$ ]] || usage
for tool in gh jq git; do
    command -v "$tool" > /dev/null 2>&1 || die "apri-tappa.sh needs $tool."
done

root=$(git rev-parse --show-toplevel 2> /dev/null) || die "Run it inside the Tecnigrafo repository."
cd "$root"
shopt -s nullglob
docs=(docs/tappe/"$tappa"-*.md)
shopt -u nullglob
[[ ${#docs[@]} -eq 1 ]] || die "Expected one docs/tappe/$tappa-*.md, found ${#docs[@]}."
doc=${docs[0]}
today=$(date +%Y-%m-%d)

if [[ $with_pr -eq 1 && $mode != show ]]; then
    [[ -z $(git status --porcelain) ]] ||
        die "The working tree has local changes: commit or stash them, or use --senza-pr."
    git fetch --quiet origin main || die "git fetch origin main failed."
    [[ $(git rev-parse HEAD) == "$(git rev-parse origin/main)" ]] ||
        die "Run it on main, up to date: git checkout main && git pull"
fi

# --- Read the plan -----------------------------------------------------------

stage_title=$(sed -nE '1s/^# *Tappa +[0-9]+\.[0-9]+ *(—|-) *//p' "$doc")
[[ -n $stage_title ]] || die "$doc does not start with \"# Tappa $tappa ...\"."

# Prints one line per row of the package table, fields separated by \x1f:
# line, Pacchetto, Scheda, Issue, Dipende da, Riferimenti, Difficoltà, Agente,
# Modello, Impegno. Exits 3 if the table is missing or a column is missing.
read_table() {
    awk -F'|' '
        function trim(s) { gsub(/^[ \t]+|[ \t]+$/, "", s); return s }
        /^## / { section6 = ($0 ~ /^## 6\./) }
        section6 && !header && /^\| *Pacchetto *\|/ {
            for (i = 2; i < NF; i++) col[trim($i)] = i
            n = split("Pacchetto Scheda Issue Dipende-da Riferimenti Difficoltà Agente Modello Impegno", want, " ")
            for (k = 1; k <= n; k++) {
                name = want[k]; gsub(/-/, " ", name)
                if (!(name in col)) { print "missing column: " name > "/dev/stderr"; exit 3 }
                idx[k] = col[name]
            }
            header = 1; next
        }
        header == 1 && /^\|[-: |]+\|$/ { header = 2; next }
        header == 2 && /^\|/ {
            line = NR
            for (k = 1; k <= n; k++) line = line "\037" trim($(idx[k]))
            print line; next
        }
        header == 2 { exit 0 }
        END { if (header != 2) exit 3 }
    ' "$1"
}

if ! table=$(read_table "$doc"); then
    die "No package table in section 6 of $doc (header \"| Pacchetto | Scheda | Issue | Dipende da | Riferimenti | Difficoltà | Agente | Modello | Impegno |\")."
fi
[[ -n $table ]] || die "The package table of $doc has no rows."

ids=()
declare -A row_line=() row_name=() row_sheet=() row_issue=() row_deps=() row_refs=() \
    row_diff=() row_agent=() row_model=() row_effort=() row_state=() row_why=()
problems=()
sheet_re='\]\(([^)]+\.md)\)'
while IFS=$'\x1f' read -r line pkg sheet issue deps refs diff agent model effort; do
    if [[ ! $pkg =~ ^([PR][0-9]+)[[:space:]]+(.+)$ ]]; then
        problems+=("line $line: the first cell must start with PN or RN and a title: \"$pkg\"")
        continue
    fi
    id=${BASH_REMATCH[1]}
    [[ -z ${row_line[$id]:-} ]] || problems+=("line $line: $id appears twice")
    ids+=("$id")
    row_line[$id]=$line
    row_name[$id]=${BASH_REMATCH[2]}
    row_sheet[$id]=''
    if [[ $sheet =~ $sheet_re ]]; then
        row_sheet[$id]=docs/tappe/${BASH_REMATCH[1]}
        [[ -f ${row_sheet[$id]} ]] || problems+=("line $line: $id: no sheet ${row_sheet[$id]}")
    fi
    row_issue[$id]=''
    [[ $issue =~ ^#([0-9]+)$ ]] && row_issue[$id]=${BASH_REMATCH[1]}
    row_deps[$id]=$deps
    row_refs[$id]=$refs
    row_diff[$id]=$diff
    row_agent[$id]=$agent
    row_model[$id]=$model
    row_effort[$id]=$effort
    [[ $diff =~ ^(facile|media|impegnativa)$ ]] ||
        problems+=("line $line: $id: difficulty \"$diff\" (facile, media, impegnativa)")
    [[ $model =~ ^(leggero|medio|forte)$ ]] ||
        problems+=("line $line: $id: model \"$model\" (leggero, medio, forte)")
    [[ $effort =~ ^(basso|medio|alto)$ ]] ||
        problems+=("line $line: $id: effort \"$effort\" (basso, medio, alto)")
    [[ $agent =~ ^[a-z0-9-]+$ ]] || problems+=("line $line: $id: agent \"$agent\"")
done <<< "$table"

# Splits a "Dipende da" cell into one dependency per line.
split_deps() {
    [[ $1 =~ ^(—|-|)$ ]] && return 0
    tr ',' '\n' <<< "$1" | sed -E 's/^[[:space:]]+|[[:space:]]+$//g' | sed '/^$/d'
}

# Which rows get an issue now. Rows come in plan order: a dependency must come
# earlier in the table.
declare -A seen=()
for id in "${ids[@]}"; do
    work=P${id#?}
    row_state[$id]=create
    [[ -n ${row_issue[$id]} ]] && row_state[$id]=exists
    if [[ $id == R* && -z ${row_line[$work]:-} ]]; then
        problems+=("$id: no package $work in the table")
    elif [[ ${row_state[$id]} == create && -z ${row_sheet[$work]} ]]; then
        row_state[$id]=wait
        if [[ $id == P* ]]; then
            row_why[$id]='no sheet yet'
        else
            row_why[$id]="$work has no sheet yet"
        fi
    fi
    while IFS= read -r dep; do
        if [[ $dep =~ ^[PR][0-9]+$ ]]; then
            if [[ -z ${seen[$dep]:-} ]]; then
                problems+=("$id depends on $dep, which is not earlier in the table")
            elif [[ ${row_state[$dep]} == wait && ${row_state[$id]} == create ]]; then
                row_state[$id]=wait
                row_why[$id]="depends on $dep, which waits"
            fi
        elif [[ ! $dep =~ ^(#[0-9]+|[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+#[0-9]+)$ ]]; then
            problems+=("$id: dependency \"$dep\" is not PN, RN, #N or owner/repo#N")
        fi
    done < <(split_deps "${row_deps[$id]}")
    seen[$id]=1
done

# --- Read GitHub -------------------------------------------------------------

labels=$(api --paginate "repos/$repo/labels?per_page=100" | jq -s 'add | map(.name)')
has_label() { jq -e --arg l "$1" 'index($l) != null' <<< "$labels" > /dev/null; }
for id in "${ids[@]}"; do
    has_label "agente:${row_agent[$id]}" ||
        problems+=("$id: no label agente:${row_agent[$id]} in $repo")
done
if [[ ${#problems[@]} -gt 0 ]]; then
    echo "The plan in $doc is not ready:" >&2
    printf -- '- %s\n' "${problems[@]}" >&2
    exit 1
fi

milestone=$(api "repos/$repo/milestones?state=all&per_page=100" |
    jq -r --arg t "v$tappa" '[.[] | select(.title == $t)][0].number // empty')
existing='[]'
if [[ -n $milestone ]]; then
    existing=$(api --paginate "repos/$repo/issues?milestone=$milestone&state=all&per_page=100" | jq -s 'add')
fi
# A row whose issue is already in the milestone keeps it.
for id in "${ids[@]}"; do
    number=$(jq -r --arg p "[$tappa-$id]" \
        '[.[] | select(.pull_request == null) | select(.title | startswith($p))][0].number // empty' \
        <<< "$existing")
    if [[ -n $number ]]; then
        [[ -z ${row_issue[$id]} || ${row_issue[$id]} == "$number" ]] ||
            die "$id: the table says #${row_issue[$id]}, the milestone has #$number."
        row_issue[$id]=$number
        row_state[$id]=exists
    fi
done

# --- Show and confirm --------------------------------------------------------

labels_of() { # id
    local kind=pacchetto
    [[ $1 == R* ]] && kind=riferimenti
    printf 'tappa-%s,%s,%s,agente:%s' "$tappa" "$kind" "${row_diff[$1]}" "${row_agent[$1]}"
}

to_create=0
echo "Stage $tappa — $stage_title ($doc)"
if [[ -n $milestone ]]; then
    echo "Milestone v$tappa: exists"
else
    echo "Milestone v$tappa: to create"
fi
has_label "tappa-$tappa" && echo "Label tappa-$tappa: exists" || echo "Label tappa-$tappa: to create"
echo
for id in "${ids[@]}"; do
    case ${row_state[$id]} in
        exists) printf '  %-4s exists  #%s\n' "$id" "${row_issue[$id]}" ;;
        wait) printf '  %-4s waits   %s: no issue now\n' "$id" "${row_why[$id]}" ;;
        create)
            to_create=$((to_create + 1))
            printf '  %-4s create  [%s-%s] %s\n' "$id" "$tappa" "$id" "${row_name[$id]}"
            printf '                labels %s; depends on %s\n' "$(labels_of "$id")" "${row_deps[$id]}"
            ;;
    esac
done
echo
if [[ $with_project -eq 1 ]]; then
    echo "Project \"$project_title\": Tappa = tg-$tappa, Tipo = pacchetto or riferimenti; Status = Bloccato for new issues that wait."
fi
if [[ $with_pr -eq 1 ]]; then
    echo "Pull request: issue numbers, state and date in $doc and docs/ROADMAP.md."
fi
case $mode in
    show)
        echo "Nothing written (--prova)."
        exit 0
        ;;
    ask)
        if ! read -r -p "Go on? [s/N] " answer < /dev/tty; then
            echo "No terminal to ask on: run it again with --si." >&2
            exit 3
        fi
        if [[ ! $answer =~ ^[sSyY] ]]; then
            echo "Nothing written."
            exit 3
        fi
        ;;
esac

# --- 1. Label and milestone --------------------------------------------------

if ! has_label "tappa-$tappa"; then
    api -X POST "repos/$repo/labels" -f name="tappa-$tappa" -f color=1d76db \
        -f description="Pacchetti della tappa $tappa" > /dev/null
    echo "Created the label tappa-$tappa."
fi
if [[ -z $milestone ]]; then
    milestone=$(api -X POST "repos/$repo/milestones" -f title="v$tappa" \
        -f description="Tappa $tappa — $stage_title" --jq '.number')
    echo "Created the milestone v$tappa."
fi

# --- 2. Issues ---------------------------------------------------------------

declare -A created=()
for id in "${ids[@]}"; do
    [[ ${row_state[$id]} == create ]] || continue
    number=$(jq -n --arg t "[$tappa-$id] ${row_name[$id]}" --arg l "$(labels_of "$id")" \
        --argjson m "$milestone" \
        '{title: $t, labels: ($l | split(",")), milestone: $m,
          body: "Issue in preparazione: il testo lo scrive scripts/apri-tappa.sh."}' |
        api -X POST "repos/$repo/issues" --input - --jq '.number')
    row_issue[$id]=$number
    created[$id]=1
    echo "Created #$number [$tappa-$id] ${row_name[$id]}"
done

# A dependency as the body shows it: "#12 (P3)" or "Matleyx78/solid-space#20".
dep_text() { # dep
    if [[ $1 =~ ^[PR][0-9]+$ ]]; then
        printf '#%s (%s)' "${row_issue[$1]}" "$1"
    else
        printf '%s' "$1"
    fi
}

sheet_link() { # path
    printf '[`%s`](%s/%s)' "$1" "$blob" "$1"
}

body_of() { # id
    local id=$1 work deps='' dep line
    while IFS= read -r dep; do
        deps+="${deps:+, }$(dep_text "$dep")"
    done < <(split_deps "${row_deps[$id]}")
    line="**Tappa:** [$tappa — $stage_title]($blob/$doc)"
    if [[ $id == P* ]]; then
        local refs=${row_refs[$id]}
        if [[ $refs =~ ^(R[0-9]+)([^0-9]|$) ]]; then
            local ref=${BASH_REMATCH[1]}
            [[ -z ${row_issue[$ref]:-} ]] || refs="#${row_issue[$ref]} ($ref)"
        fi
        cat << EOF
$line · **Pacchetto:** $id
**Scheda:** $(sheet_link "${row_sheet[$id]}")
**Difficoltà:** ${row_diff[$id]} · **Modello:** ${row_model[$id]} · **Impegno:** ${row_effort[$id]}
**Dipende da:** ${deps:-—}
**Riferimenti:** $refs

Leggi la **scheda** del pacchetto: file, interfacce, test e criterio di "fatto". Fa fede la scheda; del piano basta la tabella dei pacchetti.

## Per occupartene

Scrivi un commento e aspetta l'assegnazione prima di iniziare. Leggi [CONTRIBUTING.md]($blob/CONTRIBUTING.md); se usi un'IA, anche [AGENTS.md]($blob/AGENTS.md).
EOF
    else
        work=P${id#?}
        echo "$line · **Riferimenti del pacchetto:** $work (#${row_issue[$work]:-…})"
        [[ -z ${row_sheet[$id]} ]] || echo "**Scheda:** $(sheet_link "${row_sheet[$id]}") (fa fede la scheda)"
        [[ -z ${row_sheet[$work]} ]] || echo "**Scheda del lavoro:** $(sheet_link "${row_sheet[$work]}")"
        cat << EOF
**Difficoltà:** ${row_diff[$id]} · **Modello:** ${row_model[$id]} · **Impegno:** ${row_effort[$id]}
**Dipende da:** ${deps:-—}

Scrivi i casi di verifica del pacchetto $work prima che il lavoro cominci: ogni condizione "Fatto quando" della scheda del lavoro ha almeno un caso. Le regole sono in [AGENTS.md]($blob/AGENTS.md), sezione "Modalità di controllo e riferimenti", e nell'[ADR 0011]($blob/docs/adr/0011-modalita-di-controllo.md). I valori geometrici che stanno nell'archivio dei casi di solid-space si prendono da lì, citando il caso ([ADR 0030]($blob/docs/adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 14).

## In breve

- Tocca solo \`tests/verifica/casi/\`, \`tests/verifica/sorgenti.json\` e \`tests/verifica/in-attesa.txt\`.
- Valori attesi da calcoli, file pubblici o programmi di riferimento, **mai** eseguendo Tecnigrafo, \`tecnigrafo-script\` o \`tecnigrafo-mcp\`.
- File presi dal web: indirizzo, versione, hash SHA-256 e licenza in \`sorgenti.json\`, link alla licenza nella pull request.
- I nuovi casi vanno in \`in-attesa.txt\` con il numero della issue del lavoro.
EOF
    fi
}

for id in "${ids[@]}"; do
    [[ -n ${created[$id]:-} ]] || continue
    body_of "$id" | jq -Rs '{body: .}' |
        api -X PATCH "repos/$repo/issues/${row_issue[$id]}" --input - > /dev/null
done

# --- 3. Native dependencies --------------------------------------------------

# Prints "<id> <state>" of the issue a dependency names, or nothing.
dep_issue() { # dep
    local where number
    if [[ $1 =~ ^[PR][0-9]+$ ]]; then
        where=$repo number=${row_issue[$1]}
    elif [[ $1 =~ ^#([0-9]+)$ ]]; then
        where=$repo number=${BASH_REMATCH[1]}
    else
        where=${1%#*} number=${1##*#}
    fi
    gh api "repos/$where/issues/$number" --jq '"\(.id) \(.state)"' 2> /dev/null || true
}

declare -A waiting=()
for id in "${ids[@]}"; do
    [[ -n ${created[$id]:-} ]] || continue
    while IFS= read -r dep; do
        read -r dep_id dep_state <<< "$(dep_issue "$dep")" || true
        if [[ -z ${dep_id:-} ]]; then
            warn "#${row_issue[$id]}: cannot read $dep; the \"Dipende da\" line still holds it"
            waiting[$id]=1
            continue
        fi
        [[ $dep_state == open ]] && waiting[$id]=1
        if ! gh api -X POST "repos/$repo/issues/${row_issue[$id]}/dependencies/blocked_by" \
            -F issue_id="$dep_id" > /dev/null 2>&1; then
            warn "#${row_issue[$id]}: GitHub did not mark \"blocked by $dep\"; the \"Dipende da\" line still holds it"
        fi
    done < <(split_deps "${row_deps[$id]}")
done

# --- 4. Project --------------------------------------------------------------

if [[ $with_project -eq 1 ]]; then
    if project_load; then
        declare -A items=()
        for id in "${ids[@]}"; do
            [[ -n ${row_issue[$id]} ]] || continue
            if ! items[$id]=$(project_add "$repo" "${row_issue[$id]}"); then
                warn "cannot add #${row_issue[$id]} to the Project"
                continue
            fi
            kind=pacchetto
            [[ $id == R* ]] && kind=riferimenti
            project_set "${items[$id]}" Tappa "tg-$tappa" || true
            project_set "${items[$id]}" Tipo "$kind" || true
        done
        # Last, so that the Project's own "item added" workflow does not undo it.
        blocked=()
        for id in "${!waiting[@]}"; do
            [[ -n ${items[$id]:-} ]] || continue
            project_set "${items[$id]}" Status Bloccato && blocked+=("$id")
        done
        # That workflow runs on GitHub's side a little later and may move the
        # new items to Da fare: wait, read the Status again and put it back.
        # Up to three rounds, as long as some item had moved.
        bloccato=$(project_option Status Bloccato)
        for round in 1 2 3; do
            [[ ${#blocked[@]} -gt 0 ]] || break
            sleep "$project_pause"
            moved=()
            for id in "${blocked[@]}"; do
                if ! value=$(project_value "${items[$id]}" Status); then
                    warn "#${row_issue[$id]}: cannot read its Status in the Project: check that it is Bloccato"
                elif [[ $value != "$bloccato" ]]; then
                    project_set "${items[$id]}" Status Bloccato && moved+=("$id")
                fi
            done
            [[ ${#moved[@]} -eq 0 ]] ||
                echo "Round $round: Status put back to Bloccato for ${#moved[@]} issue(s)."
            blocked=("${moved[@]}")
        done
        echo "Issues added to the Project \"$project_title\"."
    else
        warn "add the issues to the Project by hand (Tappa = tg-$tappa, Tipo = pacchetto or riferimenti)"
    fi
fi

# --- 5. Documents and pull request -------------------------------------------

[[ $with_pr -eq 1 ]] || { echo "Done (--senza-pr): the documents are unchanged."; exit 0; }

numbers=''
for id in "${ids[@]}"; do
    [[ -n ${row_issue[$id]} ]] && numbers+="${row_line[$id]}=${row_issue[$id]} "
done

# Writes the issue numbers in the Issue column of the table.
write_numbers() { # file numbers
    awk -F'|' -v OFS='|' -v numbers="$2" '
        BEGIN { n = split(numbers, pairs, " ")
                for (k = 1; k <= n; k++) { split(pairs[k], kv, "="); num[kv[1]] = kv[2] } }
        /^## / { section6 = ($0 ~ /^## 6\./) }
        section6 && !col && /^\| *Pacchetto *\|/ {
            for (i = 2; i < NF; i++) { c = $i; gsub(/^ +| +$/, "", c); if (c == "Issue") col = i }
        }
        (NR in num) && col { $col = " #" num[NR] " " }
        { print }
    ' "$1" > "$1.tmp" && mv "$1.tmp" "$1"
}

branch=docs/apertura-$tappa
if git rev-parse --verify --quiet "refs/heads/$branch" > /dev/null ||
    git ls-remote --exit-code --heads origin "$branch" > /dev/null 2>&1; then
    branch=docs/apertura-$tappa-$(date +%Y%m%d-%H%M)
fi
git switch --quiet -c "$branch"

write_numbers "$doc" "$numbers"
opened=0
if grep -qE '^- \*\*Stato:\*\* *(in programma|in preparazione)' "$doc"; then
    opened=1
    sed -i -E 's/^(- \*\*Stato:\*\*) *(in programma|in preparazione).*/\1 aperta/' "$doc"
    if grep -q '^- \*\*Apertura:\*\*' "$doc"; then
        sed -i -E "/^- \*\*Apertura:\*\*/s/AAAA-MM-GG/$today/" "$doc"
    else
        sed -i -E "/^- \*\*Milestone GitHub:\*\*/a - **Apertura:** $today" "$doc"
    fi
    awk -F'|' -v OFS='|' -v v="$tappa" '
        /^\| *[0-9]+ *\| *[0-9.]+ *\|/ { c = $3; gsub(/ /, "", c); if (c == v) $5 = " **aperta** " }
        { print }
    ' docs/ROADMAP.md > docs/ROADMAP.md.tmp && mv docs/ROADMAP.md.tmp docs/ROADMAP.md
fi

if [[ -z $(git status --porcelain) ]]; then
    git switch --quiet main
    git branch --quiet -D "$branch"
    echo "Done: the documents already had every number, no pull request."
    exit 0
fi
if [[ $opened -eq 1 ]]; then
    subject="docs(tappe): open stage $tappa"
    pr_title="Apertura della tappa $tappa"
else
    subject="docs(tappe): add the new issues of stage $tappa"
    pr_title="Nuove issue della tappa $tappa"
fi
git add "$doc" docs/ROADMAP.md
git commit --quiet -m "$subject" -m "Issue numbers written by scripts/apri-tappa.sh."
git push --quiet -u origin "$branch" || die "git push failed: the commit is on the local branch $branch."
git switch --quiet main

pr_body="Scritta da \`scripts/apri-tappa.sh $tappa\`: numeri delle issue nella tabella dei pacchetti"
[[ $opened -eq 1 ]] && pr_body+=", stato e data di apertura nel documento della tappa e nella ROADMAP"
pr_body+=". Le issue, il milestone \`v$tappa\` e le etichette sono già su GitHub; controlla le issue nel milestone e nel Project, poi unisci."
pr=$(jq -n --arg t "$pr_title" --arg h "$branch" --arg b "$pr_body" \
    '{title: $t, head: $h, base: "main", body: $b}' |
    api -X POST "repos/$repo/pulls" --input - --jq '.html_url')
echo "Pull request: $pr"
