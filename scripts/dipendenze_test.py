# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/dipendenze.py, without network: GitHub is replaced by
# answers written here (docs/tappe/fuori-tappa/F4-dipendenze.md).
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import base64
import io
import json
import os
import shutil
import subprocess
import re
import sys
import tempfile
import unittest
import unittest.mock
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import dipendenze as d  # noqa: E402

TG = "Matleyx78/tecnigrafo"
SS = "Matleyx78/solid-space"
OWNER = "Matleyx78"

# --- Plans -------------------------------------------------------------------


def tg_row(id_: str, issue: int | None, deps: str, agent: str = "claude") -> str:
    kind = "Riferimenti" if id_.startswith("R") else "Lavoro"
    sheet = "—" if id_.startswith("R") else f"[{id_}](0.7/{id_}.md)"
    number = f"#{issue}" if issue else "—"
    return f"| {id_} {kind} {id_} | {sheet} | {number} | {deps} | — | media | {agent} | medio | medio |"


def tg_plan(rows: list[str], requests: list[str] | None = None, closing: bool = True) -> str:
    out = ["# Tappa 0.7 — Comandi di modifica", "", "## 5. Prerequisiti", ""]
    if requests is not None:
        out += ["### Richieste a solid-space", ""]
        if closing:
            out += ["| Richiesta | Pacchetti che ne dipendono | Si chiude con |", "|---|---|---|"]
        else:
            out += ["| Richiesta | Pacchetti che ne dipendono |", "|---|---|"]
        out += requests + ["", "### Decisioni", ""]
    out += ["## 6. Pacchetti di lavoro", "",
            "| Pacchetto | Scheda | Issue | Dipende da | Riferimenti | Difficoltà | Agente | Modello | Impegno |",
            "|---|---|---|---|---|---|---|---|---|"]
    out += rows + ["", "## 7. Prova manuale", ""]
    return "\n".join(out) + "\n"


# The rows of the stage 0.7 of Tecnigrafo before 2026-11-01, when they cited the
# requests (only the second half, with the rows it names).
ROWS_07 = [
    tg_row("P0", 333, "—"),
    tg_row("P7", 346, "—", "codex"),
    tg_row("R14", 356, f"{SS}#20, {SS}#23", "gemini"),
    tg_row("P14", 357, f"P0, {SS}#20, {SS}#23, R14"),
    tg_row("R15", 358, "P7", "gemini"),
    tg_row("P15", 359, "P7, P14, R15", "codex"),
    tg_row("R17", 362, f"{SS}#22", "gemini"),
    tg_row("P17", 363, f"P14, {SS}#22, R17"),
    tg_row("R19", 366, f"{SS}#21", "gemini"),
    tg_row("P19", 367, f"P15, {SS}#21, R19"),
    tg_row("R22", 372, f"{SS}#23", "gemini"),
    tg_row("P22", 373, f"P14, {SS}#23, R22", "codex"),
]
REQUESTS_07 = [
    f"| [{SS}#20](https://github.com/{SS}/issues/20) — intersezioni | R14, P14 | P14 |",
    f"| [{SS}#21](https://github.com/{SS}/issues/21) — raccordi | R19, P19 | P19 |",
    f"| [{SS}#22](https://github.com/{SS}/issues/22) — curva parallela | R17, P17 | P17 |",
    f"| [{SS}#23](https://github.com/{SS}/issues/23) — tolleranze | R14, P14, R22, P22 | P14, P22 |",
]

# The package table of the stage 0.2 of solid-space: id, issue, dependencies.
SS_PACKAGES = [
    ("P1", 34, ""), ("P2", 35, "P1 (#34)"), ("P3", 36, "P2 (#35)"),
    ("R4a", 37, "P1 (#34)"), ("R4b", 38, "P1 (#34)"),
    ("P4", 39, "P2 (#35), P3 (#36), R4a (#37), R4b (#38)"),
    ("R5a", 40, "P1 (#34)"), ("R5b", 41, "P1 (#34)"),
    ("P5", 42, "P2 (#35), P3 (#36), R5a (#40), R5b (#41)"),
    ("P6", 43, "P3 (#36)"),
    ("R7a", 44, "P1 (#34)"), ("R7b", 45, "P1 (#34)"),
    ("P7", 46, "P5 (#42), P6 (#43), R7a (#44), R7b (#45)"),
    ("R8a", 47, "P1 (#34)"), ("R8b", 48, "P1 (#34)"),
    ("P8", 49, "P4 (#39), P7 (#46), R8a (#47), R8b (#48)"),
    ("R9", 50, "P1 (#34)"), ("P9", 51, "P4 (#39), P7 (#46), R9 (#50)"),
    ("R10a", 52, "P1 (#34)"), ("R10b", 53, "P1 (#34)"),
    ("P10", 54, "P9 (#51), R10a (#52), R10b (#53)"),
    ("R11", 55, "P1 (#34)"), ("P11", 56, "P10 (#54), R11 (#55)"),
    ("P12", 57, "P9 (#51)"),
    ("R13", 58, "P1 (#34)"), ("P13", 59, "P7 (#46), R13 (#58)"),
    ("R14a", 60, "P1 (#34)"), ("R14b", 61, "P1 (#34), R13 (#58)"), ("R14c", 62, "P1 (#34)"),
    ("P14", 63, "P10 (#54), P12 (#57), P13 (#59), R14a (#60), R14b (#61), R14c (#62)"),
    ("R15a", 64, "P1 (#34)"), ("R15b", 65, "P1 (#34)"),
    ("P15", 66, "P10 (#54), P12 (#57), R15a (#64), R15b (#65)"),
    ("R16a", 67, "P1 (#34)"), ("R16b", 68, "P1 (#34), R13 (#58)"), ("R16c", 69, "P1 (#34)"),
    ("P16", 70, "P15 (#66), R16a (#67), R16b (#68), R16c (#69)"),
]
SS_UNBLOCKS = [
    (356, "R4b, R8a, R8b, R9, R10a, R10b, R11"),
    (357, "P4, P5, P6, P7, P8, P9, P10, P11"),
    (362, "R13, R14a, R14b, R14c"),
    (363, "P12, P13, P14 (con P4–P10)"),
    (366, "R15a, R15b, R16a, R16b, R16c"),
    (367, "P12, P15, P16 (con P4–P10)"),
    (372, "R4b, R8a, R8b"),
    (373, "P4, P8 (con P5, P6, P7)"),
]
# What the plan 0.2 of solid-space has since the review of the contract: C, with
# no number, which later packages wait for.
SS_CONTRACT = [
    ("C", 133, ""),
    ("R19", 129, "P1 (#34), C (#133)"),
    ("P19", 130, "P13 (#59), P14 (#63), C (#133), R19 (#129)"),
]


def ss_plan(packages=SS_PACKAGES, unblocks=SS_UNBLOCKS) -> str:
    out = ["# Tappa 0.2", "", "## 6. Pacchetti di lavoro", "",
           "| Pacchetto | Scheda | Issue | Dipende da | Riferimenti | Difficoltà | Agente | Modello | Impegno |",
           "|---|---|---|---|---|---|---|---|---|"]
    for id_, issue, deps in packages:
        out.append(f"| {id_} Titolo | — | #{issue} | {deps or '—'} | — | media | codex | medio | medio |")
    out += ["", "### Che cosa sblocca in Tecnigrafo", "", "Testo.", "",
            "| Issue di Tecnigrafo | Che cosa fa | Pacchetti di solid-space che servono |",
            "|---|---|---|"]
    out += [f"| {TG}#{n} (X) | cosa | {cell} |" for n, cell in unblocks]
    out += ["", "### Ordine consigliato", ""]
    return "\n".join(out) + "\n"


SS_ROADMAP = "\n".join([
    "# Roadmap", "", "## Stato", "",
    "| Tappa | Versione | Per la tappa di Tecnigrafo | Componente | Obiettivo | Nata da | Stato |",
    "|---|---|---|---|---|---|---|",
    "| 1 | 0.1 | — | tutti | Fondamenta | fondamenta | **chiusa** ([scheda](tappe/0.1-fondamenta.md)) |",
    "| 2 | 0.2 | 0.7, comandi di modifica | `geom2d` | Piano | richieste | **aperta** dal 2026-10-30 "
    "([scheda e piano](tappe/0.2-geometria-del-piano.md), "
    f"[milestone v0.2](https://github.com/{SS}/milestone/2), issue #34–#72) |",
    "", "## Regole", ""]) + "\n"


