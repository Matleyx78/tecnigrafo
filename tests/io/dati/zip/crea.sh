#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Creates the test archives of tests/io/zip_test.cpp with tools other than
# Tecnigrafo (docs/adr/0019, point 3): Info-ZIP zip 3.0 and Python's zipfile
# module. Run it from anywhere; it rewrites the *.zip files of this folder.
# The archives are committed: the tests never run this script.
#
# Made with zip 3.0 (Debian/Ubuntu package, bzip2 support) and Python 3.13.
# Windows Explorer archives are missing: they need Windows.

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

export TZ=UTC LC_ALL=C.UTF-8
cd "$work"

# The same contents that zip_test.cpp expects: a compressible text, an empty
# file and a file in a folder, once with an ASCII name and once without.
mkdir cartella
for _ in $(seq 40); do printf 'Tecnigrafo: archivio di prova.\n'; done >testo.txt
: >vuoto.txt
for _ in $(seq 20); do printf 'Città, perché, €.\n'; done >cartella/dati.txt
cp cartella/dati.txt cartella/città.txt
touch -d '2026-10-23 08:00:00' testo.txt vuoto.txt cartella/dati.txt cartella/città.txt cartella

rm -f "$here"/*.zip

# Info-ZIP: deflate, stored, and data descriptors (zip writes them when its
# output is a pipe). The entry "cartella/" is the folder itself.
zip -q "$here/zip-deflate.zip" testo.txt vuoto.txt cartella cartella/dati.txt
zip -q -0 "$here/zip-stored.zip" testo.txt vuoto.txt cartella cartella/dati.txt
zip -q - testo.txt vuoto.txt | cat >"$here/zip-descriptor.zip"

# With a UTF-8 locale zip writes a non-ASCII name as it is, without bit 11:
# Tecnigrafo reads it as UTF-8 because it is valid UTF-8 (#190).
zip -q "$here/zip-nome-non-marcato.zip" cartella/città.txt

# Info-ZIP archives that Tecnigrafo must refuse as unsupported.
zip -q -P segreta "$here/zip-encrypted.zip" testo.txt
zip -q -fz "$here/zip-zip64.zip" testo.txt
zip -q -Z bzip2 "$here/zip-bzip2.zip" testo.txt

# Python's zipfile: stored and deflated parts, bit 11 for the non-ASCII name,
# comments; data descriptors on an output that cannot seek; no parts at all.
python3 - "$here" <<'PYTHON'
import io
import sys
import zipfile
from pathlib import Path

here = Path(sys.argv[1])
date = (2026, 10, 23, 8, 0, 0)
text = Path("testo.txt").read_bytes()
unicode_text = Path("cartella/dati.txt").read_bytes()


def write_parts(archive):
    for name, data, method in (
        ("mimetype", b"application/x-tecnigrafo", zipfile.ZIP_STORED),
        ("testo.txt", text, zipfile.ZIP_DEFLATED),
        ("vuoto.txt", b"", zipfile.ZIP_DEFLATED),
        ("cartella/città.txt", unicode_text, zipfile.ZIP_DEFLATED),
    ):
        info = zipfile.ZipInfo(name, date_time=date)
        info.compress_type = method
        archive.writestr(info, data)


with zipfile.ZipFile(here / "python.zip", "w") as archive:
    write_parts(archive)
    archive.getinfo("testo.txt").comment = b"commento della parte"
    archive.comment = b"commento dell'archivio"


class Unseekable(io.RawIOBase):
    def __init__(self, target):
        self.target = target

    def writable(self):
        return True

    def seekable(self):
        return False

    def write(self, data):
        return self.target.write(data)


with open(here / "python-descriptor.zip", "wb") as target:
    with zipfile.ZipFile(Unseekable(target), "w") as archive:
        write_parts(archive)

with zipfile.ZipFile(here / "python-empty.zip", "w"):
    pass
PYTHON
