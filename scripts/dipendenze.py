#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Checks the dependencies of the issues of a stage ("tappa") of Tecnigrafo
# against the plan, also across solid-space, and lines them up
# (docs/tappe/fuori-tappa/F4-dipendenze.md; docs/GUIDA-RESPONSABILE.md, B6
# and B11).
#
#     scripts/dipendenze.py <tappa> [--applica] [--senza-project]
#
# The plan is the only source: the table of section 6 of
# docs/tappe/<tappa>-*.md, read with the rules of scripts/apri-tappa.sh, and
# the table "Richieste a solid-space" of section 5. For every open issue of
# the table it says whether the issue is ready (every issue of its
# "Dipende da" is closed) or which open issues block it, in both
# repositories, and it compares the plan with:
#
#   2. the "**Dipende da:**" line of the issue text (notes in brackets do not
#      count);
#   3. the native "blocked by" relations of the issue;
#   4. the Status in the Project "Tecnigrafo": Bloccato for a ready issue, or
#      Da fare or empty for a blocked one, is wrong; other values are left
#      alone;
#   5. a dependency that is a request of solid-space (label "richiesta") while
#      the stage of solid-space for this stage is open: it proposes the new
#      "Dipende da" cell, with the packages of the row of the issue in the
#      table "Che cosa sblocca in Tecnigrafo" of the plan of solid-space,
#      minus those already covered by the dependencies;
#   6. a request whose issues in the column "Si chiude con" are all closed: its
#      success condition is to be checked and the request closed, by hand;
#   7. an agente:... label that differs from the plan: information only, since
#      the label decides.
#
# --applica       after showing the changes for checks 2-4 and asking
#                 "Go on? [s/N]", rewrite the "Dipende da" lines, add and remove
#                 "blocked by" relations and set the Status in the Project.
#                 It never writes the plan (copy the proposals of check 5 into
#                 it with a pull request), never closes an issue and never
#                 writes to solid-space.
# --senza-project skip the Project (check 4).
#
# Exit status: 0 nothing to fix or to check (checks 2-6), 1 something to fix
# or to check, 2 a read failed, a table cannot be read, or a change of
# --applica failed (the others are still made), 3 not confirmed.
#
# Needs Python 3.11 and gh, logged in with read access to Tecnigrafo (write
# access for --applica). The Project needs the "project" scope, once:
# gh auth refresh -s project. solid-space is public: when gh cannot read it
# (for example in an agent session without it), reads fall back to curl
# without a token, then its issues to the search API and its files to
# TG_SS_DIR, if set. Run it from the Tecnigrafo repository: it reads the plan
# of the working tree.
#
# TG_REPO            repository of the stage (default Matleyx78/tecnigrafo)
# TG_SS_REPO         repository of solid-space (default Matleyx78/solid-space)
# TG_PROJECT_OWNER   user that owns the Project (default Matleyx78)
# TG_PROJECT_TITLE   title of the Project (default Tecnigrafo)
# TG_PROJECT_NUMBER  number of the Project, instead of looking it up by title
# TG_SS_DIR          a checkout of main of solid-space, for its files when
#                    GitHub refuses them

from __future__ import annotations

import argparse
import base64
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, Iterable

STATUS_FIELD = "Status"
BLOCKED = "Bloccato"
TODO = "Da fare"
IN_CHARGE = "AA-Presa in carico"


class Failure(Exception):
    """A read failed or a table cannot be read: exit status 2."""


# --- References ----------------------------------------------------------------


@dataclass(frozen=True, eq=False)
class Ref:
    """An issue of a repository; the repository is kept as written, compared
    without case."""

    repo: str
    number: int

    @property
    def key(self) -> tuple[str, int]:
        return (self.repo.lower(), self.number)

    def __eq__(self, other: object) -> bool:
        return isinstance(other, Ref) and self.key == other.key

    def __hash__(self) -> int:
        return hash(self.key)

    def text(self, home: str) -> str:
        if self.repo.lower() == home.lower():
            return f"#{self.number}"
        return f"{self.repo}#{self.number}"


REF_RE = re.compile(r"(?:\b([A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+))?#(\d+)\b")


def refs_in(text: str, home: str) -> list[Ref]:
    """The issue references in a text, in order and without repetitions;
    anything in brackets is a note and is ignored."""
    text = re.sub(r"\([^)]*\)", "", text)
    out: list[Ref] = []
    for m in REF_RE.finditer(text):
        ref = Ref(m.group(1) or home, int(m.group(2)))
        if ref not in out:
            out.append(ref)
    return out


# --- Markdown tables -------------------------------------------------------------


def cells(line: str) -> list[str]:
    """The cells of a table row, split on every "|" as awk -F'|' does."""
    parts = line.split("|")
    return [p.strip() for p in parts[1:-1]]


SEPARATOR_RE = re.compile(r"^\|[-: |]+\|$")


@dataclass
class Table:
    header: list[str]
    rows: list[tuple[int, dict[str, str]]]  # (line number, cell by column)


