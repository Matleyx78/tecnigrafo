# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
"""Independent, raw DXF answers according to ADR 0018 and ADR 0020."""

import argparse
from collections import Counter
from pathlib import Path

import ezdxf
from ezdxf.colors import aci2rgb, int2rgb
from ezdxf.entities import factory
from ezdxf.lldxf.extendedtags import ExtendedTags
from ezdxf.lldxf.loader import load_dxf_structure
from ezdxf.lldxf.tagger import ascii_tags_loader, tag_compiler
from ezdxf.sections.header import HeaderSection


def number(value):
    value = float(value)
    if value == 0:
        return "0"
    return repr(value).removesuffix(".0")


def point(value, dimensions=2):
    return ",".join(number(v) for v in tuple(value)[:dimensions])


def quoted(value):
    return '"' + "".join(
        "\\" + c if c in '\\"' else
        f"\\u{{{ord(c):02X}}}" if ord(c) < 32 or ord(c) == 127 else c
        for c in value
    ) + '"'


def aci_answer(index):
    if not 1 <= index <= 255:
        raise ValueError("ACI index must be between 1 and 255")
    return ",".join(str(channel) for channel in aci2rgb(index))


def print_standard_answers():
    print("=== ezdxf: standard ===")
    for index in range(1, 256):
        print(f"? aci {index} -> {aci_answer(index)}")


def color_answer(attributes, layer=False):
    if attributes.hasattr("true_color"):
        return ",".join(str(channel) for channel in int2rgb(attributes.true_color))
    index = attributes.get("color", 7 if layer else 256)
    if layer:
        index = abs(index)
    return {0: "byblock", 256: "bylayer"}.get(index, str(index))


def lineweight_answer(value):
    if value in (-1, -2, -3):
        return {-1: "bylayer", -2: "byblock", -3: "default"}[value]
    return "0" if value == 0 else f"{value / 100:.2f}"


def layer_answer(layer):
    d = layer.dxf
    fields = [quoted(d.name), f"color={color_answer(d, layer=True)}",
              f"linetype={quoted(d.get('linetype', 'Continuous'))}",
              f"lineweight={lineweight_answer(d.get('lineweight', -3))}"]
    flags = d.get("flags", 0)
    for state, active in (("off", d.get("color", 7) < 0),
                          ("frozen", flags & 1), ("locked", flags & 4),
                          ("noplot", not d.get("plot", 1))):
        if active:
            fields.append(state)
    return " ".join(fields)


def linetype_answer(linetype):
    # Group 49 keeps signed gaps, dots and the simple elements of complex types.
    pattern = [tag.value for tag in linetype.pattern_tags.tags if tag.code == 49]
    written = ",".join(number(value) for value in pattern) or "continuous"
    return (f"{quoted(linetype.dxf.name)} pattern={written} "
            f"description={quoted(linetype.dxf.get('description', ''))}")


def managed(entity, model_owner):
    """Classify file entities, without converting OCS or consulting Tecnigrafo."""
    d = entity.dxf
    kind = entity.dxftype()
    if kind not in {"LINE", "CIRCLE", "ARC", "LWPOLYLINE", "TEXT"}:
        return False
    if d.get("paperspace", 0) == 1 or d.get("owner") not in (None, model_owner):
        return False
    x, y, z = d.get("extrusion", (0, 0, 1))
    if abs(x) > 1e-12 or abs(y) > 1e-12 or z not in (-1, 1):
        return False
    if kind in {"CIRCLE", "ARC"} and d.radius <= 0:
        return False
    if kind == "ARC" and d.start_angle == d.end_angle:
        return False
    if kind == "TEXT" and (d.height <= 0 or d.width <= 0):
        return False
    return kind != "LWPOLYLINE" or len(entity) > 0


def entity_answer(entity, layer_names=None, linetype_names=None):
    d = entity.dxf
    kind = entity.dxftype()
    fields = [kind]
    extra = []

    def add(name, value):
        fields.append(f"{name}={value}")

    def optional(name, value, default=0):
        if value != default:
            extra.append(f"{name}={number(value)}")

    if kind == "LINE":
        add("start", point(d.start))
        add("end", point(d.end))
        optional("start-z", d.start.z)
        optional("end-z", d.end.z)
    elif kind in {"CIRCLE", "ARC"}:
        add("center", point(d.center))
        add("radius", number(d.radius))
        if kind == "ARC":
            add("start-angle", number(d.start_angle))
            add("end-angle", number(d.end_angle))
        optional("z", d.center.z)
    elif kind == "LWPOLYLINE":
        add("closed", "yes" if entity.closed else "no")
        if d.const_width:
            add("width", number(d.const_width))
        for x, y, start, end, bulge in entity.get_points("xyseb"):
            add("vertex", point((x, y)))
            for name, value in (("bulge", bulge), ("start-width", start), ("end-width", end)):
                if value:
                    add(name, number(value))
        optional("elevation", d.elevation)
    elif kind == "TEXT":
        alignment = d.get("align_point", (0, 0, 0))
        add("position", point(d.insert))
        add("alignment-point", point(alignment))
        for name, value in (("height", d.height), ("rotation", d.rotation),
                            ("width-factor", d.width), ("oblique", d.oblique)):
            add(name, number(value))
        add("style", quoted(d.style))
        add("mirror-x", "yes" if d.text_generation_flag & 2 else "no")
        add("mirror-y", "yes" if d.text_generation_flag & 4 else "no")
        add("horizontal", ("left", "center", "right", "aligned", "middle", "fit")[d.halign])
        add("vertical", ("baseline", "bottom", "middle", "top")[d.valign])
        add("value", quoted(d.text))
        optional("z", d.insert.z)
        optional("alignment-z", tuple(alignment)[2])
    optional("thickness", d.get("thickness", 0))
    extrusion = tuple(d.get("extrusion", (0, 0, 1)))
    fields.extend(extra)
    if extrusion != (0, 0, 1):
        add("extrusion", point(extrusion, 3))
    layer = d.get("layer", "0")
    layer = (layer_names or {}).get(layer.casefold(), layer)
    if layer.casefold() != "0":
        add("layer", quoted(layer))
    color = color_answer(d)
    if color != "bylayer":
        add("color", color)
    linetype = d.get("linetype", "ByLayer")
    linetype = (linetype_names or {}).get(linetype.casefold(), linetype)
    if linetype.casefold() != "bylayer":
        add("linetype", quoted(linetype))
    scale = d.get("ltscale", 1)
    if scale != 1:
        add("ltscale", number(scale))
    weight = d.get("lineweight", -1)
    if weight != -1:
        add("lineweight", lineweight_answer(weight))
    return " ".join(fields)


