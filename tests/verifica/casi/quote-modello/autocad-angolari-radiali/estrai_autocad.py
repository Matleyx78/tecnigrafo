# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
"""Extract stored AutoCAD graphics for RT24; do not render dimensions.

From the repository root:
  python3 tests/verifica/casi/quote-modello/autocad-angolari-radiali/estrai_autocad.py \
      path/to/autocad-quote.dxf > autocad-quote.txt

PT24, updated by PR #836, authorizes reading LINE/ARC/SOLID and MTEXT
directly. POINT definition markers are omitted. Coordinates, block order,
SOLID vertex order and the original MTEXT contents are retained. The bundled
font, rather than AutoCAD's SHX font, determines the reference TEXT boxes.
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
    if hashlib.sha256(path.read_bytes()).hexdigest() != SHA256:
        raise ValueError("The DXF does not match the pinned source hash")
    drawing = ezdxf.readfile(path)
    dimensions = []
    for dimension in drawing.modelspace().query("DIMENSION"):
        kind = dimension.dxf.dimtype & 7
        if kind not in (2, 3, 4):
            continue
        record = {
            "handle": dimension.dxf.handle,
            "block": dimension.dxf.geometry,
            "kind": {2: "angular", 3: "diameter", 4: "radius"}[kind],
            "style": dimension.dxf.dimstyle,
            "definition": {
                str(code): point(dimension.dxf.get(name))
                for code, name in ((10, "defpoint"), (13, "defpoint2"),
                                   (14, "defpoint3"), (15, "defpoint4"),
                                   (16, "defpoint5"))
                if dimension.dxf.hasattr(name)
            },
            "text_point": point(dimension.dxf.text_midpoint),
            "user_text_position": bool(dimension.dxf.dimtype & 128),
            "measurement": dimension.dxf.actual_measurement,
            "text_override": dimension.dxf.get("text", ""),
            "graphics": [],
        }
        for entity in drawing.blocks[dimension.dxf.geometry]:
            kind = entity.dxftype()
            if kind == "POINT":
                continue
            item = {"type": kind}
            if kind == "LINE":
                item.update(start=point(entity.dxf.start), end=point(entity.dxf.end))
            elif kind == "ARC":
                item.update(center=point(entity.dxf.center), radius=entity.dxf.radius,
                            start_angle=entity.dxf.start_angle, end_angle=entity.dxf.end_angle)
            elif kind == "SOLID":
                item["vertices"] = [point(entity.dxf.get(f"vtx{i}")) for i in range(4)]
            elif kind == "MTEXT":
                item.update(insertion=point(entity.dxf.insert),
                            height=entity.dxf.char_height, rotation=entity.get_rotation(),
                            attachment=entity.dxf.attachment_point,
                            style=entity.dxf.get("style", "Standard"),
                            text=entity.text, plain_text=entity.plain_text())
            else:
                raise ValueError(f"Unexpected {kind} in {dimension.dxf.geometry}")
            record["graphics"].append(item)
        dimensions.append(record)
    return {"source_commit": COMMIT, "source_sha256": SHA256,
            "ezdxf_version": ezdxf.__version__, "dimensions": dimensions}


if __name__ == "__main__":
    result = extract(Path(sys.argv[1]))
    print("# SPDX-License-Identifier: GPL-3.0-or-later")
    print("# Copyright (C) 2026 The Tecnigrafo authors")
    print(json.dumps(result, ensure_ascii=False, indent=2))