# --- A fake GitHub -----------------------------------------------------------------


def issue(number: int, state: str = "open", labels=(), body: str = "", title: str = "") -> dict:
    return {"number": number, "id": 1000 + number, "title": title or f"[0.7] {number}", "state": state,
            "labels": [{"name": n} for n in labels], "body": body}


def ss_issue(number: int, state: str = "closed", labels=()) -> dict:
    tag = next((f"0.2-{i}" for i, n, _ in SS_PACKAGES if n == number), None)
    data = issue(number, state, labels, title=f"[{tag}] titolo" if tag else "Richiesta: x")
    data["id"] = 5000 + number
    return data


def relation(repo: str, number: int, state: str = "closed") -> dict:
    return {"id": (5000 if repo == SS else 1000) + number, "number": number, "state": state,
            "repository_url": f"https://api.github.com/repos/{repo}"}


class FakeGitHub(d.GitHub):
    def __init__(self, responses: dict):
        super().__init__(public=[SS])
        self.responses = responses
        self.sent: list[tuple[str, str, dict | None]] = []
        self.read: list[str] = []

    def get(self, path, paginate=False):
        self.read.append(path)
        if path not in self.responses:
            raise d.Failure(f"GitHub refused: gh api {path}")
        return self.responses[path]

    def send(self, method, path, body=None):
        self.sent.append((method, path, body))
        return {}


class World:
    """The answers of GitHub for a stage: issues of Tecnigrafo and of
    solid-space, relations, Project; the tests change them."""

    def __init__(self, plan: str, tg: list[dict], ss: list[dict] | None = None,
                 relations: dict[int, list[dict]] | None = None, project: dict[int, str | None] | None = None,
                 ss_text: str | None = None, requests: list[dict] | None = None):
        self.plan = plan
        r: dict = {f"repos/{TG}/issues?state=all&per_page=100&labels=tappa-0.7": tg}
        for i in tg:
            r[f"repos/{TG}/issues/{i['number']}/dependencies/blocked_by?per_page=100"] = \
                (relations or {}).get(i["number"], [])
        r[f"repos/{SS}/contents/docs/ROADMAP.md?ref=main"] = {"content": b64(SS_ROADMAP)}
        r[f"repos/{SS}/contents/docs/tappe/0.2-geometria-del-piano.md?ref=main"] = \
            {"content": b64(ss_text or ss_plan())}
        r[f"repos/{SS}/issues?state=all&per_page=100&milestone=2"] = ss or []
        for q in requests or []:
            r[f"repos/{SS}/issues/{q['number']}"] = q
        if project is not None:
            r[f"users/{OWNER}/projectsV2?per_page=100"] = [{"number": 3, "title": "Tecnigrafo"}]
            r[f"users/{OWNER}/projectsV2/3/fields?per_page=100"] = [
                {"id": 77, "name": "Status", "options": [
                    {"id": "o1", "name": {"raw": "Da fare", "html": "Da fare"}},
                    {"id": "o2", "name": {"raw": "Bloccato", "html": "Bloccato"}},
                    {"id": "o3", "name": {"raw": "Fatto", "html": "Fatto"}}]}]
            items = []
            for n, status in project.items():
                fields = []
                if status:
                    option = {"Da fare": "o1", "Bloccato": "o2", "Fatto": "o3"}[status]
                    fields = [{"id": 77, "name": "Status",
                               "value": {"id": option, "name": {"raw": status, "html": status}}}]
                items.append({"id": 900 + n, "content": {
                    "number": n, "repository_url": f"https://api.github.com/repos/{TG}"}, "fields": fields})
            r[f"users/{OWNER}/projectsV2/3/items?per_page=100&fields=77"] = items
        self.responses = r

    def run(self, *args: str, answer: str = "s") -> tuple[int, str, str, FakeGitHub]:
        gh = FakeGitHub(self.responses)
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "docs" / "tappe").mkdir(parents=True)
            (Path(tmp) / "docs" / "tappe" / "0.7-modifiche.md").write_text(self.plan, encoding="utf-8")
            out, err = io.StringIO(), io.StringIO()
            with redirect_stdout(out), redirect_stderr(err):
                code = d.main(["0.7", *args], gh=gh, root=Path(tmp), ask=lambda _: answer)
        return code, out.getvalue(), err.getvalue(), gh


def b64(text: str) -> str:
    return base64.b64encode(text.encode("utf-8")).decode("ascii")


def body(deps: str) -> str:
    return f"**Tappa:** 0.7\n**Dipende da:** {deps}\n\nTesto."


# --- Tests ---------------------------------------------------------------------------


class PlanTable(unittest.TestCase):
    def test_prefixed_plan_and_closing_packages(self):
        text = tg_plan([tg_row("PT1", 10, "—"), tg_row("RT14a", 11, "PT1", "gemini"),
                        tg_row("PT14", 12, f"PT1, RT14a, #9, {SS}#20")],
                       [f"| {SS}#20 | PT14 | PT14 |"])
        plan = d.parse_plan(text.replace("0.7", "0.8"), "piano.md", TG)
        self.assertEqual([r.id for r in plan.rows], ["PT1", "RT14a", "PT14"])
        self.assertEqual([d.dep_ref(x, plan, TG) for x in plan.row("PT14").deps],
                         [d.Ref(TG, 10), d.Ref(TG, 11), d.Ref(TG, 9), d.Ref(SS, 20)])
        self.assertEqual(plan.requests[0].closes_with, ["PT14"])
        ctx = d.Context("0.8", TG, SS, plan, d.Issues(FakeGitHub({})), lambda _: [])
        self.assertEqual(d.dep_text("RT14a", ctx), "#11 (RT14a)")

    def test_prefix_must_match_stage(self):
        for stage, id_, expected in (("0.8", "P3", "PTN or RTN"),
                                     ("0.7.5", "PT3", "PN or RN")):
            with self.subTest(stage=stage):
                text = tg_plan([tg_row(id_, 10, "—")]).replace("0.7", stage)
                with self.assertRaisesRegex(d.Failure, f"{expected} for stage {stage}"):
                    d.parse_plan(text, "piano.md", TG)
        text = tg_plan([tg_row("PT3", 10, "P2")]).replace("0.7", "0.8")
        with self.assertRaisesRegex(d.Failure, "dependency P2 must use PTN or RTN"):
            d.parse_plan(text, "piano.md", TG)

    def test_four_forms(self):
        plan = d.parse_plan(tg_plan([tg_row("P1", 10, "—"), tg_row("R2", 11, "—", "gemini"),
                                     tg_row("P2", 12, f"P1, #9, {SS}#20, R2")]), "piano.md", TG)
        row = plan.row("P2")
        self.assertEqual(row.deps, ["P1", "#9", f"{SS}#20", "R2"])
        self.assertEqual([d.dep_ref(x, plan, TG) for x in row.deps],
                         [d.Ref(TG, 10), d.Ref(TG, 9), d.Ref(SS, 20), d.Ref(TG, 11)])
        self.assertEqual(row.line, 11)

    def test_bad_row_names_its_line(self):
        text = tg_plan([tg_row("P1", 10, "—"), tg_row("P2", 12, "P1, qualcosa")])
        with self.assertRaisesRegex(d.Failure, r"line 10: P2: dependency \"qualcosa\""):
            d.parse_plan(text, "piano.md", TG)
        with self.assertRaisesRegex(d.Failure, r"line 9: P1 depends on P2, which is not earlier"):
            d.parse_plan(tg_plan([tg_row("P1", 10, "P2"), tg_row("P2", 12, "—")]), "piano.md", TG)

    def test_letter_after_the_number(self):
        # A block of references split in two (R17b, #481), as a row and as a dependency.
        plan = d.parse_plan(tg_plan([tg_row("R17", 10, "—", "gemini"), tg_row("R17b", 11, "#9", "gemini"),
                                     tg_row("P17", 12, "R17, R17b")]), "piano.md", TG)
        self.assertEqual([r.id for r in plan.rows], ["R17", "R17b", "P17"])
        self.assertEqual(plan.row("P17").deps, ["R17", "R17b"])
        self.assertEqual(d.dep_ref("R17b", plan, TG), d.Ref(TG, 11))
        with self.assertRaisesRegex(d.Failure, r"P17 depends on R17b, which is not earlier"):
            d.parse_plan(tg_plan([tg_row("P17", 12, "R17b"), tg_row("R17b", 11, "—", "gemini")]),
                         "piano.md", TG)

    def test_missing_table(self):
        with self.assertRaisesRegex(d.Failure, "no package table in section 6"):
            d.parse_plan("# Tappa 0.7 — x\n\n## 6. Pacchetti\n\nNiente.\n", "piano.md", TG)

    def test_requests(self):
        plan = d.parse_plan(tg_plan(ROWS_07, REQUESTS_07), "piano.md", TG)
        self.assertEqual([(q.ref.number, q.closes_with) for q in plan.requests],
                         [(20, ["P14"]), (21, ["P19"]), (22, ["P17"]), (23, ["P14", "P22"])])
        old = d.parse_plan(tg_plan(ROWS_07, [r.rsplit("|", 2)[0] + "|" for r in REQUESTS_07], closing=False),
                           "piano.md", TG)
        self.assertEqual([q.closes_with for q in old.requests], [None] * 4)
        self.assertIn("check 6 skipped", old.notes[0])