def table_at(lines: list[str], start: int, path: str, first: str) -> Table | None:
    """The first table whose header starts with column `first`, looking from
    line index `start` to the next heading of the same or a higher level.
    None if there is none."""
    for i in range(start, len(lines)):
        line = lines[i]
        if i > start and re.match(r"^#{1,6} ", line) and _level(line) <= _level(lines[start]):
            return None
        if line.startswith("|") and cells(line)[:1] == [first]:
            header = cells(line)
            if i + 1 >= len(lines) or not SEPARATOR_RE.match(lines[i + 1].strip()):
                raise Failure(f"{path}:{i + 1}: the table has no separator line")
            rows = []
            for j in range(i + 2, len(lines)):
                if not lines[j].startswith("|"):
                    break
                row = cells(lines[j])
                rows.append((j + 1, {h: (row[k] if k < len(row) else "") for k, h in enumerate(header)}))
            return Table(header, rows)
    return None


def _level(line: str) -> int:
    m = re.match(r"^(#+) ", line)
    return len(m.group(1)) if m else 7


def heading_index(lines: list[str], pattern: str) -> int | None:
    for i, line in enumerate(lines):
        if re.match(r"^#{1,6} ", line) and re.search(pattern, line):
            return i
    return None


# --- The plan of Tecnigrafo ------------------------------------------------------

PLAN_COLUMNS = ["Pacchetto", "Scheda", "Issue", "Dipende da", "Riferimenti", "Difficoltà",
                "Agente", "Modello", "Impegno"]
TG_ID_RE = re.compile(r"^([PR][0-9]+)\s+(.+)$")
DEP_RE = re.compile(r"^(?:[PR][0-9]+|#[0-9]+|[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+#[0-9]+)$")


@dataclass
class Row:
    id: str
    line: int
    issue: int | None
    deps: list[str]
    agent: str


@dataclass
class Request:
    ref: Ref
    line: int
    closes_with: list[str] | None  # None: the plan has no "Si chiude con" column


@dataclass
class Plan:
    path: str
    rows: list[Row]
    requests: list[Request]
    notes: list[str] = field(default_factory=list)

    def row(self, id_: str) -> Row | None:
        return next((r for r in self.rows if r.id == id_), None)


def split_deps(cell: str) -> list[str]:
    if cell in ("", "—", "-"):
        return []
    return [d.strip() for d in cell.split(",") if d.strip()]


def parse_plan(text: str, path: str, home: str) -> Plan:
    """Reads the package table of section 6 and the table of the requests of
    section 5, with the rules of apri-tappa.sh."""
    lines = text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if line.startswith("## 6."):
            start = i
            break
    table = table_at(lines, start, path, "Pacchetto") if start is not None else None
    if table is None:
        raise Failure(f"{path}: no package table in section 6 (header \"| "
                      + " | ".join(PLAN_COLUMNS) + " |\")")
    missing = [c for c in PLAN_COLUMNS if c not in table.header]
    if missing:
        raise Failure(f"{path}: the package table has no column {', '.join(missing)}")
    if not table.rows:
        raise Failure(f"{path}: the package table has no rows")
    rows: list[Row] = []
    problems: list[str] = []
    seen: set[str] = set()
    for line, c in table.rows:
        m = TG_ID_RE.match(c["Pacchetto"])
        if not m:
            problems.append(f"line {line}: the first cell must start with PN or RN and a title: "
                            f"\"{c['Pacchetto']}\"")
            continue
        id_ = m.group(1)
        if id_ in seen:
            problems.append(f"line {line}: {id_} appears twice")
        im = re.match(r"^#([0-9]+)$", c["Issue"])
        deps = split_deps(c["Dipende da"])
        for dep in deps:
            if re.match(r"^[PR][0-9]+$", dep):
                if dep not in seen:
                    problems.append(f"line {line}: {id_} depends on {dep}, which is not earlier in the table")
            elif not DEP_RE.match(dep):
                problems.append(f"line {line}: {id_}: dependency \"{dep}\" is not PN, RN, #N or owner/repo#N")
        if c["Difficoltà"] not in ("facile", "media", "impegnativa"):
            problems.append(f"line {line}: {id_}: difficulty \"{c['Difficoltà']}\" (facile, media, impegnativa)")
        if c["Modello"] not in ("leggero", "medio", "forte"):
            problems.append(f"line {line}: {id_}: model \"{c['Modello']}\" (leggero, medio, forte)")
        if c["Impegno"] not in ("basso", "medio", "alto"):
            problems.append(f"line {line}: {id_}: effort \"{c['Impegno']}\" (basso, medio, alto)")
        if not re.match(r"^[a-z0-9-]+$", c["Agente"]):
            problems.append(f"line {line}: {id_}: agent \"{c['Agente']}\"")
        seen.add(id_)
        rows.append(Row(id_, line, int(im.group(1)) if im else None, deps, c["Agente"]))
    if problems:
        raise Failure(f"{path}: " + f"\n{path}: ".join(problems))

    plan = Plan(path, rows, [])
    at = heading_index(lines, r"^### +Richieste a solid-space")
    req = table_at(lines, at, path, "Richiesta") if at is not None else None
    if req is not None:
        has_closing = "Si chiude con" in req.header
        if not has_closing:
            plan.notes.append(f"{path}: the table \"Richieste a solid-space\" has no column "
                              "\"Si chiude con\": check 6 skipped")
        for line, c in req.rows:
            found = refs_in(c["Richiesta"], home)
            if not found:
                raise Failure(f"{path}:{line}: no issue in the cell \"Richiesta\"")
            closes = None
            if has_closing:
                closes = re.findall(r"\b[PR][0-9]+\b", c["Si chiude con"])
                for id_ in closes:
                    if plan.row(id_) is None:
                        raise Failure(f"{path}:{line}: \"Si chiude con\" names {id_}, not in the package table")
            plan.requests.append(Request(found[0], line, closes))
    return plan


