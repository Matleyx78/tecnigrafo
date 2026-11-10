#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Measures the packages of one or more stages ("tappe") of Tecnigrafo: for
# every issue of the stage, its main pull request and the pull requests that
# correct it, with commits, files, lines, CI runs, correction rounds, hours
# and claims (docs/tappe/0.7.5/P39-misura-pacchetti.md). It joins in one
# program the scripts of section 7.2 of the analysis
# docs/tappe/0.7.5/analisi-pacchetti-medi.md, with the definitions of its
# section 1.
#
#     scripts/misura_pacchetti.py --tappe 0.7.5 --dati <folder> --scarica
#     scripts/misura_pacchetti.py --tappe 0.7.5 --dati <folder>
#     scripts/misura_pacchetti.py --tappe 0.7 0.7.5 --dati <folder>
#
# --scarica  reads from GitHub (REST only, one page at a time with &page=N,
#            because the proxy of the cloud sessions refuses
#            gh api --paginate) and saves everything in <folder>: issues,
#            pull requests, CI runs, details and files of the pull requests
#            used, issue comments, the table of section 6 of every plan and
#            the reference cases touched by the reference packages, read
#            from the checkout (--radice). Details and comments already in
#            <folder> are not read again.
# Without --scarica it only reads <folder>, without network: the measure is
# repeatable from the saved data.
#
# It prints the tables in Markdown and writes <folder>/misura.json, one row
# per issue.
#
# Definitions (section 1 of the analysis):
#
#   main pull request    the earliest merged pull request, opened after the
#                        issue, that closes the issue ("Closes #N") or has its
#                        code in the title (a pull request of the plan that
#                        creates the package and names it is not its main one);
#   correction           a pull request merged after the main one that cites
#                        the code of the package (0.7.5-P24) in the title or,
#                        when its title cites no code, in the description, or
#                        closes its issue, and is not the main pull request
#                        of an issue; it corrects the references when it
#                        touches only tests/verifica/, the documents when it
#                        touches only docs/ or Markdown files, the code
#                        otherwise;
#   lines, files         additions plus deletions of the main pull request,
#                        third_party/ excluded, tests included;
#   code lines           the same, without tests (tests/, *_test.*), documents
#                        (docs/, *.md) and resources (resources/): the measure
#                        of condition 2 of section 5 of the analysis;
#   CI runs              runs of ci.yml with the event pull_request on the
#                        branch of the pull request, between its opening and
#                        its merge;
#   correction round     a failed run before the last green run; cancelled
#                        runs are counted apart;
#   claim                a comment of the issue with "presa in carico";
#   fine case            a question .tgq whose answer has a number with an
#                        exponent or at least four decimals, or that follows a
#                        "!tolerance" line, or asks a tangency, fillet, offset
#                        or chamfer construction; a script .tgs whose
#                        atteso.txt has such a number. The others are ordinary.
#
# Planned difficulty, agent, model and effort come from the table of
# section 6 of each plan; the declared tool, model and effort from the
# section "Uso dell'IA" of the main pull request, and only from there. Where
# they are missing the value is "non dichiarato", never deduced. The author is
# the declared tool; only for an old pull request with no such section it is
# the first tool named in its description, as in the analysis, and that guess
# is never used as a declared value. The only exception is written in
# DECLARED_BY_MAINTAINER and always marked as such.
#
# Exit status: 0 done, 2 a read failed or a saved file is missing.
#
# Environment: TG_REPO repository (default Matleyx78/tecnigrafo).

from __future__ import annotations

import argparse
import json
import os
import re
import statistics
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime, timedelta
from pathlib import Path
from typing import Callable, Iterable

NOT_DECLARED = "non dichiarato"
REPO = os.environ.get("TG_REPO", "Matleyx78/tecnigrafo")
CASES = "tests/verifica/casi/"

# Effort used in stage 0.7.5, declared by the maintainer on 2026-11-10 because
# the pull requests did not write it (docs/tappe/0.7.5/P39-misura-pacchetti.md,
# "Regole della tappa che valgono qui"). Key: (stage, difficulty, tool).
DECLARED_BY_MAINTAINER = {
    ("0.7.5", "media", "claude"): ("Claude Opus 5.5", "medio"),
    ("0.7.5", "media", "codex"): ("Codex sol", "medio"),
    ("0.7.5", "impegnativa", "claude"): ("Claude Opus 5.5", "alto"),
    ("0.7.5", "impegnativa", "codex"): ("Codex sol o astra light", "alto"),
}

# Hand assignments of the analysis (section 7.1), for stages 0.4-0.7: pull
# requests that carry neither "Closes" nor the code of the package.
MANUAL_FIX = {
    145: "0.4-R15", 204: "0.5-R12", 205: "0.5-R12", 208: "0.5-R15", 214: "0.5-R16", 216: "0.5-R16",
    444: "0.7-R10", 445: "0.7-R10", 459: "0.7-R14", 463: "0.7-R22", 474: "0.7-R15",
    523: "0.7-R21", 524: "0.7-R21", 527: "0.7-R23", 507: "0.7-R20",
}
MANUAL_CARD_FIX = {200: "0.5-P10", 528: "0.7-P23"}
# Not corrections: #273 cites the number of an issue in a plan; #441 and #443
# (tools out of the stages) name pending files of reference packages.
EXCLUDED = {273, 441, 443}
# Author where the work changed hands, read in the issue comments (analysis,
# section 1, "Autore").
AUTHOR_OVERRIDE = {
    "0.4-P5": "claude", "0.4-P9": "codex", "0.7-P18": "codex", "0.7-P22": "codex", "0.7-P25": "codex",
    "0.5-R10": "gemini", "0.7-R9": "gemini-argon", "0.7-R16": "gemini-argon",
}


class Failure(Exception):
    """A read failed or a saved file is missing: exit status 2."""


# --- Codes of packages -----------------------------------------------------------

CODE_RE = re.compile(r"(?<![\w.])(\d+\.\d+(?:\.\d+)?)-(PT|RT|CT|P|R|C)(\d+[a-z]?(?:-Qt)?)(?!\w)")


def version(stage: str) -> tuple[int, ...]:
    return tuple(int(x) for x in stage.split("."))


def prefix_fits(stage: str, prefix: str) -> bool:
    """P/R/C up to 0.7.5, PT/RT/CT from 0.8 (LAVORO-IN-PARALLELO.md, "Come
    leggere i riferimenti")."""
    new = version(stage) >= (0, 8)
    return (len(prefix) == 2) == new