class StageNumbers(unittest.TestCase):
    def test_valid_stage_arguments(self):
        for stage in ("0.7", "0.7.5", "0.8", "0.8.1", "1.0", "10.20.30"):
            with self.subTest(stage=stage), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                plans = root / "docs" / "tappe"
                plans.mkdir(parents=True)
                (plans / f"{stage}-test.md").write_text(
                    tg_plan([tg_row("PT1" if tuple(map(int, stage.split(".")[:2])) >= (0, 8) else "P1",
                                    None, "—")]).replace("0.7", stage), encoding="utf-8")
                gh = FakeGitHub({f"repos/{TG}/issues?state=all&per_page=100&labels=tappa-{stage}": []})
                out = io.StringIO()
                with redirect_stdout(out):
                    self.assertEqual(d.main([stage, "--senza-project"], gh=gh, root=root), 0)
                self.assertIn(f"Stage {stage}, plan docs/tappe/{stage}-test.md", out.getvalue())
                self.assertEqual(gh.sent, [])

    def test_invalid_stage_arguments(self):
        for stage in ("0.7.05", "0.7.", "0.7.5.1", "00.7", "0.07", "0", "0.7\n"):
            with self.subTest(stage=stage), redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    d.main([stage], gh=FakeGitHub({}), root=Path("unused"))
                self.assertEqual(error.exception.code, 2)

    def test_roadmap_requires_the_whole_stage(self):
        for stage, other in (("0.7", "0.7.5"), ("0.7.5", "0.7")):
            with self.subTest(stage=stage):
                roadmap = SS_ROADMAP.replace("0.7,", f"{stage},")
                self.assertIsNotNone(d.parse_ss_roadmap(roadmap, stage))
                self.assertIsNone(d.parse_ss_roadmap(roadmap, other))


class RealPlan(unittest.TestCase):
    """The plan of the open stage, as it is in the repository: a change to the
    plan that the tools cannot read shows up here, not when the maintainer runs
    them."""

    def test_open_stage_plan_parses(self):
        root = Path(__file__).resolve().parent.parent
        roadmap = (root / "docs" / "ROADMAP.md").read_text(encoding="utf-8")
        found = re.findall(r"^\|[^\n]*\|\s*\*\*aperta\*\*\s*\|\s*\[[^]]*\]\((tappe/[^)]+\.md)\)",
                           roadmap, re.M)
        if not found:
            self.skipTest("no open stage in docs/ROADMAP.md")
        for rel in found:
            path = root / "docs" / rel
            plan = d.parse_plan(path.read_text(encoding="utf-8"), str(path), TG)
            self.assertTrue(plan.rows, rel)


class Unblocks(unittest.TestCase):
    order = [p[0] for p in SS_PACKAGES]

    def test_prefixed_ss_plan_ranges_and_contract(self):
        packages = [(re.sub(r"^([PR])", r"\1S", id_), n,
                     re.sub(r"\b([PR])(?=[0-9])", r"\1S", deps))
                    for id_, n, deps in SS_PACKAGES]
        packages += [("CS", 133, ""), ("PS19", 130, "PS11 (#56), CS (#133)")]
        plan = d.parse_ss_plan(ss_plan(packages, [(357, "PS4–PS7, RS14a–c, CS")]), "ss.md")
        self.assertEqual(plan.unblocks[357], ["PS4", "PS5", "PS6", "PS7",
                                               "RS14a", "RS14b", "RS14c", "CS"])
        self.assertEqual(plan.issue["PS11"], 56)
        self.assertEqual(plan.deps["PS19"], ["PS11", "CS"])
        self.assertEqual(d.expand("PS4-7", plan.order, "ss.md"), ["PS4", "PS5", "PS6", "PS7"])
        self.assertEqual(d.expand("RS14a–RS14c", plan.order, "ss.md"),
                         ["RS14a", "RS14b", "RS14c"])
        with self.assertRaisesRegex(d.Failure, "mixes package kinds"):
            d.expand("PS4–RS14a", plan.order, "ss.md")

    def test_names_ranges_brackets(self):
        e = lambda cell: d.expand(cell, self.order, "x")  # noqa: E731
        self.assertEqual(e("P8, R14a"), ["P8", "R14a"])
        self.assertEqual(e("P4–P7"), ["P4", "P5", "P6", "P7"])
        self.assertEqual(e("P4-P6"), ["P4", "P5", "P6"])
        self.assertEqual(e("R14a–c"), ["R14a", "R14b", "R14c"])
        self.assertEqual(e("P12, P13, P14 (con P4–P10)"),
                         ["P12", "P13", "P14", "P4", "P5", "P6", "P7", "P8", "P9", "P10"])
        with self.assertRaisesRegex(d.Failure, "P99 is not in the package table"):
            e("P99")

    def test_ss_plan(self):
        plan = d.parse_ss_plan(ss_plan(), "ss.md")
        self.assertEqual(plan.issue["P11"], 56)
        self.assertEqual(plan.deps["P11"], ["P10", "R11"])
        self.assertIn("P4", plan.closure(["P11"]))
        self.assertNotIn("P8", plan.closure(["P11"]))
        self.assertEqual(plan.unblocks[363][:3], ["P12", "P13", "P14"])

    def test_roadmap(self):
        stage = d.parse_ss_roadmap(SS_ROADMAP, "0.7")
        self.assertEqual((stage.version, stage.open, stage.plan_path, stage.milestone),
                         ("0.2", True, "docs/tappe/0.2-geometria-del-piano.md", 2))
        self.assertIsNone(d.parse_ss_roadmap(SS_ROADMAP, "0.8"))


