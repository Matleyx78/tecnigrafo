# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
"""Read AutoCAD's stored dimension blocks; never render or regenerate them.

Run from the repository root with the pinned public DXF downloaded separately:
  python3 tests/verifica/casi/quote-modello/autocad-lineari/estrai_autocad.py \
      autocad-quote.dxf > autocad-quote.txt

The owner authorized this independent extraction in issue #706 (2026-11-16).
Only linear/aligned dimensions belong to RT23. POINTs are not drawn by Tecnigrafo.
The output preserves block order and DXF SOLID vertex order. MTEXT layout in
Tecnigrafo uses the bundled font and PT13, not AutoCAD's font metrics.
"""

import hashlib
import json
from pathlib import Path
import sys

import ezdxf

COMMIT = "5170f1860953b5d076aa435bd65daa725d431745"
SHA256 = "7a444a6306ac94f4e049d5d4af3c2df1ca66602fb798a583738ace0bd0cba394"


def point(value):
    return [float(value.x), float(value.y)]


def extract(path):
    assert hashlib.sha256(path.read_bytes()).hexdigest() == SHA256, "wrong DXF"
    drawing = ezdxf.readfile(path)
    dimensions = []
    for dimension in drawing.modelspace().query("DIMENSION"):
        kind = dimension.dxf.dimtype & 7
        if kind not in (0, 1):
            continue
        record = {
            "handle": dimension.dxf.handle,
            "block": dimension.dxf.geometry,
            "kind": "linear" if kind == 0 else "aligned",
            "style": dimension.dxf.dimstyle,
            "origin1": point(dimension.dxf.defpoint2),
            "origin2": point(dimension.dxf.defpoint3),
            "line_point": point(dimension.dxf.defpoint),
            "rotation": dimension.dxf.get("angle", 0),
            "text_point": point(dimension.dxf.text_midpoint),
            "measurement": dimension.dxf.actual_measurement,
            "text_override": dimension.dxf.get("text", ""),
            "graphics": [],
        }
        for entity in drawing.blocks[dimension.dxf.geometry]:
            kind = entity.dxftype()
            item = {"type": kind}
            if kind == "POINT":
                continue
            if kind == "LINE":
                item.update(start=point(entity.dxf.start), end=point(entity.dxf.end))
            elif kind == "SOLID":
                item["vertices"] = [point(entity.dxf.get(f"vtx{i}")) for i in range(4)]
            elif kind == "MTEXT":
                item.update(insertion=point(entity.dxf.insert),
                            height=entity.dxf.char_height, rotation=entity.get_rotation(),
                            attachment=entity.dxf.attachment_point,
                            style=entity.dxf.get("style", "Standard"),
                            text=entity.text, plain_text=entity.plain_text())
            else:
                raise ValueError(f"unexpected {kind} in {dimension.dxf.geometry}")
            record["graphics"].append(item)
        dimensions.append(record)
    return {"source_commit": COMMIT, "source_sha256": SHA256,
            "ezdxf_version": ezdxf.__version__, "dimensions": dimensions}


if __name__ == "__main__":
    result = extract(Path(sys.argv[1]))
    print("# SPDX-License-Identifier: GPL-3.0-or-later")
    print("# Copyright (C) 2026 The Tecnigrafo authors")
    print(json.dumps(result, ensure_ascii=False, indent=2))
