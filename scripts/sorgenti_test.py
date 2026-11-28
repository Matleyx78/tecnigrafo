# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/sorgenti.py: the public files listed in sorgenti.json and
# in one file per block of references, tests/verifica/sorgenti/.
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import sorgenti  # noqa: E402


def source(name: str) -> dict:
    return {"name": name, "url": f"https://example.invalid/{name}", "version": "1",
            "sha256": "0" * 64, "license": "CC0-1.0", "license-url": "https://example.invalid/l"}


def write(path: Path, *names: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps({"format": 1, "sources": [source(n) for n in names]}))


class Sorgenti(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        write(self.root / "sorgenti.json", "a", "b")
        write(self.root / "sorgenti" / "0.8.2-RT3.json", "c")
        write(self.root / "sorgenti" / "0.8.2-RT10.json", "d")

    def tearDown(self):
        self.tmp.cleanup()

    def test_all_names_manifest_first(self):
        self.assertEqual([s["name"] for s in sorgenti.leggi(self.root)], ["a", "b", "d", "c"])
        self.assertEqual(sorgenti.problemi(self.root), [])

    def test_duplicate_name_across_files(self):
        write(self.root / "sorgenti" / "0.8.2-RT4.json", "b")
        problems = sorgenti.problemi(self.root)
        self.assertEqual(len(problems), 1)
        self.assertIn("duplicate source name b", problems[0])

    def test_missing_field(self):
        broken = source("e")
        del broken["sha256"]
        (self.root / "sorgenti" / "0.8.2-RT5.json").write_text(
            json.dumps({"format": 1, "sources": [broken]}))
        self.assertIn("missing sha256", sorgenti.problemi(self.root)[0])

    def test_closing_collects_and_empties_the_folder(self):
        moved = sorgenti.raccogli(self.root)
        self.assertEqual([p.name for p in moved], ["0.8.2-RT10.json", "0.8.2-RT3.json"])
        self.assertEqual(list((self.root / "sorgenti").glob("*.json")), [])
        self.assertEqual([s["name"] for s in sorgenti.leggi(self.root)], ["a", "b", "d", "c"])

    def test_closing_refuses_duplicates(self):
        write(self.root / "sorgenti" / "0.8.2-RT4.json", "a")
        with self.assertRaises(ValueError):
            sorgenti.raccogli(self.root)
        self.assertTrue((self.root / "sorgenti" / "0.8.2-RT4.json").exists())

    def test_the_repository_is_valid(self):
        self.assertEqual(sorgenti.problemi(), [])


if __name__ == "__main__":
    unittest.main()