class ContractReview(unittest.TestCase):
    """The package C of solid-space, the review of the contract of the stage."""

    order = [p[0] for p in SS_PACKAGES + SS_CONTRACT]

    def test_plan_with_c(self):
        text = ss_plan(SS_PACKAGES + SS_CONTRACT).replace(
            "| C Titolo |", "| C Revisione del contratto: `offset.hpp` e `tangency.hpp` |")
        self.assertIn("| C Revisione del contratto: `offset.hpp` e `tangency.hpp` |", text)
        plan = d.parse_ss_plan(text, "ss.md")
        self.assertEqual(plan.order[-3:], ["C", "R19", "P19"])
        self.assertEqual(plan.issue["C"], 133)
        self.assertEqual(plan.deps["C"], [])
        self.assertEqual(plan.deps["R19"], ["P1", "C"])
        self.assertEqual(plan.deps["P19"], ["P13", "P14", "C", "R19"])
        self.assertIn("C", plan.closure(["P19"]))
        self.assertNotIn("C", plan.closure(["P14"]))
        self.assertEqual(plan.id_of(133), "C")

    def test_expand_c(self):
        e = lambda cell: d.expand(cell, self.order, "x")  # noqa: E731
        self.assertEqual(e("C"), ["C"])
        self.assertEqual(e("R19, C"), ["R19", "C"])
        self.assertEqual(e("P4–P7, C (con R19)"), ["P4", "P5", "P6", "P7", "C", "R19"])
        # a range of one kind never takes C, even if C sits between its ends
        self.assertEqual(e("P15–P19"), ["P15", "P16", "P19"])
        self.assertEqual(e("R14a–c, P8"), ["R14a", "R14b", "R14c", "P8"])
        with self.assertRaisesRegex(d.Failure, "C is not in the package table"):
            d.expand("C", [p[0] for p in SS_PACKAGES], "x")

    def test_c_is_not_the_start_of_a_word(self):
        e = lambda cell: d.expand(cell, self.order, "x")  # noqa: E731
        self.assertEqual(e("Come Casi Cosa Cx C1, P8"), ["P8"])
        plan = d.parse_ss_plan(ss_plan(SS_PACKAGES + SS_CONTRACT, SS_UNBLOCKS + [(364, "Come P8, Casi")]), "ss.md")
        self.assertEqual(plan.unblocks[364], ["P8"])

    def test_unblocks_with_c(self):
        unblocks = [(357, "P4, P8, C")]
        plan = d.parse_ss_plan(ss_plan(SS_PACKAGES + SS_CONTRACT, unblocks), "ss.md")
        self.assertEqual(plan.unblocks[357], ["P4", "P8", "C"])

    def test_bad_first_cell_names_c(self):
        text = ss_plan(SS_PACKAGES + SS_CONTRACT).replace("| C Titolo |", "| Revisione del contratto |")
        with self.assertRaisesRegex(d.Failure, r"ss\.md:[0-9]+: the first cell must start with PN, RN, C or PSN, RSN, CS"):
            d.parse_ss_plan(text, "ss.md")

    def test_proposal_names_c(self):
        w = world_07()
        unblocks = [(n, f"{cell}, C" if n == 357 else cell) for n, cell in SS_UNBLOCKS]
        w.responses[f"repos/{SS}/contents/docs/tappe/0.2-geometria-del-piano.md?ref=main"] = \
            {"content": b64(ss_plan(SS_PACKAGES + SS_CONTRACT, unblocks))}
        w.responses[f"repos/{SS}/issues?state=all&per_page=100&milestone=2"].append(ss_issue(133))
        code, out, _, _ = w.run("--senza-project")
        self.assertEqual(code, 1, out)
        self.assertRegex(out, r"P14 \(docs/tappe/0\.7-modifiche\.md:[0-9]+\): \"Dipende da\" cites the request "
                              rf".*; with the packages of solid-space it is: \"P0, {SS}#49, {SS}#56, {SS}#133, R14\"")


def aligned_world(**changes) -> World:
    """Rows already citing the packages, as after 2026-11-01."""
    rows = [tg_row("P1", 10, "—"), tg_row("R2", 11, f"{SS}#38", "gemini"),
            tg_row("P2", 12, f"P1, {SS}#56, R2")]
    tg = [issue(10, "closed"), issue(11, labels=["agente:gemini"], body=body(f"{SS}#38 (0.2-R4b)")),
          issue(12, labels=["agente:claude"], body=body(f"#10 (P1), {SS}#56 (0.2-P11), #11 (R2)"))]
    relations = {11: [relation(SS, 38)], 12: [relation(TG, 10), relation(SS, 56, "open"), relation(TG, 11, "open")]}
    ss = [ss_issue(38), ss_issue(56, "open")]
    args = dict(plan=tg_plan(rows), tg=tg, ss=ss, relations=relations, project={11: "Da fare", 12: "Bloccato"})
    args.update(changes)
    return World(**args)


class Checks(unittest.TestCase):
    def test_aligned_exit_0(self):
        code, out, err, gh = aligned_world().run()
        self.assertEqual(code, 0, out + err)
        self.assertIn("Ready:\n  #11 [0.7-R2] agente:gemini\n", out)
        self.assertIn(f"Blocked:\n  #12 [0.7-P2] agente:claude by {SS}#56, #11\n", out)
        self.assertIn("Nothing to fix or to check.", out)

    def test_ready_and_blocked_show_the_difficulty(self):
        w = aligned_world()
        listed = w.responses[f"repos/{TG}/issues?state=all&per_page=100&labels=tappa-0.7"]
        listed[1]["labels"].append({"name": "media"})
        listed[2]["labels"].insert(0, {"name": "facile"})
        code, out, err, _ = w.run()
        self.assertEqual(code, 0, out + err)
        self.assertIn("Ready:\n  #11 [0.7-R2] media agente:gemini\n", out)
        self.assertIn(f"Blocked:\n  #12 [0.7-P2] facile agente:claude by {SS}#56, #11\n", out)

    def test_text_both_ways_and_notes_ignored(self):
        w = aligned_world()
        w.responses[f"repos/{TG}/issues?state=all&per_page=100&labels=tappa-0.7"][2]["body"] = \
            body(f"#10 (P1), {SS}#20 (richiesta (vecchia)), #11")
        code, out, _, _ = w.run()
        self.assertEqual(code, 1)
        self.assertIn(f"#12 [0.7-P2]: text \"Dipende da\": missing {SS}#56; not in the plan {SS}#20", out)
        self.assertNotIn("#11 [0.7-R2]: text", out)

    def test_relations_both_ways(self):
        w = aligned_world()
        w.responses[f"repos/{TG}/issues/12/dependencies/blocked_by?per_page=100"] = \
            [relation(TG, 10), relation(SS, 23, "open")]
        code, out, _, _ = w.run()
        self.assertEqual(code, 1)
        self.assertIn(f"#12 [0.7-P2]: \"blocked by\": missing {SS}#56, #11; not in the plan {SS}#23", out)

    def test_project(self):
        code, out, _, _ = aligned_world(project={11: "Bloccato", 12: None}).run()
        self.assertEqual(code, 1)
        self.assertIn("#11 [0.7-R2]: Project Status Bloccato, should be Da fare", out)
        self.assertIn("#12 [0.7-P2]: Project Status (empty), should be Bloccato", out)
        code, out, _, _ = aligned_world(project={11: "Fatto"}).run()
        self.assertIn("#12 [0.7-P2]: not in the Project", out)
        self.assertNotIn("#11 [0.7-R2]: Project", out)
        code, out, _, gh = aligned_world(project=None).run("--senza-project")
        self.assertEqual(code, 0, out)
        self.assertFalse(any(p.startswith("users/") for p in gh.read))

    def test_read_failure_exit_2(self):
        w = aligned_world()
        del w.responses[f"repos/{TG}/issues/12/dependencies/blocked_by?per_page=100"]
        code, _, err, _ = w.run()
        self.assertEqual(code, 2)
        self.assertIn("GitHub refused", err)

    def test_bad_plan_exit_2(self):
        code, _, err, _ = aligned_world(plan=tg_plan([tg_row("P1", 10, "boh")])).run()
        self.assertEqual(code, 2)
        self.assertIn("line 9", err)

    def test_agent_label_is_information(self):
        w = aligned_world()
        w.responses[f"repos/{TG}/issues?state=all&per_page=100&labels=tappa-0.7"][1]["labels"] = \
            [{"name": "agente:codex"}]
        code, out, _, _ = w.run()
        self.assertEqual(code, 0, out)
        self.assertIn("#11 [0.7-R2]: label agente:codex, plan agente:gemini", out)


def world_07(closed_tg=(333, 346, 358), open_ss=(56, 61, 63, 66, 68, 70), requests_open=True) -> World:
    """The stage 0.7 before 2026-11-01, with its rows citing the requests."""
    rows_deps = {r.split("|")[1].split()[0]: r.split("|")[4].strip() for r in ROWS_07}
    tg = []
    for r in ROWS_07:
        id_, number = r.split("|")[1].split()[0], int(r.split("|")[3].strip()[1:])
        deps = rows_deps[id_]
        text = ", ".join(x for x in deps.split(", ")) if deps != "—" else "—"
        tg.append(issue(number, "closed" if number in closed_tg else "open", body=body(text)))
    ss = [ss_issue(n, "open" if n in open_ss else "closed") for _, n, _ in SS_PACKAGES]
    requests = [ss_issue(n, "open" if requests_open else "closed", ["richiesta"]) for n in (20, 21, 22, 23)]
    return World(tg_plan(ROWS_07, REQUESTS_07), tg, ss, requests=requests)


