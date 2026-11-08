# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/confronta_sistemi.py, on artifacts written in temporary
# folders (docs/tappe/0.7.5/P5-confronto-e-controllo-incrociato.md).
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import io
import re
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))

import confronta_sistemi as c  # noqa: E402

REPORT = (b"tecnigrafo-report 1\n\n[final]\nentities:\n"
          b"  257 LINE start=0,0 end=10,5\nprompt: Command:\n")
QUESTIONS = b"L = LINE start=0,0 end=3,4\n? length L -> 5\n"
SYSTEMS = ("verifica-linux-release", "verifica-windows", "verifica-macos")

NO_EXCEPTIONS = "# Sistemi\n\n## Eccezioni\n\nNessuna.\n"


class ConfrontaSistemiTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.exceptions = self.root / "sistemi.md"
        self.exceptions.write_text(NO_EXCEPTIONS, encoding="utf-8")
        for system in SYSTEMS:
            self.write(system, "line/base", "ottenuto.txt", REPORT)
            self.write(system, "modello/lunghezza", "ottenuto.tgq", QUESTIONS)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write(self, system: str, case: str, name: str, content: bytes) -> None:
        folder = self.root / system / "tests" / "verifica" / "casi" / case
        (folder / name).parent.mkdir(parents=True, exist_ok=True)
        (folder / name).write_bytes(content)

    def run_script(self, *extra: str, environment: dict[str, str] | None = None) -> tuple[int, str]:
        output = io.StringIO()
        arguments = ["--eccezioni", str(self.exceptions), *extra,
                     *(str(self.root / system) for system in SYSTEMS)]
        with mock.patch.dict("os.environ", environment or {}, clear=False), \
                redirect_stdout(output), redirect_stderr(output):
            status = c.main(arguments)
        return status, output.getvalue()

    def test_equal_reports(self) -> None:
        status, output = self.run_script(environment={"GITHUB_ACTIONS": "false"})
        self.assertEqual(status, 0, output)
        self.assertIn("OK: 2 cases equal", output)
        self.assertNotIn("::", output)

    def test_files_of_the_scripts_and_images_are_not_compared(self) -> None:
        self.write(SYSTEMS[1], "line/base", "file/plan.dxf", b"different")
        self.write(SYSTEMS[2], "line/base", "immagine.png", b"different")
        status, output = self.run_script()
        self.assertEqual(status, 0, output)

    def test_a_different_line_names_the_case_and_the_line(self) -> None:
        self.write(SYSTEMS[1], "line/base", "ottenuto.txt",
                   REPORT.replace(b"end=10,5", b"end=10.000000000000002,5"))
        status, output = self.run_script(environment={"GITHUB_ACTIONS": "true"})
        self.assertEqual(status, 1, output)
        self.assertIn("error: line/base: ottenuto.txt, line 5 differs:", output)
        self.assertRegex(output, r"verifica-linux-release\s+  257 LINE start=0,0 end=10,5\n")
        self.assertRegex(output, r"verifica-windows\s+  257 LINE start=0,0 end=10\.000000000000002,5\n")
        self.assertIn("::error title=Cross-platform comparison::line/base: ottenuto.txt, line 5 differs:%0A", output)
        self.assertNotIn("modello/lunghezza", output)

    def test_a_carriage_return_is_a_difference(self) -> None:
        self.write(SYSTEMS[1], "modello/lunghezza", "ottenuto.tgq", QUESTIONS.replace(b"\n", b"\r\n"))
        status, output = self.run_script()
        self.assertEqual(status, 1, output)
        self.assertIn("modello/lunghezza: ottenuto.tgq, line 1 differs:", output)
        self.assertIn("L = LINE start=0,0 end=3,4\\u{0D}", output)

    def test_a_shorter_report_shows_the_missing_line(self) -> None:
        self.write(SYSTEMS[2], "line/base", "ottenuto.txt", REPORT.rsplit(b"prompt", 1)[0])
        status, output = self.run_script()
        self.assertEqual(status, 1, output)
        self.assertIn("line/base: ottenuto.txt, line 6 differs:", output)
        self.assertRegex(output, r"verifica-macos\s+\(missing\)")

    def test_many_different_lines_are_cut(self) -> None:
        many = b"".join(b"line %d\n" % index for index in range(30))
        other = b"".join(b"other %d\n" % index for index in range(30))
        self.write(SYSTEMS[0], "line/lunga", "ottenuto.txt", many)
        self.write(SYSTEMS[1], "line/lunga", "ottenuto.txt", other)
        self.write(SYSTEMS[2], "line/lunga", "ottenuto.txt", many)
        status, output = self.run_script()
        self.assertEqual(status, 1, output)
        self.assertEqual(output.count("line/lunga: ottenuto.txt, line"), c.MAX_LINES_PER_REPORT)
        self.assertIn("line/lunga: ottenuto.txt: 10 more differing lines", output)

    def test_a_case_missing_on_a_system(self) -> None:
        for system in SYSTEMS[:2]:
            self.write(system, "arc/nuovo", "ottenuto.txt", REPORT)
        status, output = self.run_script()
        self.assertEqual(status, 1, output)
        self.assertIn("error: arc/nuovo: missing on verifica-macos", output)

    def test_a_case_without_reports_everywhere_is_equal(self) -> None:
        for system in SYSTEMS:
            (self.root / system / "tests" / "verifica" / "casi" / "interfaccia" / "livello-2").mkdir(parents=True)
        status, output = self.run_script()
        self.assertEqual(status, 0, output)
        self.assertIn("OK: 3 cases equal", output)

    def test_a_report_missing_on_a_system(self) -> None:
        self.write(SYSTEMS[0], "line/base", "ottenuto-confine.txt", REPORT)
        status, output = self.run_script()
        self.assertEqual(status, 1, output)
        self.assertIn("error: line/base: ottenuto-confine.txt missing on verifica-windows, verifica-macos",
                      output)

    def test_a_case_in_the_exceptions_is_skipped_with_a_warning(self) -> None:
        self.write(SYSTEMS[1], "line/base", "ottenuto.txt", REPORT.replace(b"10,5", b"10,6"))
        self.exceptions.write_text(
            "# Sistemi\n\n## Eccezioni\n\n"
            "- `line/base`: the last digit of atan2 differs on MSVC (#999)\n\n"
            "## Altro\n\n- not an exception\n", encoding="utf-8")
        status, output = self.run_script(environment={"GITHUB_ACTIONS": "true"})
        self.assertEqual(status, 0, output)
        self.assertIn("warning: line/base: not compared, it is an exception: "
                      "the last digit of atan2 differs on MSVC (#999)", output)
        self.assertIn("::warning title=Cross-platform comparison::line/base:", output)
        self.assertIn("OK: 1 cases equal", output)

    def test_an_exception_no_system_has_is_an_error(self) -> None:
        self.exceptions.write_text("## Eccezioni\n\n- `line/sparito`: gone\n", encoding="utf-8")
        status, output = self.run_script()
        self.assertEqual(status, 1, output)
        self.assertIn("line/sparito: listed in the exceptions, but no system has it", output)

    def test_variable_lines_are_ignored(self) -> None:
        self.write(SYSTEMS[1], "line/base", "ottenuto.txt", REPORT + b"time: 12 ms\n")
        self.write(SYSTEMS[2], "line/base", "ottenuto.txt", REPORT + b"time: 7 ms\n")
        self.write(SYSTEMS[0], "line/base", "ottenuto.txt", REPORT + b"time: 3 ms\n")
        with mock.patch.object(c, "VARIABLE_LINES", [re.compile(rb"time: \d+ ms")]):
            status, output = self.run_script()
        self.assertEqual(status, 0, output)

    def test_invalid_inputs(self) -> None:
        self.exceptions.write_text("## Eccezioni\n\n- line/base senza apici\n", encoding="utf-8")
        status, output = self.run_script()
        self.assertEqual(status, 2, output)
        self.assertIn("line 3: an exception is", output)

        self.exceptions.write_text("# Sistemi\n", encoding="utf-8")
        status, output = self.run_script()
        self.assertEqual(status, 2, output)
        self.assertIn("no section", output)

        self.exceptions.write_text(NO_EXCEPTIONS, encoding="utf-8")
        (self.root / "vuoto" / "tests" / "verifica" / "casi").mkdir(parents=True)
        output = io.StringIO()
        with redirect_stdout(output), redirect_stderr(output):
            status = c.main(["--eccezioni", str(self.exceptions), str(self.root / SYSTEMS[0]),
                             str(self.root / "vuoto")])
        self.assertEqual(status, 2, output.getvalue())
        self.assertIn("vuoto: no case", output.getvalue())

    def test_the_list_of_the_repository_is_valid(self) -> None:
        self.assertEqual(c.read_exceptions(c.DEFAULT_EXCEPTIONS), {})


if __name__ == "__main__":
    unittest.main()
