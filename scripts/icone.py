#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# Checks the icons of Tecnigrafo against the rules of docs/ICONE.md and the
# geometry that each icon declares (docs/adr/0040-icone-a-ruoli.md, point 4;
# docs/tappe/0.7.5/P11-icone-a-ruoli.md).
#
#     scripts/icone.py [folder]        (default: resources/icons/)
#
# Every *.svg of the folder is read. The grid comes from the name:
# only <name>-16.svg and <name>-32.svg are accepted (ADR 0040).
# The old 24 px grid is no longer supported. A problem is printed as
# "file:line: message"; the exit status is 1 when there is any.
#
# The geometry is declared on a point (a filled circle):
#
#   data-on="<id>"         its centre lies on the element with that id
#                          (segment, circle, rectangle, polyline, polygon, or
#                          any segment or arc of a path);
#   data-center-of="<id>"  its centre is the centre of the circle with that
#                          id, or of every arc of the path with that id.
#
# Both are checked within kTolerance pixels.
#
# The ink is aligned to the pixels (docs/tappe/0.7.5/P40, point 4; the
# decision of 2026-11-10 in docs/DECISIONI.md): the stroke is 1 px, and every
# horizontal or vertical stroked segment has its middle line on the middle of
# a pixel (n + 0.5), so that it covers one whole row or column; a stroked
# circle or arc has its centre plus and minus its radius there, in x and in y;
# a point has its centre there (its edges on whole pixels); a filled shape
# without stroke has its horizontal and vertical sides on whole pixels.
# Diagonals and other curves are not aligned. Checked within ALIGNMENT.
#
# Standard library only, Python 3.11 (the CI job "Scripts (Python)").

from __future__ import annotations

import math
import re
import sys
import xml.parsers.expat
from dataclasses import dataclass, field
from pathlib import Path

TOLERANCE = 0.1  # px, docs/adr/0040, point 4
ALIGNMENT = 0.01  # px, docs/tappe/0.7.5/P40, point 4
EPSILON = 1e-6  # px, for comparisons of written numbers

ALLOWED_ELEMENTS = frozenset(
    {"svg", "g", "path", "circle", "line", "rect", "polyline", "polygon"})
DRAWN_ELEMENTS = frozenset({"path", "circle", "line", "rect", "polyline", "polygon"})
ROLES = ("geo", "pt", "cost", "esist")
# The presentation attributes that an element passes to its children.
INHERITED = ("fill", "stroke", "stroke-width", "stroke-linecap", "stroke-linejoin",
             "stroke-dasharray")
DECLARATIONS = ("data-on", "data-center-of")


@dataclass(frozen=True)
class Grid:
    """The rules that depend on the grid (docs/ICONE.md, "Due griglie")."""

    size: int
    stroke: float  # width of every stroke
    margin: float  # px of each side left free of ink
    dot: float  # radius of a point
    dash: float  # shortest dash or gap of a dashed stroke


# The stroke of 1 px of the decision of 2026-11-10, which replaces the one of
# docs/adr/0040, point 3 (docs/ICONE.md, "Due griglie").
GRIDS = {
    16: Grid(16, 1.0, 1.0, 1.5, 2.0),
    32: Grid(32, 1.0, 2.0, 2.5, 2.0),
}


def grid_of(name: str) -> Grid | None:
    """The grid of the file `name`, from its suffix."""
    for size in (16, 32):
        if name.endswith(f"-{size}.svg"):
            return GRIDS[size]
    return None


@dataclass
class Element:
    name: str
    attributes: dict[str, str]
    line: int
    parent: Element | None = None
    children: list[Element] = field(default_factory=list)


@dataclass(frozen=True)
class Problem:
    line: int
    message: str


class Malformed(Exception):
    def __init__(self, line: int, message: str) -> None:
        super().__init__(message)
        self.line = line