def dep_ref(dep: str, plan: Plan, home: str) -> Ref | None:
    """The issue of a dependency of the plan; None for a row without issue."""
    if re.match(r"^[PR][0-9]+$", dep):
        row = plan.row(dep)
        return Ref(home, row.issue) if row and row.issue else None
    if dep.startswith("#"):
        return Ref(home, int(dep[1:]))
    repo, number = dep.rsplit("#", 1)
    return Ref(repo, int(number))


# --- The plan of solid-space ----------------------------------------------------

SS_ID = r"[PR][0-9]+[a-z]?"


@dataclass
class SsStage:
    version: str
    open: bool
    plan_path: str | None
    milestone: int | None


@dataclass
class SsPlan:
    order: list[str]                     # package ids in table order
    issue: dict[str, int]                # id -> issue number
    deps: dict[str, list[str]]           # id -> ids of its "Dipende da"
    unblocks: dict[int, list[str]]       # Tecnigrafo issue -> ids that it needs

    def id_of(self, number: int) -> str | None:
        return next((i for i, n in self.issue.items() if n == number), None)

    def closure(self, ids: Iterable[str]) -> set[str]:
        """The ids and everything they depend on."""
        out: set[str] = set()
        todo = list(ids)
        while todo:
            id_ = todo.pop()
            if id_ not in out:
                out.add(id_)
                todo.extend(self.deps.get(id_, []))
        return out


def parse_ss_roadmap(text: str, tappa: str) -> SsStage | None:
    """The stage of solid-space for the stage `tappa` of Tecnigrafo."""
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("|") and "Per la tappa di Tecnigrafo" in cells(line):
            table = table_at(lines, i, "ROADMAP.md", cells(line)[0])
            break
    else:
        return None
    for _, c in table.rows:
        for_tg = c.get("Per la tappa di Tecnigrafo", "")
        if re.match(rf"^{re.escape(tappa)}(?![0-9])", for_tg):
            state = c.get("Stato", "")
            plan = re.search(r"\]\((tappe/[^)]+\.md)\)", state)
            milestone = re.search(r"/milestone/([0-9]+)", state)
            return SsStage(c.get("Versione", ""), bool(re.match(r"^\**aperta\b", state)),
                           "docs/" + plan.group(1) if plan else None,
                           int(milestone.group(1)) if milestone else None)
    return None


def expand(cell: str, order: list[str], where: str) -> list[str]:
    """Package ids in a cell: names (P8, R14a), ranges (P4–P10, R14a–c, with
    the long or short dash, over the packages of the same kind in table
    order) and brackets (they count too)."""
    out: list[str] = []
    for m in re.finditer(rf"(?<![A-Za-z0-9])({SS_ID})(?:\s*[–-]\s*([PR]?[0-9]+[a-z]?|[a-z]\b))?", cell):
        first, last = m.group(1), m.group(2)
        if not last:
            names = [first]
        else:
            if re.match(r"^[a-z]$", last):
                last = re.match(r"^[PR][0-9]+", first).group(0) + last
            elif not last[0] in "PR":
                last = first[0] + last
            for name in (first, last):
                if name not in order:
                    raise Failure(f"{where}: {name} is not in the package table")
            a, b = order.index(first), order.index(last)
            if a > b:
                raise Failure(f"{where}: the range {first}–{last} goes backwards")
            names = [n for n in order[a:b + 1] if n[0] == first[0]]
        for n in names:
            if n not in order:
                raise Failure(f"{where}: {n} is not in the package table")
            if n not in out:
                out.append(n)
    return out


