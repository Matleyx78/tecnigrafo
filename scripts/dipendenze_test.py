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
import sys
import tempfile
import unittest
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


class Unblocks(unittest.TestCase):
    order = [p[0] for p in SS_PACKAGES]

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


if __name__ == "__main__":
    unittest.main()
