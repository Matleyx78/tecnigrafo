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


def icon32(body: str, stroke: str = "1") -> bytes:
    """A 32 px icon whose group carries the stroke of the grid."""
    return (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">\n'
            f'<g fill="none" stroke="currentColor" stroke-width="{stroke}" stroke-linecap="round" '
            'stroke-linejoin="round">\n' + body + "\n</g>\n</svg>\n").encode()


def dot(x: float, y: float, extra: str = "") -> str:
    return f'<circle class="pt" cx="{x}" cy="{y}" r="2.5" fill="currentColor" stroke="none" {extra}/>'


# An arc through three points, centre (15.5, 15.5), radius 10, from (5.5,
# 15.5) to (25.5, 15.5) over the top: (15.5, 5.5) is on it. Centre, radius and
# points are on the pixels (docs/tappe/0.7.5/P40, point 4).
ARC = '<path id="arc" class="geo" d="M5.5 15.5 A10 10 0 0 1 25.5 15.5"/>'


def messages(data: bytes, name: str = "test-32.svg") -> list[str]:
    return [p.message for p in icone.check_data(data, name)]


class CorrectIcons(unittest.TestCase):
    def test_correct_icon_has_no_problems(self):
        body = (ARC + '\n<line class="cost" x1="15.5" y1="15.5" x2="15.5" y2="5.5"/>\n'
                + dot(5.5, 15.5, 'data-on="arc"') + "\n" + dot(15.5, 5.5, 'data-on="arc"') + "\n"
                + dot(25.5, 15.5, 'data-on="arc"') + "\n" + dot(15.5, 15.5, 'data-center-of="arc"'))
        self.assertEqual(messages(icon32(body)), [])

    def test_the_example_of_icone_md_is_correct(self):
        for name in ("draw-circle-cr-32.svg", "draw-circle-cr-16.svg"):
            with self.subTest(name=name):
                self.assertEqual(icone.check_file(ICONS / name), [])

    def test_relative_commands_and_curves_are_read(self):
        body = ('<path id="p" class="esist" d="m6.5 6.5 h10 v4 c0 4 4 4 4 8 q0 4 4 4 t2 2 z"/>\n'
                + dot(11.5, 6.5, 'data-on="p"'))
        self.assertEqual(messages(icon32(body)), [])

    def test_glued_arc_flags_are_read(self):
        body = ('<path id="arc" class="geo" d="M5.5 15.5A10 10 0 0125.5 15.5"/>\n'
                + dot(15.5, 5.5, 'data-on="arc"'))
        self.assertEqual(messages(icon32(body)), [])

    def test_role_on_the_group_is_inherited(self):
        data = (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">'
                '<g class="geo" fill="none" stroke="currentColor" stroke-width="1" '
                'stroke-linecap="round" stroke-linejoin="round"><path d="M3 3 L13 13"/></g></svg>').encode()
        self.assertEqual(messages(data, "test-16.svg"), [])


