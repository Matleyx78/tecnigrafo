#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# The issues that a merge frees (docs/agenti/catena.md, "Dopo il merge";
# docs/adr/0053, point 6; docs/DECISIONI.md, 2026-11-28, Catena 7). Run by
# .github/workflows/dopo-il-merge.yml after the lines "Avvia dopo il merge".
#
# An open issue is freed by the merge of a pull request when:
# - it has an agente:claude or agente:codex label;
# - one of the issues of its "**Dipende da:**" or "**Riferimenti:**" lines is
#   one the pull request closes (Closes #N);
# - all the issues of those lines in this repository are now closed (the ones
#   of another repository, owner/repo#N, are checked by scripts/avvia.sh);
# - it is not held (AA-Presa in carico), it has no start or "Presa in carico"
#   comment of the owner's account and no open pull request names it.
#
# At most LIMITE (default 8) chains are open at once: open pull requests not
# yet `pronta`, plus held issues without a pull request. The freed issues
# beyond the limit are printed as "oltre", to start by hand with
# scripts/avvia.sh.
#
#     python3 scripts/liberati.py <pr> [already started...]
#
# Prints JSON: {"avvia": [N, ...], "oltre": [N, ...]}. Environment:
# GH_TOKEN (or GITHUB_TOKEN), GITHUB_REPOSITORY, LIMITE.

from __future__ import annotations

import json
import os
import re
import sys
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import catena  # noqa: E402

API = "https://api.github.com"
HELD = "AA-Presa in carico"
AGENTS = {"agente:claude", "agente:codex"}
DEP_LINE = re.compile(r"^\*\*(?:Dipende da|Riferimenti):\*\*(.*)$", re.MULTILINE)
SAME_REPO = re.compile(r"(?<![\w./-])#(\d+)")
STARTED = re.compile(r"^\s*@(?:claude|codex)\b|Presa in carico", re.IGNORECASE | re.MULTILINE)


def dipendenze(body: str) -> list[int]:
    """The issues of this repository named in the dependency lines."""
    out: list[int] = []
    for line in DEP_LINE.findall(catena.fuori_dal_codice(body or "")):
        for n in SAME_REPO.findall(line):
            if int(n) not in out:
                out.append(int(n))
    return out


def candidati(aperte: list[dict], chiuse: set[int]) -> list[int]:
    """The open issues (as the REST API lists them, pull requests included)
    freed by closing `chiuse`, sorted. Starts and linked pull requests are
    checked afterwards, with the comments."""
    numeri_aperti = {r["number"] for r in aperte}
    out = []
    for r in aperte:
        if "pull_request" in r:
            continue
        labels = {lab["name"] for lab in r["labels"]}
        if not labels & AGENTS or HELD in labels:
            continue
        deps = dipendenze(r.get("body") or "")
        if not set(deps) & chiuse:
            continue
        if any(n in numeri_aperti for n in deps):
            continue
        out.append(r["number"])
    return sorted(out)


def catene_aperte(aperte: list[dict]) -> int:
    """Open pull requests not yet `pronta`, plus held issues that no open pull
    request names."""
    nominate = set()
    count = 0
    for r in aperte:
        if "pull_request" in r:
            nominate.update(catena.collegate(r.get("body") or ""))
            if "pronta" not in {lab["name"] for lab in r["labels"]}:
                count += 1
    for r in aperte:
        if "pull_request" not in r and r["number"] not in nominate \
                and HELD in {lab["name"] for lab in r["labels"]}:
            count += 1
    return count


def scegli(aperte: list[dict], chiuse: set[int], gia: list[int], avviata, limite: int) -> dict:
    """The freed issues to start now and the ones beyond the limit. `gia` are
    the issues already started by the lines "Avvia dopo il merge"; `avviata(n)`
    is True when issue n has a start or "Presa in carico" comment."""
    nominate = set()
    for r in aperte:
        if "pull_request" in r:
            nominate.update(catena.collegate(r.get("body") or ""))
    liberi = [n for n in candidati(aperte, chiuse)
              if n not in gia and n not in nominate and not avviata(n)]
    posti = max(0, limite - catene_aperte(aperte) - len(gia))
    return {"avvia": liberi[:posti], "oltre": liberi[posti:]}


def call(path: str, token: str):
    req = urllib.request.Request(f"{API}/{path}", headers={
        "Authorization": f"Bearer {token}", "Accept": "application/vnd.github+json"})
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read() or "null")


def main(argv: list[str]) -> int:
    if len(argv) < 2 or not argv[1].isdigit():
        print(f"usage: {argv[0]} <pr> [already started...]", file=sys.stderr)
        return 2
    repo = os.environ.get("GITHUB_REPOSITORY", "Matleyx78/tecnigrafo")
    token = os.environ.get("GH_TOKEN") or os.environ["GITHUB_TOKEN"]
    owner = repo.split("/")[0].lower()
    limite = int(os.environ.get("LIMITE") or "8")
    pr = call(f"repos/{repo}/pulls/{argv[1]}", token)
    chiuse = {n for n in catena.chiude(pr.get("body") or "")
              if call(f"repos/{repo}/issues/{n}", token)["state"] == "closed"}
    aperte: list[dict] = []
    for page in range(1, 6):
        batch = call(f"repos/{repo}/issues?state=open&per_page=100&page={page}", token)
        aperte += batch
        if len(batch) < 100:
            break

    def avviata(n: int) -> bool:
        comments = call(f"repos/{repo}/issues/{n}/comments?per_page=100", token)
        return any(c["user"]["login"].lower() == owner and STARTED.search(c.get("body") or "")
                   for c in comments)

    gia = [int(n) for n in argv[2:]]
    print(json.dumps(scegli(aperte, chiuse, gia, avviata, limite)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
