# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
"""Independent, raw DXF answers according to ADR 0018 and ADR 0020."""

import argparse
from collections import Counter
import math
from pathlib import Path

import ezdxf
from ezdxf.colors import aci2rgb, int2rgb
from ezdxf.entities import Text, factory
from ezdxf.lldxf.extendedtags import ExtendedTags
from ezdxf.lldxf.loader import load_dxf_structure
from ezdxf.lldxf.tagger import ascii_tags_loader, tag_compiler
from ezdxf.sections.header import HeaderSection


STAGE_08_ENTITY_TYPES = frozenset({
    "SOLID", "ELLIPSE", "MTEXT", "ATTDEF", "ATTRIB", "INSERT", "DIMENSION", "HATCH",
})

# Work packages add a type here when Tecnigrafo starts handling it.  This stage
# declaration affects only count/unhandled; every other oracle answer remains
# independent from the application (ADR 0020).
ACTIVE_STAGE_08_ENTITY_TYPES = frozenset()


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


def style_answer(style):
    d = style.dxf
    flags = d.get("generation_flags", 0)
    return (f"{quoted(d.name)} font={quoted(d.get('font', ''))} "
            f"height={number(d.get('height', 0))} width-factor={number(d.get('width', 1))} "
            f"oblique={number(d.get('oblique', 0))} "
            f"backwards={'yes' if flags & 2 else 'no'} upside-down={'yes' if flags & 4 else 'no'}")


DIMSTYLE_FIELDS = tuple(sorted(("dimscale", "dimasz", "dimexo", "dimexe", "dimtxt", "dimgap",
                   "dimtad", "dimtih", "dimtoh", "dimdec", "dimadec", "dimdsep",
                   "dimzin", "dimazin", "dimlfac", "dimrnd", "dimpost", "dimtxsty",
                   "dimcen", "dimse1", "dimse2", "dimsd1", "dimsd2", "dimclrd",
                   "dimclre", "dimclrt", "dimlwd", "dimlwe")))
ISO_25_DEFAULTS = {
    "dimscale": 1.0, "dimasz": 2.5, "dimexo": 0.625, "dimexe": 1.25,
    "dimtxt": 2.5, "dimgap": 0.625, "dimtad": 1, "dimtih": 0,
    "dimtoh": 0, "dimdec": 2, "dimadec": 0, "dimdsep": 44,
    "dimzin": 8, "dimazin": 0, "dimlfac": 1.0, "dimrnd": 0.0,
    "dimpost": "", "dimtxsty": "Standard", "dimcen": 2.5, "dimse1": 0,
    "dimse2": 0, "dimsd1": 0, "dimsd2": 0, "dimclrd": 0, "dimclre": 0,
    "dimclrt": 0, "dimlwd": -2, "dimlwe": -2,
}
DXF_DIMSTYLE_DEFAULTS = {
    **ISO_25_DEFAULTS,
    "dimasz": 0.18, "dimcen": 0.09, "dimdec": 4, "dimdsep": 46,
    "dimexe": 0.18, "dimexo": 0.0625, "dimgap": 0.09, "dimtad": 0,
    "dimtih": 1, "dimtoh": 1, "dimtxt": 0.18, "dimzin": 0,
}


def dimstyle_answer(style, baseline=None):
    # The report only exposes essential overrides.  ISO-25 is the baseline;
    # keep the complete record name when no essential value differs.
    d = style.dxf
    fields = [quoted(d.name)]
    for name in DIMSTYLE_FIELDS:
        value = d.get(name) if d.hasattr(name) else DXF_DIMSTYLE_DEFAULTS[name]
        if baseline is None or value != baseline.get(name):
            written = quoted(value) if isinstance(value, str) else number(value)
            fields.append(f"{name.upper()}:{written}")
    return " ".join(fields)