def codes(text: str) -> list[str]:
    """Codes of packages in a text, in order and without repetitions:
    0.7-P5, 0.7.5-R28b, 0.8-PT12."""
    out: list[str] = []
    for m in CODE_RE.finditer(text or ""):
        stage, prefix, num = m.groups()
        if not prefix_fits(stage, prefix):
            continue
        c = f"{stage}-{prefix}{num}"
        if c not in out:
            out.append(c)
    return out


def code_stage(code: str) -> str:
    return code.split("-", 1)[0]


def code_letter(code: str) -> str:
    """P, R or C, whatever the prefix, of a code (0.7-P5) or a package (P5)."""
    return code.split("-", 1)[-1][:1]


# --- Pull request text -----------------------------------------------------------

COMMENT_RE = re.compile(r"<!--.*?-->", re.S)


def clean(body: str | None) -> str:
    return COMMENT_RE.sub("", body or "")


def ai_section(body: str | None) -> str:
    """The text of the section "Uso dell'IA" of a pull request, without the
    comments of the template; empty when there is none."""
    m = re.search(r"(?ims)^#+\s*uso dell.?ia\s*$(.*?)(?=^#+\s|\Z)", clean(body))
    return m.group(1).strip() if m else ""


TOOLS = [("claude code", "claude"), ("codex", "codex"), ("antigravity", "gemini"),
         ("gemini", "gemini"), ("copilot", "copilot"), ("claude", "claude")]

MODEL_RES = [
    (re.compile(r"\b(opus|sonnet|haiku|fable|mythos)\s*(\d+(?:\.\d+)?)?", re.I),
     lambda m: "Claude " + m.group(1).capitalize() + (f" {m.group(2)}" if m.group(2) else "")),
    (re.compile(r"\bgemini\s*3\.8\s*flash\b|\bflash\b", re.I), lambda m: "Gemini 3.8 Flash"),
    (re.compile(r"\bgemini\s*4\s*argon\b|\bargon\b", re.I), lambda m: "Gemini 4 Argon"),
    (re.compile(r"\bgpt-?\s*(\d+(?:\.\d+)?)", re.I), lambda m: f"GPT-{m.group(1)}"),
]
CODEX_VARIANT_RE = re.compile(r"\b(sol|astra)(\s+light)?\b", re.I)
EFFORT_RE = re.compile(r"\bimpegno\s+(non noto|molto alto|massimo|basso|medio|alto)\b", re.I)
EFFORT_EN_RE = re.compile(r"\b(?:effort|reasoning)\s*[:=]?\s*(low|medium|high|xhigh)\b"
                          r"|\b(?:sol|astra)\s+(low|medium|high|xhigh)\b", re.I)
EFFORT_EN = {"low": "basso", "medium": "medio", "high": "alto", "xhigh": "molto alto"}


@dataclass
class Declared:
    tool: str      # claude, codex, gemini, copilot or "?"
    model: str     # models in order, joined by " + ", or NOT_DECLARED
    effort: str    # basso, medio, alto, molto alto, massimo, non noto, or NOT_DECLARED


NO_AI = "nessuna"


def first_tool(text: str) -> str:
    low = text.lower()
    hits = [(low.find(k), v) for k, v in TOOLS if k in low]
    return min(hits)[1] if hits else "?"


def declared(body: str | None) -> Declared:
    """Tool, model and effort written in the section "Uso dell'IA", and only
    there: "?" and NOT_DECLARED where the section does not say them, NO_AI
    when it ticks "Nessuna IA usata" and not "IA usata"."""
    s = ai_section(body)
    low = s.lower()
    if re.search(r"\[x\]\s*nessuna ia", low) and not re.search(r"\[x\]\s*ia usata", low):
        return Declared(NO_AI, NOT_DECLARED, NOT_DECLARED)
    tool = first_tool(s)
    found: list[tuple[int, str]] = []
    for rx, name in MODEL_RES:
        for m in rx.finditer(s):
            found.append((m.start(), name(m)))
    found.sort()
    models: list[str] = []
    for _, n in found:
        if n.startswith("GPT-"):
            v = CODEX_VARIANT_RE.search(s)
            if v:
                n = f"{n} {v.group(1).lower()}{' light' if v.group(2) else ''}"
        if n not in models:
            models.append(n)
    if not models:
        v = CODEX_VARIANT_RE.search(s)
        if v and tool == "codex":
            models.append(f"Codex {v.group(1).lower()}{' light' if v.group(2) else ''}")
    e = EFFORT_RE.search(s)
    if e:
        effort = e.group(1).lower()
    else:
        en = EFFORT_EN_RE.search(s)
        effort = EFFORT_EN[(en.group(1) or en.group(2)).lower()] if en else NOT_DECLARED
    return Declared(tool, " + ".join(models) if models else NOT_DECLARED, effort)


def author_hint(body: str | None) -> str:
    """For the author of old pull requests without the section "Uso dell'IA"
    (before the template had it): the first tool named in the description, as
    in the analysis. Never a declared value; "?" when the section exists."""
    return first_tool(clean(body)) if not ai_section(body) else "?"


def model_family(model: str) -> str:
    """The family of a model, to compare a pull request with a declaration:
    opus, sonnet, flash, argon, codex (GPT or sol/astra), ..."""
    low = model.lower()
    for k in ("opus", "sonnet", "haiku", "fable", "mythos", "flash", "argon"):
        if k in low:
            return k
    if "gpt" in low or "codex" in low or "sol" in low or "astra" in low:
        return "codex"
    return low


# --- Plans ----------------------------------------------------------------------


@dataclass
class PlanRow:
    package: str          # P1, R28b, PT5
    issue: int | None
    deps: list[str]       # packages of "Dipende da", same stage
    difficulty: str
    agent: str
    model: str            # strength class: forte, medio, leggero
    effort: str


def split_row(line: str) -> list[str]:
    return [c.strip() for c in line.strip().strip("|").split("|")]


