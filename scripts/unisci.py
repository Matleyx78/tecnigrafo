#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# The merge command of the owner (docs/GUIDA-RESPONSABILE.md, A3; docs/DECISIONI.md,
# 2026-11-28, Catena 6), run by .github/workflows/unisci.yml when the owner
# puts the label AAA-UNISCI on a pull request. In order:
#
# 1. the review, as the rules ask: every pull request has `pronta`; a package
#    (`[X.Y-PTN]`, `[FN]`) also a report «Revisione di <agent>» of the agent
#    that is not the one of its issue; a correction of the specs
#    ("Revisione della correzione: da fare") the «Verbale della revisione
#    della correzione»;
# 2. every check of the last commit green (Submodule pointer included), no
#    `decisione`, the issue «main rosso» closed;
# 3. if main has changed since the branch left it with files other than
#    documents (*.md), the branch is updated with main (merge commit) and the
#    CI awaited, then step 2 again;
# 4. squash and merge, with the owner's Signed-off-by line, as the owner (the
#    RELAY_PAT secret), so that "Dopo il merge" starts.
#
# When a step fails it answers in the pull request why, and merges nothing.
# Either way the label AAA-UNISCI is taken away at the end, so that putting it
# back tries again.
# The pull requests that touch app/ or resources/ keep the gallery to look at
# before the command (A3, point 2): the script cannot judge it.
#
#     python3 scripts/unisci.py <pr>
#
# Environment: GH_TOKEN (the RELAY_PAT secret), GITHUB_REPOSITORY, DRY_RUN=1
# to only print.

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import catena  # noqa: E402

API = "https://api.github.com"
SIGNED_OFF = "Signed-off-by: Matleyx78 <18114626+Matleyx78@users.noreply.github.com>"
PACKAGE_TITLE = re.compile(r"\[\d+\.\d+(?:\.\d+)?-PT\d+[a-z]?\]|\[F\d+\]")
CORRECTION = "Revisione della correzione: da fare"
REPORT = re.compile(r"^\s*\**Revisione di (Claude|Codex)\b", re.IGNORECASE | re.MULTILINE)
CORRECTION_REPORT = re.compile(r"^\s*\**Verbale della revisione della correzione", re.MULTILINE)
GREEN = {"success", "skipped", "neutral"}
LABEL = "AAA-UNISCI"


def manca_revisione(pr: dict, labels: set[str], comments: list[str],
                    executor: str | None) -> str | None:
    """Why the review is missing, or None when it is there."""
    if "pronta" not in labels:
        return "manca l'etichetta `pronta`"
    body = pr.get("body") or ""
    if CORRECTION in body and not any(CORRECTION_REPORT.search(c) for c in comments):
        return "manca il «Verbale della revisione della correzione»"
    if PACKAGE_TITLE.search(pr.get("title") or ""):
        reviewers = [m.group(1).lower() for c in comments for m in [REPORT.search(c)] if m]
        if not reviewers:
            return "manca il verbale «Revisione di <agente>»"
        if executor and reviewers[-1] == executor:
            return f"l'ultimo verbale è di {executor}, che è l'esecutore: serve quello dell'altro agente"
    return None


def controlli_rossi(check_runs: list[dict]) -> list[str]:
    """The checks of the last commit that are not finished and green."""
    return [f"{r['name']} ({r.get('conclusion') or r['status']})" for r in check_runs
            if r["status"] != "completed" or r.get("conclusion") not in GREEN]


def tocca_codice(files: list[str]) -> bool:
    """True when the files changed on main since the branch left it are not all
    documents: then the branch must be tried again with them."""
    return any(not f.endswith(".md") for f in files)


def messaggio(commits: list[str]) -> str:
    """The squash message: the messages of the commits, then the owner's
    Signed-off-by, once."""
    parts = [c.strip() for c in commits if c.strip()]
    text = "\n\n".join(f"* {p}" for p in parts) if len(parts) > 1 else (parts[0] if parts else "")
    lines = [line for line in text.splitlines() if line.strip() != SIGNED_OFF]
    return "\n".join(lines).rstrip() + "\n\n" + SIGNED_OFF


