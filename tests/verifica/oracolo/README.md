<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->

# ezdxf oracle

The oracle is independent verification infrastructure (ADR 0011, section 6;
ADR 0020). It reads DXF files with ezdxf, never runs Tecnigrafo, and is not
linked to or distributed with the application. Install and run it only in CI.
The pinned wheels target the Verifica runner: Ubuntu 24.04, CPython 3.12,
x86_64. `requirements.txt` pins every transitive dependency and its SHA-256.

## Reading answers

Open the pull request's CI run, then **Verifica → Oracolo ezdxf: answers**.
Every public source listed in `tests/verifica/sorgenti.json` has a heading and
copyable `? version`, `? units`, `? count`, `? unhandled`, `? layers`, `? linetypes`, and `? entity` answers.
Answers are printed even when the preceding verification failed. If there
are no public sources, the log says so explicitly.

Once per printing invocation, the `=== ezdxf: standard ===` section lists
`? aci <n> -> r,g,b` for all indices 1–255, including when no public sources
are listed. The standard section contains only ACI answers. The ISO linetypes
`ACAD_ISO02W100` and `ACAD_ISO10W100` come from the AutoCAD DXF source
`autocad-colori-layer-tipilinea`, through its `? linetypes D` answer
(ADR 0029); ezdxf's standard catalog does not define them.

The standalone command for one drawing is:

```sh
python3 tests/verifica/oracolo/ezdxf_attesi.py drawing.dxf --name D
```

Numbers, property order, quoting, widths, elevation, thickness and extrusion
follow `docs/verifica/rapporto.md`; angles remain in degrees. Entities are
classified by ADR 0018, section 3, including paper space, non-model owners,
unsupported extrusion and degenerate geometry. Block definitions are excluded.
For negative extrusion the log includes
`; OCS: convert by hand (ADR 0018, point 5)` and **raw file coordinates**.
Those entity answers require manual conversion before copying into a case.
Font-derived properties such as `~box` are outside this oracle's scope.

Layer rows contain color, linetype and lineweight, then active states in order:
`off frozen locked noplot`. Linetype rows contain the signed group-49 pattern
(including zero-length dots), or `continuous`, and the quoted description.
The three built-in types ByBlock, ByLayer and Continuous are omitted.
Both table answers preserve file order and return `none` for an empty table.
Original table records are read independently: defaults inserted by ezdxf do
not become answers for a file without tables.

Entity answers append `layer=`, `color=`, `linetype=`, `ltscale=` and
`lineweight=`, in that order, only when different from the defaults.
Names use the spelling in the file's tables. RGB (group 420) takes precedence
over ACI (group 62), including for switched-off layers; a negative layer ACI
still supplies the `off` state. Lineweights use millimetres with two decimals,
`0`, `bylayer`, `byblock` or `default`.

## Declaring a case

The very first line of `domande.tgq` must be:

```text
; oracle: ezdxf
D = DXF source=public-source-name
? version D -> AC1032
? count D -> LINE 1
? entity D 2F -> LINE start=0,0 end=10,0
```

Use a source identifier from `sorgenti.json`. Authors add sources in a reference
pull request, read the oracle log, then copy the independent answers into cases.
Do not derive expected values by executing Tecnigrafo.

`controlla_casi.py` checks version, units, managed/unhandled counts, layers,
linetypes and entity answers for each source drawing, plus source-independent
`aci` questions. Counts and table rows are unordered lists; quoted semicolons
do not split rows; numbers use the
case's absolute/relative tolerances, updated by each `!tolerance` directive
(defaults: `1e-9` absolute, zero relative). Quoted text is compared exactly.
Derived properties are ignored. Unsupported questions, including `roundtrip`
and geometry queries, are logged as skipped. Negative-OCS entity questions
requiring manual conversion are also explicitly skipped; `LINE` already uses
global coordinates and can still be compared.

```sh
python3 tests/verifica/oracolo/controlla_casi.py tests/verifica/casi build/verifica-sorgenti
```

The command prints source answers, checks declared cases and audits all output
DXFs and `drawing.dxf` members of CADX files under
`build/ci/tests/verifica/casi*`. Both audit errors and automatic repairs fail
the check. `--outputs PATH` overrides the output root and can be repeated.
No public sources, no declared cases and no output drawings is successful:
the log reports their absence and still prints the standard ACI section.

CI splits the invocation: `--print-only` runs with `if: always()` to expose
source answers after failed verification; `--check-only` runs normally, so
checks and audit are never excused by the printing step.

## Updating ezdxf

Use a dedicated, approved infrastructure package. Download the wheels for the
CI interpreter and platform, pin ezdxf and every transitive dependency, and
record the SHA-256 of each wheel in `requirements.txt`. Install with
`python3 -m pip install --require-hashes --only-binary=:all: -r ...` in CI.
Recheck **all** declared oracle cases and inspect every changed answer before
accepting the update. Reference changes belong in a separate reference PR;
never adjust them silently to make the oracle pass.

## Infrastructure self-test

The normal CI oracle check also uses `--self-test`. It creates handwritten
DXF probes in a temporary directory, prints their standalone oracle output, and
checks missing units, unsupported entities, negative OCS, paper space, foreign
owners, degenerate geometry, handle lookup, case tolerances, mismatch diagnostics,
exact text, unordered counts and DXF/CADX audit success and failure.
A further handwritten DXF has two layers, one dash-dot linetype, RGB colors,
lineweights and all four layer states. It checks signed gaps and dots, exact
quoting, case-insensitive table lookup, RGB precedence, inherited properties,
missing tables, all 255 standard answers, invalid ACI indices, positive
comparisons and deliberate mismatches for every new question. It never
adds reference cases or derives expected values from Tecnigrafo. The probe output
is in **Verifica → Oracolo ezdxf** and is copied into the package PR as text.
