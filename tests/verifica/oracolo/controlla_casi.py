# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
"""Print oracle answers, check declared references and audit written drawings."""

import argparse
import io
import json
import math
from pathlib import Path
import re
import sys
import zipfile

import ezdxf
from ezdxf_attesi import DrawingAnswers


# Quoted text is indivisible: digits inside a text value are never approximate.
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|[A-Za-z_~][\w~-]*|[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?|[^\s]')
DEFINITION = re.compile(r"(\w+)\s*=\s*DXF\s+source=([a-z0-9-]+)\s*$")
QUESTION = re.compile(r"\?\s*(version|units|count|unhandled|entity)\s+(\w+)(?:\s+([0-9A-Fa-f]+))?\s*->\s*(.*)$")


def equal_tokens(expected, actual, absolute, relative):
    left, right = TOKEN.findall(expected), TOKEN.findall(actual)
    if len(left) != len(right):
        return False
    for a, b in zip(left, right):
        if a == b:
            continue
        try:
            x, y = float(a), float(b)
        except ValueError:
            return False
        if not math.isclose(x, y, abs_tol=absolute, rel_tol=relative):
            return False
    return True


def equal_answer(question, expected, actual, absolute, relative):
    if question in {"count", "unhandled"}:
        remaining = actual.split(";")
        for item in expected.split(";"):
            match = next((i for i, value in enumerate(remaining)
                          if equal_tokens(item, value, absolute, relative)), None)
            if match is None:
                return False
            remaining.pop(match)
        return not remaining
    if question == "entity":
        # Derived font geometry is deliberately outside the DXF oracle's scope.
        expected = " ".join(token for token in re.findall(r'[^\s"=]+=(?:"(?:\\.|[^"\\])*"|[^\s]+)|\S+', expected)
                            if not token.startswith("~"))
    return equal_tokens(expected, actual, absolute, relative)


def check_case(path, sources):
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    if not lines or lines[0].strip() != "; oracle: ezdxf":
        return 0, 0
    drawings = {}
    absolute, relative = 1e-9, 0
    checked = failures = 0
    print(f"=== Oracle case: {path.parent} ===")
    for line_number, line in enumerate(lines, 1):
        line = line.strip()
        if not line or line.startswith(";"):
            continue
        if line.startswith("!tolerance"):
            _, kind, value = line.split()
            value = float(value)
            if not math.isfinite(value) or value < 0:
                raise ValueError(f"{path}:{line_number}: invalid tolerance")
            if kind == "absolute":
                absolute = value
            elif kind == "relative":
                relative = value
            elif kind not in {"channel", "pixels"}:
                raise ValueError(f"{path}:{line_number}: unknown tolerance {kind}")
            continue
        definition = DEFINITION.fullmatch(line)
        if definition:
            name, source = definition.groups()
            if name in drawings:
                raise ValueError(f"{path}:{line_number}: duplicate drawing {name}")
            drawings[name] = DrawingAnswers(sources / source)
            continue
        match = QUESTION.fullmatch(line)
        if not match:
            print(f"{path}:{line_number}: skipped (outside oracle scope): {line}")
            continue
        question, name, handle, expected = match.groups()
        if name not in drawings:
            raise ValueError(f"{path}:{line_number}: no oracle source for {name}")
        if (question == "entity") != (handle is not None):
            raise ValueError(f"{path}:{line_number}: invalid oracle question: {line}")
        drawing = drawings[name]
        if question == "entity":
            record = drawing.entities.get(format(int(handle, 16), "X"))
            if record:
                entity, supported = record
                if (supported and entity.dxftype() != "LINE"
                        and tuple(entity.dxf.get("extrusion", (0, 0, 1))) == (0, 0, -1)):
                    print(f"{path}:{line_number}: skipped OCS conversion (ADR 0018, point 5)")
                    continue
        actual = drawing.answer(question, handle)
        checked += 1
        if not equal_answer(question, expected, actual, absolute, relative):
            failures += 1
            print(f"{path}:{line_number}: {line.split('->', 1)[0].strip()}\n"
                  f"  expected: {expected}\n  ezdxf:    {actual}")
    return checked, failures


def audit_outputs(outputs):
    count = failures = 0
    paths = sorted({p for root in outputs for p in root.rglob("*")
                    if p.is_file() and p.suffix.lower() in {".dxf", ".cadx"}})
    for path in paths:
        count += 1
        try:
            if path.suffix.lower() == ".cadx":
                with zipfile.ZipFile(path) as archive:
                    data = archive.read("drawing.dxf")
                # Native drawings contain a UTF-8 R2018 DXF (ADR 0019).
                drawing = ezdxf.read(io.StringIO(data.decode("utf-8-sig")))
            else:
                drawing = ezdxf.readfile(path)
            auditor = drawing.audit()
            # Repairs also reveal invalid output, even if no errors remain.
            problems = list(auditor.errors) + list(auditor.fixes)
            if problems:
                failures += 1
                for problem in problems:
                    print(f"{path}: audit {problem.code}: {problem.message}")
            else:
                print(f"{path}: audit OK")
        except (OSError, ValueError, KeyError, zipfile.BadZipFile, ezdxf.DXFError) as error:
            failures += 1
            print(f"{path}: audit failed: {error}")
    print(f"Audit: {count} drawings, {failures} failures.")
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cases", type=Path)
    parser.add_argument("sources", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--print-only", action="store_true",
                      help="print public source answers, including after a failed verification")
    mode.add_argument("--check-only", action="store_true",
                      help="check declared cases and audit output, without printing source answers")
    parser.add_argument("--outputs", type=Path, action="append",
                        help="case output root; repeatable; default: build/ci/tests/verifica/casi*")
    args = parser.parse_args()
    failures = 0
    if not args.check_only:
        manifest = args.cases.parent / "sorgenti.json"
        sources = json.loads(manifest.read_text(encoding="utf-8"))["sources"] if manifest.exists() else []
        if not sources:
            print("No public sources: nothing to print.")
        for source in sources:
            print(f"=== ezdxf: {source['name']} ({source['url']}) ===", flush=True)
            try:
                DrawingAnswers(args.sources / source["name"]).print_answers()
            except (OSError, ValueError, ezdxf.DXFError) as error:
                failures += 1
                print(f"{source['name']}: {error}")
    if not args.print_only:
        cases = questions = 0
        for path in sorted(args.cases.rglob("domande.tgq")):
            if path.read_text(encoding="utf-8-sig").splitlines()[:1] != ["; oracle: ezdxf"]:
                continue
            cases += 1
            try:
                checked, failed = check_case(path, args.sources)
                questions += checked
                failures += failed
            except (OSError, ValueError, ezdxf.DXFError) as error:
                failures += 1
                print(f"{path}: {error}")
        print(f"Oracle: {cases} declared cases, {questions} checked questions.")
        if not cases:
            print("No declared oracle cases: nothing to compare.")
        outputs = args.outputs
        if outputs is None:
            repository = Path(__file__).resolve().parents[3]
            outputs = sorted((repository / "build/ci/tests/verifica").glob("casi*"))
        failures += audit_outputs(outputs)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
