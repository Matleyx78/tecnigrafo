#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Compares the reports of the verification cases written by the Release jobs
# of Linux, Windows and macOS (docs/adr/0038, point 5;
# docs/tappe/0.7.5/P5-confronto-e-controllo-incrociato.md;
# docs/verifica/sistemi.md).
#
#     scripts/confronta_sistemi.py [--eccezioni FILE] DIR DIR [DIR ...]
#
# Every DIR is the root of the verification artifact of one system, as
# actions/download-artifact writes it; the name of the folder is the name of
# the system in the messages (verifica-linux-release, verifica-windows, ...).
# Under every DIR, tests/verifica/casi/<area>/<name>/ is the output folder of
# the level 1 case <area>/<name>, and its reports are the files
# ottenuto.txt (script), ottenuto.tgq (questions) and ottenuto-confine.txt
# (pending script, up to its boundary). Images (casi2/), the files written by
# the scripts (file/) and the logs of ctest are not compared.
#
# For every case the reports must exist on every system, with the same names
# (a level 2 case writes none at level 1, on every system), and be equal line
# by line, byte for byte (a CR at the end of a line is a
# difference), except the lines matched by VARIABLE_LINES below, which change
# by construction and are not results. A case listed in the section
# "Eccezioni" of docs/verifica/sistemi.md is skipped, with a warning that
# names it.
#
# Exit status: 0 every compared case is equal on every system, 1 at least one
# difference or missing report, 2 wrong arguments, a folder without cases, or
# an invalid list of exceptions.

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Lines of a report that change by construction and are not results: none
# today. The report has no paths and no times (docs/verifica/rapporto.md,
# "Struttura"): file names are written as the script wrote them, relative to
# the folder of the case. A pattern added here is matched against the whole
# line (re.fullmatch), and docs/verifica/sistemi.md explains it.
VARIABLE_LINES: list[re.Pattern[bytes]] = []

# The folder of the level 1 cases inside an artifact, and the report files.
CASES_FOLDER = Path("tests") / "verifica" / "casi"
REPORT_NAMES = ("ottenuto.txt", "ottenuto.tgq", "ottenuto-confine.txt")

# At most this many differing lines are written for one report.
MAX_LINES_PER_REPORT = 20

DEFAULT_EXCEPTIONS = Path(__file__).resolve().parent.parent / "docs" / "verifica" / "sistemi.md"
EXCEPTIONS_HEADING = "## Eccezioni"
EXCEPTION_LINE = re.compile(r"- `([a-z0-9][a-z0-9-]*/[a-z0-9][a-z0-9-]*)`: (\S.*)")


class InputError(Exception):
    """Wrong arguments, a folder without cases or an invalid exceptions file."""


@dataclass
class System:
    name: str
    # case -> report name -> content
    cases: dict[str, dict[str, bytes]] = field(default_factory=dict)


def read_exceptions(path: Path) -> dict[str, str]:
    """The cases of the section "Eccezioni" of `path`, with their reason.

    Inside the section every line starting with "- " must be
    "- `<area>/<name>`: <reason>"; other lines are free text.
    """
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as error:
        raise InputError(f"{path}: {error.strerror}") from error
    exceptions: dict[str, str] = {}
    inside = False
    found = False
    for number, line in enumerate(lines, start=1):
        if line.startswith("## "):
            inside = line.rstrip() == EXCEPTIONS_HEADING
            found = found or inside
            continue
        if line.startswith("# "):
            inside = False
            continue
        if not inside or not line.startswith("- "):
            continue
        match = EXCEPTION_LINE.fullmatch(line.rstrip())
        if not match:
            raise InputError(f"{path}, line {number}: an exception is \"- `<area>/<name>`: <reason>\"")
        case = match.group(1)
        if case in exceptions:
            raise InputError(f"{path}, line {number}: {case} is listed twice")
        exceptions[case] = match.group(2)
    if not found:
        raise InputError(f"{path}: no section \"{EXCEPTIONS_HEADING}\"")
    return exceptions


def read_system(root: Path) -> System:
    """The reports of every case under `root`."""
    system = System(root.name)
    folder = root / CASES_FOLDER
    if not folder.is_dir():
        raise InputError(f"{root}: no folder {CASES_FOLDER.as_posix()}")
    for area in sorted(folder.iterdir()):
        if not area.is_dir():
            continue
        for case in sorted(area.iterdir()):
            if not case.is_dir():
                continue
            reports = {}
            for name in REPORT_NAMES:
                path = case / name
                if path.is_file():
                    reports[name] = path.read_bytes()
            system.cases[f"{area.name}/{case.name}"] = reports
    if not system.cases:
        raise InputError(f"{root}: no case in {CASES_FOLDER.as_posix()}")
    return system