def managed(entity, model_owner):
    """Classify file entities, without converting OCS or consulting Tecnigrafo."""
    d = entity.dxf
    kind = entity.dxftype()
    if kind not in {"LINE", "CIRCLE", "ARC", "LWPOLYLINE", "TEXT", "SOLID",
                    "ELLIPSE", "MTEXT", "ATTDEF", "ATTRIB", "INSERT",
                    "DIMENSION", "HATCH"}:
        return False
    if d.get("paperspace", 0) == 1 or (kind != "ATTRIB" and
                                       d.get("owner") not in (None, model_owner)):
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
    if kind in {"ATTDEF", "ATTRIB"} and (d.height <= 0 or d.width <= 0
                                           or entity.has_embedded_mtext_entity):
        return False
    if kind == "MTEXT" and (d.char_height <= 0 or entity.has_columns):
        return False
    if kind == "ELLIPSE" and (not 0 < d.ratio <= 1
                               or tuple(d.major_axis) == (0, 0, 0)):
        return False
    if kind == "INSERT" and (d.get("row_count", 1) != 1
                              or d.get("column_count", 1) != 1):
        return False
    if kind == "HATCH":
        if entity.gradient is not None:
            return False
        for path in entity.paths:
            if not path.type == 1 and any(edge.type not in {1, 2}
                                         for edge in path.edges):
                return False
    return kind != "LWPOLYLINE" or len(entity) > 0


def text_fields(d):
    alignment = d.get("align_point", (0, 0, 0))
    return [f"position={point(d.insert)}", f"alignment-point={point(alignment)}",
            f"height={number(d.height)}", f"rotation={number(d.rotation)}",
            f"width-factor={number(d.width)}", f"oblique={number(d.oblique)}",
            f"style={quoted(d.style)}",
            f"mirror-x={'yes' if d.text_generation_flag & 2 else 'no'}",
            f"mirror-y={'yes' if d.text_generation_flag & 4 else 'no'}",
            f"horizontal={('left', 'center', 'right', 'aligned', 'middle', 'fit')[d.halign]}",
            f"vertical={('baseline', 'bottom', 'middle', 'top')[d.valign]}",
            f"value={quoted(d.text)}"]


def hatch_loop(path):
    flags = path.path_type_flags
    prefix = "+".join(name for bit, name in ((1, "external"), (16, "outermost"),
                                               (4, "derived")) if flags & bit)
    fields = ["loop=" + (prefix + "+" if prefix else "") +
              ("polyline" if path.type == 1 else "edges")]
    if path.type == 1:
        for x, y, bulge in path.vertices:
            fields.append(f"vertex={point((x, y))}")
            if bulge:
                fields.append(f"bulge={number(bulge)}")
    else:
        for edge in path.edges:
            if edge.type == 1:
                fields.append(f"line={point(edge.start)};{point(edge.end)}")
            elif edge.type == 2:
                if math.isclose((edge.end_angle - edge.start_angle) % 360, 0):
                    fields.append(f"circle={point(edge.center)};{number(edge.radius)}")
                else:
                    fields.append(f"arc={point(edge.center)};{number(edge.radius)};"
                                  f"{number(edge.start_angle % 360)};"
                                  f"{number(edge.end_angle % 360)}")
    return fields