class Proposals(unittest.TestCase):
    def test_rows_of_07(self):
        code, out, _, _ = world_07().run("--senza-project")
        self.assertEqual(code, 1)
        expected = {
            "R14": f"{SS}#38, {SS}#47, {SS}#48, {SS}#50, {SS}#52, {SS}#53, {SS}#55",
            "P14": f"P0, {SS}#49, {SS}#56, R14",
            "R17": f"{SS}#60, {SS}#61, {SS}#62",
            "P17": f"P14, {SS}#63, R17",
            "R19": f"{SS}#64, {SS}#65, {SS}#67, {SS}#68, {SS}#69",
            "P19": f"P15, {SS}#70, R19",
            "R22": f"{SS}#38, {SS}#47, {SS}#48",
            "P22": "P14, R22",
        }
        for id_, cell in expected.items():
            self.assertRegex(out, rf"{id_} \(docs/tappe/0\.7-modifiche\.md:[0-9]+\): "
                             rf"\"Dipende da\" cites the request .*; with the packages of solid-space it is: "
                             rf"\"{cell}\"")

    def test_closed_stage_proposes_nothing(self):
        w = world_07()
        w.responses[f"repos/{SS}/contents/docs/ROADMAP.md?ref=main"] = \
            {"content": b64(SS_ROADMAP.replace("**aperta**", "in preparazione"))}
        code, out, _, _ = w.run("--senza-project")
        self.assertNotIn("with the packages of solid-space", out)
        self.assertIn("the stage 0.2 of solid-space is not open", out)


class Requests(unittest.TestCase):
    def test_close_when_all_closed(self):
        code, out, _, _ = world_07(closed_tg=(333, 346, 358, 357, 373)).run("--senza-project")
        self.assertIn(f"{SS}#23: P14 (#357), P22 (#373) merged: check its success condition", out)
        self.assertIn(f"{SS}#20: P14 (#357) merged", out)
        self.assertNotIn(f"{SS}#22:", out)

    def test_not_while_one_is_open(self):
        _, out, _, _ = world_07(closed_tg=(333, 346, 358, 357)).run("--senza-project")
        self.assertNotIn(f"{SS}#23:", out)
        self.assertIn(f"{SS}#20: P14 (#357) merged", out)


class Applica(unittest.TestCase):
    def broken(self) -> World:
        w = aligned_world(project={11: "Bloccato", 12: "Bloccato"})
        w.responses[f"repos/{TG}/issues?state=all&per_page=100&labels=tappa-0.7"][2]["body"] = \
            body(f"#10 (P1), {SS}#20, #11 (R2)")
        w.responses[f"repos/{TG}/issues/12/dependencies/blocked_by?per_page=100"] = \
            [relation(TG, 10), relation(SS, 20, "open"), relation(TG, 11, "open")]
        return w

    def test_calls(self):
        code, out, err, gh = self.broken().run("--applica")
        self.assertEqual(code, 1, out + err)
        self.assertIn((f"PATCH", f"repos/{TG}/issues/12",
                       {"body": body(f"#10 (P1), {SS}#56 (0.2-P11), #11 (R2)")}), gh.sent)
        self.assertIn(("POST", f"repos/{TG}/issues/12/dependencies/blocked_by", {"issue_id": 5056}), gh.sent)
        self.assertIn(("DELETE", f"repos/{TG}/issues/12/dependencies/blocked_by/5020", None), gh.sent)
        self.assertIn(("PATCH", f"users/{OWNER}/projectsV2/3/items/911", {"fields": [{"id": 77, "value": "o1"}]}),
                      gh.sent)
        self.assertEqual(len(gh.sent), 4)
        self.assertFalse(any(p.startswith(f"repos/{SS}") for _, p, _ in gh.sent))

    def test_not_confirmed(self):
        code, out, _, gh = self.broken().run("--applica", answer="n")
        self.assertEqual(code, 3)
        self.assertEqual(gh.sent, [])
        self.assertIn("Nothing changed.", out)

    def test_never_writes_elsewhere(self):
        with self.assertRaises(AssertionError):
            d.apply([d.Change(d.Ref(TG, 1), "x", "POST", f"repos/{SS}/issues/1/comments", {})],
                    FakeGitHub({}), TG, OWNER)

    def test_failure_exit_2(self):
        class Refusing(FakeGitHub):
            def send(self, method, path, body=None):
                if method == "DELETE":
                    raise d.Failure("no")
                return super().send(method, path, body)

        w = self.broken()
        gh = Refusing(w.responses)
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / "docs" / "tappe").mkdir(parents=True)
            (Path(tmp) / "docs" / "tappe" / "0.7-modifiche.md").write_text(w.plan, encoding="utf-8")
            with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()) as err:
                code = d.main(["0.7", "--applica"], gh=gh, root=Path(tmp), ask=lambda _: "s")
        self.assertEqual(code, 2)
        self.assertEqual(len(gh.sent), 3)  # the others are still made
        self.assertIn("Failed:", err.getvalue())


class Fallbacks(unittest.TestCase):
    def test_search_for_public_issues(self):
        w = aligned_world()
        del w.responses[f"repos/{SS}/issues?state=all&per_page=100&milestone=2"]
        w.responses[f"search/issues?q=repo%3A{SS}+is%3Aissue&per_page=100"] = [ss_issue(38), ss_issue(56, "open")]
        code, out, err, _ = w.run()
        self.assertEqual(code, 0, out + err)

    def test_json_values(self):
        self.assertEqual(d.json_values('[1, 2]\n[3]'), [[1, 2], [3]])
        self.assertEqual(d.json_values('{"a": 1}'), [{"a": 1}])


# --- The submodule of solid-space (0.7.5-P6) ------------------------------------------

# Three commits of main of solid-space, in this order, and one outside main.
OLD, MID, NEW = "1" * 40, "2" * 40, "3" * 40
OFF = "9" * 40
MAIN_ORDER = [OLD, MID, NEW]


def compare_status(base: str, head: str) -> str | None:
    """What the compare API of GitHub says of two commits of the fake
    solid-space: main is NEW; OFF diverges; anything else is unknown."""
    order = MAIN_ORDER + ["main"]
    full = {c[:7]: c for c in MAIN_ORDER}
    base, head = full.get(base, base), full.get(head, head)
    rank = {c: i for i, c in enumerate(order)}
    rank["main"] = rank[NEW]
    if OFF in (base, head):
        return "diverged"
    if base not in rank or head not in rank:
        return None
    if rank[base] == rank[head]:
        return "identical"
    return "ahead" if rank[head] > rank[base] else "behind"


def xref(pr: int, text: str, merged: str | None = "2026-11-01T10:00:00Z", repo: str = SS) -> dict:
    """A "cross-referenced" event of a timeline: a pull request that mentions
    the issue."""
    return {"event": "cross-referenced", "source": {"type": "issue", "issue": {
        "number": pr, "body": text, "pull_request": {"merged_at": merged},
        "repository": {"full_name": repo}}}}


def pull(pr: int, sha: str, merged_at: str, base: str = "main") -> dict:
    return {"number": pr, "merged_at": merged_at, "merge_commit_sha": sha, "base": {"ref": base}}


def package_body(deps: str, line: str | None = None) -> str:
    text = f"**Tappa:** 0.8 · **Pacchetto:** PT3\n**Dipende da:** {deps}\n"
    if line:
        text += line + "\n"
    return text + "**Riferimenti:** #20\n\nTesto."


class SubmoduleWorld:
    """The answers of GitHub for --sottomodulo and --controlla-puntatore."""

    def __init__(self):
        self.responses: dict = {f"repos/{TG}/issues?state=open&labels=pacchetto&per_page=100": []}

    def tg_issue(self, number: int, text: str, labels=("pacchetto", "agente:claude"), title: str = "") -> dict:
        data = issue(number, "open", labels, text, title or f"[0.8-PT{number}] lavoro")
        self.responses[f"repos/{TG}/issues?state=open&labels=pacchetto&per_page=100"].append(data)
        self.responses[f"repos/{TG}/issues/{number}"] = data
        self.responses[f"repos/{TG}/issues/{number}/comments?per_page=100"] = []
        return data

    def package(self, number: int, state: str = "closed", reason: str | None = "completed",
                events=(), labels=(), tag: str | None = None) -> None:
        data = issue(number, state, labels, title=f"[{tag or f'0.3-PS{number}'}] pacchetto")
        data["state_reason"] = reason
        self.responses[f"repos/{SS}/issues/{number}"] = data
        self.responses[f"repos/{SS}/issues/{number}/timeline?per_page=100"] = list(events)

    def pulls(self, *pulls: dict) -> None:
        for p in pulls:
            self.responses[f"repos/{SS}/pulls/{p['number']}"] = p

    def run(self, *args: str) -> tuple[int, str, str, "CompareGitHub"]:
        gh = CompareGitHub(self.responses)
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            code = d.main(list(args), gh=gh, root=Path("unused"))
        return code, out.getvalue(), err.getvalue(), gh