def parse(data: bytes) -> tuple[Element | None, list[str]]:
    """The element tree of `data`, with line numbers, and its comments."""
    parser = xml.parsers.expat.ParserCreate()
    root: list[Element] = []
    stack: list[Element] = []
    comments: list[str] = []

    def start(name: str, attributes: dict[str, str]) -> None:
        parent = stack[-1] if stack else None
        element = Element(name, dict(attributes), parser.CurrentLineNumber, parent)
        if parent is None:
            root.append(element)
        else:
            parent.children.append(element)
        stack.append(element)

    def end(_name: str) -> None:
        stack.pop()

    parser.StartElementHandler = start
    parser.EndElementHandler = end
    parser.CommentHandler = comments.append
    try:
        parser.Parse(data, True)
    except xml.parsers.expat.ExpatError as e:
        raise Malformed(e.lineno, f"not well-formed XML: {xml.parsers.expat.errors.messages[e.code]}") from e
    return (root[0] if root else None), comments


# --- Geometry ----------------------------------------------------------------

Point = tuple[float, float]


@dataclass(frozen=True)
class Segment:
    a: Point
    b: Point


@dataclass(frozen=True)
class Arc:
    """An elliptical arc in centre form (SVG 1.1, appendix F.6.5)."""

    start: Point
    end: Point
    center: Point
    rx: float
    ry: float
    phi: float  # rotation of the x axis, radians
    theta: float  # start angle, radians
    delta: float  # signed sweep, radians

    def at(self, t: float) -> Point:
        cos_phi, sin_phi = math.cos(self.phi), math.sin(self.phi)
        x, y = self.rx * math.cos(t), self.ry * math.sin(t)
        return (self.center[0] + cos_phi * x - sin_phi * y,
                self.center[1] + sin_phi * x + cos_phi * y)

    @property
    def circular(self) -> bool:
        return abs(self.rx - self.ry) <= EPSILON


@dataclass(frozen=True)
class Curve:
    """A quadratic or cubic Bézier curve."""

    points: tuple[Point, ...]

    def at(self, t: float) -> Point:
        points = list(self.points)
        while len(points) > 1:
            points = [((1 - t) * p[0] + t * q[0], (1 - t) * p[1] + t * q[1])
                      for p, q in zip(points, points[1:])]
        return points[0]


@dataclass(frozen=True)
class Circle:
    center: Point
    r: float


Piece = Segment | Arc | Curve | Circle
SAMPLES = 720  # points per arc or curve when one is sampled


def angle_between(u: Point, v: Point) -> float:
    sign = -1.0 if u[0] * v[1] - u[1] * v[0] < 0 else 1.0
    dot = u[0] * v[0] + u[1] * v[1]
    norm = math.hypot(*u) * math.hypot(*v)
    return sign * math.acos(max(-1.0, min(1.0, dot / norm)))


def arc_from_endpoints(start: Point, end: Point, rx: float, ry: float, rotation: float,
                       large: bool, sweep: bool) -> Piece:
    """The arc of an SVG "A" command, as SVG 1.1, appendix F.6.5, computes it."""
    if math.dist(start, end) <= EPSILON:
        return Segment(start, end)  # the arc is omitted
    rx, ry = abs(rx), abs(ry)
    if rx <= EPSILON or ry <= EPSILON:
        return Segment(start, end)
    phi = math.radians(rotation % 360.0)
    cos_phi, sin_phi = math.cos(phi), math.sin(phi)
    dx, dy = (start[0] - end[0]) / 2, (start[1] - end[1]) / 2
    x1 = cos_phi * dx + sin_phi * dy
    y1 = -sin_phi * dx + cos_phi * dy
    scale = x1 * x1 / (rx * rx) + y1 * y1 / (ry * ry)
    if scale > 1:
        rx, ry = rx * math.sqrt(scale), ry * math.sqrt(scale)
    numerator = rx * rx * ry * ry - rx * rx * y1 * y1 - ry * ry * x1 * x1
    denominator = rx * rx * y1 * y1 + ry * ry * x1 * x1
    coefficient = math.sqrt(max(0.0, numerator / denominator))
    if large == sweep:
        coefficient = -coefficient
    cx1 = coefficient * rx * y1 / ry
    cy1 = -coefficient * ry * x1 / rx
    center = (cos_phi * cx1 - sin_phi * cy1 + (start[0] + end[0]) / 2,
              sin_phi * cx1 + cos_phi * cy1 + (start[1] + end[1]) / 2)
    u = ((x1 - cx1) / rx, (y1 - cy1) / ry)
    v = ((-x1 - cx1) / rx, (-y1 - cy1) / ry)
    theta = angle_between((1.0, 0.0), u)
    delta = angle_between(u, v)
    if not sweep and delta > 0:
        delta -= 2 * math.pi
    elif sweep and delta < 0:
        delta += 2 * math.pi
    return Arc(start, end, center, rx, ry, phi, theta, delta)