def parse_ss_plan(text: str, path: str) -> SsPlan:
    lines = text.splitlines()
    start = next((i for i, line in enumerate(lines) if line.startswith("## 6.")), None)
    table = table_at(lines, start, path, "Pacchetto") if start is not None else None
    if table is None or "Issue" not in table.header or "Dipende da" not in table.header:
        raise Failure(f"{path}: no package table in section 6 with the columns Issue and Dipende da")
    plan = SsPlan([], {}, {}, {})
    for line, c in table.rows:
        m = re.match(rf"^({SS_ID})\s", c["Pacchetto"] + " ")
        im = re.match(r"^#([0-9]+)$", c["Issue"])
        if not m:
            raise Failure(f"{path}:{line}: the first cell must start with PN or RN: \"{c['Pacchetto']}\"")
        id_ = m.group(1)
        plan.order.append(id_)
        if im:
            plan.issue[id_] = int(im.group(1))
        plan.deps[id_] = re.findall(rf"\b({SS_ID})\b", re.sub(r"\([^)]*\)", "", c["Dipende da"]))
    at = heading_index(lines, r"Che cosa sblocca in Tecnigrafo")
    if at is None:
        raise Failure(f"{path}: no section \"Che cosa sblocca in Tecnigrafo\"")
    first = next((cells(line)[0] for line in lines[at:] if line.startswith("|")), None)
    unblocks = table_at(lines, at, path, first) if first else None
    if unblocks is None:
        raise Failure(f"{path}:{at + 1}: no table under \"Che cosa sblocca in Tecnigrafo\"")
    column = "Pacchetti di solid-space che servono"
    if column not in unblocks.header:
        column = unblocks.header[-1]
    for line, c in unblocks.rows:
        m = re.search(r"#([0-9]+)", c[unblocks.header[0]])
        if not m:
            raise Failure(f"{path}:{line}: no Tecnigrafo issue in the first cell")
        plan.unblocks[int(m.group(1))] = expand(c[column], plan.order, f"{path}:{line}")
    return plan


# --- GitHub ----------------------------------------------------------------------


def json_values(text: str) -> list:
    """The JSON values printed one after the other (gh api --paginate)."""
    out, pos, decoder = [], 0, json.JSONDecoder()
    text = text.strip()
    while pos < len(text):
        value, pos = decoder.raw_decode(text, pos)
        out.append(value)
        while pos < len(text) and text[pos].isspace():
            pos += 1
    return out


class GitHub:
    """Reads and writes through gh; reads of a public repository fall back to
    curl without a token. Tests replace get() and send()."""

    def __init__(self, public: Iterable[str] = ()):
        self.public = [p.lower() for p in public]

    def is_public(self, repo: str) -> bool:
        return repo.lower() in self.public

    def get(self, path: str, paginate: bool = False):
        args = ["gh", "api", path] + (["--paginate"] if paginate else [])
        r = subprocess.run(args, capture_output=True, text=True)
        if r.returncode == 0:
            values = json_values(r.stdout)
            if paginate and values and all(isinstance(v, list) for v in values):
                return [x for v in values for x in v]
            if paginate and values and all(isinstance(v, dict) and "items" in v for v in values):
                return [x for v in values for x in v["items"]]
            return values[0] if values else None
        if any(path.lower().startswith(f"repos/{p}/") for p in self.public):
            return self._curl(path, paginate)
        raise Failure(f"GitHub refused: gh api {path}\n{(r.stderr or r.stdout).strip()}")

    def _curl(self, path: str, paginate: bool):
        url = f"https://api.github.com/{path}"
        out: list = []
        while True:
            r = subprocess.run(["curl", "-sS", "-f", "-D", "-", "-H", "Accept: application/vnd.github+json",
                                url], capture_output=True, text=True)
            if r.returncode != 0:
                raise Failure(f"GitHub did not answer: {url}\n{r.stderr.strip()}")
            head, _, body = r.stdout.partition("\r\n\r\n")
            while head.startswith("HTTP/") and body.startswith("HTTP/"):  # proxy headers
                head, _, body = body.partition("\r\n\r\n")
            value = json.loads(body)
            if not paginate or not isinstance(value, list):
                return value
            out.extend(value)
            nxt = re.search(r'<([^>]+)>;\s*rel="next"', head)
            if not nxt:
                return out
            url = nxt.group(1)

    def send(self, method: str, path: str, body: dict | None = None):
        args = ["gh", "api", "-X", method, path]
        if body is not None:
            args += ["--input", "-"]
        r = subprocess.run(args, input=json.dumps(body) if body is not None else None,
                           capture_output=True, text=True)
        if r.returncode != 0:
            raise Failure(f"GitHub refused: gh api -X {method} {path}\n{(r.stderr or r.stdout).strip()}")
        return json_values(r.stdout)[0] if r.stdout.strip() else None


@dataclass
class Issue:
    ref: Ref
    id: int
    title: str
    open: bool
    labels: list[str]
    body: str

    @property
    def tag(self) -> str | None:
        m = re.match(r"^\[([^\]]+)\]", self.title)
        return m.group(1) if m else None


def to_issue(repo: str, data: dict) -> Issue:
    return Issue(Ref(repo, data["number"]), data["id"], data.get("title", ""), data["state"] == "open",
                 [label["name"] for label in data.get("labels", [])], data.get("body") or "")


