# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
"""Print oracle answers, check declared references and audit written drawings."""

import argparse
from contextlib import redirect_stdout
import io
import json
import math
from pathlib import Path
import re
import sys
import subprocess
import tempfile
import zipfile

import ezdxf
from ezdxf.entities import factory
from ezdxf_attesi import (DIMSTYLE_FIELDS, ISO_25_DEFAULTS, DrawingAnswers, aci_answer,
                          entity_answer, print_standard_answers)


# Quoted text is indivisible: digits inside a text value are never approximate.
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|[A-Za-z_~][\w~-]*|[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?|[^\s]')
DEFINITION = re.compile(r"(\w+)\s*=\s*DXF\s+source=([a-z0-9-]+)\s*$")
QUESTION = re.compile(
    r"\?\s*(version|units|count|unhandled|entity|layers|linetypes|blocks|styles|dimstyles"
    r"|generated|grid|attdisp)"
    r"\s+(\w+)(?:\s+([0-9A-Fa-f]+))?\s*->\s*(.*)$")

ACI_QUESTION = re.compile(r"\?\s*aci\s+([+-]?\d+)\s*->\s*(.*)$")


def answer_items(answer):
    """Split lists without treating quoted semicolons as separators."""
    items = [""]
    for token in re.findall(r'"(?:\\.|[^"\\])*"|;|[^";]+', answer):
        if token == ";":
            items.append("")
        else:
            items[-1] += token
    return items


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
    if question in {"count", "unhandled", "layers", "linetypes", "blocks", "styles", "dimstyles"}:
        remaining = answer_items(actual)
        for item in answer_items(expected):
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
    checked = 0
    failures = []
    print(f"=== Oracle case: {path.parent} ===")
    for line_number, line in enumerate(lines, 1):
        line = line.strip()
        if not line or line.startswith(";"):
            continue
        try:
            if line.startswith("!tolerance"):
                _, kind, value = line.split()
                value = float(value)
                if not math.isfinite(value) or value < 0:
                    raise ValueError("invalid tolerance")
                if kind == "absolute":
                    absolute = value
                elif kind == "relative":
                    relative = value
                elif kind not in {"channel", "pixels"}:
                    raise ValueError(f"unknown tolerance {kind}")
                continue
            definition = DEFINITION.fullmatch(line)
            if definition:
                name, source = definition.groups()
                if name in drawings:
                    raise ValueError(f"duplicate drawing {name}")
                drawings[name] = DrawingAnswers(sources / source)
                continue
            aci = ACI_QUESTION.fullmatch(line)
            if aci:
                index, expected = aci.groups()
                actual = aci_answer(int(index))
                checked += 1
                if not equal_answer("aci", expected, actual, absolute, relative):
                    failures.append(f"{path}:{line_number}: ? aci {index}\n"
                                    f"  expected: {expected}\n  ezdxf:    {actual}")
                continue
            match = QUESTION.fullmatch(line)
            if not match:
                print(f"{path}:{line_number}: skipped (outside oracle scope): {line}")
                continue
            question, name, handle, expected = match.groups()
            if name not in drawings:
                raise ValueError(f"no oracle source for {name}")
            if (question in {"entity", "generated"}) != (handle is not None):
                raise ValueError(f"invalid oracle question: {line}")
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
                failures.append(f"{path}:{line_number}: {line.split('->', 1)[0].strip()}\n"
                                f"  expected: {expected}\n  ezdxf:    {actual}")
        except (OSError, ValueError, ezdxf.DXFError) as error:
            failures.append(f"{path}:{line_number}: {error}")
    for failure in failures:
        print(failure)
    return checked, len(failures)


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


def self_test():
    """Exercise the oracle in CI without adding or changing reference cases."""
    failures = []

    def record_failure(name, expected, actual):
        failures.append(f"oracle self-test: {name}\n"
                        f"  expected: {expected}\n  actual:   {actual!r}")

    def require(name, expected, actual):
        if actual != expected:
            record_failure(name, repr(expected), actual)

    def require_contains(name, expected, actual, *, present=True):
        if (expected in actual) != present:
            relation = "contains" if present else "omits"
            record_failure(name, f"{relation} {expected!r}", actual)

    def dxf(path, entities, units=None, tables=()):
        header = [(0, "SECTION"), (2, "HEADER"), (9, "$ACADVER"), (1, "AC1015")]
        if units is not None:
            header += [(9, "$INSUNITS"), (70, units)]
        groups = header + [(0, "ENDSEC")]
        if tables:
            groups += [(0, "SECTION"), (2, "TABLES")] + list(tables) + [(0, "ENDSEC")]
        groups += [(0, "SECTION"), (2, "ENTITIES")]
        groups += entities + [(0, "ENDSEC"), (0, "EOF")]
        path.write_text("".join(f"{code}\n{value}\n" for code, value in groups), encoding="utf-8")

    with tempfile.TemporaryDirectory(prefix="tecnigrafo-oracle-") as directory:
        root = Path(directory)
        source = root / "unsupported"
        mirrored = root / "negative-ocs"
        # Handwritten DXF records, not values exported by Tecnigrafo or ezdxf.
        line = [(0, "LINE"), (5, "2F"), (100, "AcDbEntity"), (8, "0"),
                (100, "AcDbLine"), (10, 0), (20, 0), (11, 10), (21, 5)]
        unsupported = [(0, "POINT"), (5, "30"), (100, "AcDbEntity"), (8, "0"),
                       (100, "AcDbPoint"), (10, 3), (20, 4)]
        circle = [(0, "CIRCLE"), (5, "31"), (100, "AcDbEntity"), (8, "0"),
                  (100, "AcDbCircle"), (10, 5), (20, 6), (30, 2), (40, 3),
                  (39, 0.5), (210, 0), (220, 0), (230, -1)]
        dxf(source, line + unsupported)
        dxf(mirrored, circle, units=4)
        properties = root / "properties"
        missing_dimstyle = root / "missing-dimstyle"
        tables = [
            (0, "TABLE"), (2, "LTYPE"), (70, 1),
            (0, "LTYPE"), (5, "40"), (100, "AcDbSymbolTableRecord"),
            (100, "AcDbLinetypeTableRecord"), (2, "DashDot"), (70, 0),
            (3, 'Dash; "dot"'), (72, 65), (73, 4), (40, 5),
            (49, 3), (74, 0), (49, -1), (74, 0),
            (49, 0), (74, 0), (49, -1), (74, 0), (0, "ENDTAB"),
            (0, "TABLE"), (2, "LAYER"), (70, 2),
            (0, "LAYER"), (5, "41"), (100, "AcDbSymbolTableRecord"),
            (100, "AcDbLayerTableRecord"), (2, "0"), (70, 0),
            (62, 7), (6, "Continuous"),
            (0, "LAYER"), (5, "42"), (100, "AcDbSymbolTableRecord"),
            (100, "AcDbLayerTableRecord"), (2, "Walls"), (70, 5),
            (62, -3), (420, 16744448), (6, "DashDot"),
            (370, 50), (290, 0), (0, "ENDTAB"),
        ]
        styled = [(code, "walls" if code == 8 else value) for code, value in line]
        styled[4:4] = [(62, 1), (420, 66051), (6, "dashdot"), (48, 2.5), (370, 35)]
        dxf(properties, styled, tables=tables)
        dimstyle_table = [
            (0, "TABLE"), (2, "DIMSTYLE"), (70, 1),
            (0, "DIMSTYLE"), (105, "50"), (100, "AcDbSymbolTableRecord"),
            (100, "AcDbDimStyleTableRecord"), (2, "EMPTY"), (70, 0),
            (0, "ENDTAB"),
        ]
        dxf(missing_dimstyle, [], tables=dimstyle_table)
        for path in (source, mirrored, properties):
            print(f"=== Handwritten DXF probe: {path.name} ===", flush=True)
            result = subprocess.run(
                [sys.executable, str(Path(__file__).with_name("ezdxf_attesi.py")), str(path)],
                capture_output=True, text=True,
            )
            print(result.stdout, end="")
            require(f"{path.name} standalone probe exit status", 0, result.returncode)
            if result.stderr:
                print(result.stderr, end="", file=sys.stderr)
        drawing = DrawingAnswers(source)
        require('missing INSUNITS must be zero', '0', drawing.answer('units'))
        require('managed count', 'LINE 1', drawing.answer('count'))
        require('unsupported count', 'POINT 1', drawing.answer('unhandled'))
        require('handle lookup', 'unhandled POINT', drawing.answer('entity', '0030'))
        require('missing handle', 'none', drawing.answer('entity', 'FF'))
        require('negative OCS must stay raw',
                'CIRCLE center=5,6 radius=3 z=2 thickness=0.5 extrusion=0,0,-1',
                DrawingAnswers(mirrored).answer('entity', '31'))

        require('do not invent missing layers', 'none', drawing.answer('layers'))
        require('do not invent missing linetypes', 'none', drawing.answer('linetypes'))
        styled_drawing = DrawingAnswers(properties)
        require('missing DIMSTYLE groups use AutoCAD DXF defaults',
                ('"EMPTY" DIMASZ:0.18 DIMCEN:0.09 DIMDEC:4 DIMDSEP:46 '
                 'DIMEXE:0.18 DIMEXO:0.0625 DIMGAP:0.09 DIMTAD:0 DIMTIH:1 '
                 'DIMTOH:1 DIMTXT:0.18 DIMZIN:0'),
                DrawingAnswers(missing_dimstyle).answer('dimstyles'))
        expected_layers = ('"0" color=7 linetype="Continuous" lineweight=default; '
                           '"Walls" color=255,128,0 linetype="DashDot" '
                           'lineweight=0.50 off frozen locked noplot')
        expected_linetypes = ('"DashDot" pattern=3,-1,0,-1 '
                              'description="Dash; \\"dot\\""')
        expected_entity = ('LINE start=0,0 end=10,5 layer="Walls" color=1,2,3 '
                           'linetype="DashDot" ltscale=2.5 lineweight=0.35')
        require('layer fields and states', expected_layers, styled_drawing.answer('layers'))
        require('signed pattern and quotes', expected_linetypes, styled_drawing.answer('linetypes'))
        require('entity properties, table spelling and RGB precedence',
                expected_entity,
                styled_drawing.answer('entity', '2F'))
        raw_entity = styled_drawing.entities["2F"][0]
        for index, expected in ((0, "byblock"), (1, "1"), (256, "bylayer")):
            raw_entity.dxf.discard("true_color")
            raw_entity.dxf.color = index
            written = styled_drawing.answer("entity", "2F")
            field = f"color={expected}" if index != 256 else "color="
            require_contains(f"entity color {index}", field, written, present=index != 256)
        for weight, expected in ((0, "0"), (-2, "byblock"), (-3, "default"), (-1, None)):
            raw_entity.dxf.lineweight = weight
            written = styled_drawing.answer("entity", "2F")
            field = f"lineweight={expected}" if expected else "lineweight="
            require_contains(f"entity lineweight {weight}", field, written, present=bool(expected))
        raw_entity.dxf.layer = "0"
        raw_entity.dxf.linetype = "BYLAYER"
        raw_entity.dxf.ltscale = 1
        require('all default entity properties must be omitted',
                'LINE start=0,0 end=10,5',
                styled_drawing.answer('entity', '2F'))
        for name in ("ByBlock", "BYLAYER", "Continuous"):
            styled_drawing.linetypes.append(factory.new("LTYPE", dxfattribs={"name": name}))
        require('omit built-in types', expected_linetypes, styled_drawing.answer('linetypes'))
        styled_drawing.linetypes[0].pattern_tags.tags.clear()
        require('empty pattern is continuous',
                '"DashDot" pattern=continuous description="Dash; \\"dot\\""',
                styled_drawing.answer('linetypes'))
        require("ACI endpoints", ("255,0,0", "255,255,255"), (aci_answer(1), aci_answer(255)))
        with redirect_stdout(io.StringIO()) as standard:
            print_standard_answers()
        standard_lines = standard.getvalue().splitlines()
        require("one standard heading and all 255 ACI answers",
                (256, ["=== ezdxf: standard ==="]), (len(standard_lines), standard_lines[:1]))
        for index, output in enumerate(standard_lines[1:], 1):
            require(f"standard ACI output {index}", f"? aci {index} -> {aci_answer(index)}", output)
        for index in (0, 256):
            try:
                answer = aci_answer(index)
            except ValueError:
                pass
            else:
                require(f"invalid ACI index {index}", "ValueError", answer)

        # A drawing written by ezdxf exercises every stage 0.8 answer family.
        stage = root / "stage-0.8.dxf"
        document = ezdxf.new("R2018")
        for name in DIMSTYLE_FIELDS:
            document.dimstyles.get("Standard").dxf.set(name, ISO_25_DEFAULTS[name])
        document.dimstyles.new("ISO-25", dxfattribs={
            name: ISO_25_DEFAULTS[name] for name in DIMSTYLE_FIELDS
        })
        block = document.blocks.new("TAGGED", base_point=(1, 2))
        attdef = block.add_attdef("CODE", insert=(0, 0), height=1.8, text="N")
        block_line = block.add_line((0, 0), (4, 0), dxfattribs={"color": 0})
        model = document.modelspace()
        insert = model.add_blockref("TAGGED", (10, 20))
        insert.add_attrib("CODE", "A1", insert=(10, 20), dxfattribs={"height": 1.8})
        insert.add_attrib("HIDDEN", "secret", insert=(10, 20),
                          dxfattribs={"height": 1.8, "flags": 1})
        insert.dxf.color = 3
        solid = model.add_solid([(0, 0), (2, 0), (0, 1), (2, 1)])
        mtext = model.add_mtext("first\\Psecond", dxfattribs={"insert": (0, 5),
                                                               "char_height": 2.5,
                                                               "width": 20})
        model.add_mtext("downward", dxfattribs={"insert": (0, 6),
                                                "char_height": 2.5,
                                                "extrusion": (0, 0, -1)})
        dimension = model.add_linear_dim(base=(0, 8), p1=(0, 0), p2=(5, 0),
                                         dimstyle="ISO-25")
        dimension.render()
        # AutoCAD always writes group 42; ezdxf's renderer leaves it out.
        dimension.dimension.dxf.actual_measurement = 5
        hatch = model.add_hatch(color=2)
        hatch.paths.add_polyline_path([(0, 0), (5, 0), (5, 5), (0, 5)],
                                      is_closed=True)
        document.saveas(stage)
        stage_answers = DrawingAnswers(stage, set())
        solid_answers = DrawingAnswers(stage, {"SOLID"})
        require_contains('inactive stage SOLID is not managed',
                         'SOLID 1',
                         stage_answers.answer('count'), present=False)
        require_contains('inactive stage SOLID is unhandled',
                         'SOLID 1',
                         stage_answers.answer('unhandled'))
        require_contains('active stage SOLID is managed', 'SOLID 1', solid_answers.answer('count'))
        require_contains('active stage SOLID is not unhandled',
                         'SOLID 1',
                         solid_answers.answer('unhandled'), present=False)
        require_contains('other stage types remain unhandled',
                         'MTEXT 2',
                         solid_answers.answer('unhandled'))
        require_contains('other stage types remain unhandled',
                         'HATCH 1',
                         solid_answers.answer('unhandled'))

        mtext_answers = DrawingAnswers(stage, {"MTEXT"})
        require_contains('upward MTEXT is managed', 'MTEXT 1', mtext_answers.answer('count'))
        require_contains('downward MTEXT is unhandled',
                         'MTEXT 1',
                         mtext_answers.answer('unhandled'))

        insert_answers = DrawingAnswers(stage, {"ATTRIB", "INSERT"})
        for question in ("count", "unhandled"):
            require_contains(f'inactive linked ATTRIB is absent from {question}',
                             'ATTRIB',
                             stage_answers.answer(question), present=False)
        require_contains('active linked ATTRIB is managed',
                         'ATTRIB 2',
                         insert_answers.answer('count'))
        require_contains('active managed ATTRIB is not unhandled',
                         'ATTRIB',
                         insert_answers.answer('unhandled'), present=False)
        attribute_handle = insert.attribs[0].dxf.handle
        attribute_answer = stage_answers.answer("entity", attribute_handle)
        require("inactive linked ATTRIB keeps its entity answer",
                "ATTRIB owner=", attribute_answer[:len("ATTRIB owner=")])
        require('activating ATTRIB must not change its entity answer',
                insert_answers.answer('entity', attribute_handle),
                stage_answers.answer('entity', attribute_handle))

        unchanged_questions = ("version", "units", "layers", "linetypes", "blocks",
                               "styles", "dimstyles")
        for question in unchanged_questions:
            require(f'activating SOLID must not change {question}',
                    solid_answers.answer(question),
                    stage_answers.answer(question))
        for handle, (entity, _) in stage_answers.entities.items():
            require(f"activating SOLID must not change entity {handle}",
                    solid_answers.answer('entity', handle),
                    stage_answers.answer('entity', handle))
            if entity.dxftype() in {"INSERT", "DIMENSION"}:
                require(f"activating SOLID must not change generated geometry for {handle}",
                        solid_answers.answer('generated', handle),
                        stage_answers.answer('generated', handle))
        attribute = insert.attribs[0]
        expected_entities = {
            solid.dxf.handle: "SOLID vertex=0,0 vertex=2,0 vertex=0,1 vertex=2,1",
            mtext.dxf.handle: ('MTEXT insertion=0,5 height=2.5 width=20 attachment=top-left '
                               'rotation=0 style="Standard" value="first\\\\Psecond"'),
            dimension.dimension.dxf.handle: ('DIMENSION kind=linear style="ISO-25" '
                                              'origin1=0,0 origin2=5,0 line-point=0,8 rotation=0 '
                                              'text-point=2.5,9.875 measurement=5 text="<>"'),
            hatch.dxf.handle: ('HATCH pattern="SOLID" solid=yes angle=0 scale=1 islands=normal '
                               'loop=external+polyline vertex=0,0 vertex=5,0 vertex=5,5 '
                               'vertex=0,5 color=2'),
            attribute.dxf.handle: (f'ATTRIB owner={insert.dxf.handle} position=10,20 '
                                   'alignment-point=0,0 height=1.8 rotation=0 width-factor=1 '
                                   'oblique=0 style="Standard" mirror-x=no mirror-y=no '
                                   'horizontal=left vertical=baseline value="A1" tag="CODE"'),
        }
        for handle, expected in expected_entities.items():
            require(f'complete {expected.split()[0]} entity answer',
                    expected,
                    stage_answers.answer('entity', handle))
        expected_attdef = ('ATTDEF position=0,0 alignment-point=0,0 height=1.8 rotation=0 '
                           'width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no '
                           'horizontal=left vertical=baseline value="N" tag="CODE" prompt=""')
        require('complete ATTDEF entity answer', expected_attdef, entity_answer(attdef))
        expected_blocks = (f'"TAGGED" base=1,2; "TAGGED" {attdef.dxf.handle} {expected_attdef}; '
                           f'"TAGGED" {block_line.dxf.handle} LINE start=0,0 end=4,0 color=byblock')
        # Rendering the dimension adds ezdxf's arrow block; check TAGGED rows only.
        tagged_rows = "; ".join(row.strip()
                                for row in answer_items(stage_answers.answer("blocks"))
                                if row.strip().startswith('"TAGGED" '))
        require('complete block rows', expected_blocks, tagged_rows)
        require('complete STYLE rows',
                '"Standard" font="txt" height=0 width-factor=1 oblique=0 backwards=no upside-down=no',
                stage_answers.answer('styles'))
        require('complete DIMSTYLE rows', '"Standard"; "ISO-25"', stage_answers.answer('dimstyles'))
        require('complete INSERT generated answer with effective properties',
                ('LINE start=9,18 end=13,18 color=0,255,0; '
                 'TEXT position=10,20 alignment-point=0,0 height=1.8 rotation=0 '
                 'width-factor=1 oblique=0 style="Standard" mirror-x=no mirror-y=no '
                 'horizontal=left vertical=baseline value="A1"'),
                stage_answers.answer('generated', insert.dxf.handle))
        require('out-of-scope DIMENSION has no generated answer',
                'none',
                stage_answers.answer('generated', dimension.dimension.dxf.handle))
        with redirect_stdout(io.StringIO()) as generated_log:
            stage_answers.print_answers()
        outside = (f"; generated {dimension.dimension.dxf.handle}: "
                   "outside oracle scope (INSERT,MTEXT,POINT)")
        require_contains('complete outside-oracle-scope log row',
                         outside,
                         generated_log.getvalue().splitlines())

        insert.attribs[0].dxf.extrusion = (1, 0, 0)
        document.saveas(stage)
        unsupported_attribute_answers = DrawingAnswers(stage, {"ATTRIB", "INSERT"})
        require_contains('every ATTRIB of an unhandled INSERT is unhandled',
                         'ATTRIB 2',
                         unsupported_attribute_answers.answer('unhandled'))
        require_contains('no ATTRIB of an unhandled INSERT is managed',
                         'ATTRIB',
                         unsupported_attribute_answers.answer('count'), present=False)
        require('an ATTRIB of an unhandled INSERT is unhandled',
                'unhandled ATTRIB',
                unsupported_attribute_answers.answer('entity', insert.attribs[1].dxf.handle))

        exclusions = root / "stage-0.8-exclusions.dxf"
        excluded_document = ezdxf.new("R2018")
        excluded_model = excluded_document.modelspace()
        missing_insert = excluded_model.add_blockref("MISSING", (0, 0))
        bad_block = excluded_document.blocks.new("BAD_ATTRIBUTE")
        bad_block.add_attdef("BAD", insert=(0, 0), height=1)
        bad_insert = excluded_model.add_blockref("BAD_ATTRIBUTE", (0, 0))
        bad_insert.add_attrib("BAD", "x", insert=(0, 0),
                              dxfattribs={"height": 1, "extrusion": (1, 0, 0)})
        missing_dimension = excluded_model.add_linear_dim(base=(0, 2), p1=(0, 0), p2=(1, 0))
        excluded_document.saveas(exclusions)
        exclusion_answers = DrawingAnswers(exclusions, {"ATTRIB", "INSERT", "DIMENSION"})
        for handle, kind in ((missing_insert.dxf.handle, "INSERT"),
                             (bad_insert.dxf.handle, "INSERT"),
                             (missing_dimension.dimension.dxf.handle, "DIMENSION")):
            require(f'excluded {kind} remains unhandled',
                    f'unhandled {kind}',
                    exclusion_answers.answer('entity', handle))

        effective = root / "stage-0.8-effective.dxf"
        effective_document = ezdxf.new("R2018")
        effective_document.layers.new("RED", dxfattribs={"color": 1})
        effective_document.layers.new("BLUE", dxfattribs={"color": 5})
        effective_block = effective_document.blocks.new("BYBLOCK")
        effective_block.add_line((0, 0), (1, 0),
                                 dxfattribs={"layer": "RED", "color": 0})
        effective_insert = effective_document.modelspace().add_blockref(
            "BYBLOCK", (0, 0), dxfattribs={"layer": "BLUE", "color": 256})
        effective_document.saveas(effective)
        effective_answers = DrawingAnswers(effective, {"INSERT"})
        require("ByBlock uses the owner's effective layer color",
                'LINE start=0,0 end=1,0 color=0,0,255',
                effective_answers.answer('generated', effective_insert.dxf.handle))
        effective_insert.dxf.color = 0
        effective_document.saveas(effective)
        require('ByBlock in a ByBlock owner is color 7',
                'LINE start=0,0 end=1,0',
                DrawingAnswers(effective, {'INSERT'}).answer('generated', effective_insert.dxf.handle))
        effective_insert.dxf.color = 256
        effective_document.layers.get("BLUE").off()
        effective_document.saveas(effective)
        require('an owner on an off layer generates no shapes',
                'none',
                DrawingAnswers(effective, {'INSERT'}).answer('generated', effective_insert.dxf.handle))
        effective_document.layers.get("BLUE").on()
        effective_document.layers.get("BLUE").freeze()
        effective_document.saveas(effective)
        require('an owner on a frozen layer generates no shapes',
                'none',
                DrawingAnswers(effective, {'INSERT'}).answer('generated', effective_insert.dxf.handle))

        # The original owner, paper-space flag and degenerate values must survive
        # ezdxf's document layout binding and affect classification.
        classified = root / "classification"
        paper = [(67, 1)]
        owner = [(330, "FFFF")]
        dxf(classified, line[:3] + paper + line[3:] +
            [(code, "32" if code == 5 else value) for code, value in line[:2]] + owner + line[2:] +
            [(code, "33" if code == 5 else 0 if code == 40 else value) for code, value in circle])
        classification = DrawingAnswers(classified)
        require('unhandled classification', 'none', classification.answer('count'))
        require('unhandled details', 'CIRCLE 1; LINE 2', classification.answer('unhandled'))

        grid_file = root / "custom-grid"
        grid_document = ezdxf.new("R2018")
        viewport = grid_document.viewports.get("*Active")[0]
        viewport.dxf.update({
            "grid_on": 0, "snap_on": 1, "grid_spacing": (0, 0), "snap_spacing": (2.5, 4),
            "snap_base": (-1.5, 2), "unknown1": 6, "major_grid_lines": 7,
            "ucs_icon": 1, "snap_rotation": 22.5, "snap_style": 1,
        })
        grid_document.viewports.add("*ACTIVE")
        grid_document.header["$ATTMODE"] = 2
        grid_document.saveas(grid_file)
        attdisp_file = root / "hidden-attributes"
        attdisp_document = ezdxf.new("R2018")
        attdisp_document.viewports.delete_config("*Active")
        attdisp_document.viewports.add("Named")
        attdisp_document.header["$ATTMODE"] = 0
        attdisp_document.saveas(attdisp_file)

        default_grid = ("grid=on snap=off grid-unit=10,10 snap-unit=10,10 snap-base=0,0 "
                        "display=3 major=5 ucsicon=3 snap-angle=0 snap-style=0")
        custom_grid = ("grid=off snap=on grid-unit=0,0 snap-unit=2.5,4 snap-base=-1.5,2 "
                       "display=6 major=7 ucsicon=1 snap-angle=22.5 snap-style=1")
        settings_probes = [(grid_file, custom_grid, "on"), (attdisp_file, default_grid, "off"),
                           (source, default_grid, "normal")]
        settings_case_lines = ["; oracle: ezdxf"]
        for index, (path, expected_grid, expected_attdisp) in enumerate(settings_probes):
            name = f"D{index}"
            settings_case_lines.append(f"{name} = DXF source={path.name}")
            with redirect_stdout(io.StringIO()) as output:
                DrawingAnswers(path).print_answers(name)
            for question, expected in (("grid", expected_grid), ("attdisp", expected_attdisp)):
                row = f"? {question} {name} -> {expected}"
                require_contains(f"{path.name} prints {question}", row, output.getvalue().splitlines())
                settings_case_lines.append(row)

        case = root / "cases/probe/domande.tgq"
        case.parent.mkdir(parents=True)
        case.write_text("\n".join(settings_case_lines) + "\n", encoding="utf-8")
        require("drawing settings case comparison", (6, 0), check_case(case, root))

        case.write_text(
            "; oracle: ezdxf\n!tolerance absolute -1\n? grid MISSING -> none\n"
            "? aci 0 -> none\n? aci 1 -> 255,0,0\n", encoding="utf-8")
        with redirect_stdout(io.StringIO()):
            result = check_case(case, root)
        require("invalid case lines do not stop later questions", (1, 3), result)

        prefix = "; oracle: ezdxf\nD = DXF source=unsupported\n"
        case.write_text(prefix + "? count D -> LINE 1\n? entity D 2f -> LINE start=0,0 end=10,5\n"
                        "? roundtrip D -> identical\n", encoding="utf-8")
        require('matching case and unsupported question', (2, 0), check_case(case, root))
        case.write_text(prefix + "!tolerance absolute 0.01\n"
                        "? entity D 2F -> LINE start=0,0 end=10.005,5\n"
                        "!tolerance absolute 0\n? entity D 2F -> LINE start=0,0 end=10.005,5\n",
                        encoding="utf-8")
        with redirect_stdout(io.StringIO()) as diagnostic:
            require('per-line tolerances and mismatch', (2, 1), check_case(case, root))
        require_contains("mismatch diagnostic shows expected value", "expected:", diagnostic.getvalue())
        require_contains("mismatch diagnostic shows oracle value", "ezdxf:", diagnostic.getvalue())
        require('unordered counts',
                True,
                equal_answer('count', 'LINE 1; ARC 2', 'ARC 2; LINE 1', 0, 0))
        require('literal text must be exact',
                True,
                not equal_answer('entity', 'TEXT value="A  B"', 'TEXT value="A B"', 1, 1))
        require('derived properties',
                True,
                equal_answer('entity', 'TEXT value="1" ~box=0,0;1,0', 'TEXT value="1"', 0, 0))
        require('relative tolerance', True, equal_answer('units', '100.01', '100', 0, 0.001))

        case.write_text('; oracle: ezdxf\nD = DXF source=properties\n'
                        f'? layers D -> {expected_layers}\n'
                        f'? linetypes D -> {expected_linetypes}\n'
                        f'? entity D 2F -> {expected_entity}\n'
                        '? aci 1 -> 255,0,0\n? aci 255 -> 255,255,255\n',
                        encoding="utf-8")
        require('all new oracle questions', (5, 0), check_case(case, root))
        case.write_text(case.read_text(encoding="utf-8")
                        .replace("color=1,2,3", "color=1,2,4")
                        .replace("pattern=3,-1,0,-1", "pattern=3,-2,0,-1")
                        .replace("off frozen", "frozen")
                        .replace("? aci 1 -> 255,0,0", "? aci 1 -> 0,0,0"),
                        encoding="utf-8")
        with redirect_stdout(io.StringIO()):
            require('new answer mismatches must fail', (5, 4), check_case(case, root))
        reversed_layers = "; ".join(reversed(answer_items(expected_layers)))
        require("unordered layer rows", True,
                equal_answer("layers", expected_layers, reversed_layers, 0, 0))
        require('extra table rows must fail',
                True,
                not equal_answer('layers', expected_layers, expected_layers + '; "extra" color=7', 0, 0))
        changed_description = expected_linetypes.replace("Dash;", "Dash ;")
        require("quoted semicolons and literal descriptions", False,
                equal_answer("linetypes", expected_linetypes, changed_description, 1, 1))
        require("linetype numeric tolerance", True,
                equal_answer("linetypes", '"A" pattern=3,-1 description=""',
                             '"A" pattern=3.001,-1 description=""', 0.01, 0))

        good = root / "good"
        good.mkdir()
        document = ezdxf.new("R2018")
        document.modelspace().add_line((0, 0), (10, 5))
        document.saveas(good / "output.dxf")
        with zipfile.ZipFile(good / "output.cadx", "w") as archive:
            archive.write(good / "output.dxf", "drawing.dxf")
        require('DXF and embedded CADX audit', 0, audit_outputs([good]))
        bad = root / "bad"
        bad.mkdir()
        (bad / "broken.dxf").write_text("invalid\n", encoding="utf-8")
        with zipfile.ZipFile(bad / "broken.cadx", "w") as archive:
            archive.writestr("other.txt", "missing drawing")
        with redirect_stdout(io.StringIO()):
            require('invalid DXF and CADX must fail', 2, audit_outputs([bad]))
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print("Oracle self-test passed: probes, drawing settings, comparison, tolerances, "
          "classification and audit.")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cases", type=Path)
    parser.add_argument("sources", type=Path)
    parser.add_argument("--self-test", action="store_true",
                        help="also exercise handwritten probes and oracle checks in temporary files")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--print-only", action="store_true",
                      help="print public source answers, including after a failed verification")
    mode.add_argument("--check-only", action="store_true",
                      help="check declared cases and audit output, without printing source answers")
    parser.add_argument("--outputs", type=Path, action="append",
                        help="case output root; repeatable; default: build/ci/tests/verifica/casi*")
    args = parser.parse_args()
    if args.self_test and self_test():
        return 1
    failures = 0
    if not args.check_only:
        source_errors = []
        print_standard_answers()
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
                source_errors.append(f"{source['name']}: {error}")
        if source_errors:
            # Keep failures inside the short CI annotation tail even after a
            # source with hundreds of entity answers.
            print("\n" * 25, end="")
            print("\n".join(source_errors))
    if not args.print_only:
        cases = questions = 0
        for path in sorted(args.cases.rglob("domande.tgq")):
            try:
                if path.read_text(encoding="utf-8-sig").splitlines()[:1] != ["; oracle: ezdxf"]:
                    continue
                cases += 1
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