def is_variable(line: bytes) -> bool:
    return any(pattern.fullmatch(line) for pattern in VARIABLE_LINES)


def show(line: bytes | None) -> str:
    """A line of a report as it is written in a message."""
    if line is None:
        return "(missing)"
    text = line.decode("utf-8", errors="backslashreplace")
    # A trailing CR is the most likely difference between systems: make it
    # visible, as any other control character.
    return "".join(ch if ch.isprintable() or ch == " " else f"\\u{{{ord(ch):02X}}}" for ch in text)


def split_lines(content: bytes) -> list[bytes]:
    """The lines of a report, without the "\\n" that ends them."""
    lines = content.split(b"\n")
    if lines and lines[-1] == b"":
        lines.pop()
    return lines


def compare_report(case: str, name: str, systems: list[System]) -> list[str]:
    """The differences of one report of one case between the systems."""
    contents = [split_lines(system.cases[case][name]) for system in systems]
    errors = []
    differing = 0
    for index in range(max(len(lines) for lines in contents)):
        row = [lines[index] if index < len(lines) else None for lines in contents]
        if all(line == row[0] for line in row):
            continue
        if all(line is not None and is_variable(line) for line in row):
            continue
        differing += 1
        if differing > MAX_LINES_PER_REPORT:
            continue
        text = [f"{case}: {name}, line {index + 1} differs:"]
        width = max(len(system.name) for system in systems)
        for system, line in zip(systems, row):
            text.append(f"  {system.name:<{width}}  {show(line)}")
        errors.append("\n".join(text))
    if differing > MAX_LINES_PER_REPORT:
        errors.append(f"{case}: {name}: {differing - MAX_LINES_PER_REPORT} more differing lines")
    return errors


def compare(systems: list[System], exceptions: dict[str, str]) -> tuple[list[str], list[str], int]:
    """The errors, the warnings and the number of compared cases."""
    errors: list[str] = []
    warnings: list[str] = []
    cases = sorted(set().union(*(system.cases for system in systems)))
    for case in sorted(set(exceptions) - set(cases)):
        errors.append(f"{case}: listed in the exceptions, but no system has it: remove it from the list")
    compared = 0
    for case in cases:
        if case in exceptions:
            warnings.append(f"{case}: not compared, it is an exception: {exceptions[case]}")
            continue
        missing = [system.name for system in systems if case not in system.cases]
        if missing:
            errors.append(f"{case}: missing on {', '.join(missing)}")
            continue
        compared += 1
        # A case with no report on any system is equal everywhere: a level 2
        # case (!level 2) is skipped at level 1 and writes no report.
        names = sorted(set().union(*(system.cases[case] for system in systems)))
        for name in names:
            missing = [system.name for system in systems if name not in system.cases[case]]
            if missing:
                errors.append(f"{case}: {name} missing on {', '.join(missing)}")
                continue
            errors.extend(compare_report(case, name, systems))
    return errors, warnings, compared


def annotate(kind: str, message: str) -> None:
    """A GitHub annotation, only inside GitHub Actions."""
    if os.environ.get("GITHUB_ACTIONS") != "true":
        return
    escaped = message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    print(f"::{kind} title=Cross-platform comparison::{escaped}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Compare the reports of the verification cases between systems.")
    parser.add_argument("--eccezioni", type=Path, default=DEFAULT_EXCEPTIONS,
                        help="list of the cases not compared (default: docs/verifica/sistemi.md)")
    parser.add_argument("folders", type=Path, nargs="+",
                        help="root of the verification artifact of every system")
    arguments = parser.parse_args(argv)
    try:
        if len(arguments.folders) < 2:
            raise InputError("at least two systems are needed")
        names = [folder.name for folder in arguments.folders]
        if len(set(names)) != len(names):
            raise InputError("two systems have the same folder name")
        exceptions = read_exceptions(arguments.eccezioni)
        systems = [read_system(folder) for folder in arguments.folders]
    except InputError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    errors, warnings, compared = compare(systems, exceptions)
    for warning in warnings:
        print(f"warning: {warning}")
        annotate("warning", warning)
    for error in errors:
        print(f"error: {error}")
    for error in errors[:10]:
        annotate("error", error)
    systems_text = ", ".join(system.name for system in systems)
    if errors:
        print(f"FAILED: {len(errors)} differences in {compared} cases between {systems_text}")
        return 1
    print(f"OK: {compared} cases equal on {systems_text}, {len(warnings)} exceptions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