PATH_TOKEN = re.compile(r"[MmLlHhVvCcSsQqTtAaZz]|[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")
ARITY = {"M": 2, "L": 2, "H": 1, "V": 1, "C": 6, "S": 4, "Q": 4, "T": 2, "A": 7, "Z": 0}


def path_tokens(d: str) -> list[str]:
    tokens: list[str] = []
    position = 0
    for match in PATH_TOKEN.finditer(d):
        if d[position:match.start()].strip(" \t\r\n,"):
            raise ValueError(f"unexpected '{d[position:match.start()].strip()}' in the path data")
        tokens.append(match.group())
        position = match.end()
    if d[position:].strip(" \t\r\n,"):
        raise ValueError(f"unexpected '{d[position:].strip()}' in the path data")
    return tokens


def read_arguments(tokens: list[str], index: int, command: str) -> tuple[list[float], int]:
    count = ARITY[command.upper()]
    values: list[float] = []
    for k in range(count):
        if index >= len(tokens) or tokens[index].isalpha():
            raise ValueError(f"command {command} needs {count} numbers")
        token = tokens[index]
        if command.upper() == "A" and k in (3, 4):
            # A flag is one character; "01" is two flags, "1.5" is not a flag.
            if token[0] not in "01":
                raise ValueError(f"arc flag '{token}' is not 0 or 1")
            values.append(float(token[0]))
            rest = token[1:]
            if rest:
                tokens[index] = rest
                continue
        else:
            values.append(float(token))
        index += 1
    return values, index


def path_pieces(d: str) -> list[Piece]:
    """The segments, arcs and curves of the path data `d`."""
    tokens = path_tokens(d)
    pieces: list[Piece] = []
    current: Point = (0.0, 0.0)
    subpath_start: Point = (0.0, 0.0)
    last_control: Point | None = None
    last_command = ""
    index = 0
    command = ""
    while index < len(tokens):
        if tokens[index].isalpha():
            command = tokens[index]
            index += 1
        elif not command or command in "Zz":
            raise ValueError("the path data does not start with a command")
        if command in "Zz":
            pieces.append(Segment(current, subpath_start))
            current = subpath_start
            last_command, last_control = command, None
            continue
        values, index = read_arguments(tokens, index, command)
        relative = command.islower()
        upper = command.upper()
        ox, oy = current if relative else (0.0, 0.0)

        def absolute(i: int) -> Point:
            return (values[i] + ox, values[i + 1] + oy)

        control: Point | None = None
        if upper == "M":
            current = subpath_start = absolute(0)
            # Further pairs after a moveto are linetos.
            command = "l" if relative else "L"
        elif upper == "L":
            end = absolute(0)
            pieces.append(Segment(current, end))
            current = end
        elif upper == "H":
            end = (values[0] + ox, current[1])
            pieces.append(Segment(current, end))
            current = end
        elif upper == "V":
            end = (current[0], values[0] + oy)
            pieces.append(Segment(current, end))
            current = end
        elif upper in ("C", "S"):
            if upper == "C":
                first, control, end = absolute(0), absolute(2), absolute(4)
            else:
                reflect = last_control if last_command.upper() in ("C", "S") else None
                first = current if reflect is None else (
                    2 * current[0] - reflect[0], 2 * current[1] - reflect[1])
                control, end = absolute(0), absolute(2)
            pieces.append(Curve((current, first, control, end)))
            current = end
        elif upper in ("Q", "T"):
            if upper == "Q":
                control, end = absolute(0), absolute(2)
            else:
                reflect = last_control if last_command.upper() in ("Q", "T") else None
                control = current if reflect is None else (
                    2 * current[0] - reflect[0], 2 * current[1] - reflect[1])
                end = absolute(0)
            pieces.append(Curve((current, control, end)))
            current = end
        elif upper == "A":
            end = absolute(5)
            pieces.append(arc_from_endpoints(current, end, values[0], values[1], values[2],
                                             values[3] != 0, values[4] != 0))
            current = end
        last_command, last_control = upper, control
    return pieces


