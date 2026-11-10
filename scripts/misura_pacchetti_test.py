# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/misura_pacchetti.py, without network: GitHub is replaced by
# answers written here (docs/tappe/0.7.5/P39-misura-pacchetti.md).
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import misura_pacchetti as mp  # noqa: E402

PLAN = """# Tappa 0.7.5

## 5. Prerequisiti

| Pacchetto | Altro |
|---|---|
| P99 | non questa |

## 6. Pacchetti di lavoro

Testo.

| Pacchetto | Scheda | Issue | Dipende da | Riferimenti | Difficoltà | Agente | Modello | Impegno |
|---|---|---|---|---|---|---|---|---|
| P1 CI: immagini | [P1](0.7.5/P1.md) | #10 | — | non servono | media | claude | medio | alto |
| R2 Riferimenti: `NEW` | — | #11 | — | è un pacchetto di riferimenti | media | gemini | leggero | medio |
| P2 `cmd`: `NEW` | [P2](0.7.5/P2.md) | #12 | P1, R2 | R2 (gemini) | media | codex | medio | — |

Dopo la tabella.

| Pacchetto | Altro |
|---|---|
| P98 | non questa |

## 7. Prova manuale
"""

OLD_PLAN = """## 6. Pacchetti di lavoro

| Pacchetto | Dipende da | Riferimenti |
|---|---|---|
| P1 Linea | — | R1 |
"""


def ai(text: str) -> str:
    return f"Closes #1\n\n## Cosa cambia\n\nTesto.\n\n## Uso dell'IA\n\n{text}\n"


class Codes(unittest.TestCase):
    def test_two_and_three_numbers_and_both_prefixes(self):
        self.assertEqual(mp.codes("[0.7-P5] e 0.7.5-R28b, poi 0.8-PT12 e 0.8-RT3a"),
                         ["0.7-P5", "0.7.5-R28b", "0.8-PT12", "0.8-RT3a"])

    def test_prefix_must_fit_the_stage(self):
        # P/R up to 0.7.5, PT/RT from 0.8.
        self.assertEqual(mp.codes("0.8-P5 0.7.5-PT5 0.7-RT2 1.0-PT1"), ["1.0-PT1"])

    def test_no_partial_matches(self):
        self.assertEqual(mp.codes("0.7.5-P24"), ["0.7.5-P24"])
        self.assertEqual(mp.codes("x0.7-P2 10.7.5-P2"), [])
        self.assertEqual(mp.codes("0.6-P3-Qt e 0.7.5-P2."), ["0.6-P3-Qt", "0.7.5-P2"])
        self.assertEqual(mp.codes("0.7.5-P1 e di nuovo 0.7.5-P1"), ["0.7.5-P1"])

    def test_letter(self):
        self.assertEqual(mp.code_letter("0.8-PT12"), "P")
        self.assertEqual(mp.code_letter("R28b"), "R")


