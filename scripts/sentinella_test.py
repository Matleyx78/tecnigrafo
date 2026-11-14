# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/sentinella.py, without network: only the choice of the
# stuck chains (docs/agenti/catena.md, "Sentinella").
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import sentinella as s  # noqa: E402

OWNER = "Matleyx78"


def comment(body: str, login: str = OWNER) -> dict:
    return {"login": login, "body": body, "created_at": "2026-11-14T18:30:00Z"}


def item(**kw) -> s.Item:
    base = dict(number=7, is_pr=False, labels={"agente:codex"}, idle=90.0,
                comments=[comment("Presa in carico da Codex")])
    base.update(kw)
    return s.Item(**base)


class Stuck(unittest.TestCase):
    def test_started_issue_without_movement_is_stuck(self):
        text = s.stuck(item(), 60)
        self.assertIsNotNone(text)
        self.assertIn("Catena ferma", text)
        self.assertIn("90 minuti", text)

    def test_threshold_and_upper_bound(self):
        self.assertIsNone(s.stuck(item(idle=59.0), 60))
        self.assertIsNotNone(s.stuck(item(idle=60.0), 60))
        self.assertIsNone(s.stuck(item(idle=24 * 60 + 1.0), 60))

    def test_not_started_is_left_alone(self):
        self.assertIsNone(s.stuck(item(comments=[comment("Una nota")]), 60))

    def test_issue_without_agent_label_is_left_alone(self):
        self.assertIsNone(s.stuck(item(labels={"idea"}), 60))

    def test_stop_labels(self):
        for label in ("pronta", "ferma", "proposta"):
            with self.subTest(label=label):
                self.assertIsNone(s.stuck(item(labels={"agente:claude", label}), 60))

    def test_issue_handled_by_open_pull_request(self):
        self.assertIsNone(s.stuck(item(handled_by_pr=True), 60))

    def test_pull_request_with_running_ci(self):
        pr = item(is_pr=True, labels={"AA-Presa in carico"})
        self.assertIsNotNone(s.stuck(pr, 60))
        pr.ci_running = True
        self.assertIsNone(s.stuck(pr, 60))

    def test_held_label_is_reported(self):
        text = s.stuck(item(labels={"agente:claude", "AA-Presa in carico"}), 60)
        self.assertIn("AA-Presa in carico", text)

    def test_decision_waiting_for_the_owner(self):
        it = item(labels={"agente:codex", "decisione"},
                  comments=[comment("Presa in carico da Codex"),
                            comment("Serve una decisione del responsabile: …")])
        self.assertIsNone(s.stuck(it, 60))

    def test_decision_written_and_not_picked_up(self):
        it = item(labels={"agente:codex", "decisione"},
                  comments=[comment("Presa in carico da Codex"),
                            comment("@codex Decisione del responsabile: A"),
                            comment("You have reached your usage limit", "chatgpt-codex-connector[bot]")])
        text = s.stuck(it, 60)
        self.assertIsNotNone(text)
        self.assertIn("Decisione non ripresa", text)

    def test_decision_with_another_stop_label(self):
        it = item(labels={"agente:codex", "decisione", "ferma"},
                  comments=[comment("@codex Decisione del responsabile: A")])
        self.assertIsNone(s.stuck(it, 60))


if __name__ == "__main__":
    unittest.main()