class GitHub:
    def __init__(self, repo: str, token: str) -> None:
        self.repo, self.token = repo, token

    def call(self, method: str, path: str, body=None):
        req = urllib.request.Request(
            f"{API}/repos/{self.repo}{path}", method=method,
            data=json.dumps(body).encode() if body is not None else None,
            headers={"Authorization": f"Bearer {self.token}",
                     "Accept": "application/vnd.github+json"})
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read() or "null")

    def comments(self, number: int) -> list[str]:
        out: list[str] = []
        for page in range(1, 6):
            batch = self.call("GET", f"/issues/{number}/comments?per_page=100&page={page}")
            out += [c.get("body") or "" for c in batch]
            if len(batch) < 100:
                break
        return out

    def check_runs(self, sha: str) -> list[dict]:
        return self.call("GET", f"/commits/{sha}/check-runs?per_page=100").get("check_runs", [])

    def executor(self, body: str) -> str | None:
        linked = catena.collegate(body)
        if not linked:
            return None
        labels = [lab["name"] for lab in self.call("GET", f"/issues/{linked[0]}")["labels"]]
        agents = [lab.split(":", 1)[1] for lab in labels if lab.startswith("agente:")]
        return agents[0] if agents else None

    def main_rosso(self) -> int | None:
        issues = self.call("GET", "/issues?state=open&per_page=100")
        return next((i["number"] for i in issues
                     if "pull_request" not in i and i["title"] == "main rosso"), None)


def answer(gh: GitHub, number: int, text: str, dry_run: bool) -> int:
    print(text)
    if not dry_run:
        gh.call("POST", f"/issues/{number}/comments",
                {"body": f"**{LABEL}** non eseguito: {text} Togli e rimetti l'etichetta per riprovare."})
    return 1


def wait_ci(number: int) -> int:
    """scripts/ci-attendi.sh on the pull request, resumed while it says 3."""
    script = Path(__file__).resolve().parent / "ci-attendi.sh"
    for _ in range(12):
        done = subprocess.run(["bash", str(script), str(number)])
        if done.returncode != 3:
            return done.returncode
    return 3


def checks_ok(gh: GitHub, number: int, dry_run: bool) -> tuple[dict | None, int]:
    pr = gh.call("GET", f"/pulls/{number}")
    labels = {lab["name"] for lab in pr["labels"]}
    if "decisione" in labels:
        return None, answer(gh, number, "la pull request ha `decisione`.", dry_run)
    red = controlli_rossi(gh.check_runs(pr["head"]["sha"]))
    if red:
        return None, answer(gh, number, "controlli non verdi: " + ", ".join(red) + ".", dry_run)
    open_red = gh.main_rosso()
    if open_red:
        return None, answer(gh, number, f"la issue «main rosso» (#{open_red}) è aperta.", dry_run)
    return pr, 0


def unisci(gh: GitHub, number: int, dry_run: bool) -> int:
    pr = gh.call("GET", f"/pulls/{number}")
    if pr["state"] != "open":
        return answer(gh, number, "la pull request non è aperta.", dry_run)
    labels = {lab["name"] for lab in pr["labels"]}
    why = manca_revisione(pr, labels, gh.comments(number), gh.executor(pr.get("body") or ""))
    if why:
        return answer(gh, number, why + ".", dry_run)
    pr, status = checks_ok(gh, number, dry_run)
    if pr is None:
        return status
    compare = gh.call("GET", f"/compare/{pr['head']['sha']}...{pr['base']['ref']}")
    if compare.get("ahead_by", 0) and tocca_codice([f["filename"] for f in compare.get("files", [])]):
        print(f"main has {compare['ahead_by']} new commits with code: updating the branch")
        if dry_run:
            return 0
        gh.call("PUT", f"/pulls/{number}/update-branch",
                {"expected_head_sha": pr["head"]["sha"]})
        time.sleep(30)
        if wait_ci(number) != 0:
            return answer(gh, number, "dopo l'aggiornamento con `main` la CI non è verde.", dry_run)
        pr, status = checks_ok(gh, number, dry_run)
        if pr is None:
            return status
    commits = [c["commit"]["message"] for c in gh.call("GET", f"/pulls/{number}/commits?per_page=100")]
    body = {"merge_method": "squash", "sha": pr["head"]["sha"],
            "commit_title": f"{pr['title']} (#{number})", "commit_message": messaggio(commits)}
    if dry_run:
        print(json.dumps(body, indent=2))
        return 0
    try:
        gh.call("PUT", f"/pulls/{number}/merge", body)
    except urllib.error.HTTPError as error:
        return answer(gh, number, f"GitHub ha rifiutato il merge ({error.code}).", dry_run)
    print(f"#{number} merged")
    return 0


def main(argv: list[str]) -> int:
    if len(argv) != 2 or not argv[1].isdigit():
        print(f"usage: {argv[0]} <pr>", file=sys.stderr)
        return 2
    gh = GitHub(os.environ.get("GITHUB_REPOSITORY", "Matleyx78/tecnigrafo"),
                os.environ["GH_TOKEN"])
    dry_run = os.environ.get("DRY_RUN") == "1"
    try:
        return unisci(gh, int(argv[1]), dry_run)
    finally:
        if not dry_run:
            try:
                gh.call("DELETE", f"/issues/{argv[1]}/labels/{LABEL}")
            except urllib.error.HTTPError:
                pass


if __name__ == "__main__":
    sys.exit(main(sys.argv))
