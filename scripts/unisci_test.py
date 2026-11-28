# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/unisci.py, without network: the checks of the merge
# label AAA-UNISCI (docs/GUIDA-RESPONSABILE.md, A3).
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import unisci  # noqa: E402

PACKAGE = {"title": "[0.8.2-PT3] Spline", "body": "Closes #10"}
REPORT = "Revisione di Claude\n\nGiro di revisione: 2\n\nNessuna segnalazione."


class Revisione(unittest.TestCase):
    def test_package_with_report_of_the_other_agent(self):
        self.assertIsNone(unisci.manca_revisione(PACKAGE, {"pronta"}, [REPORT], "codex"))

    def test_package_needs_pronta_and_a_report(self):
        self.assertIn("pronta", unisci.manca_revisione(PACKAGE, set(), [REPORT], "codex"))
        self.assertIn("Revisione di", unisci.manca_revisione(PACKAGE, {"pronta"}, ["Presa in carico"],
                                                             "codex"))

    def test_report_of_the_executor_is_not_enough(self):
        why = unisci.manca_revisione(PACKAGE, {"pronta"}, [REPORT], "claude")
        self.assertIn("esecutore", why)

    def test_correction_of_the_specs_needs_its_report(self):
        pr = {"title": "docs(0.8.2): decisioni", "body": "Revisione della correzione: da fare\n"}
        self.assertIn("Verbale", unisci.manca_revisione(pr, {"pronta"}, [], None))
        self.assertIsNone(unisci.manca_revisione(
            pr, {"pronta"}, ["Verbale della revisione della correzione (Codex)"], None))

    def test_references_and_others_need_pronta(self):
        rt = {"title": "test(verifica): spline [0.8.2-RT3]", "body": "Avvia dopo il merge: #10"}
        self.assertIsNone(unisci.manca_revisione(rt, {"pronta"}, [], "claude"))
        self.assertIsNotNone(unisci.manca_revisione(rt, set(), [], "claude"))


class Altro(unittest.TestCase):
    def test_red_or_running_checks(self):
        runs = [{"name": "Linux", "status": "completed", "conclusion": "success"},
                {"name": "Fuzz", "status": "completed", "conclusion": "skipped"},
                {"name": "Windows", "status": "in_progress", "conclusion": None},
                {"name": "Submodule pointer (ADR 0030)", "status": "completed", "conclusion": "failure"}]
        self.assertEqual(unisci.controlli_rossi(runs),
                         ["Windows (in_progress)", "Submodule pointer (ADR 0030) (failure)"])

    def test_only_documents_on_main_need_no_update(self):
        self.assertFalse(unisci.tocca_codice(["docs/DECISIONI.md", "README.md"]))
        self.assertTrue(unisci.tocca_codice(["docs/DECISIONI.md", "core/src/a.cpp"]))
        self.assertTrue(unisci.tocca_codice(["tests/verifica/casi/a/domande.tgq"]))

    def test_message_signed_once(self):
        msg = unisci.messaggio(["feat(core): add spline\n\nAssisted-by: Codex",
                                "fix: review\n\n" + unisci.SIGNED_OFF])
        self.assertEqual(msg.count(unisci.SIGNED_OFF), 1)
        self.assertTrue(msg.endswith(unisci.SIGNED_OFF))
        self.assertIn("* feat(core): add spline", msg)
        self.assertEqual(unisci.messaggio(["one commit"]), "one commit\n\n" + unisci.SIGNED_OFF)


if __name__ == "__main__":
    unittest.main()