def entity_answer(entity, layer_names=None, linetype_names=None, properties=True, owner=None):
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
        fields.extend(text_fields(d))
        alignment = d.get("align_point", (0, 0, 0))
        optional("z", d.insert.z)
        optional("alignment-z", tuple(alignment)[2])
    elif kind == "SOLID":
        for vertex in (d.vtx0, d.vtx1, d.vtx2, d.vtx3):
            add("vertex", point(vertex))
    elif kind == "ELLIPSE":
        add("center", point(d.center)); add("major-axis", point(d.major_axis))
        add("ratio", number(d.ratio))
        if not (math.isclose(d.start_param, 0) and math.isclose(d.end_param, math.tau)):
            add("start", number(math.degrees(d.start_param)))
            add("end", number(math.degrees(d.end_param)))
    elif kind == "MTEXT":
        add("insertion", point(d.insert)); add("height", number(d.char_height))
        add("width", number(d.get("width", 0)))
        add("attachment", ("top-left", "top-center", "top-right", "middle-left",
                           "middle-center", "middle-right", "bottom-left",
                           "bottom-center", "bottom-right")[d.attachment_point - 1])
        add("rotation", number(entity.get_rotation()))
        add("style", quoted(d.style))
        optional("line-spacing", d.get("line_spacing_factor", 1), 1)
        if d.get("line_spacing_style", 1) == 2: add("line-spacing-style", "exactly")
        if d.get("flow_direction", 1) != 1: add("direction", "by-style")
        add("value", quoted(entity.text))
    elif kind in {"ATTDEF", "ATTRIB"}:
        if kind == "ATTRIB": add("owner", owner or d.get("owner", "0"))
        fields.extend(text_fields(d)); add("tag", quoted(d.tag))
        if kind == "ATTDEF": add("prompt", quoted(d.get("prompt", "")))
        flags = d.get("flags", 0)
        for bit, name in ((1, "invisible"), (2, "constant"), (4, "verify"), (8, "preset")):
            if flags & bit and (kind == "ATTDEF" or name == "invisible"): add(name, "yes")
        if kind == "ATTRIB":
            alignment = d.get("align_point", (0, 0, 0))
            optional("z", d.insert.z)
            optional("alignment-z", tuple(alignment)[2])
    elif kind == "INSERT":
        add("block", quoted(d.name)); add("insertion", point(d.insert))
        add("scale", point((d.get("xscale", 1), d.get("yscale", 1))))
        add("rotation", number(d.get("rotation", 0)))
    elif kind == "DIMENSION":
        raw = d.get("dimtype", 0) & 7
        names = {0: "linear", 1: "aligned", 2: "angular", 3: "diameter", 4: "radius"}
        dimkind = names.get(raw, "other"); add("kind", dimkind); add("style", quoted(d.get("dimstyle", "Standard")))
        if dimkind in {"linear", "aligned"}:
            add("origin1", point(d.defpoint2)); add("origin2", point(d.defpoint3)); add("line-point", point(d.defpoint))
            if dimkind == "linear": add("rotation", number(d.get("angle", 0)))
        elif dimkind == "angular":
            add("line-point", point(d.defpoint)); add("first", f"{point(d.defpoint2)};{point(d.defpoint3)}")
            add("second", f"{point(d.defpoint)};{point(d.defpoint4)}"); add("arc-point", point(d.defpoint5))
        elif dimkind == "radius": add("center", point(d.defpoint)); add("point", point(d.defpoint4))
        elif dimkind == "diameter": add("point1", point(d.defpoint4)); add("point2", point(d.defpoint))
        else: add("type", str(raw))
        add("text-point", point(d.text_midpoint)); add("measurement", number(d.get("actual_measurement", 0)))
        if d.get("text", ""): add("text", quoted(d.text))
        optional("text-rotation", d.get("text_rotation", 0))
        if d.get("dimtype", 0) & 128: add("user-text-position", "yes")
        if entity.doc is not None:
            overrides = entity.override().dimstyle_attribs
            written = []
            for name in DIMSTYLE_FIELDS:
                if name in overrides:
                    value = overrides[name]
                    value = quoted(value) if isinstance(value, str) else number(value)
                    written.append(f"{name.upper()}:{value}")
            if written:
                add("overrides", ",".join(written))
    elif kind == "HATCH":
        add("pattern", quoted(d.pattern_name)); add("solid", "yes" if d.solid_fill else "no")
        add("angle", number(d.get("pattern_angle", 0))); add("scale", number(d.get("pattern_scale", 1)))
        if d.get("pattern_double", 0): add("double", "yes")
        add("islands", {0: "normal", 1: "outer", 2: "ignore"}.get(d.get("hatch_style", 0), "normal"))
        for path in entity.paths: fields.extend(hatch_loop(path))
        for seed in entity.seeds: add("seed", point(seed))
    if d.is_supported("thickness"):
        optional("thickness", d.get("thickness", 0))
    extrusion = tuple(d.get("extrusion", (0, 0, 1))) if d.is_supported("extrusion") else (0, 0, 1)
    fields.extend(extra)
    if extrusion != (0, 0, 1):
        add("extrusion", point(extrusion, 3))
    if not properties:
        return " ".join(fields)
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
    def __init__(self, path, active_stage_08_types=None):
        self.active_stage_08_types = (ACTIVE_STAGE_08_ENTITY_TYPES
                                      if active_stage_08_types is None
                                      else frozenset(active_stage_08_types))
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
        self.styles = []
        self.dimstyles = []
        for tags in sections.get("TABLES", [])[1:]:
            kind = tags[0].value
            if kind in {"LAYER", "LTYPE", "STYLE", "DIMSTYLE"}:
                entry = factory.load(ExtendedTags(tags))
                {"LAYER": self.layers, "LTYPE": self.linetypes,
                 "STYLE": self.styles, "DIMSTYLE": self.dimstyles}[kind].append(entry)
        self.layer_names = {entry.dxf.name.casefold(): entry.dxf.name for entry in self.layers}
        self.linetype_names = {entry.dxf.name.casefold(): entry.dxf.name for entry in self.linetypes}
        self.entities = {}
        self.bound_entities = self.document.entitydb
        # Block definitions, linked vertices/attributes and sequence terminators
        # are not top-level drawing entities.
        for tags in sections.get("ENTITIES", [])[1:]:
            if tags[0].value in {"VERTEX", "SEQEND"}:
                continue
            entity = factory.load(ExtendedTags(tags))
            handle = entity.dxf.get("handle")
            if handle is None:
                raise ValueError("oracle entities require an explicit DXF handle")
            self.entities[handle.upper()] = (entity, managed(entity, model_owner))
        # An ATTRIB belongs to the INSERT it follows; group 330 may name the
        # layout instead (ezdxf writes the block record there).
        self.attribute_owners = {
            attrib.dxf.handle.upper(): entity.dxf.handle
            for entity, _ in self.entities.values() if entity.dxftype() == "INSERT"
            for attrib in self.bound_entities[entity.dxf.handle].attribs
        }
        self.linked_attribute_handles = set(self.attribute_owners)
        self.blocks = []
        for block in self.document.blocks:
            name = block.name
            record = block.block_record
            if name.startswith("*D") or record.is_any_layout or block.block.dxf.get("xref_path", ""):
                continue
            self.blocks.append(block)
        block_names = {block.name.casefold() for block in self.blocks}
        dimension_block_names = {block.name.casefold() for block in self.document.blocks
                                 if not block.block_record.is_any_layout}
        for handle, (entity, supported) in list(self.entities.items()):
            if entity.dxftype() == "INSERT" and entity.dxf.name.casefold() not in block_names:
                supported = False
            if entity.dxftype() == "INSERT":
                bound = self.bound_entities.get(entity.dxf.handle)
                if bound is not None and any(not managed(attrib, entity.dxf.handle)
                                             for attrib in bound.attribs):
                    supported = False
            if entity.dxftype() == "DIMENSION":
                geometry = entity.dxf.get("geometry", "")
                if not geometry or geometry.casefold() not in dimension_block_names:
                    supported = False
            self.entities[handle] = (entity, supported)

    def answer(self, question, handle=None):
        if question == "version":
            return self.header.get("$ACADVER", "none") if self.header else "none"
        if question == "units":
            return str(self.header.get("$INSUNITS", 0) if self.header else 0)
        if question in {"count", "unhandled"}:
            def counted_as_managed(entity, supported):
                kind = entity.dxftype()
                return supported and (kind not in STAGE_08_ENTITY_TYPES
                                      or kind in self.active_stage_08_types)

            # Inactive linked attributes stay inside Tecnigrafo's raw INSERT.
            counts = Counter(e.dxftype() for handle, (e, supported) in self.entities.items()
                             if not (handle in self.linked_attribute_handles
                                     and "ATTRIB" not in self.active_stage_08_types)
                             and counted_as_managed(e, supported) == (question == "count"))
            return "; ".join(f"{kind} {count}" for kind, count in sorted(counts.items())) or "none"
        if question == "layers":
            return "; ".join(layer_answer(entry) for entry in self.layers) or "none"
        if question == "linetypes":
            return "; ".join(linetype_answer(entry) for entry in self.linetypes
                             if entry.dxf.name.casefold() not in
                             {"byblock", "bylayer", "continuous"}) or "none"
        if question == "styles":
            return "; ".join(style_answer(entry) for entry in self.styles) or "none"
        if question == "dimstyles":
            return "; ".join(dimstyle_answer(entry, ISO_25_DEFAULTS)
                             for entry in self.dimstyles) or "none"
        if question == "blocks":
            rows = []
            for block in self.blocks:
                d = block.block.dxf
                head = f"{quoted(block.name)} base={point(d.base_point)}"
                record = block.block_record.dxf
                if record.get("explode", 1) == 0:
                    head += " explodable=no"
                if record.get("scale", 0) == 1:
                    head += " uniform=yes"
                if record.get("units", 0) != 0:
                    head += f" units={record.units}"
                if d.get("description", ""):
                    head += f" description={quoted(d.description)}"
                rows.append(head)
                for entity in block:
                    if entity.dxftype() not in {"ATTRIB", "SEQEND"}:
                        rows.append(f"{quoted(block.name)} {entity.dxf.handle} {entity_answer(entity)}")
            return "; ".join(rows) or "none"
        if question == "generated":
            answer, _ = self.generated_answer(handle)
            return answer
        if question == "entity":
            record = self.entities.get(format(int(handle, 16), "X"))
            if record is None:
                return "none"
            entity, supported = record
            answered_entity = (self.bound_entities[entity.dxf.handle]
                               if entity.dxftype() == "DIMENSION" else entity)
            owner = self.attribute_owners.get(entity.dxf.handle.upper())
            return (entity_answer(answered_entity, self.layer_names, self.linetype_names,
                                  owner=owner)
                    if supported else "unhandled " + entity.dxftype())
        raise ValueError(f"unsupported oracle question: {question}")

    def effective_properties(self, entity, owner):
        """Return visible generated properties resolved through owner and layers."""
        def layer_attributes(name):
            try:
                return self.document.layers.get(name).dxf
            except ezdxf.DXFTableEntryError:
                return self.document.layers.get("0").dxf

        owner_layer = layer_attributes(owner.dxf.get("layer", "0"))
        owner_flags = owner_layer.get("flags", 0)
        if owner_layer.get("color", 7) < 0 or owner_flags & 1:
            return None
        layer_name = entity.dxf.get("layer", "0")
        if layer_name.casefold() == "0":
            layer_name = owner.dxf.get("layer", "0")
        layer = layer_attributes(layer_name)
        flags = layer.get("flags", 0)
        if layer.get("color", 7) < 0 or flags & 1:
            return None

        def effective_color(attributes):
            color = attributes.get("color", 256)
            if color == 0:
                return owner_color
            if color == 256:
                return color_answer(layer, layer=True)
            return color_answer(attributes)

        def effective_linetype(attributes):
            name = attributes.get("linetype", "ByLayer")
            if name.casefold() == "byblock":
                return owner_linetype
            if name.casefold() == "bylayer":
                return layer.get("linetype", "Continuous")
            return name

        def effective_weight(attributes):
            weight = attributes.get("lineweight", -1)
            if weight == -2:
                return owner_weight
            if weight == -1:
                weight = layer.get("lineweight", -3)
            return 25 if weight == -3 else weight

        owner_color = (color_answer(owner.dxf) if (owner.dxf.hasattr("true_color") or
                                                    owner.dxf.get("color", 256) != 256)
                       else color_answer(owner_layer, layer=True))
        owner_linetype = owner.dxf.get("linetype", "ByLayer")
        if owner_linetype.casefold() == "bylayer":
            owner_linetype = owner_layer.get("linetype", "Continuous")
        elif owner_linetype.casefold() == "byblock":
            owner_linetype = "Continuous"
        owner_weight = owner.dxf.get("lineweight", -1)
        if owner_weight == -1:
            owner_weight = owner_layer.get("lineweight", -3)
        if owner_weight in (-2, -3):
            owner_weight = 25

        color = effective_color(entity.dxf)
        if color.isdigit():
            color = aci_answer(int(color))
        fields = []
        if color != "255,255,255":
            fields.append(f"color={color}")
        linetype = effective_linetype(entity.dxf)
        if linetype.casefold() != "continuous":
            fields.append(f"linetype={quoted(linetype)}")
        scale = self.header.get("$LTSCALE", 1) * entity.dxf.get("ltscale", 1)
        if scale != 1:
            fields.append(f"ltscale={number(scale)}")
        weight = effective_weight(entity.dxf)
        if weight != 25:
            fields.append(f"lineweight={lineweight_answer(weight)}")
        if flags & 4:
            fields.append("locked=yes")
        return fields

    def generated_answer(self, handle):
        record = self.entities.get(format(int(handle, 16), "X"))
        if record is None or record[0].dxftype() not in {"INSERT", "DIMENSION"}:
            return "none", ()
        owner = self.bound_entities[record[0].dxf.handle]
        try:
            owner_layer = self.document.layers.get(owner.dxf.get("layer", "0")).dxf
        except ezdxf.DXFTableEntryError:
            owner_layer = self.document.layers.get("0").dxf
        if owner_layer.get("color", 7) < 0 or owner_layer.get("flags", 0) & 1:
            return "none", ()
        try:
            generated = list(owner.virtual_entities())
        except (AttributeError, ezdxf.DXFError):
            return "none", ()
        shape_types = {"LINE", "ARC", "CIRCLE", "ELLIPSE", "LWPOLYLINE", "TEXT", "SOLID"}
        outside = tuple(sorted({entity.dxftype() for entity in generated
                                if entity.dxftype() not in shape_types}))
        if outside:
            return "none", outside
        if record[0].dxftype() == "INSERT":
            for attrib in owner.attribs:
                if not attrib.dxf.get("flags", 0) & 1:
                    attributes = {name: value for name, value in
                                  attrib.dxf.all_existing_dxf_attribs().items()
                                  if name in Text.DXFATTRIBS and name not in {"handle", "owner"}}
                    generated.append(Text.new(dxfattribs=attributes))
        rows = []
        for entity in generated:
            properties = self.effective_properties(entity, owner)
            if properties is not None:
                row = entity_answer(entity, properties=False)
                rows.append(" ".join((row, *properties)))
        return "; ".join(rows) or "none", ()

    def print_answers(self, name="D"):
        for question in ("version", "units", "count", "unhandled", "layers", "linetypes",
                         "blocks", "styles", "dimstyles"):
            print(f"? {question} {name} -> {self.answer(question)}")
        for handle, (entity, supported) in self.entities.items():
            if not supported:
                continue
            if tuple(entity.dxf.get("extrusion", (0, 0, 1))) == (0, 0, -1):
                print("; OCS: convert by hand (ADR 0018, point 5)")
            print(f"? entity {name} {handle} -> {self.answer('entity', handle)}")
            if entity.dxftype() in {"INSERT", "DIMENSION"}:
                generated, outside = self.generated_answer(handle)
                if outside:
                    print(f"; generated {handle}: outside oracle scope ({','.join(outside)})")
                else:
                    print(f"? generated {name} {handle} -> {generated}")


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