class Declared(unittest.TestCase):
    def test_tool_model_and_effort(self):
        d = mp.declared(ai("- [x] IA usata: Claude Code, Claude Opus 5.5, impegno medio — ne rispondo io"))
        self.assertEqual((d.tool, d.model, d.effort), ("claude", "Claude Opus 5.5", "medio"))

    def test_without_effort(self):
        d = mp.declared(ai("- [x] IA usata: Codex, GPT-6.1 Sol. Il commit riporta `Assisted-by: Codex`."))
        self.assertEqual((d.tool, d.model, d.effort), ("codex", "GPT-6.1 sol", mp.NOT_DECLARED))

    def test_without_model(self):
        d = mp.declared(ai("- [x] IA usata: Codex — commit con Assisted-by."))
        self.assertEqual((d.tool, d.model, d.effort), ("codex", mp.NOT_DECLARED, mp.NOT_DECLARED))

    def test_effort_not_known_and_in_brackets(self):
        self.assertEqual(mp.declared(ai("Codex in ChatGPT Work, GPT-6, impegno non noto.")).effort, "non noto")
        self.assertEqual(mp.declared(ai("Claude Code, Claude Opus 5.5 (impegno alto)")).effort, "alto")
        self.assertEqual(mp.declared(ai("Codex, GPT-6.1 sol medium")).effort, "medio")

    def test_template_comment_is_not_a_declaration(self):
        body = ("## Uso dell'IA\n\n- [ ] Nessuna IA usata\n- [x] IA usata: <!-- strumento, modello e impegno, "
                "es. Claude Code, Claude Opus 5.5, impegno medio --> Antigravity, Gemini 3.8 Flash — ok\n")
        d = mp.declared(body)
        self.assertEqual((d.tool, d.model, d.effort), ("gemini", "Gemini 3.8 Flash", mp.NOT_DECLARED))

    def test_several_models_in_order(self):
        d = mp.declared(ai("IA usata: Codex, GPT-6 (prima versione); Claude Code, Claude Opus 5.5 (correzioni)"))
        self.assertEqual((d.tool, d.model), ("codex", "GPT-6 + Claude Opus 5.5"))
        d = mp.declared(ai("IA usata: Antigravity, Gemini 3.8 Flash e Gemini 4 Argon"))
        self.assertEqual(d.model, "Gemini 3.8 Flash + Gemini 4 Argon")

    def test_no_section(self):
        # Nothing is declared without the section; for the author of an old
        # pull request the first tool named in the text is only a hint.
        body = "Closes #3\n\nScritto con Codex, GPT-6, impegno alto; rivisto con Claude Code."
        d = mp.declared(body)
        self.assertEqual((d.tool, d.model, d.effort), ("?", mp.NOT_DECLARED, mp.NOT_DECLARED))
        self.assertEqual(mp.author_hint(body), "codex")
        self.assertEqual(mp.author_hint("Closes #3\n\nNiente."), "?")

    def test_mention_outside_the_section_is_not_declared(self):
        body = ("Corregge un problema trovato da Claude.\n\n## Uso dell'IA\n\n"
                "- [x] Nessuna IA usata\n- [ ] IA usata: <!-- strumento, modello e impegno -->\n")
        d = mp.declared(body)
        self.assertEqual((d.tool, d.model, d.effort), (mp.NO_AI, mp.NOT_DECLARED, mp.NOT_DECLARED))
        self.assertEqual(mp.author_hint(body), "?")
        # A section that names no tool: still nothing taken from the text.
        body = "Corregge un problema trovato da Claude.\n\n## Uso dell'IA\n\n- [x] IA usata\n"
        self.assertEqual((mp.declared(body).tool, mp.author_hint(body)), ("?", "?"))

    def test_no_ai_and_maintainer_declaration(self):
        row = {"tappa": "0.7.5", "diff": "media", "autore": "nessuna",
               "main": {"declared": {"tool": mp.NO_AI, "model": mp.NOT_DECLARED, "effort": mp.NOT_DECLARED}}}
        self.assertEqual(mp.declaration(row)["impegno"], mp.NOT_DECLARED)
        row["main"]["author_hint"] = "claude"
        row["code"] = "0.7.5-P1"
        self.assertEqual(mp.author(row), mp.NO_AI)


class Plan(unittest.TestCase):
    def test_reads_the_table_of_section_6(self):
        rows = mp.plan_table(PLAN)
        self.assertEqual([r.package for r in rows], ["P1", "R2", "P2"])
        p1, r2, p2 = rows
        self.assertEqual((p1.issue, p1.deps, p1.difficulty, p1.agent, p1.model, p1.effort),
                         (10, [], "media", "claude", "medio", "alto"))
        self.assertEqual((r2.issue, r2.agent, r2.model), (11, "gemini", "leggero"))
        self.assertEqual((p2.deps, p2.effort), (["P1", "R2"], mp.NOT_DECLARED))

    def test_missing_columns_are_not_declared(self):
        (row,) = mp.plan_table(OLD_PLAN)
        self.assertEqual((row.package, row.issue, row.difficulty, row.model, row.effort),
                         ("P1", None, mp.NOT_DECLARED, mp.NOT_DECLARED, mp.NOT_DECLARED))

    def test_no_section_6(self):
        self.assertEqual(mp.plan_table("# Piano\n\n## 1. Obiettivo\n"), [])


class Rounds(unittest.TestCase):
    def test_failed_cancelled_and_green(self):
        r = mp.rounds(["failure", "cancelled", "failure", "success", "failure", "success"])
        self.assertEqual((r["runs"], r["failure"], r["cancelled"], r["success"], r["fail_before_green"]),
                         (6, 3, 1, 2, 3))

    def test_failure_after_the_last_green_is_not_a_round(self):
        self.assertEqual(mp.rounds(["success", "failure"])["fail_before_green"], 0)
        self.assertEqual(mp.rounds(["cancelled", "success"])["fail_before_green"], 0)

    def test_never_green(self):
        r = mp.rounds(["failure", "failure"])
        self.assertEqual((r["failure"], r["fail_before_green"]), (2, 0))
        self.assertEqual(mp.rounds([])["runs"], 0)


