# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/liberati.py, without network: which open issues a merge
# frees and how many start within the limit of open chains
# (docs/agenti/catena.md, "Dopo il merge").
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import liberati  # noqa: E402


def issue(number: int, body: str = "", labels=("agente:codex",)) -> dict:
    return {"number": number, "body": body, "labels": [{"name": n} for n in labels]}


def pr(number: int, body: str = "", labels=()) -> dict:
    r = issue(number, body, labels)
    r["pull_request"] = {}
    return r


class Dipendenze(unittest.TestCase):
    def test_lines_and_other_repositories(self):
        body = ("**Dipende da:** #3, #4 e Matleyx78/solid-space#40\n"
                "**Riferimenti:** #5\n"
                "Testo che cita #6\n"
                "```\n**Dipende da:** #7\n```\n")
        self.assertEqual(liberati.dipendenze(body), [3, 4, 5])


class Candidati(unittest.TestCase):
    def test_freed_only_when_every_dependency_is_closed(self):
        aperte = [issue(10, "**Riferimenti:** #5"),
                  issue(11, "**Dipende da:** #5, #6"),
                  issue(6, "", labels=("agente:claude", "AA-Presa in carico")),
                  issue(12, "**Dipende da:** #4")]
        self.assertEqual(liberati.candidati(aperte, {5}), [10])

    def test_needs_an_agent_and_no_held_label(self):
        aperte = [issue(10, "**Dipende da:** #5", labels=("idea",)),
                  issue(11, "**Dipende da:** #5", labels=("agente:claude", "AA-Presa in carico")),
                  issue(12, "**Dipende da:** #5", labels=("agente:claude",))]
        self.assertEqual(liberati.candidati(aperte, {5}), [12])

    def test_pull_requests_are_not_candidates(self):
        self.assertEqual(liberati.candidati([pr(10, "**Dipende da:** #5")], {5}), [])


class Scegli(unittest.TestCase):
    def test_limit_of_open_chains(self):
        aperte = ([issue(n, "**Dipende da:** #5") for n in (10, 11, 12)]
                  + [pr(n) for n in range(20, 25)]          # five open, not ready
                  + [pr(30, labels=("pronta",))])            # ready: not counted
        out = liberati.scegli(aperte, {5}, gia=[40], avviata=lambda n: False, limite=8)
        # 8 - 5 open pull requests - 1 started by its line = 2 places.
        self.assertEqual(out, {"avvia": [10, 11], "oltre": [12]})

    def test_started_or_named_issues_are_left_alone(self):
        aperte = [issue(10, "**Dipende da:** #5"), issue(11, "**Dipende da:** #5"),
                  issue(12, "**Dipende da:** #5"), pr(20, "Closes #11")]
        out = liberati.scegli(aperte, {5}, gia=[12], avviata=lambda n: n == 10, limite=8)
        self.assertEqual(out, {"avvia": [], "oltre": []})

    def test_held_issue_without_pull_request_counts(self):
        aperte = [issue(10, "**Dipende da:** #5"),
                  issue(11, labels=("agente:claude", "AA-Presa in carico"))]
        self.assertEqual(liberati.catene_aperte(aperte), 1)
        self.assertEqual(liberati.scegli(aperte, {5}, [], lambda n: False, limite=1),
                         {"avvia": [], "oltre": [10]})


if __name__ == "__main__":
    unittest.main()