def samples(piece: Piece) -> list[Point]:
    """Points along `piece`, for bounds and for distances to curves."""
    if isinstance(piece, Segment):
        return [piece.a, piece.b]
    if isinstance(piece, Circle):
        return [(piece.center[0] + piece.r * math.cos(t), piece.center[1] + piece.r * math.sin(t))
                for t in (2 * math.pi * k / SAMPLES for k in range(SAMPLES))]
    if isinstance(piece, Arc):
        return [piece.at(piece.theta + piece.delta * k / SAMPLES) for k in range(SAMPLES + 1)]
    return [piece.at(k / SAMPLES) for k in range(SAMPLES + 1)]


def segment_distance(p: Point, a: Point, b: Point) -> float:
    dx, dy = b[0] - a[0], b[1] - a[1]
    length = dx * dx + dy * dy
    if length <= 0:
        return math.dist(p, a)
    t = max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / length))
    return math.dist(p, (a[0] + t * dx, a[1] + t * dy))


def distance(p: Point, piece: Piece) -> float:
    """The distance from `p` to `piece`: exact for segments, circles and
    circular arcs, from samples for the other curves."""
    if isinstance(piece, Segment):
        return segment_distance(p, piece.a, piece.b)
    if isinstance(piece, Circle):
        return abs(math.dist(p, piece.center) - piece.r)
    if isinstance(piece, Arc) and piece.circular:
        angle = math.atan2(p[1] - piece.center[1], p[0] - piece.center[0])
        start = piece.theta + piece.phi
        if piece.delta >= 0:
            inside = (angle - start) % (2 * math.pi) <= piece.delta + EPSILON
        else:
            inside = (start - angle) % (2 * math.pi) <= -piece.delta + EPSILON
        if inside:
            return abs(math.dist(p, piece.center) - piece.rx)
        return min(math.dist(p, piece.start), math.dist(p, piece.end))
    points = samples(piece)
    return min(segment_distance(p, a, b) for a, b in zip(points, points[1:]))


# --- The checks --------------------------------------------------------------


def number(value: str | None) -> float | None:
    if value is None:
        return None
    try:
        return float(value.strip().removesuffix("px"))
    except ValueError:
        return None


def fmt(value: float) -> str:
    return f"{value:.3f}".rstrip("0").rstrip(".")


def on_whole(value: float) -> bool:
    return abs(value - round(value)) <= ALIGNMENT


def on_half(value: float) -> bool:
    return on_whole(value - 0.5)


def point_text(p: Point) -> str:
    return f"({fmt(p[0])}, {fmt(p[1])})"


def numbers_in(value: str) -> list[float]:
    return [float(v) for v in re.findall(r"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?", value)]


def pieces_of(element: Element) -> list[Piece]:
    """The geometry of a drawn element; ValueError if it cannot be read."""
    a = element.attributes

    def required(name: str) -> float:
        value = number(a.get(name, "0"))
        if value is None:
            raise ValueError(f'{name}="{a.get(name)}" is not a number')
        return value

    if element.name == "circle":
        return [Circle((required("cx"), required("cy")), required("r"))]
    if element.name == "line":
        return [Segment((required("x1"), required("y1")), (required("x2"), required("y2")))]
    if element.name == "rect":
        x, y, w, h = required("x"), required("y"), required("width"), required("height")
        corners = [(x, y), (x + w, y), (x + w, y + h), (x, y + h)]
        return [Segment(p, q) for p, q in zip(corners, corners[1:] + corners[:1])]
    if element.name in ("polyline", "polygon"):
        values = numbers_in(a.get("points", ""))
        if len(values) % 2 or len(values) < 4:
            raise ValueError("points must be at least two pairs of numbers")
        points = list(zip(values[::2], values[1::2]))
        if element.name == "polygon":
            points.append(points[0])
        return [Segment(p, q) for p, q in zip(points, points[1:])]
    if element.name == "path":
        return path_pieces(a.get("d", ""))
    return []


def resolved(element: Element, name: str) -> str | None:
    """The value of the inherited attribute `name` for `element`."""
    node: Element | None = element
    while node is not None:
        if name in node.attributes:
            return node.attributes[name]
        node = node.parent
    return None