class Cases(unittest.TestCase):
    def test_fine_and_ordinary_questions(self):
        tgq = "\n".join([
            "? intersect L1 C1 -> 2 points",               # ordinary
            "? point P -> 1.25,3.5",                       # ordinary
            "? point Q -> 1.41421,0",                      # four decimals: fine
            "? length L -> 1e-9",                          # exponent: fine
            "? fillet L1 L2 5 -> arc",                     # construction: fine
            "!tolerance 1e-6",
            "? point R -> 1,2",                            # after !tolerance: fine
        ])
        c = mp.classify_cases({"domande.tgq": tgq})
        self.assertEqual((c["domande"], c["domande_fini"]), (6, 4))

    def test_fine_and_ordinary_scripts(self):
        self.assertEqual(mp.classify_cases({"script.tgs": "LINE\n0,0\n10,0\n",
                                            "atteso.txt": "line 0,0 10,0\n"})["script_fini"], 0)
        c = mp.classify_cases({"script.tgs": "CIRCLE\n0,0\n1\n", "atteso.txt": "area 3.14159\n"})
        self.assertEqual((c["script"], c["script_fini"]), (1, 1))

    def test_case_folders_and_correction_kind(self):
        files = [["tests/verifica/casi/a/b/script.tgs", 3], ["tests/verifica/casi/a/b/atteso.txt", 1],
                 ["tests/verifica/in-attesa/0.7.5-R2.txt", 1], ["tests/verifica/casi/c/domande.tgq", 2]]
        self.assertEqual(mp.case_dirs(files), ["tests/verifica/casi/a/b", "tests/verifica/casi/c"])
        self.assertEqual(mp.correction_kind(files), "riferimenti")
        self.assertEqual(mp.correction_kind([["docs/x.md", 1], ["README.md", 1]]), "documenti")
        self.assertEqual(mp.correction_kind([["tests/verifica/casi/a/x.tgs", 1], ["cmd/src/a.cpp", 1]]), "codice")


# --- A small stage on disk ----------------------------------------------------------


def issue(number: int, title: str, labels: list[str], created: str = "2026-11-07T10:00:00Z") -> dict:
    return {"number": number, "title": title, "created_at": created, "state": "closed",
            "labels": [{"name": n} for n in labels]}


def pull(number: int, title: str, body: str, head: str, created: str, merged: str | None,
         closed: str | None = None) -> dict:
    return {"number": number, "title": title, "state": "closed", "created_at": created, "merged_at": merged,
            "closed_at": closed or merged, "body": body, "head": head, "labels": []}


def run(id_: int, head: str, created: str, conclusion: str, minutes: int = 15) -> dict:
    h, m = int(created[11:13]), int(created[14:16]) + minutes
    end = f"{created[:11]}{h + m // 60:02d}:{m % 60:02d}:00Z"
    return {"id": id_, "head_branch": head, "status": "completed", "conclusion": conclusion,
            "created_at": created, "updated_at": end, "run_started_at": created}


CLAUDE = ai("- [x] IA usata: Claude Code, Claude Opus 5.5 — ne rispondo io")
CODEX = ai("- [x] IA usata: Codex, GPT-6.1 Sol, impegno alto.")
GEMINI = ai("- [x] IA usata: Antigravity, Gemini 3.8 Flash")

ISSUES = [
    issue(10, "[0.7.5-P1] CI: immagini", ["pacchetto", "media", "agente:claude", "tappa-0.7.5"]),
    issue(11, "[0.7.5-R2] Riferimenti: NEW", ["riferimenti", "media", "agente:gemini", "tappa-0.7.5"]),
    issue(12, "[0.7.5-P2] cmd: NEW", ["pacchetto", "media", "agente:codex", "tappa-0.7.5"]),
    issue(13, "[0.7.5-P3] Misura", ["pacchetto", "media", "agente:claude", "tappa-0.7.5"],
          created="2026-11-08T09:00:00Z"),
    {"number": 14, "title": "una pull request", "created_at": "2026-11-07T10:00:00Z", "state": "closed",
     "labels": [], "pull_request": {}},
]