class Issues:
    """Issues read once, by reference."""

    def __init__(self, gh: GitHub):
        self.gh = gh
        self.cache: dict[Ref, Issue] = {}
        self.searched: set[str] = set()

    def _add(self, repo: str, items: list) -> None:
        for data in items or []:
            if "pull_request" not in data:
                issue = to_issue(repo, data)
                self.cache[issue.ref] = issue

    def _search(self, repo: str) -> None:
        """Every issue of a public repository through the search API, which
        an agent session reaches even when the repository is not attached."""
        if repo.lower() in self.searched:
            return
        self.searched.add(repo.lower())
        self._add(repo, self.gh.get(f"search/issues?q=repo%3A{repo}+is%3Aissue&per_page=100", paginate=True))

    def preload(self, repo: str, query: str) -> None:
        try:
            self._add(repo, self.gh.get(f"repos/{repo}/issues?state=all&per_page=100&{query}", paginate=True))
        except Failure:
            if not self.gh.is_public(repo):
                raise
            self._search(repo)

    def __getitem__(self, ref: Ref) -> Issue:
        if ref not in self.cache:
            try:
                self.cache[ref] = to_issue(ref.repo, self.gh.get(f"repos/{ref.repo}/issues/{ref.number}"))
            except Failure:
                if not self.gh.is_public(ref.repo):
                    raise
                self._search(ref.repo)
                if ref not in self.cache:
                    raise
        return self.cache[ref]


def file_from(gh: GitHub, repo: str, path: str) -> str:
    """A file of `main` of a repository; if GitHub refuses it and TG_SS_DIR
    names a checkout of that repository, the file of that checkout."""
    try:
        data = gh.get(f"repos/{repo}/contents/{path}?ref=main")
        return base64.b64decode(data["content"]).decode("utf-8")
    except Failure:
        local = os.environ.get("TG_SS_DIR")
        if not local or not (Path(local) / path).is_file():
            raise
        return (Path(local) / path).read_text(encoding="utf-8")


# --- The Project -----------------------------------------------------------------


@dataclass
class Project:
    owner: str
    number: int
    field_id: int | None
    options: dict[str, str]                       # option name -> id
    status: dict[Ref, tuple[str, str | None]]     # issue -> (item id, Status)


def _name(value) -> str | None:
    if isinstance(value, dict):
        value = value.get("raw", value.get("name"))
        if isinstance(value, dict):
            value = value.get("raw")
    return value if isinstance(value, str) else None


def load_project(gh: GitHub, owner: str, title: str, number: str | None) -> Project:
    if not number:
        projects = gh.get(f"users/{owner}/projectsV2?per_page=100", paginate=True) or []
        found = [p for p in projects if p.get("title") == title]
        if not found:
            raise Failure(f"no project titled \"{title}\" for {owner}")
        number = found[0]["number"]
    fields = gh.get(f"users/{owner}/projectsV2/{number}/fields?per_page=100", paginate=True) or []
    status = next((f for f in fields if f.get("name") == STATUS_FIELD), None)
    options = {}
    if status:
        for o in status.get("options") or []:
            options[_name(o.get("name")) or ""] = o.get("id")
    project = Project(owner, int(number), status.get("id") if status else None, options, {})
    by_id = {v: k for k, v in options.items()}
    query = f"&fields={project.field_id}" if project.field_id else ""
    for item in gh.get(f"users/{owner}/projectsV2/{number}/items?per_page=100{query}", paginate=True) or []:
        content = item.get("content") or {}
        url = content.get("repository_url") or content.get("html_url") or content.get("url") or ""
        m = re.search(r"(?:repos/|github\.com/)([^/]+/[^/]+?)(?:/issues/[0-9]+)?/?$", url)
        if not m or content.get("number") is None:
            continue
        value = None
        for f in item.get("fields") or []:
            if f.get("name") == STATUS_FIELD or f.get("id") == project.field_id:
                v = f.get("value")
                value = _name(v.get("name")) if isinstance(v, dict) and "name" in v else None
                if value is None and isinstance(v, dict):
                    value = by_id.get(v.get("id"))
        project.status[Ref(m.group(1), content["number"])] = (item["id"], value)
    return project


# --- The checks ------------------------------------------------------------------


@dataclass
class Change:
    """A change that --applica makes."""

    issue: Ref
    what: str
    method: str
    path: str
    body: dict | None = None


@dataclass
class Report:
    ready: list[str] = field(default_factory=list)
    blocked: list[str] = field(default_factory=list)
    waiting: list[str] = field(default_factory=list)
    fix: list[str] = field(default_factory=list)
    verify: list[str] = field(default_factory=list)
    info: list[str] = field(default_factory=list)
    changes: list[Change] = field(default_factory=list)

    def lines(self) -> list[str]:
        out = []
        for title, items in (("Ready", self.ready), ("Blocked", self.blocked),
                             ("Without an issue", self.waiting), ("To fix", self.fix),
                             ("To check", self.verify), ("Information", self.info)):
            if items:
                out.append(f"{title}:")
                out.extend(f"  {i}" for i in items)
        return out