class CompareGitHub(FakeGitHub):
    def get(self, path, paginate=False):
        m = re.fullmatch(rf"repos/{SS}/compare/([^.]+)\.\.\.(.+)", path)
        if m:
            self.read.append(path)
            status = compare_status(m.group(1), m.group(2))
            if status is None:
                raise d.Failure(f"GitHub refused: gh api {path}")
            return {"status": status}
        return super().get(path, paginate)


def ss_line(sha: str, tag: str, pr: int) -> str:
    return f"**Sottomodulo di solid-space:** {sha} (chiude {tag} con la pull request {SS}#{pr})"


class Transport(unittest.TestCase):
    """GitHub.get itself, with subprocess.run replaced: gh refuses a read of
    solid-space and curl, without a token, answers."""

    def answers(self, *curl_outputs: str):
        calls: list[list[str]] = []
        outputs = list(curl_outputs)

        def run(args, **kwargs):
            calls.append(args)
            if args[0] == "gh":
                return subprocess.CompletedProcess(args, 1, "", "HTTP 403: blocked")
            return subprocess.CompletedProcess(args, 0, outputs.pop(0), "")
        return calls, unittest.mock.patch.object(d.subprocess, "run", run)

    @staticmethod
    def http(body: str, link: str = "", crlf: bool = False, proxy: bool = False) -> str:
        """What curl -D - prints, after text=True (LF) or as it is (CRLF)."""
        head = "HTTP/2 200\ncontent-type: application/json\n" + (f'link: <{link}>; rel="next"\n' if link else "")
        text = ("HTTP/1.1 200 Connection established\n\n" if proxy else "") + head + "\n" + body
        return text.replace("\n", "\r\n") if crlf else text

    def test_fallback_reads_the_body(self):
        for crlf in (False, True):
            for proxy in (False, True):
                with self.subTest(crlf=crlf, proxy=proxy):
                    calls, patch = self.answers(self.http('{"number": 56, "state": "closed"}', crlf=crlf,
                                                          proxy=proxy))
                    with patch:
                        value = d.GitHub(public=[SS]).get(f"repos/{SS}/issues/56")
                    self.assertEqual(value, {"number": 56, "state": "closed"})
                    self.assertEqual([c[0] for c in calls], ["gh", "curl"])
                    self.assertNotIn("Authorization", " ".join(calls[1]))

    def test_fallback_follows_the_pages(self):
        url = f"https://api.github.com/repos/{SS}/issues/56/timeline?per_page=100&page=2"
        calls, patch = self.answers(self.http('[{"event": "a"}]', link=url), self.http('[{"event": "b"}]'))
        with patch:
            value = d.GitHub(public=[SS]).get(f"repos/{SS}/issues/56/timeline?per_page=100", paginate=True)
        self.assertEqual(value, [{"event": "a"}, {"event": "b"}])
        self.assertEqual(calls[2][-1], url)

    def test_no_fallback_for_tecnigrafo(self):
        _, patch = self.answers()
        with patch, self.assertRaises(d.Failure):
            d.GitHub(public=[SS]).get(f"repos/{TG}/issues/1")


class ClosingKeywords(unittest.TestCase):
    def test_keywords(self):
        ref = d.Ref(SS, 56)
        for text in ("Closes #56", "closes: #56", "Fixes #56.", "RESOLVED #56", f"Closes {SS}#56",
                     "Testo.\n\nClose #56\n"):
            with self.subTest(text=text):
                self.assertTrue(d.closes(text, ref, SS))
        for text in ("Closes #560", "Chiude #56", "See #56", "Closes #5", f"Closes {TG}#56", "Closes#56"):
            with self.subTest(text=text):
                self.assertFalse(d.closes(text, ref, SS))

    def test_closed_issues_of_a_pull_request(self):
        text = f"Closes #12, fixes {TG}#13 and resolves {SS}#14.\nCloses #12\nSee #15."
        self.assertEqual(d.closed_issues(text, TG), [d.Ref(TG, 12), d.Ref(TG, 13)])
        self.assertEqual(d.closed_issues("Chiude #12", TG), [])


class Submodule(unittest.TestCase):
    """--sottomodulo: the line of the commit in the open packages."""

    def world(self, line: str | None = None) -> SubmoduleWorld:
        """PT3 (#30) depends on #29 and on PS56 of solid-space, closed by
        pull request 80 at commit MID."""
        w = SubmoduleWorld()
        w.tg_issue(30, package_body(f"#29 (RT3), {SS}#56 (0.3-PS56)", line))
        w.package(56, events=[xref(80, "Closes #56")])
        w.pulls(pull(80, MID, "2026-11-02T10:00:00Z"))
        return w

    def test_missing_line_is_written(self):
        w = self.world()
        code, out, err, gh = w.run("--sottomodulo")
        self.assertEqual(code, 1, out + err)
        self.assertIn(f"To write:\n  #30 [0.8-PT30]: {ss_line(MID, '0.3-PS56', 80)}", out)
        self.assertEqual(gh.sent, [])
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertEqual(gh.sent, [("PATCH", f"repos/{TG}/issues/30",
                                    {"body": package_body(f"#29 (RT3), {SS}#56 (0.3-PS56)",
                                                          ss_line(MID, "0.3-PS56", 80))})])

    def test_same_line_does_nothing(self):
        for line in (ss_line(MID, "0.3-PS56", 80), f"**Sottomodulo di solid-space:** {MID[:7]} (a mano)"):
            with self.subTest(line=line):
                code, out, err, gh = self.world(line).run("--sottomodulo", "--applica")
                self.assertEqual(code, 0, out + err)
                self.assertIn("Nothing to write, report or check.", out)
                self.assertEqual(gh.sent, [])
                self.assertNotIn(f"repos/{TG}/issues/30/comments?per_page=100", gh.read)

    def test_different_line_is_reported_not_changed(self):
        w = self.world(ss_line(OLD, "0.3-PS56", 70))
        code, out, err, gh = w.run("--sottomodulo")
        self.assertEqual(code, 1, out + err)
        self.assertIn(f"Different line:\n  #30 [0.8-PT30]: the line says {OLD}, the packages {MID}", out)
        self.assertEqual(gh.sent, [])
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertEqual(len(gh.sent), 1)
        method, path, data = gh.sent[0]
        self.assertEqual((method, path), ("POST", f"repos/{TG}/issues/30/comments"))
        self.assertIn(f"dice `{OLD}`", data["body"])
        self.assertIn(f"è `{MID}` (chiude 0.3-PS56 con la pull request {SS}#80)", data["body"])
        self.assertIn("più recente", data["body"])
        self.assertIn("La riga non è stata cambiata", data["body"])
        self.assertFalse(any(m == "PATCH" for m, _, _ in gh.sent))
        # Once reported, the same pair of commits is not reported again.
        w.responses[f"repos/{TG}/issues/30/comments?per_page=100"] = [{"body": data["body"]}]
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertEqual(gh.sent, [])
        self.assertIn("already reported", out)

    def test_why_the_line_differs(self):
        for line, why in ((NEW, "successivo"), (OFF, "non è su `main`")):
            with self.subTest(line=line):
                w = self.world(ss_line(line, "0.3-PS56", 90))
                _, _, _, gh = w.run("--sottomodulo", "--applica")
                self.assertIn(why, gh.sent[0][2]["body"])

    def test_dependencies_not_all_closed(self):
        w = self.world()
        w.tg_issue(31, package_body(f"{SS}#56, {SS}#57"))
        w.package(57, state="open", reason=None)
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertNotIn("#31", out)
        self.assertEqual([p for _, p, _ in gh.sent], [f"repos/{TG}/issues/30"])
        self.assertNotIn(f"repos/{SS}/issues/57/timeline?per_page=100", gh.read)

    def test_issues_without_solid_space_are_left_alone(self):
        w = SubmoduleWorld()
        w.tg_issue(32, package_body("#29 (RT3)"))
        w.tg_issue(33, "Senza dipendenze.")
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertEqual(gh.sent, [])

    def test_two_packages_take_the_latest_commit(self):
        w = SubmoduleWorld()
        w.tg_issue(30, package_body(f"{SS}#56 (0.3-PS56), {SS}#57 (0.3-PS57)"))
        # PS57 merged before PS56: its commit is not the one to point to.
        w.package(56, events=[xref(80, "Closes #56")])
        w.package(57, events=[xref(81, f"Fixes {SS}#57")])
        w.pulls(pull(80, NEW, "2026-11-05T10:00:00Z"), pull(81, MID, "2026-11-03T10:00:00Z"))
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertIn(ss_line(NEW, "0.3-PS56", 80), gh.sent[0][2]["body"])

    def test_only_merged_pull_requests_into_main_that_close_it(self):
        w = SubmoduleWorld()
        w.tg_issue(30, package_body(f"{SS}#56"))
        w.package(56, events=[
            xref(80, "Closes #56"),
            xref(81, "See #56: a later merged pull request that only mentions it"),
            xref(82, "Closes #56", merged=None),                  # not merged
            xref(83, "Closes #56"),                               # merged into another branch
            xref(84, f"Closes {SS}#56", repo=TG),                 # another repository
            xref(85, "Closes #56"),                               # a fix that closes it again
        ])
        w.pulls(pull(80, OLD, "2026-11-01T10:00:00Z"), pull(83, OFF, "2026-11-09T10:00:00Z", base="dev"),
                pull(85, MID, "2026-11-04T10:00:00Z"))
        code, out, err, gh = w.run("--sottomodulo")
        self.assertEqual(code, 1, out + err)
        self.assertIn(ss_line(MID, "0.3-PS56", 85), out)
        self.assertNotIn(f"repos/{SS}/pulls/81", gh.read)
        self.assertNotIn(f"repos/{SS}/pulls/82", gh.read)

    def test_closed_without_a_pull_request_or_not_planned(self):
        w = SubmoduleWorld()
        w.tg_issue(30, package_body(f"{SS}#56, {SS}#57, {SS}#58"))
        w.package(56, events=[xref(80, "Closes #56")])
        w.package(57, events=[])
        w.package(58, reason="not_planned", events=[xref(81, "Closes #58")])
        w.pulls(pull(80, MID, "2026-11-02T10:00:00Z"), pull(81, NEW, "2026-11-03T10:00:00Z"))
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 1, out + err)
        self.assertIn(f"#30 [0.8-PT30]: no pull request merged into main of solid-space closes {SS}#57", out)
        self.assertIn(f"#30 [0.8-PT30]: {SS}#58 was closed as not planned", out)
        self.assertEqual(gh.sent, [])

    def test_requests_do_not_count(self):
        w = SubmoduleWorld()
        w.tg_issue(30, package_body(f"{SS}#20 (richiesta), {SS}#56"))
        w.tg_issue(31, package_body(f"{SS}#20"))
        w.package(20, events=[], labels=["richiesta"])
        w.package(56, events=[xref(80, "Closes #56")])
        w.pulls(pull(80, MID, "2026-11-02T10:00:00Z"))
        code, out, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 0, out + err)
        self.assertIn("#31 [0.8-PT31]: its dependencies in solid-space are requests only", out)
        self.assertEqual([p for _, p, _ in gh.sent], [f"repos/{TG}/issues/30"])

    def test_issue_changed_after_the_list_is_not_overwritten(self):
        # The maintainer writes the line by hand after the list was read: the
        # issue is read again before the write, and nothing is written.
        for line in (ss_line(MID, "0.3-PS56", 80), ss_line(OLD, "0.3-PS56", 70)):
            with self.subTest(line=line):
                w = self.world(None if line.endswith("#80)") else line)
                w.responses[f"repos/{TG}/issues/30"] = dict(
                    w.responses[f"repos/{TG}/issues/30"],
                    body=package_body(f"#29 (RT3), {SS}#56 (0.3-PS56), #44", ss_line(NEW, "a mano", 1)))
                code, out, err, gh = w.run("--sottomodulo", "--applica")
                self.assertEqual(code, 1, out + err)
                self.assertEqual(gh.sent, [])
                self.assertIn("skipped #30", out)
                self.assertIn("the issue changed after it was read", out)

    def test_never_writes_to_solid_space_and_read_failure(self):
        w = self.world(ss_line(OLD, "0.3-PS56", 70))
        w.tg_issue(31, package_body(f"{SS}#57"))
        w.package(57, events=[xref(81, "Closes #57")])
        w.pulls(pull(81, NEW, "2026-11-03T10:00:00Z"))
        _, _, _, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(len(gh.sent), 2)
        self.assertTrue(all(p.startswith(f"repos/{TG}/issues/") for _, p, _ in gh.sent))
        del w.responses[f"repos/{SS}/pulls/81"]
        code, _, err, gh = w.run("--sottomodulo", "--applica")
        self.assertEqual(code, 2)
        self.assertIn("GitHub refused", err)
        self.assertEqual(gh.sent, [])