PULLS = [
    # P1: the main pull request closes the issue; a later one with the code in
    # the title is a correction of the code, one touching only documents is
    # another pull request.
    pull(20, "ci: immagini", "Closes #10\n" + CLAUDE, "p1", "2026-11-07T11:00:00Z", "2026-11-07T13:00:00Z"),
    pull(21, "fix: 0.7.5-P1 su macOS", CLAUDE, "fix-p1", "2026-11-07T14:00:00Z", "2026-11-07T15:00:00Z"),
    pull(22, "docs: scheda", "Corregge la scheda di 0.7.5-P1.", "docs-p1", "2026-11-07T16:00:00Z",
         "2026-11-07T16:30:00Z"),
    # R2: main by its code in the title; a correction of the cases cites the
    # code only in the description.
    pull(23, "[0.7.5-R2] Riferimenti", GEMINI, "r2", "2026-11-07T11:00:00Z", "2026-11-07T12:00:00Z"),
    pull(24, "test: casi di NEW", "Corregge i casi di 0.7.5-R2.\n" + GEMINI, "fix-r2",
         "2026-11-07T17:00:00Z", "2026-11-07T17:30:00Z"),
    # P2: a first attempt not merged, then the main one.
    pull(25, "[0.7.5-P2] primo tentativo", CODEX, "p2-a", "2026-11-07T12:30:00Z", None, "2026-11-07T12:50:00Z"),
    pull(26, "[0.7.5-P2] cmd: NEW", "Closes #12\n" + CODEX, "p2", "2026-11-07T13:00:00Z", "2026-11-07T18:00:00Z"),
    # Cites P1 in the description but was merged before it: not a correction.
    pull(27, "chore: strumenti", "Serve a 0.7.5-P1.", "tools", "2026-11-07T10:30:00Z", "2026-11-07T11:30:00Z"),
    # Its title is about another package: the code of P1 in the description
    # does not make it a correction of P1.
    pull(29, "[0.7.5-P9] altro", "Come in 0.7.5-P1.", "p9", "2026-11-07T19:00:00Z", "2026-11-07T19:30:00Z"),
    # Names P3 in the title, but was opened before its issue: the plan
    # pull request that creates it, not its main pull request.
    pull(28, "docs: nuovo 0.7.5-P3", CLAUDE, "plan", "2026-11-08T08:00:00Z", "2026-11-08T08:30:00Z"),
]

RUNS = [
    run(1, "p1", "2026-11-07T11:01:00Z", "failure"),
    run(2, "p1", "2026-11-07T11:30:00Z", "cancelled", 2),
    run(3, "p1", "2026-11-07T11:33:00Z", "success"),
    run(4, "fix-p1", "2026-11-07T14:01:00Z", "success"),
    run(5, "r2", "2026-11-07T11:01:00Z", "success"),
    run(6, "fix-r2", "2026-11-07T17:01:00Z", "success"),
    run(7, "p2-a", "2026-11-07T12:31:00Z", "failure"),
    run(8, "p2", "2026-11-07T13:01:00Z", "success"),
    run(9, "p2", "2026-11-07T14:00:00Z", "failure"),
    run(10, "p2", "2026-11-07T15:00:00Z", "success"),
    run(11, "p1", "2026-11-06T10:00:00Z", "failure"),  # an older branch with the same name
]


def details(files: list[list]) -> dict:
    add = sum(n for _, n in files)
    return {"commits": 2, "additions": add, "deletions": 0, "changed_files": len(files), "files": files}


DETAILS = {
    "20": details([[".github/workflows/ci.yml", 40], ["third_party/solid-space", 2], ["docs/ci.md", 10]]),
    "21": details([["scripts/ci.sh", 5]]),
    "22": details([["docs/tappe/0.7.5/P1.md", 3]]),
    "23": details([["tests/verifica/casi/apertura/new/script.tgs", 30],
                   ["tests/verifica/casi/apertura/new/atteso.txt", 20]]),
    "24": details([["tests/verifica/casi/apertura/new/atteso.txt", 2]]),
    "25": details([["cmd/src/new.cpp", 50]]),
    "26": details([["cmd/src/new.cpp", 300], ["cmd/tests/new_test.cpp", 100]]),
}

COMMENTS = {
    "10": [["2026-11-07T10:30:00Z", "Presa in carico da Claude"]],
    "11": [["2026-11-07T10:40:00Z", "Presa in carico da Gemini"]],
    "12": [["2026-11-07T10:00:00Z", "Presa in carico da Codex"], ["2026-11-07T11:00:00Z", "Mi fermo: buco."],
           ["2026-11-07T12:00:00Z", "Ripresa in carico da Codex"]],
    "13": [],
}