def plan_table(text: str) -> list[PlanRow]:
    """The table of the packages in section 6 of a plan: the first table of
    that section whose header starts with "Pacchetto". Columns are read by
    name; a missing column gives NOT_DECLARED."""
    m = re.search(r"(?ms)^## 6\..*?(?=^## \d|\Z)", text)
    if not m:
        return []
    lines = m.group(0).splitlines()
    rows: list[PlanRow] = []
    header: list[str] | None = None
    for i, line in enumerate(lines):
        if header is None:
            if line.startswith("| Pacchetto |") and i + 1 < len(lines) and lines[i + 1].startswith("|---"):
                header = [h.lower() for h in split_row(line)]
            continue
        if not line.startswith("|"):
            if rows:
                break
            continue
        if line.startswith("|---"):
            continue
        cells = split_row(line)
        cell = dict(zip(header, cells))

        def value(name: str) -> str:
            v = cell.get(name, "").strip()
            return v if v and v not in ("—", "-") else NOT_DECLARED

        first = cell.get("pacchetto", "").split()
        package = first[0] if first else ""
        issue = re.search(r"#(\d+)", cell.get("issue", ""))
        deps = re.findall(r"\b((?:PT|RT|CT|P|R|C)\d+[a-z]?)\b", cell.get("dipende da", ""))
        rows.append(PlanRow(package, int(issue.group(1)) if issue else None, deps,
                            value("difficoltà"), value("agente"), value("modello"), value("impegno")))
    return rows


# --- Reading from GitHub ---------------------------------------------------------


class GitHub:
    """Reads through gh api, REST only. Tests replace get()."""

    def get(self, path: str):
        r = subprocess.run(["gh", "api", path], capture_output=True, text=True)
        if r.returncode != 0:
            raise Failure(f"GitHub refused: gh api {path}\n{(r.stderr or r.stdout).strip()}")
        return json.loads(r.stdout)

    def pages(self, path: str, key: str | None = None, stop: Callable[[list], bool] | None = None) -> list:
        """All the pages of a list, one at a time with &page=N (no
        --paginate). stop(page) ends the reading after that page."""
        out: list = []
        page = 1
        sep = "&" if "?" in path else "?"
        while True:
            d = self.get(f"{path}{sep}per_page=100&page={page}")
            items = d[key] if key else d
            if not items:
                return out
            out.extend(items)
            if len(items) < 100 or (stop and stop(items)):
                return out
            page += 1


def ts(s: str) -> datetime:
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


class Store:
    """The data folder."""

    def __init__(self, folder: Path):
        self.folder = folder

    def path(self, name: str) -> Path:
        return self.folder / name

    def has(self, name: str) -> bool:
        return self.path(name).exists()

    def load(self, name: str):
        try:
            return json.loads(self.path(name).read_text(encoding="utf-8"))
        except FileNotFoundError:
            raise Failure(f"missing in the data folder: {self.path(name)} (run with --scarica)") from None

    def save(self, name: str, value) -> None:
        self.folder.mkdir(parents=True, exist_ok=True)
        self.path(name).write_text(json.dumps(value, ensure_ascii=False, indent=1), encoding="utf-8")


def plan_file(root: Path, stage: str) -> Path | None:
    found = sorted((root / "docs" / "tappe").glob(f"{stage}-*.md"))
    return found[0] if found else None


def download(gh: GitHub, store: Store, stages: list[str], root: Path) -> None:
    for stage in stages:
        store.save(f"issues-{stage}.json", gh.pages(f"repos/{REPO}/issues?labels=tappa-{stage}&state=all"))
        pf = plan_file(root, stage)
        store.save(f"piano-{stage}.json", {"file": str(pf.relative_to(root)) if pf else None,
                                           "text": pf.read_text(encoding="utf-8") if pf else ""})
    issues = [x for s in stages for x in store.load(f"issues-{s}.json") if "pull_request" not in x]
    since = min(ts(x["created_at"]) for x in issues) - timedelta(days=2) if issues else None

    pulls = gh.pages(f"repos/{REPO}/pulls?state=all")
    store.save("pulls.json", [{
        "number": p["number"], "title": p["title"], "state": p["state"], "created_at": p["created_at"],
        "merged_at": p["merged_at"], "closed_at": p["closed_at"], "body": p["body"],
        "head": p["head"]["ref"], "head_sha": p["head"]["sha"], "labels": [lb["name"] for lb in p["labels"]],
    } for p in pulls])
    head_sha = {p["number"]: p["head"]["sha"] for p in pulls}

    def older(page: list) -> bool:
        return since is not None and ts(page[-1]["created_at"]) < since
    runs = gh.pages(f"repos/{REPO}/actions/workflows/ci.yml/runs?event=pull_request", "workflow_runs", older)
    store.save("runs.json", [{
        k: r[k] for k in ("id", "head_branch", "status", "conclusion", "created_at", "updated_at",
                          "run_started_at")} for r in runs])

    m = measure(store, stages, need_details=False)
    details = store.load("pull-details.json") if store.has("pull-details.json") else {}
    for n in sorted(m.used_pulls):
        # Saved details are reused only for the same head commit: a pull
        # request pushed again since the last download is read again.
        if str(n) in details and details[str(n)].get("head_sha") == head_sha.get(n):
            continue
        o = gh.get(f"repos/{REPO}/pulls/{n}")
        files = gh.pages(f"repos/{REPO}/pulls/{n}/files")
        details[str(n)] = {"head_sha": o["head"]["sha"], "commits": o["commits"], "additions": o["additions"],
                           "deletions": o["deletions"], "changed_files": o["changed_files"],
                           "files": [[f["filename"], f["additions"] + f["deletions"]] for f in files]}
    store.save("pull-details.json", details)

    comments = store.load("commenti.json") if store.has("commenti.json") else {}
    for x in issues:
        comments[str(x["number"])] = [[c["created_at"], c["body"][:400]]
                                      for c in gh.pages(f"repos/{REPO}/issues/{x['number']}/comments")]
    store.save("commenti.json", comments)

    cases: dict[str, dict[str, str]] = {}
    for row in measure(store, stages).rows:
        if row["kind"] != "riferimenti" or not row["main"]:
            continue
        for d in case_dirs(details[str(row["main"]["pr"])]["files"]):
            folder = root / d
            if folder.is_dir():
                cases[d] = {f.name: f.read_text(encoding="utf-8", errors="replace")
                            for f in sorted(folder.iterdir()) if f.is_file()}
    store.save("casi.json", cases)


# --- Measure --------------------------------------------------------------------


def case_dirs(files: Iterable) -> list[str]:
    """The case folders touched by a pull request: the parent folder of every
    file under tests/verifica/casi/."""
    out = []
    for f, _ in files:
        if f.startswith(CASES) and "/" in f[len(CASES):]:
            d = f.rsplit("/", 1)[0]
            if d not in out:
                out.append(d)
    return out