class Pointer(unittest.TestCase):
    """--controlla-puntatore: the check of a pull request that moves the
    submodule."""

    def world(self, pr_text: str = "Descrizione.\n\nCloses #30", line: str | None = None) -> SubmoduleWorld:
        w = SubmoduleWorld()
        w.tg_issue(30, package_body(f"{SS}#56", line if line is not None else ss_line(MID, "0.3-PS56", 80)))
        w.responses[f"repos/{TG}/pulls/40"] = {"number": 40, "body": pr_text}
        return w

    def check(self, w: SubmoduleWorld, commit: str, pr: str | None = "40", **env) -> tuple[int, str]:
        args = ["--controlla-puntatore", commit] + (["--pr", pr] if pr else [])
        with unittest.mock.patch.dict(os.environ, env):
            code, out, err, gh = w.run(*args)
        self.assertEqual(gh.sent, [])
        return code, out + err

    def test_right_pointer(self):
        for commit in (MID, NEW, MID.upper()):
            with self.subTest(commit=commit):
                code, out = self.check(self.world(), commit)
                self.assertEqual(code, 0, out)
                self.assertIn("right", out)

    def test_earlier_than_the_line(self):
        code, out = self.check(self.world(), OLD)
        self.assertEqual(code, 1, out)
        self.assertIn(f"points to {OLD}, earlier than {MID}, the commit that #30 names", out)

    def test_not_on_main(self):
        code, out = self.check(self.world(), OFF)
        self.assertEqual(code, 1, out)
        self.assertIn(f"{OFF}, which is not a commit of main of {SS}", out)
        code, out = self.check(self.world(), "8" * 40)
        self.assertEqual(code, 1, out)
        self.assertIn("unknown commit", out)

    def test_issue_without_the_line(self):
        code, out = self.check(self.world(line=""), NEW)
        self.assertEqual(code, 1, out)
        self.assertIn("#30 has no line \"**Sottomodulo di solid-space:** <commit>\": run the workflow "
                      "\"Submodule commit\"", out)

    def test_line_off_main(self):
        code, out = self.check(self.world(line=ss_line(OFF, "0.3-PS56", 80)), NEW)
        self.assertEqual(code, 1, out)
        self.assertIn(f"#30 names {OFF}, which is not a commit of main", out)

    def test_without_an_issue_only_main(self):
        for text, pr in (("Aggiorna il sottomodulo.", "40"), ("Closes #30", None)):
            with self.subTest(pr=pr):
                code, out = self.check(self.world(text, line=""), OLD, pr)
                self.assertEqual(code, 0, out)
                self.assertIn("only main checked", out)
        code, out = self.check(self.world("Aggiorna il sottomodulo."), OFF)
        self.assertEqual(code, 1, out)

    def test_every_closed_issue_needs_its_line(self):
        # #30 has a right line, #31 none: the line of #30 is not enough.
        w = self.world("Closes #30, closes #31")
        w.tg_issue(31, package_body(f"{SS}#57"))
        code, out = self.check(w, NEW)
        self.assertEqual(code, 1, out)
        self.assertIn("#31 has no line", out)
        self.assertNotIn("#30 has no line", out)
        self.assertIn(f"#30 names {MID}: {NEW} is the same or later", out)

    def test_annotation_in_github_actions(self):
        code, out = self.check(self.world(), OLD, GITHUB_ACTIONS="true")
        self.assertEqual(code, 1)
        self.assertIn("::error title=Submodule pointer::third_party/solid-space points to", out)

    def test_arguments(self):
        for args in (["--sottomodulo", "0.8"], ["--controlla-puntatore", MID, "--sottomodulo"],
                     ["--pr", "4", "0.8"], ["--controlla-puntatore", "not-a-commit"],
                     ["--controlla-puntatore", MID, "--applica"], ["--sottomodulo", "--senza-project"], []):
            with self.subTest(args=args), redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    d.main(args, gh=FakeGitHub({}), root=Path("unused"))
                self.assertEqual(error.exception.code, 2)