CASES = {"tests/verifica/casi/apertura/new": {"script.tgs": "NEW\n!report\n", "atteso.txt": "documents: a\n"}}


def write_stage(folder: Path) -> mp.Store:
    store = mp.Store(folder)
    store.save("issues-0.7.5.json", ISSUES)
    store.save("piano-0.7.5.json", {"file": "docs/tappe/0.7.5-x.md", "text": PLAN})
    store.save("pulls.json", PULLS)
    store.save("runs.json", RUNS)
    store.save("pull-details.json", DETAILS)
    store.save("commenti.json", COMMENTS)
    store.save("casi.json", CASES)
    return store


class Stage(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.store = write_stage(Path(self.tmp.name))
        self.rows = {r["code"]: r for r in mp.measure(self.store, ["0.7.5"]).rows}

    def tearDown(self):
        self.tmp.cleanup()

    def test_issues_kinds_and_plan(self):
        self.assertEqual(sorted(self.rows), ["0.7.5-P1", "0.7.5-P2", "0.7.5-P3", "0.7.5-R2"])
        p1 = self.rows["0.7.5-P1"]
        self.assertEqual((p1["kind"], p1["diff"], p1["agenti"]), ("pacchetto", "media", ["claude"]))
        self.assertEqual(p1["previsto"]["impegno"], "alto")
        self.assertEqual(self.rows["0.7.5-R2"]["kind"], "riferimenti")
        self.assertIsNone(self.rows["0.7.5-P3"]["previsto"])

    def test_main_pull_request_and_corrections(self):
        p1 = self.rows["0.7.5-P1"]
        self.assertEqual(p1["main"]["pr"], 20)
        self.assertEqual([f["pr"] for f in p1["fixes_code"]], [21])
        self.assertEqual([f["pr"] for f in p1["fixes_ref"]], [])
        self.assertEqual([f["pr"] for f in p1["fixes_doc"]], [22])
        r2 = self.rows["0.7.5-R2"]
        self.assertEqual((r2["main"]["pr"], [f["pr"] for f in r2["fixes_ref"]]), (23, [24]))
        p2 = self.rows["0.7.5-P2"]
        self.assertEqual((p2["main"]["pr"], [f["pr"] for f in p2["attempts"]]), (26, [25]))
        self.assertEqual(p2["fixes_code"] + p2["fixes_ref"] + p2["fixes_doc"], [])
        self.assertIsNone(self.rows["0.7.5-P3"]["main"])

    def test_size_without_third_party(self):
        m = self.rows["0.7.5-P1"]["main"]
        self.assertEqual((m["files_own"], m["lines_own"], m["commits"]), (2, 50, 2))

    def test_runs_and_rounds(self):
        m = self.rows["0.7.5-P1"]["main"]
        self.assertEqual((m["runs"], m["failure"], m["cancelled"], m["fail_before_green"]), (3, 1, 1, 1))
        m = self.rows["0.7.5-P2"]["main"]
        self.assertEqual((m["runs"], m["failure"], m["fail_before_green"]), (3, 1, 1))
        self.assertEqual(self.rows["0.7.5-P2"]["attempts"][0]["failure"], 1)

    def test_hours_and_claims(self):
        p1 = self.rows["0.7.5-P1"]
        self.assertEqual((p1["main"]["hours"], p1["lead"], p1["stopped"], p1["last_work"]), (2.0, 2.5, 0.0, 2.5))
        p2 = self.rows["0.7.5-P2"]
        # First claim 10:00, last before the pull request 12:00, merged 18:00.
        self.assertEqual((p2["lead"], p2["stopped"], p2["last_work"]), (8.0, 2.0, 6.0))

    def test_declared_and_maintainer_effort(self):
        p1 = self.rows["0.7.5-P1"]["dichiarato"]
        self.assertEqual((p1["modello"], p1["impegno"], p1["fonte_impegno"], p1["differenza"]),
                         ("Claude Opus 5.5", "medio", "responsabile", None))
        p2 = self.rows["0.7.5-P2"]["dichiarato"]
        self.assertEqual((p2["modello"], p2["impegno"], p2["fonte_impegno"]), ("GPT-6.1 sol", "alto", "pull request"))
        r2 = self.rows["0.7.5-R2"]["dichiarato"]
        self.assertEqual((r2["modello"], r2["impegno"]), ("Gemini 3.8 Flash", mp.NOT_DECLARED))

    def test_cases_of_the_reference_package(self):
        self.assertEqual(self.rows["0.7.5-R2"]["casi"],
                         {"domande": 0, "domande_fini": 0, "script": 1, "script_fini": 0})

    def test_report_offline(self):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            code = mp.main(["--tappe", "0.7.5", "--dati", self.tmp.name])
        self.assertEqual(code, 0, err.getvalue())
        text = out.getvalue()
        self.assertIn("## Tappa 0.7.5", text)
        self.assertIn("| pacchetto | media | 2 |", text)
        self.assertIn("Percorso critico realizzato: P1 → P2", text)
        self.assertIn("0.7.5-P3 (chiusa)", text)
        self.assertTrue((Path(self.tmp.name) / "misura.json").exists())

    def test_missing_data_is_an_error(self):
        (Path(self.tmp.name) / "runs.json").unlink()
        err = io.StringIO()
        with redirect_stdout(io.StringIO()), redirect_stderr(err):
            self.assertEqual(mp.main(["--tappe", "0.7.5", "--dati", self.tmp.name]), 2)
        self.assertIn("--scarica", err.getvalue())


class Maintainer(unittest.TestCase):
    def row(self, model: str, effort: str = mp.NOT_DECLARED, tool: str = "claude", diff: str = "media",
            stage: str = "0.7.5") -> dict:
        main = {"declared": {"tool": tool, "model": model, "effort": effort}}
        return {"tappa": stage, "diff": diff, "autore": tool, "main": main}

    def test_effort_of_the_pull_request_wins(self):
        d = mp.declaration(self.row("Claude Opus 5.5", "alto"))
        self.assertEqual((d["impegno"], d["fonte_impegno"]), ("alto", "pull request"))

    def test_model_differs_from_the_declaration(self):
        d = mp.declaration(self.row("Claude Sonnet 5.5"))
        self.assertEqual(d["impegno"], "medio")
        self.assertIn("Claude Sonnet 5.5", d["differenza"])
        self.assertIn("astra", mp.declaration(self.row("GPT-6.1 astra", tool="codex"))["differenza"])
        self.assertIsNone(mp.declaration(self.row("GPT-6", tool="codex"))["differenza"])

    def test_only_for_the_declared_stage_and_kinds(self):
        self.assertEqual(mp.declaration(self.row("Claude Opus 5.5", stage="0.7"))["impegno"], mp.NOT_DECLARED)
        self.assertEqual(mp.declaration(self.row("Gemini 3.8 Flash", tool="gemini", diff="facile"))["impegno"],
                         mp.NOT_DECLARED)
        d = mp.declaration(self.row(mp.NOT_DECLARED, tool="codex", diff="impegnativa"))
        self.assertEqual((d["modello"], d["impegno"]), ("Codex sol o astra light (responsabile)", "alto"))
        self.assertEqual(mp.declaration(self.row("GPT-6", "non noto", tool="gemini"))["impegno"], "non noto")


class FakeGitHub(mp.GitHub):
    """Answers from a table; refuses --paginate and anything not in it."""

    def __init__(self, answers: dict[str, object]):
        self.answers = answers
        self.asked: list[str] = []

    def get(self, path: str):
        self.asked.append(path)
        if path not in self.answers:
            raise mp.Failure(f"unexpected: {path}")
        return self.answers[path]


def gh_pull(p: dict) -> dict:
    return {k: p[k] for k in ("number", "title", "state", "created_at", "merged_at", "closed_at", "body")} | {
        "head": {"ref": p["head"], "sha": "0"}, "labels": []}


class Download(unittest.TestCase):
    def test_pages_one_at_a_time_and_saves(self):
        R = f"repos/{mp.REPO}"
        many = [{"filename": f"f{k}.cpp", "additions": 1, "deletions": 0} for k in range(100)]
        answers: dict[str, object] = {
            f"{R}/issues?labels=tappa-0.7.5&state=all&per_page=100&page=1": ISSUES,
            f"{R}/pulls?state=all&per_page=100&page=1": [gh_pull(p) for p in PULLS],
            f"{R}/actions/workflows/ci.yml/runs?event=pull_request&per_page=100&page=1":
                {"workflow_runs": [r | {"head_sha": "0"} for r in RUNS]},
        }
        for n, d in DETAILS.items():
            answers[f"{R}/pulls/{n}"] = {k: d[k] for k in ("commits", "additions", "deletions", "changed_files")} | {
                "head": {"sha": "0"}}
            answers[f"{R}/pulls/{n}/files?per_page=100&page=1"] = [
                {"filename": f, "additions": ln, "deletions": 0} for f, ln in d["files"]]
        for n, cs in COMMENTS.items():
            answers[f"{R}/issues/{n}/comments?per_page=100&page=1"] = [
                {"created_at": t, "body": b} for t, b in cs]
        answers[f"{R}/pulls/21/files?per_page=100&page=1"] = many
        answers[f"{R}/pulls/21/files?per_page=100&page=2"] = [{"filename": "x.cpp", "additions": 1, "deletions": 0}]
        gh = FakeGitHub(answers)
        with tempfile.TemporaryDirectory() as tmp, tempfile.TemporaryDirectory() as root:
            r = Path(root)
            (r / "docs" / "tappe").mkdir(parents=True)
            (r / "docs" / "tappe" / "0.7.5-correzione.md").write_text(PLAN, encoding="utf-8")
            case = r / "tests" / "verifica" / "casi" / "apertura" / "new"
            case.mkdir(parents=True)
            (case / "script.tgs").write_text("NEW\n", encoding="utf-8")
            (case / "atteso.txt").write_text("area 2.00001\n", encoding="utf-8")
            with redirect_stdout(io.StringIO()):
                code = mp.main(["--tappe", "0.7.5", "--dati", tmp, "--scarica"], gh=gh, root=r)
            self.assertEqual(code, 0)
            self.assertTrue(all("--paginate" not in p for p in gh.asked))
            saved = json.loads((Path(tmp) / "pull-details.json").read_text(encoding="utf-8"))
            self.assertEqual(len(saved["21"]["files"]), 101)
            self.assertEqual(sorted(saved), sorted(DETAILS))
            plan = json.loads((Path(tmp) / "piano-0.7.5.json").read_text(encoding="utf-8"))
            self.assertEqual(plan["file"], "docs/tappe/0.7.5-correzione.md")
            cases = json.loads((Path(tmp) / "casi.json").read_text(encoding="utf-8"))
            self.assertEqual(list(cases), ["tests/verifica/casi/apertura/new"])
            rows = {r["code"]: r for r in json.loads((Path(tmp) / "misura.json").read_text(encoding="utf-8"))}
            self.assertEqual(rows["0.7.5-R2"]["casi"]["script_fini"], 1)
            # A second download reads lists and comments again, details only
            # for a pull request whose head changed in between.
            gh.asked.clear()
            with redirect_stdout(io.StringIO()):
                mp.main(["--tappe", "0.7.5", "--dati", tmp, "--scarica"], gh=gh, root=r)
            self.assertFalse(any(p.endswith("/pulls/20") for p in gh.asked))
            pulls = [gh_pull(p) for p in PULLS]
            pulls[0]["head"]["sha"] = "1"
            answers[f"{R}/pulls?state=all&per_page=100&page=1"] = pulls
            answers[f"{R}/pulls/20"] = {"commits": 3, "additions": 70, "deletions": 0, "changed_files": 3,
                                        "head": {"sha": "1"}}
            answers[f"{R}/pulls/20/files?per_page=100&page=1"] = [
                {"filename": f, "additions": ln, "deletions": 0}
                for f, ln in DETAILS["20"]["files"] + [["scripts/ci.sh", 20]]]
            gh.asked.clear()
            with redirect_stdout(io.StringIO()):
                mp.main(["--tappe", "0.7.5", "--dati", tmp, "--scarica"], gh=gh, root=r)
            self.assertTrue(any(p.endswith("/pulls/20") for p in gh.asked))
            self.assertFalse(any(p.endswith("/pulls/26") for p in gh.asked))
            rows = {r["code"]: r for r in json.loads((Path(tmp) / "misura.json").read_text(encoding="utf-8"))}
            main = rows["0.7.5-P1"]["main"]
            self.assertEqual((main["commits"], main["files_own"], main["lines_own"]), (3, 3, 70))

    def test_bad_stage(self):
        with redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            mp.main(["--tappe", "0.7.5x", "--dati", "x"])


if __name__ == "__main__":
    unittest.main()
