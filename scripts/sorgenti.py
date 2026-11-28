#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# The public files of the verification cases (docs/verifica/casi.md,
# "tests/verifica/sorgenti.json"; docs/DECISIONI.md, 2026-11-28, Catena 13).
# They are listed in tests/verifica/sorgenti.json and, during a stage, in one
# file per block of references, tests/verifica/sorgenti/<X.Y>-RT<N>.json, in
# the same format: no two blocks of references in parallel write the same
# file. Every name is unique across all the files.
#
#     python3 scripts/sorgenti.py elenco    # the names, one per line
#     python3 scripts/sorgenti.py controlla # format and unique names
#     python3 scripts/sorgenti.py raccogli  # closing of the stage: move the
#                                           # entries of sorgenti/ into
#                                           # sorgenti.json, empty the folder
#
# --radice <folder> reads another tests/verifica (default: the one of this
# repository).

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent / "tests" / "verifica"
FIELDS = ("name", "url", "version", "sha256", "license", "license-url")


def files(root: Path) -> list[Path]:
    """sorgenti.json first, then the files of sorgenti/ in name order."""
    out = [root / "sorgenti.json"] if (root / "sorgenti.json").exists() else []
    folder = root / "sorgenti"
    if folder.is_dir():
        out += sorted(folder.glob("*.json"))
    return out


def load(path: Path) -> list[dict]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("format") != 1 or not isinstance(data.get("sources"), list):
        raise ValueError(f"{path}: expected format 1 and a list of sources")
    return data["sources"]


def leggi(root: Path = ROOT) -> list[dict]:
    """All the sources, sorgenti.json first."""
    out: list[dict] = []
    for path in files(root):
        out += load(path)
    return out


def problemi(root: Path = ROOT) -> list[str]:
    out: list[str] = []
    seen: dict[str, Path] = {}
    for path in files(root):
        try:
            sources = load(path)
        except (OSError, ValueError) as error:
            out.append(str(error))
            continue
        for index, source in enumerate(sources):
            missing = [f for f in FIELDS if not isinstance(source.get(f), str) or not source[f]]
            if missing:
                out.append(f"{path}: source {index}: missing {', '.join(missing)}")
                continue
            name = source["name"]
            if name in seen:
                out.append(f"{path}: duplicate source name {name} (also in {seen[name].name})")
            seen[name] = path
    return out


def raccogli(root: Path = ROOT) -> list[Path]:
    """Moves the entries of sorgenti/ to the end of sorgenti.json and deletes
    their files; returns the files moved."""
    problems = problemi(root)
    if problems:
        raise ValueError("\n".join(problems))
    moved = [p for p in files(root) if p.parent.name == "sorgenti"]
    if not moved:
        return []
    manifest = root / "sorgenti.json"
    sources = load(manifest) if manifest.exists() else []
    for path in moved:
        sources += load(path)
    manifest.write_text(json.dumps({"format": 1, "sources": sources}, indent=2,
                                   ensure_ascii=False) + "\n", encoding="utf-8")
    for path in moved:
        path.unlink()
    return moved


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("azione", choices=("elenco", "controlla", "raccogli"))
    parser.add_argument("--radice", type=Path, default=ROOT)
    args = parser.parse_args(argv[1:])
    if args.azione == "elenco":
        for source in leggi(args.radice):
            print(source["name"])
        return 0
    if args.azione == "controlla":
        problems = problemi(args.radice)
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1 if problems else 0
    try:
        moved = raccogli(args.radice)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 1
    for path in moved:
        print(f"moved into sorgenti.json: {path.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
