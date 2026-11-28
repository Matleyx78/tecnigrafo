#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# The sentinel of the chain of agents (docs/agenti/catena.md, "Sentinella";
# docs/adr/0053): labels as `ferma`, with a comment, the chains that stopped
# without saying so. Run by .github/workflows/sentinella.yml.
#
# An open issue or pull request is a stuck chain when:
# - the chain has started on it (a "Presa in carico" comment or a mention of
#   an agent);
# - it has none of the labels that mean "waiting for the owner" or "finished"
#   (pronta, decisione, ferma, proposta), unless it has `decisione` and the
#   last comment of a person is a start written for an agent (a decision, a
#   resume, a hand-over) that no agent picked up;
# - an issue has an agente: label and no open pull request linked to it;
# - a pull request has no CI run still going on its last commit (the CI of
#   Tecnigrafo can take more than an hour);
# - nothing has moved for at least SOGLIA_MINUTI minutes (and at most 24
#   hours: older items predate the chain and are left alone).
#
# Only comments of the owner of the repository count as a start or a decision
# (the agents write with the owner's account too); a comment of anyone else,
# for example a contributor, never does.
#
# A pull request and its linked issues (Closes #N, "Avvia dopo il merge: #N",
# scripts/catena.py) are one chain: the hand-overs to Codex are written in the
# issue, so the pull request counts their comments and their movement too,
# and the issue is left to the pull request.
#
# Before labelling a pull request `ferma`, the sentinel checks whether the
# next step is one and certain (docs/DECISIONI.md, 2026-11-28, Catena 2): the
# CI of the last commit is green, `AA-Presa in carico` is gone, there is no
# `decisione`, no review report, triage, analysis or earlier hand-over to
# review. Then
# it takes the step itself and says so in a comment:
# - a pull request of a package (`[X.Y-PTN]`, `[FN]`) or a correction of the
#   specs ("Revisione della correzione: da fare"): scripts/passa.sh to the
#   other agent of the linked issue, review round 1;
# - a pull request of references (`[X.Y-RTN]`): the label `pronta`.
# In every other case, or when the hand-over fails, it labels `ferma` as
# before.
#
# Environment: GITHUB_TOKEN, GITHUB_REPOSITORY, SOGLIA_MINUTI (default 60),
# GH_TOKEN_UTENTE (the RELAY_PAT secret, for scripts/passa.sh), DRY_RUN=1 to
# only print.

from __future__ import annotations

import datetime as dt
import json
import os
import re
import subprocess
import sys
import urllib.request
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import catena  # noqa: E402

API = "https://api.github.com"
STOP_LABELS = {"pronta", "decisione", "ferma", "proposta"}
HELD = "AA-Presa in carico"
STARTED = re.compile(r"Presa in carico|@claude|@codex", re.IGNORECASE)
# A comment of the owner that starts an agent: a decision, a resume written by
# scripts/avvia.sh, a hand-over written by scripts/passa.sh.
AGENT_START = re.compile(r"^\s*@(?:claude|codex)\b", re.IGNORECASE)
MAX_IDLE = 24 * 60
# A review report, an analysis or a review hand-over already written: after
# them the next step is not certain any more.
REPORT = re.compile(r"Giro di revisione:|^\s*\**(?:Revisione di |Analisi di |Smistamento di |Ripresa dopo la decisione"
                    r"|Verbale della revisione della correzione)", re.MULTILINE)
REVIEW_HANDOVER = re.compile(r"Revisione \(revisore\)|Passaggio fatto dalla sentinella")
REFERENCES_TITLE = re.compile(r"\[\d+\.\d+(?:\.\d+)?-RT\d+[a-z]?\]")
PACKAGE_TITLE = re.compile(r"\[\d+\.\d+(?:\.\d+)?-PT\d+[a-z]?\]|\[F\d+\]")
CORRECTION = "Revisione della correzione: da fare"
AGENTS = ("claude", "codex")


@dataclass
class Item:
    number: int
    is_pr: bool
    labels: set[str]
    idle: float               # minutes since the last update, linked issues included
    comments: list[dict]      # {"login": ..., "body": ..., "created_at": ...},
                              # of the linked issues too, by time
    handled_by_pr: bool = False
    ci_running: bool = False
    linked: list[int] = field(default_factory=list)
    title: str = ""
    body: str = ""


def decision_pending(labels: set[str]) -> bool:
    return "decisione" in labels and not labels & (STOP_LABELS - {"decisione"})