def is_code(name: str) -> bool:
    """A file that counts as code (definition in the header)."""
    base = name.rsplit("/", 1)[-1]
    return not (name.startswith(("third_party/", "tests/", "docs/", "resources/")) or name.endswith(".md")
                or "_test." in base)


def correction_kind(files: Iterable) -> str:
    """riferimenti (only tests/verifica/), documenti (only docs/ or .md) or
    codice."""
    names = [f for f, _ in files]
    if names and all(f.startswith("tests/verifica/") for f in names):
        return "riferimenti"
    if names and all(f.startswith("docs/") or f.endswith(".md") for f in names):
        return "documenti"
    return "codice"


FINE_NUMBER = re.compile(r"(\d\.\d{4,}|\de[-+]?\d)", re.I)
FINE_QUESTIONS = {"tangents", "fillet", "circle-ttr", "offset", "chamfer"}


def classify_cases(files: dict[str, str]) -> dict[str, int]:
    """Fine and ordinary cases of one case folder (definition in the header)."""
    c = {"domande": 0, "domande_fini": 0, "script": 0, "script_fini": 0}
    for name, txt in sorted(files.items()):
        if name.endswith(".tgq"):
            tolerance = False
            for line in txt.splitlines():
                s = line.strip()
                if s.startswith("!tolerance"):
                    tolerance = True
                if s.startswith("?"):
                    c["domande"] += 1
                    parts = s[1:].split()
                    q = parts[0] if parts else ""
                    ask, _, ans = s.partition("->")
                    if tolerance or q in FINE_QUESTIONS or FINE_NUMBER.search(ans) or FINE_NUMBER.search(ask):
                        c["domande_fini"] += 1
        elif name.endswith(".tgs"):
            c["script"] += 1
            if FINE_NUMBER.search(files.get("atteso.txt", "")) or "!tolerance" in txt:
                c["script_fini"] += 1
    return c


@dataclass
class Measure:
    rows: list[dict]
    used_pulls: set[int]
    runs: list[dict]
    plans: dict[str, list[PlanRow]]


def measure(store: Store, stages: list[str], need_details: bool = True) -> Measure:
    pulls = {p["number"]: p for p in store.load("pulls.json")}
    runs = store.load("runs.json")
    plans = {s: plan_table(store.load(f"piano-{s}.json")["text"]) for s in stages}
    issues = []
    for stage in stages:
        plan_by_issue = {r.issue: r for r in plans[stage] if r.issue}
        plan_by_code = {f"{stage}-{r.package}": r for r in plans[stage]}
        for x in store.load(f"issues-{stage}.json"):
            if "pull_request" in x:
                continue
            labels = [lb["name"] for lb in x["labels"]]
            cs = [c for c in codes(x["title"]) if code_stage(c) == stage]
            code = cs[0] if cs else f"{stage} (#{x['number']})"
            kind = "pacchetto" if "pacchetto" in labels else "riferimenti" if "riferimenti" in labels else "altro"
            diff = next((lb for lb in labels if lb in ("facile", "media", "impegnativa")), None)
            pr = plan_by_issue.get(x["number"]) or plan_by_code.get(code)
            issues.append({
                "issue": x["number"], "tappa": stage, "code": code, "title": x["title"], "kind": kind,
                "diff": diff, "state": x["state"], "created": x["created_at"],
                "agenti": [lb[7:] for lb in labels if lb.startswith("agente:")],
                "previsto": {"difficolta": pr.difficulty, "agente": pr.agent, "modello": pr.model,
                             "impegno": pr.effort, "dipende": pr.deps} if pr else None,
            })
    by_num = {i["issue"]: i for i in issues}
    by_code = {i["code"]: i for i in issues}

    closes: dict[int, list[int]] = {}
    for p in pulls.values():
        for n in re.findall(r"(?i)\b(?:closes|fixes|resolves)\s+#(\d+)", clean(p["body"])):
            closes.setdefault(int(n), []).append(p["number"])
    title_codes = {n: codes(p["title"]) for n, p in pulls.items()}
    body_codes = {n: codes(clean(p["body"])) for n, p in pulls.items()}

    manual = set(MANUAL_FIX) | set(MANUAL_CARD_FIX) | EXCLUDED
    main_of: dict[int, int] = {}
    for i in issues:
        cand = set(closes.get(i["issue"], [])) | {n for n, cs in title_codes.items() if i["code"] in cs}
        c = [n for n in cand if pulls[n]["merged_at"] and n not in manual
             and ts(pulls[n]["created_at"]) >= ts(i["created"])]
        if c:
            main_of[i["issue"]] = min(c, key=lambda n: pulls[n]["created_at"])
    mains = set(main_of.values())

    fixes: dict[int, list[int]] = {i["issue"]: [] for i in issues}
    attempts: dict[int, list[int]] = {i["issue"]: [] for i in issues}
    card_fixes: dict[int, list[int]] = {i["issue"]: [] for i in issues}
    for n, p in sorted(pulls.items()):
        if n in mains or n in EXCLUDED:
            continue
        if n in MANUAL_CARD_FIX:
            if MANUAL_CARD_FIX[n] in by_code:
                card_fixes[by_code[MANUAL_CARD_FIX[n]]["issue"]].append(n)
            continue
        if n in MANUAL_FIX:
            if MANUAL_FIX[n] in by_code:
                fixes[by_code[MANUAL_FIX[n]]["issue"]].append(n)
            continue
        targets = [by_code[c] for c in title_codes[n] if c in by_code]
        if not p["merged_at"]:
            targets += [by_num[k] for k in by_num if n in closes.get(k, [])]
            for t in {t["issue"]: t for t in targets}.values():
                attempts[t["issue"]].append(n)
            continue
        if not title_codes[n]:
            targets += [by_code[c] for c in body_codes[n] if c in by_code]
        targets += [by_num[k] for k in by_num if n in closes.get(k, [])]
        for t in {t["issue"]: t for t in targets}.values():
            main = main_of.get(t["issue"])
            if main and ts(p["merged_at"]) > ts(pulls[main]["merged_at"]):
                fixes[t["issue"]].append(n)

    used = set(mains) | {n for v in (fixes, attempts, card_fixes) for ns in v.values() for n in ns}
    if not need_details:
        return Measure(issues, used, runs, plans)

    details = store.load("pull-details.json")
    comments = {int(k): v for k, v in store.load("commenti.json").items()}
    cases = store.load("casi.json") if store.has("casi.json") else {}

    def ci(p: dict) -> dict:
        start, end = ts(p["created_at"]), ts(p["merged_at"] or p["closed_at"] or p["created_at"])
        rs = sorted((r for r in runs if r["head_branch"] == p["head"]
                     and start.timestamp() - 120 <= ts(r["created_at"]).timestamp() <= end.timestamp() + 60),
                    key=lambda r: r["created_at"])
        return rounds([r["conclusion"] for r in rs]) | {"run_ids": [r["id"] for r in rs]}

    def prrow(n: int) -> dict:
        p = pulls[n]
        d = details.get(str(n))
        if d is None:
            raise Failure(f"missing details of pull request #{n} (run with --scarica)")
        end = p["merged_at"] or p["closed_at"]
        own = [(f, ln) for f, ln in d["files"] if not f.startswith("third_party/")]
        return {"pr": n, "head": p["head"], "created": p["created_at"], "merged": p["merged_at"],
                "hours": round((ts(end) - ts(p["created_at"])).total_seconds() / 3600, 2) if end else None,
                "commits": d["commits"], "files_own": len(own), "lines_own": sum(ln for _, ln in own),
                "files_code": sum(1 for f, _ in own if is_code(f)),
                "lines_code": sum(ln for f, ln in own if is_code(f)),
                "touches_app": any(f.startswith(("app/", "tests/app/")) for f, _ in own),
                "kind": correction_kind(d["files"]), "declared": declared(p["body"]).__dict__,
                "author_hint": author_hint(p["body"]),
                **ci(p)}

    rows = []
    for i in sorted(issues, key=lambda i: i["issue"]):
        r = dict(i)
        r["main"] = prrow(main_of[i["issue"]]) if i["issue"] in main_of else None
        fx = [prrow(n) for n in sorted(fixes[i["issue"]])]
        r["fixes_ref"] = [f for f in fx if f["kind"] == "riferimenti"]
        r["fixes_code"] = [f for f in fx if f["kind"] == "codice"]
        r["fixes_doc"] = [f for f in fx if f["kind"] == "documenti"] + [prrow(n) for n in card_fixes[i["issue"]]]
        r["attempts"] = [prrow(n) for n in sorted(attempts[i["issue"]])]
        claims = sorted(c[0] for c in comments.get(i["issue"], []) if re.search(r"(?i)presa in carico", c[1]))
        r["claims"] = claims
        if r["main"] and r["main"]["merged"]:
            m = r["main"]
            r["lead"] = round((ts(m["merged"]) - ts(claims[0])).total_seconds() / 3600, 2) if claims else None
            before = [c for c in claims if ts(c) <= ts(m["created"])]
            r["stopped"] = round((ts(before[-1]) - ts(claims[0])).total_seconds() / 3600, 2) if before else None
            r["last_work"] = round((ts(m["merged"]) - ts(before[-1])).total_seconds() / 3600, 2) if before else None
        else:
            r["lead"] = r["stopped"] = r["last_work"] = None
        r["autore"] = author(r)
        r["dichiarato"] = declaration(r)
        if r["kind"] == "riferimenti" and r["main"]:
            per_dir = {d: classify_cases(cases[d]) for d in case_dirs(details[str(r["main"]["pr"])]["files"])
                       if d in cases}
            r["casi"] = {k: sum(c[k] for c in per_dir.values())
                         for k in ("domande", "domande_fini", "script", "script_fini")}
        rows.append(r)
    return Measure(rows, used, runs, plans)


