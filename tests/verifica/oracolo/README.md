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
copyable `? version`, `? units`, `? count`, `? unhandled`, and `? entity` answers.
Answers are printed even when the preceding verification failed. If there
are no public sources, the log says so explicitly.

The standalone command is:

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

`controlla_casi.py` checks version, units, managed/unhandled counts and entity
answers for each source drawing. Counts are unordered lists; numbers use the
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
No public sources, no declared cases and no output drawings is a successful
no-op, explicitly reported in the log.

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