def needs_comments(it: Item, soglia: int) -> bool:
    """The checks that need no comments: True if the item may be stuck."""
    pending = decision_pending(it.labels)
    if it.labels & STOP_LABELS and not pending:
        return False
    if not it.is_pr and not any(l.startswith("agente:") for l in it.labels):
        return False
    if not it.is_pr and it.handled_by_pr and not pending:
        return False
    if it.is_pr and it.ci_running:
        return False
    return soglia <= it.idle <= MAX_IDLE


def stuck(it: Item, soglia: int, owner: str) -> str | None:
    """The comment to write if the item is a stuck chain, else None. `owner`
    is the login of the owner of the repository."""
    if not needs_comments(it, soglia):
        return None
    mine = [c for c in it.comments if c["login"].lower() == owner.lower()]
    if not any(STARTED.search(c["body"]) for c in mine):
        return None
    pending = decision_pending(it.labels)
    if pending:
        # Waiting for the owner is fine; a start that nobody picked up is not:
        # the last comment of the owner's account (agents included) starts an
        # agent. Bots and other accounts do not count.
        if not mine or not AGENT_START.search(mine[-1]["body"]):
            return None
    last = it.comments[-1] if it.comments else None
    text = ("**Decisione non ripresa** (sentinella): " if pending
            else "**Catena ferma** (sentinella): ")
    text += f"nessun movimento da {int(it.idle)} minuti"
    if last:
        text += f"; ultimo commento di {last['login']} alle {last['created_at'][11:16]} UTC"
    text += "."
    if HELD in it.labels:
        text += ("\n\nL'etichetta `AA-Presa in carico` è rimasta: l'agente si è probabilmente"
                 " interrotto (errore, tempo scaduto, quota). Toglila prima di riavviare.")
    text += "\n\nPer riprendere: togli `ferma` e riavvia l'ultimo passaggio."
    return text


def passaggio(it: Item, ci_green: bool, executor: str | None) -> tuple[str, str] | None:
    """The step the sentinel takes instead of `ferma` on a stuck pull request,
    as (action, argument): ("revisione", reviewer) or ("pronta", ""); None when
    the step is not one and certain. `executor` is the agent of the agente:
    label of the linked issue."""
    if not it.is_pr or not ci_green or HELD in it.labels or "decisione" in it.labels:
        return None
    if any(REPORT.search(c["body"]) or REVIEW_HANDOVER.search(c["body"]) for c in it.comments):
        return None
    if REFERENCES_TITLE.search(it.title):
        return ("pronta", "")
    if PACKAGE_TITLE.search(it.title) or CORRECTION in it.body:
        if executor not in AGENTS:
            return None
        return ("revisione", AGENTS[1 - AGENTS.index(executor)])
    return None


def passaggio_text(action: str, reviewer: str) -> str:
    text = ("**Passaggio fatto dalla sentinella**: CI verde, `AA-Presa in carico` tolta,"
            " nessun verbale e nessuna `decisione`, ma il passaggio finale mancava. ")
    if action == "pronta":
        text += "Pull request di riferimenti: messa l'etichetta `pronta` (li rivede il responsabile)."
    else:
        text += f"Passata in revisione a {reviewer} (giro 1) con `scripts/passa.sh`."
    return text + " (docs/agenti/catena.md, \"Sentinella\")"


class GitHub:
    def __init__(self, repo: str, token: str) -> None:
        self.repo, self.token = repo, token

    def call(self, method: str, path: str, body=None):
        req = urllib.request.Request(
            f"{API}/repos/{self.repo}{path}",
            method=method,
            data=json.dumps(body).encode() if body is not None else None,
            headers={"Authorization": f"Bearer {self.token}",
                     "Accept": "application/vnd.github+json"},
        )
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read() or "null")

    def ci_running(self, number: int) -> bool:
        sha = self.call("GET", f"/pulls/{number}")["head"]["sha"]
        runs = self.call("GET", f"/commits/{sha}/check-runs?per_page=100")
        return any(r["status"] != "completed" for r in runs.get("check_runs", []))

    def ci_green(self, number: int) -> bool:
        """Every check run of the last commit has finished, none failed, and at
        least one passed."""
        sha = self.call("GET", f"/pulls/{number}")["head"]["sha"]
        runs = self.call("GET", f"/commits/{sha}/check-runs?per_page=100").get("check_runs", [])
        return (any(r.get("conclusion") == "success" for r in runs)
                and all(r["status"] == "completed"
                        and r.get("conclusion") in {"success", "skipped", "neutral"} for r in runs))

    def executor(self, linked: list[int]) -> str | None:
        """The agent of the agente: label of the first linked issue."""
        if not linked:
            return None
        labels = [lab["name"] for lab in self.call("GET", f"/issues/{linked[0]}")["labels"]]
        agents = [lab.split(":", 1)[1] for lab in labels if lab.startswith("agente:")]
        return agents[0] if agents else None

    def comments(self, number: int) -> list[dict]:
        raw = self.call("GET", f"/issues/{number}/comments?per_page=100")
        return [{"login": c["user"]["login"], "body": c.get("body") or "",
                 "created_at": c["created_at"]} for c in raw]