@dataclass
class Context:
    tappa: str
    home: str
    ss_repo: str
    plan: Plan
    issues: Issues
    relations: Callable[[Ref], list[dict]]
    project: Project | None = None
    ss_stage: SsStage | None = None
    ss_plan: SsPlan | None = None


def dep_text(dep: str, ctx: Context) -> str:
    """A dependency as the issue text shows it: "#12 (P3)", or for another
    repository "owner/repo#49 (0.2-P8)" with the tag of the issue's title."""
    ref = dep_ref(dep, ctx.plan, ctx.home)
    if re.match(r"^[PR][0-9]+$", dep):
        return f"#{ref.number} ({dep})"
    if ref.repo.lower() == ctx.home.lower():
        return f"#{ref.number}"
    tag = ctx.issues[ref].tag
    return f"{ref.repo}#{ref.number}" + (f" ({tag})" if tag else "")


DEPS_LINE_RE = re.compile(r"^\*\*Dipende da:\*\*.*$", re.M)


def is_request(ref: Ref, ctx: Context) -> bool:
    return ref.repo.lower() == ctx.ss_repo.lower() and "richiesta" in ctx.issues[ref].labels


def propose(ctx: Context) -> dict[str, list[str]]:
    """Check 5: the new "Dipende da" of the rows that cite a request, in plan
    order; a row that cites another row with a proposal sees the proposal."""
    ss, plan = ctx.ss_plan, ctx.plan
    effective: dict[str, list[str]] = {}
    proposals: dict[str, list[str]] = {}

    def ss_id(dep: str) -> str | None:
        if "/" not in dep:
            return None
        ref = dep_ref(dep, plan, ctx.home)
        return ss.id_of(ref.number) if ref.repo.lower() == ctx.ss_repo.lower() else None

    def covered(deps: list[str]) -> set[str]:
        out: set[str] = set()
        for dep in deps:
            if re.match(r"^[PR][0-9]+$", dep):
                out |= covered(effective.get(dep, []))
            elif (i := ss_id(dep)):
                out |= ss.closure([i])
        return out

    for row in plan.rows:
        effective[row.id] = row.deps
        requests = [d for d in row.deps if "/" in d and is_request(dep_ref(d, plan, ctx.home), ctx)]
        if not requests:
            continue
        if row.issue is None or row.issue not in ss.unblocks:
            ctx_note = f"{row.id}: cites {', '.join(requests)}, but #{row.issue} is not in the table " \
                       "\"Che cosa sblocca in Tecnigrafo\" of solid-space: nothing to propose"
            proposals.setdefault("__notes__", []).append(ctx_note)
            continue
        kept = [d for d in row.deps if d not in requests]
        already = covered(kept)
        wanted = [i for i in ss.unblocks[row.issue] if i not in already]
        minimal = [i for i in wanted
                   if not any(i in ss.closure(ss.deps.get(o, [])) for o in wanted if o != i)]
        new_ss = [f"{ctx.ss_repo}#{ss.issue[i]}" for i in ss.order if i in minimal and i in ss.issue]
        missing = [i for i in minimal if i not in ss.issue]
        if missing:
            proposals.setdefault("__notes__", []).append(
                f"{row.id}: the packages {', '.join(missing)} of solid-space have no issue yet")
        at = row.deps.index(requests[0])
        new = [d for d in row.deps[:at] if d not in requests] + new_ss + \
              [d for d in row.deps[at:] if d not in requests]
        effective[row.id] = new
        proposals[row.id] = new
    return proposals