def role_of(element: Element) -> str | None:
    """The role class of `element` or of its nearest ancestor with one."""
    node: Element | None = element
    while node is not None:
        for token in node.attributes.get("class", "").split():
            if token in ROLES:
                return token
        node = node.parent
    return None


def walk(element: Element) -> list[Element]:
    result = [element]
    for child in element.children:
        result.extend(walk(child))
    return result


class Checker:
    def __init__(self, grid: Grid) -> None:
        self.grid = grid
        self.problems: list[Problem] = []

    def report(self, element: Element | int, message: str) -> None:
        line = element if isinstance(element, int) else element.line
        self.problems.append(Problem(line, message))

    def root(self, root: Element) -> None:
        if root.name != "svg":
            self.report(root, f"the root element is <{root.name}>, not <svg>")
            return
        size = str(self.grid.size)
        a = root.attributes
        view_box = numbers_in(a.get("viewBox", ""))
        if (a.get("width") != size or a.get("height") != size
                or view_box != [0, 0, self.grid.size, self.grid.size]):
            self.report(root, f'the {size} px grid is width="{size}" height="{size}" '
                              f'viewBox="0 0 {size} {size}"')

    def attributes(self, element: Element) -> None:
        g = self.grid
        for key, value in element.attributes.items():
            if key in ("fill", "stroke") and value not in ("none", "currentColor"):
                self.report(element, f'{key}="{value}": a colour is written; only none or currentColor')
            elif key in ("style", "color", "transform"):
                self.report(element, f"the attribute {key} is not allowed")
            elif key == "stroke-width" and number(value) != g.stroke:
                self.report(element, f'stroke-width="{value}": the stroke of the {g.size} px grid is {fmt(g.stroke)}')
            elif key in ("stroke-linecap", "stroke-linejoin") and value != "round":
                self.report(element, f'{key}="{value}": caps and joins are round')
            elif key == "class":
                self.classes(element, value.split())
            elif key.startswith("data-") and key not in DECLARATIONS:
                self.report(element, f"the attribute {key} is not one of {', '.join(DECLARATIONS)}")
            elif key == "stroke-dasharray" and value != "none":
                dashes = numbers_in(value)
                if not dashes or min(dashes) < g.dash - EPSILON:
                    self.report(element, f'stroke-dasharray="{value}": every dash and gap is at least '
                                         f'{fmt(g.dash)}')

    def classes(self, element: Element, tokens: list[str]) -> None:
        for token in tokens:
            if token == "accent":
                self.report(element, 'class="accent" is gone: an indicated point or an active element is pt')
            elif token not in ROLES:
                self.report(element, f'class="{token}": the roles are {", ".join(ROLES)}')
        if sum(token in ROLES for token in tokens) > 1:
            self.report(element, "an element has one role only")

    def is_dot(self, element: Element) -> bool:
        return (element.name == "circle" and resolved(element, "fill") == "currentColor"
                and resolved(element, "stroke") == "none")

    def drawn(self, element: Element, pieces: list[Piece]) -> None:
        g = self.grid
        fill, stroke = resolved(element, "fill"), resolved(element, "stroke")
        if fill is None or stroke is None:
            missing = "fill" if fill is None else "stroke"
            self.report(element, f"{missing} is not set: the default would be black")
        if role_of(element) is None:
            self.report(element, f"no role class ({', '.join(ROLES)}) on the element or its group")
        if fill == "currentColor":
            radius = number(element.attributes.get("r"))
            if not self.is_dot(element) or radius is None or abs(radius - g.dot) > EPSILON:
                self.report(element, f'only points are filled: circles with r="{fmt(g.dot)}" and no stroke')
            elif role_of(element) != "pt":
                self.report(element, "a point is of class pt")
        width = 0.0
        if stroke == "currentColor":
            width = number(resolved(element, "stroke-width")) or 1.0
            if abs(width - g.stroke) > EPSILON:
                self.report(element, f"the stroke is {fmt(width)}, the {g.size} px grid has {fmt(g.stroke)}")
            for key in ("stroke-linecap", "stroke-linejoin"):
                if resolved(element, key) != "round":
                    self.report(element, f"{key} must be round")
        self.margin(element, pieces, width / 2)
        if stroke == "currentColor":
            self.stroked_alignment(element, pieces)
        elif self.is_dot(element):
            self.dot_alignment(element, pieces)
        elif fill == "currentColor":
            self.fill_alignment(element, pieces)

    def misaligned(self, element: Element, what: str, axis: str, value: float, target: str) -> None:
        name = element.attributes.get("id")
        owner = f"'{name}'" if name is not None else f"<{element.name}>"
        self.report(element, f"{owner}: {what} at {axis}={fmt(value)} is not on {target}")

    def stroked_alignment(self, element: Element, pieces: list[Piece]) -> None:
        """A stroke of 1 px covers whole pixels: its middle line on n + 0.5."""
        target = "the middle of a pixel (n + 0.5)"
        for piece in pieces:
            if isinstance(piece, Segment):
                (ax, ay), (bx, by) = piece.a, piece.b
                if math.dist(piece.a, piece.b) <= EPSILON:
                    continue
                if abs(ay - by) <= EPSILON and not on_half(ay):
                    self.misaligned(element, "the horizontal segment", "y", ay, target)
                elif abs(ax - bx) <= EPSILON and not on_half(ax):
                    self.misaligned(element, "the vertical segment", "x", ax, target)
            elif isinstance(piece, Circle):
                self.round_alignment(element, "the circle", piece.center, piece.r, piece.r, target)
            elif isinstance(piece, Arc):
                self.round_alignment(element, "the arc", piece.center, piece.rx, piece.ry, target)

    def round_alignment(self, element: Element, what: str, center: Point, rx: float, ry: float,
                        target: str) -> None:
        for axis, c, r in (("x", center[0], rx), ("y", center[1], ry)):
            for edge in (c - r, c + r):
                if not on_half(edge):
                    self.misaligned(element, f"{what} (centre {point_text(center)}, radius {fmt(r)})",
                                    axis, edge, target)

    def dot_alignment(self, element: Element, pieces: list[Piece]) -> None:
        """A point has its centre on n + 0.5, and so its edges on whole pixels."""
        center = pieces[0].center  # type: ignore[union-attr]
        for axis, value in (("x", center[0]), ("y", center[1])):
            if not on_half(value):
                self.misaligned(element, f"the centre of the point {point_text(center)}", axis, value,
                                "the middle of a pixel (n + 0.5)")

    def fill_alignment(self, element: Element, pieces: list[Piece]) -> None:
        """A fill without stroke has its horizontal and vertical sides on whole pixels."""
        for piece in pieces:
            if not isinstance(piece, Segment) or math.dist(piece.a, piece.b) <= EPSILON:
                continue
            (ax, ay), (bx, by) = piece.a, piece.b
            if abs(ay - by) <= EPSILON and not on_whole(ay):
                self.misaligned(element, "the horizontal side", "y", ay, "a whole pixel")
            elif abs(ax - bx) <= EPSILON and not on_whole(ax):
                self.misaligned(element, "the vertical side", "x", ax, "a whole pixel")

    def margin(self, element: Element, pieces: list[Piece], half_width: float) -> None:
        points = [p for piece in pieces for p in samples(piece)]
        if not points:
            return
        low = min(min(p[0] for p in points), min(p[1] for p in points)) - half_width
        high = max(max(p[0] for p in points), max(p[1] for p in points)) + half_width
        m, size = self.grid.margin, self.grid.size
        if low < m - 1e-3 or high > size - m + 1e-3:
            self.report(element, f"ink in the {fmt(m)} px margin: it spans {fmt(low)} to {fmt(high)}, "
                                 f"the drawing area is {fmt(m)} to {fmt(size - m)}")

    def declarations(self, element: Element, ids: dict[str, Element],
                     geometry: dict[int, list[Piece]]) -> None:
        declared = [key for key in DECLARATIONS if key in element.attributes]
        if not declared:
            return
        if not self.is_dot(element) or role_of(element) != "pt":
            self.report(element, f"{declared[0]} is declared on a point only (a filled circle of class pt)")
            return
        pieces = geometry.get(id(element))
        if not pieces:
            return
        center = pieces[0].center  # type: ignore[union-attr]
        for key in declared:
            target_id = element.attributes[key]
            target = ids.get(target_id)
            if target is None:
                self.report(element, f'{key}="{target_id}": no element has that id')
                continue
            target_pieces = geometry.get(id(target))
            if target_pieces is None:
                self.report(element, f'{key}="{target_id}": <{target.name}> has no geometry')
                continue
            if key == "data-on":
                self.on(element, center, target_id, target_pieces)
            else:
                self.center_of(element, center, target_id, target_pieces)

    def on(self, element: Element, center: Point, target_id: str, pieces: list[Piece]) -> None:
        gap = min(distance(center, piece) for piece in pieces)
        if gap > TOLERANCE:
            self.report(element, f"the point {point_text(center)} is {fmt(gap)} px from '{target_id}', "
                                 f"which it is declared on (tolerance {fmt(TOLERANCE)})")

    def center_of(self, element: Element, center: Point, target_id: str, pieces: list[Piece]) -> None:
        curves = [p for p in pieces if isinstance(p, (Circle, Arc))]
        if not curves or len(curves) != len([p for p in pieces if not isinstance(p, Segment)]):
            self.report(element, f"'{target_id}' is not a circle or a path of arcs: it has no centre")
            return
        for curve in curves:
            if isinstance(curve, Arc) and not curve.circular:
                self.report(element, f"'{target_id}' has an elliptical arc: it has no single centre")
                return
            gap = math.dist(center, curve.center)
            if gap > TOLERANCE:
                radius = curve.r if isinstance(curve, Circle) else curve.rx
                ends = ""
                if isinstance(curve, Arc):
                    ends = (f"; its ends are {fmt(math.dist(center, curve.start))} and "
                            f"{fmt(math.dist(center, curve.end))} from the point")
                self.report(element, f"the point {point_text(center)} is not the centre "
                                     f"{point_text(curve.center)} of '{target_id}' (radius {fmt(radius)}"
                                     f"{ends}): {fmt(gap)} px away")