def find(raw: list[dict], now: dt.datetime, soglia: int, gh,
         owner: str) -> list[tuple[Item, str]]:
    """The stuck chains among the open issues and pull requests `raw` (as the
    REST API lists them), with the comment to write. `gh` gives
    ci_running(number) and comments(number)."""
    idle_of = {}
    for r in raw:
        updated = dt.datetime.fromisoformat(r["updated_at"].replace("Z", "+00:00"))
        idle_of[r["number"]] = (now - updated).total_seconds() / 60
    handled = set()
    linked_of = {}
    for r in raw:
        if "pull_request" in r:
            linked_of[r["number"]] = catena.collegate(r.get("body") or "")
            handled.update(linked_of[r["number"]])

    found = []
    for r in raw:
        is_pr = "pull_request" in r
        linked = linked_of.get(r["number"], []) if is_pr else []
        idle = min([idle_of[r["number"]]] + [idle_of[n] for n in linked if n in idle_of])
        it = Item(number=r["number"], is_pr=is_pr,
                  labels={lab["name"] for lab in r["labels"]},
                  idle=idle, comments=[], handled_by_pr=r["number"] in handled,
                  linked=linked, title=r.get("title") or "", body=r.get("body") or "")
        if not needs_comments(it, soglia):
            continue
        if it.is_pr:
            it.ci_running = gh.ci_running(it.number)
            if it.ci_running:
                continue
        it.comments = gh.comments(it.number)
        for n in it.linked:
            it.comments += gh.comments(n)
        it.comments.sort(key=lambda c: c["created_at"])
        text = stuck(it, soglia, owner)
        if text is not None:
            found.append((it, text))
    return found


def main() -> int:
    repo = os.environ["GITHUB_REPOSITORY"]
    gh = GitHub(repo, os.environ["GITHUB_TOKEN"])
    soglia = int(os.environ.get("SOGLIA_MINUTI") or "60")
    dry_run = os.environ.get("DRY_RUN") == "1"
    now = dt.datetime.now(dt.timezone.utc)
    raw = gh.call("GET", "/issues?state=open&per_page=100")
    for it, text in find(raw, now, soglia, gh, repo.split("/")[0]):
        kind = "PR" if it.is_pr else "issue"
        step = None
        if it.is_pr:
            step = passaggio(it, gh.ci_green(it.number), gh.executor(it.linked))
        if step and fai_passaggio(gh, it, step, dry_run):
            continue
        print(f"{kind} #{it.number}: ferma da {int(it.idle)} min"
              + (" (AA rimasta)" if HELD in it.labels else ""))
        if not dry_run:
            gh.call("POST", f"/issues/{it.number}/labels", {"labels": ["ferma"]})
            gh.call("POST", f"/issues/{it.number}/comments", {"body": text})
    return 0


def fai_passaggio(gh, it: Item, step: tuple[str, str], dry_run: bool) -> bool:
    """Takes the certain step; False when it could not, so the caller labels
    `ferma` as before."""
    action, reviewer = step
    print(f"PR #{it.number}: passaggio della sentinella: {action} {reviewer}".rstrip())
    if dry_run:
        return True
    if action == "pronta":
        gh.call("POST", f"/issues/{it.number}/labels", {"labels": ["pronta"]})
    else:
        if not os.environ.get("GH_TOKEN_UTENTE"):
            print("  no GH_TOKEN_UTENTE: cannot hand over, labelling ferma")
            return False
        script = Path(__file__).resolve().parent / "passa.sh"
        done = subprocess.run(["bash", str(script), reviewer, str(it.number), "revisione", "1"],
                              capture_output=True, text=True)
        if done.returncode != 0:
            print(f"  scripts/passa.sh failed: {done.stderr.strip()}")
            return False
    gh.call("POST", f"/issues/{it.number}/comments", {"body": passaggio_text(action, reviewer)})
    return True

if __name__ == "__main__":
    sys.exit(main())