def check(ctx: Context) -> Report:
    report = Report()
    plan, home = ctx.plan, ctx.home
    report.info.extend(plan.notes)

    proposals: dict[str, list[str]] = {}
    if ctx.ss_plan is not None:
        proposals = propose(ctx)
        report.info.extend(proposals.pop("__notes__", []))
    elif ctx.ss_stage is not None and not ctx.ss_stage.open:
        cited = sorted({d for r in plan.rows for d in r.deps
                        if "/" in d and is_request(dep_ref(d, plan, home), ctx)})
        if cited:
            report.info.append(f"the stage {ctx.ss_stage.version} of solid-space is not open: "
                               f"{', '.join(cited)} stay in \"Dipende da\" for now")

    for row in plan.rows:
        if row.issue is None:
            report.waiting.append(f"{row.id}: no issue yet")
            continue
        ref = Ref(home, row.issue)
        issue = ctx.issues[ref]
        name = f"#{row.issue} [{ctx.tappa}-{row.id}]"
        if not issue.open:
            continue

        # 1. ready or blocked
        blockers, unknown = [], []
        wanted: list[Ref] = []
        for dep in row.deps:
            dref = dep_ref(dep, plan, home)
            if dref is None:
                unknown.append(dep)
                continue
            wanted.append(dref)
            if ctx.issues[dref].open:
                blockers.append(dref.text(home))
        mark = " (in charge)" if IN_CHARGE in issue.labels else ""
        agents = [lb for lb in issue.labels if lb.startswith("agente:")]
        who = f" {', '.join(agents)}" if agents else ""
        ready = not blockers and not unknown
        if ready:
            report.ready.append(f"{name}{who}{mark}")
        else:
            report.blocked.append(f"{name}{who}{mark} by " + ", ".join(blockers + [f"{u} (no issue)" for u in unknown]))

        # 2. text
        m = DEPS_LINE_RE.search(issue.body)
        if m is None:
            report.fix.append(f"{name}: no \"**Dipende da:**\" line in the text: add it by hand")
        else:
            have = refs_in(m.group(0), home)
            missing = [r for r in wanted if r not in have]
            extra = [r for r in have if r not in wanted]
            if (missing or extra) and not unknown:
                report.fix.append(f"{name}: text \"Dipende da\"" + _diff(missing, extra, home))
                line = "**Dipende da:** " + (", ".join(dep_text(d, ctx) for d in row.deps) or "—")
                report.changes.append(Change(ref, f"text: {line}", "PATCH", f"repos/{home}/issues/{row.issue}",
                                             {"body": issue.body[:m.start()] + line + issue.body[m.end():]}))

        # 3. relations
        relations = ctx.relations(ref)
        have_rel = {Ref(_repo_of(r), r["number"]): r for r in relations}
        missing = [r for r in wanted if r not in have_rel]
        extra = [r for r in have_rel if r not in wanted]
        if missing or extra:
            report.fix.append(f"{name}: \"blocked by\"" + _diff(missing, extra, home))
            for r in missing:
                report.changes.append(Change(ref, f"blocked by {r.text(home)}: add", "POST",
                                             f"repos/{home}/issues/{row.issue}/dependencies/blocked_by",
                                             {"issue_id": ctx.issues[r].id}))
            for r in extra:
                report.changes.append(Change(ref, f"blocked by {r.text(home)}: remove", "DELETE",
                                             f"repos/{home}/issues/{row.issue}/dependencies/blocked_by/"
                                             f"{have_rel[r]['id']}"))

        # 4. Project
        if ctx.project is not None:
            item = ctx.project.status.get(ref)
            if item is None:
                report.fix.append(f"{name}: not in the Project: scripts/apri-tappa.sh {ctx.tappa} --solo-project")
            else:
                status = item[1]
                target = None
                if ready and status == BLOCKED:
                    target = TODO
                elif not ready and status in (TODO, None):
                    target = BLOCKED
                if target:
                    report.fix.append(f"{name}: Project Status {status or '(empty)'}, should be {target}")
                    option = ctx.project.options.get(target)
                    if option is None or ctx.project.field_id is None:
                        report.info.append(f"the Project has no Status \"{target}\": add it, then run again")
                    else:
                        report.changes.append(Change(
                            ref, f"Project Status: {target}", "PATCH",
                            f"users/{ctx.project.owner}/projectsV2/{ctx.project.number}/items/{item[0]}",
                            {"fields": [{"id": ctx.project.field_id, "value": option}]}))

        # 5. request instead of packages
        if row.id in proposals:
            old = [d for d in row.deps if "/" in d and is_request(dep_ref(d, plan, home), ctx)]
            report.fix.append(f"{row.id} ({plan.path}:{row.line}): \"Dipende da\" cites the request "
                              f"{', '.join(old)}; with the packages of solid-space it is: "
                              f"\"{', '.join(proposals[row.id]) or '—'}\"")

        # 7. labels
        if agents != [f"agente:{row.agent}"]:
            report.info.append(f"{name}: label {', '.join(agents) or '(none)'}, plan agente:{row.agent}")

    # 6. requests to close
    for req in plan.requests:
        if req.closes_with is None or not ctx.issues[req.ref].open:
            continue
        rows = [plan.row(i) for i in req.closes_with]
        if rows and all(r.issue and not ctx.issues[Ref(home, r.issue)].open for r in rows):
            done = ", ".join(f"{r.id} (#{r.issue})" for r in rows)
            report.verify.append(f"{req.ref.text(home)}: {done} merged: check its success condition "
                                 "(\"quando è soddisfatta\") and close it")
    return report


def _repo_of(issue: dict) -> str:
    url = issue.get("repository_url") or issue.get("html_url") or ""
    m = re.search(r"(?:repos/|github\.com/)([^/]+/[^/]+?)(?:/issues/[0-9]+)?/?$", url)
    if not m:
        raise Failure(f"cannot tell the repository of {url or issue}")
    return m.group(1)


def _diff(missing: list[Ref], extra: list[Ref], home: str) -> str:
    parts = []
    if missing:
        parts.append("missing " + ", ".join(r.text(home) for r in missing))
    if extra:
        parts.append("not in the plan " + ", ".join(r.text(home) for r in extra))
    return ": " + "; ".join(parts)


# --- Main ------------------------------------------------------------------------


