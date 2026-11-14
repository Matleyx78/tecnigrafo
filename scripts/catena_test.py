# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/catena.py: the lines of a pull request description that the
# chain of agents reads (docs/agenti/catena.md).
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import io
import sys
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import catena as c  # noqa: E402


class AvviaDopoIlMerge(unittest.TestCase):
    def test_line_of_its_own(self):
        self.assertEqual(c.avvia_dopo_il_merge("Testo.\n\nAvvia dopo il merge: #31\n"), [31])

    def test_spaces_list_mark_and_case(self):
        text = "  avvia dopo il merge:#31  \n- Avvia dopo il merge: #32\n* Avvia dopo il merge: #31"
        self.assertEqual(c.avvia_dopo_il_merge(text), [31, 32])

    def test_quotation_in_prose_is_not_a_line(self):
        for text in ("Abbiamo rimosso la riga Avvia dopo il merge: #31",
                     "Avvia dopo il merge: #31 e poi #32",
                     "`Avvia dopo il merge: #31`",
                     "> Avvia dopo il merge: #31"):
            with self.subTest(text=text):
                self.assertEqual(c.avvia_dopo_il_merge(text), [])

    def test_crlf(self):
        self.assertEqual(c.avvia_dopo_il_merge("A\r\nAvvia dopo il merge: #7\r\nB"), [7])

    def test_empty(self):
        self.assertEqual(c.avvia_dopo_il_merge(None), [])


class Collegate(unittest.TestCase):
    def test_closes_first_then_start_lines(self):
        text = "Avvia dopo il merge: #5\nCloses #9\nFixes #9"
        self.assertEqual(c.collegate(text), [9, 5])

    def test_only_start_line(self):
        text = "Revisione della correzione: da fare\nAvvia dopo il merge: #12"
        self.assertEqual(c.collegate(text), [12])

    def test_quoted_start_line_links_nothing(self):
        self.assertEqual(c.collegate("La riga Avvia dopo il merge: #12 è tolta"), [])


class Main(unittest.TestCase):
    def run_main(self, arg: str, text: str) -> str:
        out = io.StringIO()
        with mock.patch.object(sys, "stdin", io.StringIO(text)), redirect_stdout(out):
            self.assertEqual(c.main(["catena.py", arg]), 0)
        return out.getvalue()

    def test_avvia(self):
        self.assertEqual(self.run_main("avvia", "Avvia dopo il merge: #3\nAvvia dopo il merge: #1"), "1\n3\n")

    def test_collegata_prints_one(self):
        self.assertEqual(self.run_main("collegata", "Closes #4\nAvvia dopo il merge: #3"), "4\n")
        self.assertEqual(self.run_main("collegata", "niente"), "")

    def test_usage(self):
        with mock.patch.object(sys, "stderr", io.StringIO()):
            self.assertEqual(c.main(["catena.py"]), 2)


if __name__ == "__main__":
    unittest.main()