class GeometryErrors(unittest.TestCase):
    def test_point_not_on_the_declared_arc(self):
        # The error of the trial of #483: the middle point of a three-point
        # arc does not lie on the arc.
        body = ARC + "\n" + dot(15.5, 8.5, 'data-on="arc"')
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("is 3 px from 'arc'", problems[0])

    def test_point_beyond_the_ends_of_the_arc(self):
        # (16, 26) is on the circle of the arc, not on the arc.
        problems = messages(icon32(ARC + "\n" + dot(15.5, 25.5, 'data-on="arc"')))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("from 'arc'", problems[0])

    def test_centre_at_a_distance_other_than_the_radius(self):
        # The other error of #483: the declared centre is 13 from the ends of
        # an arc of radius 10.
        problems = messages(icon32(ARC + "\n" + dot(15.5, 27.5, 'data-center-of="arc"')))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("is not the centre (15.5, 15.5) of 'arc' (radius 10", problems[0])
        self.assertIn(f"its ends are {icone.fmt(math.hypot(10, 12))}", problems[0])

    def test_centre_of_a_circle(self):
        # The centre of a point off the middle of a pixel is also reported by
        # the alignment; here only the declaration counts.
        def centre_problems(x: float) -> list[str]:
            body = '<circle id="c" class="geo" cx="15.5" cy="15.5" r="9"/>\n' + dot(x, 15.5, 'data-center-of="c"')
            return [p for p in messages(icon32(body)) if "is not the centre" in p]

        self.assertEqual(centre_problems(15.55), [])
        self.assertEqual(len(centre_problems(15.7)), 1)

    def test_centre_of_a_segment_is_refused(self):
        body = '<line id="l" class="esist" x1="6.5" y1="6.5" x2="20.5" y2="6.5"/>\n' + dot(13.5, 6.5, 'data-center-of="l"')
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("has no centre", problems[0])

    def test_missing_id(self):
        problems = messages(icon32(ARC + "\n" + dot(15.5, 5.5, 'data-on="arco"')))
        self.assertEqual(problems, ['data-on="arco": no element has that id'])

    def test_declaration_only_on_a_point(self):
        body = ARC + '\n<line class="cost" x1="15.5" y1="15.5" x2="15.5" y2="5.5" data-on="arc"/>'
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
        problems = messages(icon32('<path class="geo" d="M6 6 L26 26"/>\n' + dot(10.5, 10.5)), "test-16.svg")
        self.assertTrue(any("the 16 px grid is" in p for p in problems), problems)
        self.assertTrue(any('circles with r="1.5"' in p for p in problems), problems)

    def test_roles(self):
        problems = messages(icon32('<path d="M6 6 L26 26"/>\n<path class="accent" d="M6 26 L26 6"/>'))
        self.assertTrue(any("no role class" in p for p in problems), problems)
        self.assertTrue(any('"accent" is gone' in p for p in problems), problems)
        problems = messages(icon32('<path class="geo pt" d="M6 6 L26 26"/>'))
        self.assertEqual(problems, ["an element has one role only"])

    def test_margin(self):
        problems = messages(icon32('<path class="geo" d="M1.5 15.5 L26 15.5"/>'))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("ink in the 2 px margin", problems[0])

    def test_filled_element_that_is_not_a_point(self):
        body = '<rect class="geo" x="8" y="8" width="16" height="16" fill="currentColor"/>'
        problems = messages(icon32(body))
        self.assertTrue(any("only points are filled" in p for p in problems), problems)

    def test_dashes_too_short(self):
        body = '<path class="cost" d="M6 15.5 L26 15.5" stroke-dasharray="1.5 2"/>'
        problems = messages(icon32(body))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("at least 2", problems[0])
        self.assertEqual(messages(icon32(body.replace("1.5 2", "2 2"))), [])

    def test_unset_fill_defaults_to_black(self):
        data = (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32">'
                '<path class="geo" d="M6 6 L26 26"/></svg>').encode()
        problems = messages(data)
        self.assertTrue(any("fill is not set" in p for p in problems), problems)

    def test_old_grid_is_rejected(self):
        data = (HEADER + '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">'
                '<g fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" '
                'stroke-linejoin="round"><path d="M5 19 L19 5"/>'
                '<circle class="accent" cx="5" cy="19" r="1.5" fill="currentColor" stroke="none"/>'
                '</g></svg>').encode()
        for name in ("old.svg", "old-24.svg"):
            with self.subTest(name=name):
                problems = messages(data, name)
                self.assertEqual(len(problems), 1, problems)
                self.assertIn("unsupported icon name", problems[0])
        for name in ("old-16.svg", "old-32.svg"):
            with self.subTest(name=name):
                self.assertTrue(any("px grid is" in p for p in messages(data, name)))

    def test_missing_grid_suffix_is_rejected_even_for_valid_role_svg(self):
        problems = messages(icon32('<path class="geo" d="M6 6 L26 26"/>'), "test.svg")
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("unsupported icon name", problems[0])

    def test_licence_and_xml(self):
        self.assertIn("no SPDX-License-Identifier comment",
                      messages(icon32('<path class="geo" d="M6 6 L26 26"/>').replace(b"SPDX", b"SPDY")))
        self.assertTrue(messages(b"<svg><g></svg>")[0].startswith("not well-formed XML"))