def rounds(conclusions: list[str | None]) -> dict:
    """Runs, failures, cancellations, successes and correction rounds (failed
    runs before the last green run) of a pull request, in time order."""
    last_ok = max((k for k, c in enumerate(conclusions) if c == "success"), default=-1)
    return {"runs": len(conclusions), "failure": conclusions.count("failure"),
            "cancelled": conclusions.count("cancelled"), "success": conclusions.count("success"),
            "fail_before_green": sum(1 for c in conclusions[:max(last_ok, 0)] if c == "failure")}


def author(row: dict) -> str:
    if row["code"] in AUTHOR_OVERRIDE:
        return AUTHOR_OVERRIDE[row["code"]]
    if not row["main"]:
        return "?"
    tool = row["main"]["declared"]["tool"]
    return tool if tool != "?" else row["main"].get("author_hint", "?")


def declaration(row: dict) -> dict:
    """Model and effort declared for the main pull request: the pull request
    first; for the effort, the maintainer's declaration where the pull request
    has none (marked "responsabile"); a model of the pull request that differs
    from that declaration is flagged."""
    out = {"modello": NOT_DECLARED, "impegno": NOT_DECLARED, "fonte_impegno": NOT_DECLARED, "differenza": None}
    if not row["main"]:
        return out
    d = row["main"]["declared"]
    out["modello"] = d["model"]
    if d["effort"] not in (NOT_DECLARED, "non noto"):
        out["impegno"], out["fonte_impegno"] = d["effort"], "pull request"
    # Keyed on the tool declared in the pull request, never on a guessed author.
    by_maint = DECLARED_BY_MAINTAINER.get((row["tappa"], row["diff"], d["tool"]))
    if by_maint:
        model, effort = by_maint
        if out["impegno"] == NOT_DECLARED:
            out["impegno"], out["fonte_impegno"] = effort, "responsabile"
        if out["modello"] == NOT_DECLARED:
            out["modello"] = f"{model} (responsabile)"
        else:
            fam = model_family(out["modello"].split(" + ")[0])
            if fam != model_family(model):
                out["differenza"] = f"pull request: {out['modello']}; responsabile: {model}"
            elif fam == "codex":
                low = out["modello"].lower()
                if ("astra" in low and "astra" not in model.lower()) or ("sol" in low and "sol" not in model.lower()):
                    out["differenza"] = f"pull request: {out['modello']}; responsabile: {model}"
    elif d["effort"] == "non noto":
        out["impegno"] = "non noto"
    return out


# --- Tables ----------------------------------------------------------------------


def med(xs: list) -> float | None:
    xs = [x for x in xs if x is not None]
    return statistics.median(xs) if xs else None


def fmt(x, d: int = 1) -> str:
    if x is None:
        return "—"
    if isinstance(x, float) and x != int(x):
        return f"{x:.{d}f}".replace(".", ",")
    return str(int(x))


