# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Tests of scripts/icone.py (docs/tappe/0.7.5/P11-icone-a-ruoli.md), and the
# check of every icon of resources/icons/: an icon that breaks the rules of
# docs/ICONE.md or its declared geometry fails the CI job "Scripts (Python)".
#
#     python3 -m unittest discover -s scripts -p '*_test.py'

from __future__ import annotations

import io
import math
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import icone  # noqa: E402

ICONS = Path(__file__).resolve().parent.parent / "resources" / "icons"

HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The Tecnigrafo authors -->
"""


def icon32(body: str) -> bytes:
    """A 32 px icon whose group carries the stroke of the grid."""
    return (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">\n'
            '<g fill="none" stroke="currentColor" stroke-width="1.75" stroke-linecap="round" '
            'stroke-linejoin="round">\n' + body + "\n</g>\n</svg>\n").encode()


def dot(x: float, y: float, extra: str = "") -> str:
    return f'<circle class="pt" cx="{x}" cy="{y}" r="2" fill="currentColor" stroke="none" {extra}/>'


# An arc through three points, centre (16, 16), radius 10, from (6, 16) to
# (26, 16) over the top: (16, 6) is on it.
ARC = '<path id="arc" class="geo" d="M6 16 A10 10 0 0 1 26 16"/>'


def messages(data: bytes, name: str = "test-32.svg") -> list[str]:
    return [p.message for p in icone.check_data(data, name)]


class CorrectIcons(unittest.TestCase):
    def test_correct_icon_has_no_problems(self):
        body = (ARC + '\n<line class="cost" x1="16" y1="16" x2="16" y2="6"/>\n'
                + dot(6, 16, 'data-on="arc"') + "\n" + dot(16, 6, 'data-on="arc"') + "\n"
                + dot(26, 16, 'data-on="arc"') + "\n" + dot(16, 16, 'data-center-of="arc"'))
        self.assertEqual(messages(icon32(body)), [])

    def test_the_example_of_icone_md_is_correct(self):
        for name in ("draw-circle-cr-32.svg", "draw-circle-cr-16.svg"):
            with self.subTest(name=name):
                self.assertEqual(icone.check_file(ICONS / name), [])

    def test_relative_commands_and_curves_are_read(self):
        body = ('<path id="p" class="esist" d="m6 6 h10 v4 c0 4 4 4 4 8 q0 4 4 4 t2 2 z"/>\n'
                + dot(11, 6, 'data-on="p"'))
        self.assertEqual(messages(icon32(body)), [])

    def test_glued_arc_flags_are_read(self):
        body = ('<path id="arc" class="geo" d="M6 16A10 10 0 0126 16"/>\n'
                + dot(16, 6, 'data-on="arc"'))
        self.assertEqual(messages(icon32(body)), [])

    def test_role_on_the_group_is_inherited(self):
        data = (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">'
                '<g class="geo" fill="none" stroke="currentColor" stroke-width="1.25" '
                'stroke-linecap="round" stroke-linejoin="round"><path d="M3 3 L13 13"/></g></svg>').encode()
        self.assertEqual(messages(data, "test-16.svg"), [])


class GeometryErrors(unittest.TestCase):
    def test_point_not_on_the_declared_arc(self):
        # The error of the trial of #483: the middle point of a three-point
        # arc does not lie on the arc.
        body = ARC + "\n" + dot(16, 9, 'data-on="arc"')
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("is 3 px from 'arc'", problems[0])

    def test_point_beyond_the_ends_of_the_arc(self):
        # (16, 26) is on the circle of the arc, not on the arc.
        problems = messages(icon32(ARC + "\n" + dot(16, 26, 'data-on="arc"')))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("from 'arc'", problems[0])

    def test_centre_at_a_distance_other_than_the_radius(self):
        # The other error of #483: the declared centre is 13 from the ends of
        # an arc of radius 10.
        problems = messages(icon32(ARC + "\n" + dot(16, 28, 'data-center-of="arc"')))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("is not the centre (16, 16) of 'arc' (radius 10", problems[0])
        self.assertIn(f"its ends are {icone.fmt(math.hypot(10, 12))}", problems[0])

    def test_centre_of_a_circle(self):
        body = '<circle id="c" class="geo" cx="16" cy="16" r="9"/>\n' + dot(16.05, 16, 'data-center-of="c"')
        self.assertEqual(messages(icon32(body)), [])
        body = '<circle id="c" class="geo" cx="16" cy="16" r="9"/>\n' + dot(16.2, 16, 'data-center-of="c"')
        self.assertEqual(len(messages(icon32(body))), 1)

    def test_centre_of_a_segment_is_refused(self):
        body = '<line id="l" class="esist" x1="6" y1="6" x2="20" y2="6"/>\n' + dot(13, 6, 'data-center-of="l"')
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("has no centre", problems[0])

    def test_missing_id(self):
        problems = messages(icon32(ARC + "\n" + dot(16, 6, 'data-on="arco"')))
        self.assertEqual(problems, ['data-on="arco": no element has that id'])

    def test_declaration_only_on_a_point(self):
        body = ARC + '\n<line class="cost" x1="16" y1="16" x2="16" y2="6" data-on="arc"/>'
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("is declared on a point only", problems[0])


class RuleErrors(unittest.TestCase):
    def test_written_colour(self):
        body = '<path class="geo" d="M6 6 L26 26" stroke="#1971c2"/>'
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn('stroke="#1971c2": a colour is written', problems[0])

    def test_element_not_allowed(self):
        body = '<path class="geo" d="M6 6 L26 26"/>\n<text class="esist" x="8" y="8">A</text>'
        problems = messages(icon32(body))
        self.assertEqual(problems, ["element <text> is not allowed"])

    def test_problem_has_the_line_of_the_element(self):
        body = '<path class="geo" d="M6 6 L26 26"/>\n<ellipse class="geo" cx="16" cy="16" rx="4" ry="2"/>'
        problems = icone.check_data(icon32(body), "test-32.svg")
        self.assertEqual([p.line for p in problems], [7])

    def test_grid_comes_from_the_name(self):
        problems = messages(icon32('<path class="geo" d="M6 6 L26 26"/>'), "test-16.svg")
        self.assertTrue(any("the 16 px grid is" in p for p in problems), problems)
        self.assertTrue(any("the stroke of the 16 px grid is 1.25" in p for p in problems), problems)

    def test_roles(self):
        problems = messages(icon32('<path d="M6 6 L26 26"/>\n<path class="accent" d="M6 26 L26 6"/>'))
        self.assertTrue(any("no role class" in p for p in problems), problems)
        self.assertTrue(any('"accent" is gone' in p for p in problems), problems)
        problems = messages(icon32('<path class="geo pt" d="M6 6 L26 26"/>'))
        self.assertEqual(problems, ["an element has one role only"])

    def test_margin(self):
        problems = messages(icon32('<path class="geo" d="M2 16 L26 16"/>'))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("ink in the 2 px margin", problems[0])

    def test_filled_element_that_is_not_a_point(self):
        body = '<rect class="geo" x="8" y="8" width="16" height="16" fill="currentColor"/>'
        problems = messages(icon32(body))
        self.assertTrue(any("only points are filled" in p for p in problems), problems)

    def test_dashes_too_short(self):
        body = '<path class="cost" d="M6 16 L26 16" stroke-dasharray="2 2"/>'
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("at least 3.5", problems[0])

    def test_unset_fill_defaults_to_black(self):
        data = (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">'
                '<path class="geo" d="M6 6 L26 26"/></svg>').encode()
        problems = messages(data)
        self.assertTrue(any("fill is not set" in p for p in problems), problems)

    def test_old_grid_keeps_its_rules(self):
        data = (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">'
                '<g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" '
                'stroke-linejoin="round"><path d="M5 19 L19 5"/>'
                '<circle class="accent" cx="5" cy="19" r="1.5" fill="currentColor" stroke="none"/>'
                '</g></svg>').encode()
        self.assertEqual(messages(data, "old.svg"), [])
        self.assertEqual(len(messages(data.replace(b"accent", b"geo"), "old.svg")), 1)

    def test_licence_and_xml(self):
        self.assertIn("no SPDX-License-Identifier comment",
                      messages(icon32('<path class="geo" d="M6 6 L26 26"/>').replace(b"SPDX", b"SPDY")))
        self.assertTrue(messages(b"<svg><g></svg>")[0].startswith("not well-formed XML"))


class CommandLine(unittest.TestCase):
    def run_main(self, argv: list[str]) -> tuple[int, str]:
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            status = icone.main(argv)
        return status, out.getvalue()

    def test_reports_file_and_line_and_fails(self):
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder) / "good-32.svg").write_bytes(icon32('<path class="geo" d="M6 6 L26 26"/>'))
            (Path(folder) / "bad-32.svg").write_bytes(icon32('<path class="geo" d="M6 6 L26 26" fill="red"/>'))
            status, out = self.run_main([folder])
        self.assertEqual(status, 1)
        self.assertIn("bad-32.svg:6: fill=\"red\"", out)
        self.assertNotIn("good-32.svg", out)

    def test_every_icon_of_the_project_passes(self):
        # The check of the CI: resources/icons/ has no problems.
        status, out = self.run_main([str(ICONS)])
        self.assertEqual(out, "")
        self.assertEqual(status, 0)


if __name__ == "__main__":
    unittest.main()