def build_context(tappa: str, root: Path, gh: GitHub, with_project: bool) -> Context:
    home = os.environ.get("TG_REPO", "Matleyx78/tecnigrafo")
    ss_repo = os.environ.get("TG_SS_REPO", "Matleyx78/solid-space")
    docs = sorted((root / "docs" / "tappe").glob(f"{tappa}-*.md"))
    if len(docs) != 1:
        raise Failure(f"expected one docs/tappe/{tappa}-*.md, found {len(docs)}")
    path = str(docs[0].relative_to(root))
    plan = parse_plan(docs[0].read_text(encoding="utf-8"), path, home)
    issues = Issues(gh)
    issues.preload(home, f"labels=tappa-{tappa}")

    def relations(ref: Ref) -> list[dict]:
        return gh.get(f"repos/{ref.repo}/issues/{ref.number}/dependencies/blocked_by?per_page=100",
                      paginate=True) or []

    ctx = Context(tappa, home, ss_repo, plan, issues, relations)
    cites_ss = any("/" in d and dep_ref(d, plan, home).repo.lower() == ss_repo.lower()
                   for r in plan.rows for d in r.deps) or any(
        q.ref.repo.lower() == ss_repo.lower() for q in plan.requests)
    if cites_ss:
        ctx.ss_stage = parse_ss_roadmap(file_from(gh, ss_repo, "docs/ROADMAP.md"), tappa)
        if ctx.ss_stage and ctx.ss_stage.milestone:
            issues.preload(ss_repo, f"milestone={ctx.ss_stage.milestone}")
        if ctx.ss_stage and ctx.ss_stage.open:
            if not ctx.ss_stage.plan_path:
                raise Failure(f"the row of the stage {ctx.ss_stage.version} in the ROADMAP of solid-space "
                              "has no link to its plan")
            ctx.ss_plan = parse_ss_plan(file_from(gh, ss_repo, ctx.ss_stage.plan_path),
                                        f"{ss_repo}:{ctx.ss_stage.plan_path}")
    if with_project:
        ctx.project = load_project(gh, os.environ.get("TG_PROJECT_OWNER", "Matleyx78"),
                                   os.environ.get("TG_PROJECT_TITLE", "Tecnigrafo"),
                                   os.environ.get("TG_PROJECT_NUMBER"))
    return ctx


def apply(changes: list[Change], gh: GitHub, home: str, owner: str) -> list[str]:
    """Makes the changes; returns the ones that failed. Only issues of `home`
    and the Project are ever written."""
    failed = []
    for c in changes:
        if not (c.path.startswith(f"repos/{home}/") or c.path.startswith(f"users/{owner}/projectsV2/")):
            raise AssertionError(f"refusing to write {c.path}")
        try:
            gh.send(c.method, c.path, c.body)
            print(f"done   #{c.issue.number} {c.what}")
        except Failure as e:
            print(f"FAILED #{c.issue.number} {c.what}: {e}", file=sys.stderr)
            failed.append(f"#{c.issue.number} {c.what}")
    return failed


def main(argv: list[str] | None = None, gh: GitHub | None = None, root: Path | None = None,
         ask: Callable[[str], str] = input) -> int:
    parser = argparse.ArgumentParser(
        prog="dipendenze.py",
        description="Check the dependencies of the issues of a stage against its plan, also across solid-space.")
    parser.add_argument("tappa")
    parser.add_argument("--applica", action="store_true")
    parser.add_argument("--senza-project", action="store_true")
    args = parser.parse_args(argv)
    if not re.match(r"^[0-9]+\.[0-9]+$", args.tappa):
        parser.error("the stage is X.Y, for example 0.7")
    ss_repo = os.environ.get("TG_SS_REPO", "Matleyx78/solid-space")
    gh = gh or GitHub(public=[ss_repo])
    if root is None:
        r = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True)
        if r.returncode != 0:
            print("Run it inside the Tecnigrafo repository.", file=sys.stderr)
            return 2
        root = Path(r.stdout.strip())
    try:
        ctx = build_context(args.tappa, root, gh, not args.senza_project)
        report = check(ctx)
    except Failure as e:
        print(e, file=sys.stderr)
        return 2
    print(f"Stage {args.tappa}, plan {ctx.plan.path}")
    for line in report.lines():
        print(line)
    status = 1 if report.fix or report.verify else 0
    if status == 0:
        print("Nothing to fix or to check.")
    if not args.applica:
        return status
    if not report.changes:
        print("Nothing to change.")
        return status
    print("Changes:")
    for c in report.changes:
        print(f"  #{c.issue.number} {c.what}")
    if ask("Go on? [s/N] ").strip().lower() not in ("s", "si", "sì", "y", "yes"):
        print("Nothing changed.")
        return 3
    failed = apply(report.changes, gh, ctx.home, os.environ.get("TG_PROJECT_OWNER", "Matleyx78"))
    if failed:
        print("Failed:\n  " + "\n  ".join(failed), file=sys.stderr)
        return 2
    return status


if __name__ == "__main__":
    sys.exit(main())