class DrawingAnswers:
    def __init__(self, path):
        self.document = ezdxf.readfile(path)
        model_owner = self.document.modelspace().block_record_handle
        # ezdxf binds ENTITIES to layouts and may replace their owner/paper-space
        # flags. Load the original entity tags independently so that classification
        # and raw answers describe the file, not those automatic repairs.
        encoding = ("utf-8" if self.document.loaded_dxfversion >= "AC1021"
                    else self.document.encoding)
        with Path(path).open(encoding=encoding) as stream:
            sections = load_dxf_structure(tag_compiler(ascii_tags_loader(stream)))
        header = sections.get("HEADER", [])
        self.header = HeaderSection.load(header[0]) if header else None
        # Read the file's table entries, not tables/defaults inserted by ezdxf.
        self.layers = []
        self.linetypes = []
        for tags in sections.get("TABLES", [])[1:]:
            kind = tags[0].value
            if kind in {"LAYER", "LTYPE"}:
                entry = factory.load(ExtendedTags(tags))
                (self.layers if kind == "LAYER" else self.linetypes).append(entry)
        self.layer_names = {entry.dxf.name.casefold(): entry.dxf.name for entry in self.layers}
        self.linetype_names = {entry.dxf.name.casefold(): entry.dxf.name for entry in self.linetypes}
        self.entities = {}
        # Block definitions, linked vertices/attributes and sequence terminators
        # are not top-level drawing entities.
        for tags in sections.get("ENTITIES", [])[1:]:
            if tags[0].value in {"VERTEX", "ATTRIB", "SEQEND"}:
                continue
            entity = factory.load(ExtendedTags(tags))
            handle = entity.dxf.get("handle")
            if handle is None:
                raise ValueError("oracle entities require an explicit DXF handle")
            self.entities[handle.upper()] = (entity, managed(entity, model_owner))

    def answer(self, question, handle=None):
        if question == "version":
            return self.header.get("$ACADVER", "none") if self.header else "none"
        if question == "units":
            return str(self.header.get("$INSUNITS", 0) if self.header else 0)
        if question in {"count", "unhandled"}:
            counts = Counter(e.dxftype() for e, supported in self.entities.values()
                             if supported == (question == "count"))
            return "; ".join(f"{kind} {count}" for kind, count in sorted(counts.items())) or "none"
        if question == "layers":
            return "; ".join(layer_answer(entry) for entry in self.layers) or "none"
        if question == "linetypes":
            return "; ".join(linetype_answer(entry) for entry in self.linetypes
                             if entry.dxf.name.casefold() not in
                             {"byblock", "bylayer", "continuous"}) or "none"
        if question == "entity":
            record = self.entities.get(format(int(handle, 16), "X"))
            if record is None:
                return "none"
            entity, supported = record
            return (entity_answer(entity, self.layer_names, self.linetype_names)
                    if supported else "unhandled " + entity.dxftype())
        raise ValueError(f"unsupported oracle question: {question}")

    def print_answers(self, name="D"):
        for question in ("version", "units", "count", "unhandled", "layers", "linetypes"):
            print(f"? {question} {name} -> {self.answer(question)}")
        for handle, (entity, supported) in self.entities.items():
            if not supported:
                continue
            if tuple(entity.dxf.get("extrusion", (0, 0, 1))) == (0, 0, -1):
                print("; OCS: convert by hand (ADR 0018, point 5)")
            print(f"? entity {name} {handle} -> {self.answer('entity', handle)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path)
    parser.add_argument("--name", default="D")
    args = parser.parse_args()
    try:
        DrawingAnswers(args.file).print_answers(args.name)
    except (OSError, ValueError, ezdxf.DXFError) as error:
        parser.exit(1, f"{args.file}: {error}\n")


if __name__ == "__main__":
    main()
