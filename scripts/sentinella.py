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
#   last comment of a person is the owner's decision that no agent picked up;
# - an issue has an agente: label and no open pull request that closes it;
# - a pull request has no CI run still going on its last commit (the CI of
#   Tecnigrafo can take more than an hour);
# - nothing has moved for at least SOGLIA_MINUTI minutes (and at most 24
#   hours: older items predate the chain and are left alone).
#
# Environment: GITHUB_TOKEN, GITHUB_REPOSITORY, SOGLIA_MINUTI (default 60),
# DRY_RUN=1 to only print.

from __future__ import annotations

import datetime as dt
import json
import os
import re
import sys
import urllib.request
from dataclasses import dataclass

API = "https://api.github.com"
STOP_LABELS = {"pronta", "decisione", "ferma", "proposta"}
HELD = "AA-Presa in carico"
STARTED = re.compile(r"Presa in carico|@claude|@codex", re.IGNORECASE)
DECIDED = re.compile(r"^\s*@\w+ Decisione del responsabile:", re.IGNORECASE)
CLOSES = re.compile(r"(?:closes|fixes|resolves)\s+#(\d+)", re.IGNORECASE)
MAX_IDLE = 24 * 60


@dataclass
class Item:
    number: int
    is_pr: bool
    labels: set[str]
    idle: float               # minutes since the last update
    comments: list[dict]      # {"login": ..., "body": ..., "created_at": ...}
    handled_by_pr: bool = False
    ci_running: bool = False


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


def stuck(it: Item, soglia: int) -> str | None:
    """The comment to write if the item is a stuck chain, else None."""
    if not needs_comments(it, soglia):
        return None
    if not any(STARTED.search(c["body"]) for c in it.comments):
        return None
    pending = decision_pending(it.labels)
    if pending:
        # Waiting for the owner is fine; an answered decision that nobody
        # picked up is not. Bot replies (for example usage limits) do not count.
        people = [c for c in it.comments if not c["login"].endswith("[bot]")]
        if not people or not DECIDED.search(people[-1]["body"]):
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

    def comments(self, number: int) -> list[dict]:
        raw = self.call("GET", f"/issues/{number}/comments?per_page=100")
        return [{"login": c["user"]["login"], "body": c.get("body") or "",
                 "created_at": c["created_at"]} for c in raw]


def main() -> int:
    repo = os.environ["GITHUB_REPOSITORY"]
    gh = GitHub(repo, os.environ["GITHUB_TOKEN"])
    soglia = int(os.environ.get("SOGLIA_MINUTI") or "60")
    dry_run = os.environ.get("DRY_RUN") == "1"
    now = dt.datetime.now(dt.timezone.utc)

    raw = gh.call("GET", "/issues?state=open&per_page=100")
    handled = set()
    for it in raw:
        if "pull_request" in it:
            handled.update(int(n) for n in CLOSES.findall(it.get("body") or ""))

    for r in raw:
        updated = dt.datetime.fromisoformat(r["updated_at"].replace("Z", "+00:00"))
        it = Item(number=r["number"], is_pr="pull_request" in r,
                  labels={lab["name"] for lab in r["labels"]},
                  idle=(now - updated).total_seconds() / 60, comments=[],
                  handled_by_pr=r["number"] in handled)
        if not needs_comments(it, soglia):
            continue
        if it.is_pr:
            it.ci_running = gh.ci_running(it.number)
            if it.ci_running:
                continue
        it.comments = gh.comments(it.number)
        text = stuck(it, soglia)
        if text is None:
            continue
        kind = "PR" if it.is_pr else "issue"
        print(f"{kind} #{it.number}: ferma da {int(it.idle)} min"
              + (" (AA rimasta)" if HELD in it.labels else ""))
        if not dry_run:
            gh.call("POST", f"/issues/{it.number}/labels", {"labels": ["ferma"]})
            gh.call("POST", f"/issues/{it.number}/comments", {"body": text})
    return 0


if __name__ == "__main__":
    sys.exit(main())