ORDER = [("pacchetto", "facile"), ("pacchetto", "media"), ("pacchetto", "impegnativa"),
         ("riferimenti", "facile"), ("riferimenti", "media"), ("riferimenti", "impegnativa"),
         ("riferimenti", "n.d.")]


def merged(rows: list[dict]) -> list[dict]:
    return [r for r in rows if r["main"] and r["main"]["merged"] and r["kind"] != "altro"]


def stage_table(rows: list[dict]) -> str:
    """Table of section 2.1 of the analysis, with the corrections split."""
    out = ["| Tipo | Difficoltà | PR | Commit (med.) | Run CI (med.; tot.) | Run fallite (tot.) | "
           "Run annullate (tot.) | PR con ≥1 giro | Ore aperta→unita (med.) | Ore presa→unita (med.) | "
           "File (med.) | Righe (med.) | Righe di codice (med.) | Correzioni dei riferimenti (tot.) | "
           "Correzioni del codice (tot.) |",
           "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    g: dict[tuple[str, str], list[dict]] = {}
    for r in merged(rows):
        g.setdefault((r["kind"], r["diff"] or "n.d."), []).append(r)
    for k in ORDER:
        if k not in g:
            continue
        rs = g[k]
        ms = [r["main"] for r in rs]
        out.append("| " + " | ".join([
            k[0], k[1], str(len(rs)), fmt(med([m["commits"] for m in ms])),
            f"{fmt(med([m['runs'] for m in ms]))}; {sum(m['runs'] for m in ms)}",
            str(sum(m["failure"] for m in ms)), str(sum(m["cancelled"] for m in ms)),
            str(sum(1 for m in ms if m["fail_before_green"] > 0)),
            fmt(med([m["hours"] for m in ms])), fmt(med([r["lead"] for r in rs])),
            fmt(med([m["files_own"] for m in ms])), fmt(med([m["lines_own"] for m in ms]), 0),
            fmt(med([m["lines_code"] for m in ms]), 0), str(sum(len(r["fixes_ref"]) for r in rs)),
            str(sum(len(r["fixes_code"]) for r in rs)),
        ]) + " |")
    return "\n".join(out)


def rank(xs: list[float]) -> list[float]:
    """Ranks with ties averaged, from 1."""
    order = sorted(range(len(xs)), key=lambda i: xs[i])
    r = [0.0] * len(xs)
    i = 0
    while i < len(order):
        j = i
        while j + 1 < len(order) and xs[order[j + 1]] == xs[order[i]]:
            j += 1
        for k in range(i, j + 1):
            r[order[k]] = (i + j) / 2 + 1
        i = j + 1
    return r


def spearman(a: list[float], b: list[float]) -> float | None:
    """Spearman's rank correlation; None when a side has no variation."""
    if len(a) < 2:
        return None
    ra, rb = rank(a), rank(b)
    ma, mb = statistics.mean(ra), statistics.mean(rb)
    den = (sum((x - ma) ** 2 for x in ra) * sum((y - mb) ** 2 for y in rb)) ** 0.5
    return sum((x - ma) * (y - mb) for x, y in zip(ra, rb)) / den if den else None


def author_table(rows: list[dict]) -> str:
    """Table of section 2.2: work packages by difficulty and author."""
    out = ["| Pacchetti di lavoro | PR | Run fallite | PR con ≥1 giro | Quota con ≥1 giro | Run (med.) | "
           "Righe (med.) | Run fallite per 1000 righe |", "|---|---|---|---|---|---|---|---|"]
    w = [r for r in merged(rows) if r["kind"] == "pacchetto"]
    for d in ("facile", "media", "impegnativa"):
        for a in sorted({r["autore"] for r in w}):
            rs = [r for r in w if r["diff"] == d and r["autore"] == a]
            if not rs:
                continue
            ms = [r["main"] for r in rs]
            g = sum(1 for m in ms if m["fail_before_green"] > 0)
            lines = sum(m["lines_own"] for m in ms)
            out.append(f"| {d}, {a} | {len(rs)} | {sum(m['failure'] for m in ms)} | {g} | "
                       f"{round(100 * g / len(rs))} % | {fmt(med([m['runs'] for m in ms]))} | "
                       f"{fmt(med([m['lines_own'] for m in ms]), 0)} | "
                       f"{fmt(1000 * sum(m['failure'] for m in ms) / lines if lines else None, 2)} |")
    out.append("")
    for flag, label in ((True, "toccano"), (False, "non toccano")):
        sel = [r for r in w if r["main"]["touches_app"] == flag]
        g = [r["code"] for r in sel if r["main"]["fail_before_green"] > 0]
        out.append(f"Pacchetti di lavoro che {label} `app/` o `tests/app/` (Qt): {len(sel)}, con almeno un giro "
                   f"{len(g)}{' (' + ', '.join(g) + ')' if g else ''}.")
    for label, sel in (("tutti", w), ("medi", [r for r in w if r["diff"] == "media"])):
        g = [r["main"]["fail_before_green"] for r in sel]
        rho = spearman([r["main"]["lines_own"] for r in sel], g)
        rho_code = spearman([r["main"]["lines_code"] for r in sel], g)
        out.append(f"Spearman tra righe e run fallite prima del verde, pacchetti di lavoro {label} ({len(sel)}): "
                   f"{fmt(round(rho, 2) if rho is not None else None, 2)}; con le righe di codice: "
                   f"{fmt(round(rho_code, 2) if rho_code is not None else None, 2)}.")
    return "\n".join(out)


def class_table(rows: list[dict]) -> str:
    """Work packages by planned strength class and effort, and by declared
    model and effort."""
    w = [r for r in merged(rows) if r["kind"] == "pacchetto"]

    def block(title: str, key: Callable[[dict], tuple]) -> list[str]:
        g: dict[tuple, list[dict]] = {}
        for r in w:
            g.setdefault(key(r), []).append(r)
        out = [title, "", "| " + " | ".join(KEYS[title]) + " | PR | PR con ≥1 giro | Run fallite (tot.) | "
               "Righe (med.) | Correzioni del codice (tot.) |",
               "|" + "---|" * (len(KEYS[title]) + 5)]
        for k in sorted(g):
            rs = g[k]
            ms = [r["main"] for r in rs]
            out.append("| " + " | ".join(k) + f" | {len(rs)} | {sum(1 for m in ms if m['fail_before_green'] > 0)} | "
                       f"{sum(m['failure'] for m in ms)} | {fmt(med([m['lines_own'] for m in ms]), 0)} | "
                       f"{sum(len(r['fixes_code']) for r in rs)} |")
        return out

    prev = "Previsti dal piano"
    decl = "Dichiarati"
    out = block(prev, lambda r: (r["diff"] or "n.d.",
                                 (r["previsto"] or {}).get("modello", NOT_DECLARED),
                                 (r["previsto"] or {}).get("impegno", NOT_DECLARED)))
    out += [""] + block(decl, lambda r: (r["autore"], r["dichiarato"]["modello"], r["dichiarato"]["impegno"],
                                         r["dichiarato"]["fonte_impegno"]))
    return "\n".join(out)


KEYS = {"Previsti dal piano": ["Difficoltà", "Fascia prevista", "Impegno previsto"],
        "Dichiarati": ["Autore", "Modello dichiarato", "Impegno", "Fonte dell'impegno"]}


def refs_table(rows: list[dict]) -> str:
    """Table of section 2.5: reference corrections by who wrote the references."""
    out = ["| Riferimenti | Chi li ha scritti (modello dichiarato) | Domande (fini) | Script (fini) | "
           "Correzioni dei riferimenti | Righe |", "|---|---|---|---|---|---|"]
    for r in merged(rows):
        if r["kind"] != "riferimenti":
            continue
        c = r.get("casi") or {}
        out.append(f"| {r['code']} | {r['autore']} ({r['dichiarato']['modello']}) | "
                   f"{c.get('domande', '—')} ({c.get('domande_fini', '—')}) | "
                   f"{c.get('script', '—')} ({c.get('script_fini', '—')}) | "
                   f"{', '.join('#' + str(f['pr']) for f in r['fixes_ref']) or '—'} | {r['main']['lines_own']} |")
    return "\n".join(out)


def times_table(rows: list[dict], plans: dict[str, list[PlanRow]]) -> str:
    """Table of section 2.4 and the realised critical path of each stage,
    from the "Dipende da" column of its plan. A stage opens at the creation
    of its first issue."""
    out = []
    for stage, plan in plans.items():
        rs = {r["code"]: r for r in rows if r["tappa"] == stage}
        if not rs:
            continue
        open_t = min(ts(r["created"]) for r in rs.values())

        def end(c: str) -> datetime | None:
            r = rs.get(f"{stage}-{c}")
            return ts(r["main"]["merged"]) if r and r["main"] and r["main"]["merged"] else None

        out += [f"Tappa {stage} (orari UTC)", "",
                "| Pacchetto | Difficoltà | Ultima dipendenza unita | Prima presa | Ultima presa prima della PR | "
                "Unito | Fermo (h) | Ultimo lavoro (h) | Attesa da pronto a presa (h) |",
                "|---|---|---|---|---|---|---|---|---|"]
        last_dep: dict[str, str | None] = {}
        waits: list[float] = []
        for p in plan:
            c = f"{stage}-{p.package}"
            r = rs.get(c)
            if not r or not end(p.package):
                continue
            deps = [d for d in p.deps if end(d)]
            last = max(deps, key=lambda d: end(d)) if deps else None
            last_dep[p.package] = last
            if code_letter(c) != "P":
                continue
            ready = max(end(last), open_t) if last else open_t
            first = ts(r["claims"][0]) if r["claims"] else None
            before = [ts(x) for x in r["claims"] if ts(x) <= ts(r["main"]["created"])]
            if first:
                waits.append(round((first - ready).total_seconds() / 3600, 1))
            out.append(f"| {p.package} | {r['diff'] or 'n.d.'} | "
                       f"{f'{last} {end(last):%d/%m %H:%M}' if last else '—'} | "
                       f"{f'{first:%d/%m %H:%M}' if first else '—'} | "
                       f"{f'{before[-1]:%d/%m %H:%M}' if before else '—'} | {end(p.package):%d/%m %H:%M} | "
                       f"{fmt(r['stopped'])} | {fmt(r['last_work'])} | "
                       f"{fmt(round((first - ready).total_seconds() / 3600, 1)) if first else '—'} |")
        work = [p.package for p in plan if code_letter(p.package) == "P" and end(p.package)]
        if not work:
            continue
        path = [max(work, key=lambda c: end(c))]
        while last_dep.get(path[-1]):
            path.append(last_dep[path[-1]])
        start = end(path[-1]) if len(path) > 1 else open_t
        node_work = sum(rs[f"{stage}-{c}"]["last_work"] or 0 for c in path[:-1])
        out += ["", f"Percorso critico realizzato: {' → '.join(reversed(path))}; dall'unione di {path[-1]} "
                f"all'ultima unione {fmt(round((end(path[0]) - start).total_seconds() / 3600, 1))} ore, "
                f"ultimo lavoro dei nodi dopo il primo {fmt(round(node_work, 1))} ore; fermi di tutti i "
                f"pacchetti {fmt(round(sum(r['stopped'] or 0 for r in rs.values()), 1))} ore; attesa da pronto "
                f"a presa, mediana {fmt(med(waits))} ore su {len(waits)} pacchetti di lavoro; dalla prima issue "
                f"all'ultima unione {fmt(round((end(path[0]) - open_t).total_seconds() / 3600, 1))} ore.", ""]
    return "\n".join(out)


def fixed_costs(rows: list[dict], runs: list[dict]) -> str:
    """Section 2.3: green run time, life of a pull request, runs and pull
    requests per stage."""
    heads = {p["head"] for r in rows for p in ([r["main"]] if r["main"] else []) + r["fixes_ref"]
             + r["fixes_code"] + r["fixes_doc"] + r["attempts"]}
    green = [(ts(x["updated_at"]) - ts(x["run_started_at"])).total_seconds() / 60 for x in runs
             if x["conclusion"] == "success" and x["head_branch"] in heads and x["run_started_at"]]
    run_by_id = {x["id"]: x for x in runs}
    to_green, to_merge = [], []
    for r in merged(rows):
        m = r["main"]
        ok = [ts(run_by_id[i]["updated_at"]) for i in m["run_ids"]
              if run_by_id[i]["conclusion"] == "success" and ts(run_by_id[i]["updated_at"]) <= ts(m["merged"])]
        if ok:
            g = max(ok)
            to_green.append((g - ts(m["created"])).total_seconds() / 3600)
            to_merge.append((ts(m["merged"]) - g).total_seconds() / 3600)
    out = [f"- Run verde, durata mediana: {fmt(med(green))} minuti ({len(green)} run verdi delle pull request "
           "principali, di correzione e dei tentativi).",
           f"- Vita della pull request principale: apertura→ultimo verde {fmt(med(to_green), 2)} ore, "
           f"verde→unione {fmt(med(to_merge), 2)} ore (mediane su {len(to_green)}).",
           f"- Pull request principali unite: {len(merged(rows))}; correzioni dei riferimenti "
           f"{sum(len(r['fixes_ref']) for r in rows)}, del codice {sum(len(r['fixes_code']) for r in rows)}, "
           f"di documenti {sum(len(r['fixes_doc']) for r in rows)}; tentativi non uniti "
           f"{sum(len(r['attempts']) for r in rows)}.",
           f"- Run delle pull request principali: {sum(r['main']['runs'] for r in merged(rows))}, di cui "
           f"{sum(r['main']['failure'] for r in merged(rows))} fallite e "
           f"{sum(r['main']['cancelled'] for r in merged(rows))} annullate."]
    return "\n".join(out)


def size_summary(rows: list[dict]) -> str:
    """Work packages against the measure of condition 2 of the analysis: at
    most 15 files and about 600 code lines, at least 150 code lines."""
    w = [r["main"] for r in merged(rows) if r["kind"] == "pacchetto"]
    return (f"Pacchetti di lavoro uniti: {len(w)}; con più di 15 file (test e documenti compresi): "
            f"{sum(1 for m in w if m['files_own'] > 15)}; con più di 600 righe di codice: "
            f"{sum(1 for m in w if m['lines_code'] > 600)}; con meno di 150 righe di codice: "
            f"{sum(1 for m in w if m['lines_code'] < 150)}; righe di codice, mediana "
            f"{fmt(med([m['lines_code'] for m in w]), 0)}.")


def raw_table(rows: list[dict]) -> str:
    """Section 7.1: one row per issue."""
    out = ["| Codice | Issue | Tipo | Diff. | Previsti (agente, fascia, impegno) | Autore | Modello dichiarato | "
           "Impegno (fonte) | PR | Commit | File | Righe | Righe di codice | Run | Fallite | Annullate | Giri | "
           "Ore PR | "
           "Ore presa→unione | Correzioni riferimenti | Correzioni codice | Altre PR |",
           "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        m = r["main"]
        p = r["previsto"] or {}
        d = r["dichiarato"]
        prev = f"{p.get('agente', NOT_DECLARED)}, {p.get('modello', NOT_DECLARED)}, {p.get('impegno', NOT_DECLARED)}" \
            if p else NOT_DECLARED
        eff = d["impegno"] if d["fonte_impegno"] == NOT_DECLARED else f"{d['impegno']} ({d['fonte_impegno']})"
        model = d["modello"] + (" ⚠" if d["differenza"] else "")
        others = [f"#{f['pr']} (documenti)" for f in r["fixes_doc"]] + \
            [f"#{f['pr']} (non unita)" for f in r["attempts"]]

        def prs(fs: list[dict]) -> str:
            return ", ".join(f"#{f['pr']}" for f in fs) or "—"
        if m:
            nums = [f"#{m['pr']}", str(m["commits"]), str(m["files_own"]), str(m["lines_own"]), str(m["lines_code"]),
                    str(m["runs"]),
                    str(m["failure"]), str(m["cancelled"]), str(m["fail_before_green"]), fmt(m["hours"]),
                    fmt(r["lead"])]
        else:
            nums = ["—"] * 11
        out.append("| " + " | ".join([r["code"], f"#{r['issue']}", r["kind"], r["diff"] or "n.d.", prev,
                                      r["autore"], model, eff, *nums, prs(r["fixes_ref"]), prs(r["fixes_code"]),
                                      ", ".join(others) or "—"]) + " |")
    differences = [f"- {r['code']}: {r['dichiarato']['differenza']}" for r in rows if r["dichiarato"]["differenza"]]
    if differences:
        out += ["", "Modello della pull request diverso dalla dichiarazione del responsabile (⚠):", *differences]
    return "\n".join(out)


def report(m: Measure, stages: list[str]) -> str:
    rows = m.rows
    parts = []
    for stage in stages:
        parts += [f"## Tappa {stage}", "", stage_table([r for r in rows if r["tappa"] == stage]), ""]
    if len(stages) > 1:
        parts += [f"## Insieme {', '.join(stages)}", "", stage_table(rows), ""]
    parts += ["## Per difficoltà e autore, pacchetti di lavoro", "", author_table(rows), "",
              "## Per fascia e impegno", "", class_table(rows), "",
              "## Misura", "", size_summary(rows), "",
              "## Costi fissi", "", fixed_costs(rows, m.runs), "",
              "## Tempi", "", times_table(rows, m.plans),
              "## Riferimenti", "", refs_table(rows), "",
              "## Per pacchetto", "", raw_table(rows), ""]
    unmatched = [f"{r['code']} ({'aperta' if r['state'] == 'open' else 'chiusa'})" for r in rows if not r["main"]]
    if unmatched:
        parts += [f"Issue senza pull request principale unita: {', '.join(unmatched)}.", ""]
    return "\n".join(parts)


def main(argv: list[str] | None = None, gh: GitHub | None = None, root: Path | None = None) -> int:
    parser = argparse.ArgumentParser(description="Measures the packages of one or more stages.")
    parser.add_argument("--tappe", nargs="+", required=True, help="stages, e.g. 0.7.5 or 0.7 0.7.5")
    parser.add_argument("--dati", required=True, type=Path, help="data folder")
    parser.add_argument("--scarica", action="store_true", help="read from GitHub and save in the data folder")
    parser.add_argument("--radice", type=Path, default=root or Path(__file__).resolve().parent.parent,
                        help="checkout of Tecnigrafo, for the plans and the cases (default: this one)")
    a = parser.parse_args(argv)
    for s in a.tappe:
        if not re.fullmatch(r"\d+\.\d+(?:\.\d+)?", s):
            parser.error(f"not a stage: {s}")
    store = Store(a.dati)
    try:
        if a.scarica:
            download(gh or GitHub(), store, a.tappe, a.radice)
        m = measure(store, a.tappe)
    except Failure as e:
        print(f"misura_pacchetti: {e}", file=sys.stderr)
        return 2
    store.save("misura.json", m.rows)
    print(report(m, a.tappe))
    return 0


if __name__ == "__main__":
    sys.exit(main())