def check_data(data: bytes, name: str) -> list[Problem]:
    """The problems of the icon `data`, named `name` (which gives the grid)."""
    try:
        root, comments = parse(data)
    except Malformed as e:
        return [Problem(e.line, str(e))]
    if root is None:
        return [Problem(1, "no root element")]
    grid = grid_of(name)
    if grid is None:
        return [Problem(root.line, "unsupported icon name: use -16.svg or -32.svg; the 24 px grid is not supported")]
    checker = Checker(grid)
    if not any("SPDX-License-Identifier: GPL-3.0-or-later" in c for c in comments):
        checker.report(1, "no SPDX-License-Identifier comment")
    checker.root(root)
    elements = walk(root)
    ids: dict[str, Element] = {}
    geometry: dict[int, list[Piece]] = {}
    for element in elements:
        if element.name not in ALLOWED_ELEMENTS:
            checker.report(element, f"element <{element.name}> is not allowed")
        checker.attributes(element)
        identifier = element.attributes.get("id")
        if identifier is not None:
            if identifier in ids:
                checker.report(element, f'id="{identifier}" is used twice')
            ids[identifier] = element
        if element.name in DRAWN_ELEMENTS:
            try:
                geometry[id(element)] = pieces_of(element)
            except ValueError as e:
                checker.report(element, str(e))
                continue
            checker.drawn(element, geometry[id(element)])
    if not geometry:
        checker.report(root, "nothing is drawn")
    for element in elements:
        checker.declarations(element, ids, geometry)
    return sorted(checker.problems, key=lambda p: p.line)


def check_file(path: Path) -> list[Problem]:
    return check_data(path.read_bytes(), path.name)


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if len(args) > 1 or (args and args[0].startswith("-")):
        print("usage: scripts/icone.py [folder]   (default: resources/icons/)", file=sys.stderr)
        return 2
    folder = Path(args[0]) if args else Path(__file__).resolve().parent.parent / "resources" / "icons"
    files = sorted(folder.glob("*.svg"))
    if not files:
        print(f"{folder}: no *.svg icons", file=sys.stderr)
        return 1
    count = 0
    for path in files:
        for problem in check_file(path):
            print(f"{path}:{problem.line}: {problem.message}")
            count += 1
    print(f"{len(files)} icons, {count} problems", file=sys.stderr)
    return 1 if count else 0


if __name__ == "__main__":
    sys.exit(main())