@unittest.skipUnless(shutil.which("bash") and shutil.which("jq"), "bash and jq required")
class MethodScripts(unittest.TestCase):
    """Exercise the shell entry points with isolated git and GitHub answers."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.bin = self.root / "bin"
        self.bin.mkdir()
        self.repo = Path(__file__).resolve().parent.parent
        self.env = {**os.environ, "PATH": f"{self.bin}{os.pathsep}{os.environ['PATH']}",
                    "CALLS": str(self.root / "calls"), "TG_REPO": TG,
                    "TG_ATTESA_WORKFLOW": "ci.yml", "TG_ATTESA_MAX": "1",
                    "TG_ATTESA_PAUSA": "1"}
        self.write_tool("git", '''#!/usr/bin/env bash
case "$*" in
    'rev-parse --show-toplevel') pwd ;;
    'rev-parse --verify '*) exit 1 ;;
    'rev-parse '*) echo aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ;;
    'ls-remote '*) exit 2 ;;
    'switch --quiet -c '*) touch .changed ;;
    'status --porcelain') if [[ -f .changed ]]; then echo ' M docs/ROADMAP.md'; fi ;;
    *) exit 0 ;;
esac
''')

    def write_tool(self, name, text):
        path = self.bin / name
        path.write_text(text, encoding="utf-8")
        path.chmod(0o755)

    def run_script(self, name, *args, **env):
        return subprocess.run(["bash", str(self.repo / "scripts" / name), *args],
                              cwd=self.root, env={**self.env, **env},
                              capture_output=True, text=True, timeout=15)

    def test_wait_selects_workflow_with_default_commit_sha_and_run_id(self):
        # A second workflow has failed on the same commit: the generic endpoint
        # deliberately returns that run, so selecting it fails this test.
        self.write_tool("gh", '''#!/usr/bin/env bash
printf '%s\\n' "$*" >> "$CALLS"
case "$2" in
    */actions/workflows/*/runs?*)
        echo '{"workflow_runs":[{"id":42,"status":"completed","conclusion":"success"}]}' ;;
    */actions/runs/42) echo '{"id":42,"status":"completed","conclusion":"success"}' ;;
    *) echo '{"workflow_runs":[{"id":99,"status":"completed","conclusion":"failure"}]}' ;;
esac
''')
        sha = "a" * 40
        for args, workflow, endpoint in (([], "ci.yml", f"workflows/ci.yml/runs?head_sha={sha}&per_page=1"),
                                         (["b" * 40], "ci.yml", f"workflows/ci.yml/runs?head_sha={'b' * 40}&per_page=1"),
                                         ([], "other.yml", f"workflows/other.yml/runs?head_sha={sha}&per_page=1"),
                                         (["42"], "other.yml", "runs/42")):
            with self.subTest(args=args, workflow=workflow):
                result = self.run_script("ci-attendi.sh", *args, TG_ATTESA_WORKFLOW=workflow)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual((self.root / "calls").read_text(), f"api repos/{TG}/actions/{endpoint}\n")
                (self.root / "calls").unlink()

    def prepare_stage(self, stage, modern=True, wrong_dep=False):
        prefix = "T" if modern else ""
        rows = [tg_row(f"R{prefix}5", None, "—", "gemini"),
                tg_row(f"P{prefix}5", None, "R5" if wrong_dep else f"R{prefix}5")]
        text = tg_plan(rows).replace("# Tappa 0.7", f"# Tappa {stage}")
        text = text.replace("## 5.", "- **Stato:** in preparazione\n\n## 5.")
        plan = self.root / "docs" / "tappe" / f"{stage}-test.md"
        plan.parent.mkdir(parents=True)
        plan.write_text(text, encoding="utf-8")
        sheet = plan.parent / "0.7" / f"P{prefix}5.md"
        sheet.parent.mkdir()
        sheet.write_text("Sheet", encoding="utf-8")
        (self.root / "docs" / "ROADMAP.md").write_text(
            "| 7 | 0.7 | old | **chiusa** | old.md |\n"
            "| 7.5 | 0.7.5 | patch | in preparazione | patch.md |\n"
            "| 8 | 0.8 | next | in preparazione | next.md |\n", encoding="utf-8")
        # Every mutation is logged and answered locally; there is no network.
        self.write_tool("gh", '''#!/usr/bin/env python3
import json, os, sys
from pathlib import Path
args = sys.argv[2:]
method = args[args.index('-X') + 1] if '-X' in args else 'GET'
path = next(a for a in args if a.startswith('repos/'))
data = json.load(sys.stdin) if '--input' in args else None
with open(os.environ['CALLS'], 'a') as log:
    log.write(json.dumps([method, path, data]) + '\\n')
if method == 'GET' and '/labels?' in path:
    print(json.dumps([{'name': 'agente:' + a} for a in ('claude', 'gemini')]))
elif method == 'GET' and '/milestones?' in path:
    print('[{"title":"v''' + stage + '''","number":8}]')
elif method == 'POST' and path.endswith('/issues'):
    counter = Path('counter')
    n = int(counter.read_text()) + 1 if counter.exists() else 10
    counter.write_text(str(n))
    print(n)
elif method == 'GET' and '/issues/' in path:
    print('1000 closed')
elif method == 'GET':
    print('[]')
elif method == 'POST' and path.endswith('/pulls'):
    print('https://example.invalid/pull/1')
else:
    print('{}')
''')

    def test_open_stage_uses_new_prefixes_in_issues_dependencies_and_pending_file(self):
        self.prepare_stage("0.8")
        result = self.run_script("apri-tappa.sh", "0.8", "--si", "--senza-project", "--senza-pr")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        calls = [json.loads(line) for line in (self.root / "calls").read_text().splitlines()]
        bodies = [data["body"] for method, _, data in calls if method == "PATCH"]
        self.assertIn("tests/verifica/in-attesa/0.8-RT5.txt", bodies[0])
        self.assertIn("**Riferimenti del pacchetto:** PT5 (#11)", bodies[0])
        self.assertIn("**Dipende da:** #10 (RT5)", bodies[1])
        titles = [data["title"] for method, path, data in calls
                  if method == "POST" and path.endswith("/issues")]
        self.assertEqual(titles, ["[0.8-RT5] Riferimenti RT5", "[0.8-PT5] Lavoro PT5"])

    def test_open_patch_stage_updates_only_the_whole_roadmap_version(self):
        self.prepare_stage("0.7.5", modern=False)
        result = self.run_script("apri-tappa.sh", "0.7.5", "--si", "--senza-project")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual((self.root / "docs" / "ROADMAP.md").read_text(),
                         "| 7 | 0.7 | old | **chiusa** | old.md |\n"
                         "| 7.5 | 0.7.5 | patch | **aperta** | patch.md |\n"
                         "| 8 | 0.8 | next | in preparazione | next.md |\n")

    def test_open_stage_rejects_old_prefix_in_rows_and_dependencies(self):
        for modern, wrong_dep in ((False, False), (True, True)):
            with self.subTest(modern=modern):
                # Each fixture gets a separate tree.
                with tempfile.TemporaryDirectory() as tmp:
                    previous = self.root
                    self.root = Path(tmp)
                    try:
                        self.prepare_stage("0.8", modern, wrong_dep)
                        result = self.run_script("apri-tappa.sh", "0.8", "--prova", "--senza-project")
                    finally:
                        self.root = previous
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertIn("PTN", result.stderr)
                self.assertIn("stage 0.8", result.stderr)

    @unittest.skipUnless(shutil.which("cmake"), "CMake required")
    def test_pending_files_accept_both_prefixes_and_split_blocks(self):
        pending = self.root / "pending"
        pending.mkdir()
        names = ["0.7-R5.txt", "0.7.5-R5a.txt", "0.8-RT5.txt", "0.8-RT14a.txt"]
        for i, name in enumerate(names):
            (pending / name).write_text(f"area/case{i} #1\n", encoding="utf-8")
        check = self.root / "check.cmake"
        check.write_text(f'''cmake_minimum_required(VERSION 3.20)
include("{self.repo / 'tests/verifica/pending_list.cmake'}")
tg_read_pending_list("{pending}" "area/case0;area/case1;area/case2;area/case3" result)
if(result_ERRORS)
    message(FATAL_ERROR "${{result_ERRORS}}")
endif()
list(LENGTH result_NAMES count)
if(NOT count EQUAL 4)
    message(FATAL_ERROR "Expected four pending cases")
endif()
''', encoding="utf-8")
        result = subprocess.run(["cmake", "-P", str(check)], capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