class Alignment(unittest.TestCase):
    """The 1 px stroke on the pixels (docs/tappe/0.7.5/P40, point 4)."""

    def test_horizontal_line_on_a_whole_coordinate_is_reported(self):
        problems = messages(icon32('<line id="l" class="geo" x1="6" y1="16" x2="26" y2="16"/>'))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("'l': the horizontal segment at y=16 is not on the middle of a pixel", problems[0])

    def test_horizontal_line_on_a_half_pixel_is_not(self):
        self.assertEqual(messages(icon32('<line class="geo" x1="6" y1="16.5" x2="26" y2="16.5"/>')), [])
        self.assertEqual(messages(icon32('<path class="geo" d="M6 16.5 H26.5 V24"/>')), [])

    def test_vertical_sides_of_paths_polylines_and_rectangles(self):
        problems = messages(icon32('<path class="geo" d="M6.5 6.5 H20.5 V20"/>'))
        self.assertEqual(problems, [])  # V20 ends at 20, but its x is 20.5
        problems = messages(icon32('<path class="geo" d="M6.5 6.5 H20 V20.5"/>'))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("the vertical segment at x=20", problems[0])
        problems = messages(icon32('<polyline class="geo" points="6.5,6.5 6.5,20 20,20"/>'))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("the horizontal segment at y=20", problems[0])
        problems = messages(icon32('<rect class="esist" x="6.5" y="6.5" width="9.5" height="9.5"/>'))
        self.assertEqual(len(problems), 2, problems)
        self.assertEqual(messages(icon32('<rect class="esist" x="6.5" y="6.5" width="10" height="9"/>')), [])

    def test_diagonals_and_curves_are_not_aligned(self):
        body = '<path class="geo" d="M6 6 L26 25 C20 20 10 20 6.25 6.1"/>'
        self.assertEqual(messages(icon32(body)), [])

    def test_point_off_the_middle_of_a_pixel_is_reported(self):
        problems = messages(icon32('<line id="l" class="geo" x1="6.5" y1="16.5" x2="26.5" y2="16.5"/>\n'
                                   + dot(16, 16.5, 'data-on="l"')))
        self.assertEqual(len(problems), 1, problems)
        self.assertIn("the centre of the point (16, 16.5) at x=16 is not on the middle of a pixel", problems[0])
        self.assertEqual(messages(icon32(dot(16.5, 16.5))), [])

    def test_circle_whose_centre_plus_radius_is_whole_is_reported(self):
        problems = messages(icon32('<circle id="c" class="geo" cx="16" cy="16" r="10"/>'))
        self.assertEqual(len(problems), 4, problems)
        self.assertIn("'c': the circle (centre (16, 16), radius 10) at x=6", problems[0])
        self.assertEqual(messages(icon32('<circle class="geo" cx="16" cy="16" r="10.5"/>')), [])
        self.assertEqual(messages(icon32('<circle class="geo" cx="15.5" cy="15.5" r="10"/>')), [])

    def test_arc_whose_centre_plus_radius_is_whole_is_reported(self):
        problems = messages(icon32('<path class="geo" d="M6 16 A10 10 0 0 1 26 16"/>'))
        self.assertEqual(len(problems), 4, problems)
        self.assertTrue(all("the arc (centre (16, 16), radius 10)" in p for p in problems), problems)

    def test_fill_without_stroke_has_its_sides_on_whole_pixels(self):
        # Only points are filled (docs/ICONE.md), so the rule is checked on its own:
        # x = 6 and x = 10 are whole, y = 6.5 and y = 10.5 are not.
        checker = icone.Checker(icone.GRIDS[32])
        rect = icone.Element("rect", {"x": "6", "y": "6.5", "width": "4", "height": "4"}, 7)
        checker.fill_alignment(rect, icone.pieces_of(rect))
        self.assertEqual(len(checker.problems), 2, checker.problems)
        self.assertTrue(all("the horizontal side" in p.message and "a whole pixel" in p.message
                            for p in checker.problems))

    def test_stroke_of_one_and_a_quarter_is_reported(self):
        problems = messages(icon32('<path class="geo" d="M6 6 L26 26"/>', stroke="1.25"))
        self.assertTrue(any('stroke-width="1.25": the stroke of the 32 px grid is 1' in p for p in problems),
                        problems)


class CommandLine(unittest.TestCase):
    def run_main(self, argv: list[str]) -> tuple[int, str]:
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            status = icone.main(argv)
        return status, out.getvalue()

    def test_legacy_filename_fails_the_command(self):
        with tempfile.TemporaryDirectory() as folder:
            (Path(folder) / "old.svg").write_bytes(icon32('<path class="geo" d="M6 6 L26 26"/>'))
            status, out = self.run_main([folder])
        self.assertEqual(status, 1)
        self.assertIn("old.svg:4: unsupported icon name", out)

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
