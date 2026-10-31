# [0.7-P14] `core`: facciata di solid-space, intersezioni, lato, regola delle tolleranze

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R14]` (#…), agente gemini: la facciata (dalle
  entità alle curve, posizioni, polilinee, unità); i valori geometrici
  dall'archivio dei casi di solid-space
- **Dipende da:** P0, Matleyx78/solid-space#20, Matleyx78/solid-space#23, R14.
  Le due richieste diventano i pacchetti di solid-space che realizzano le
  intestazioni qui sotto, quando la tappa 0.2 di solid-space li avrà (ADR 0030,
  punto 13)
- **Modulo:** `core` (più le domande in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2 e 4; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md) punti
  1, 2, 5 e 7; [ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md)
  punti 2–10; gli ADR di solid-space
  [0009](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0009-regola-delle-tolleranze-del-piano.md)
  (Decisione) e
  [0010](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0010-posizione-e-intersezioni-nel-piano.md)
  (Decisione); in `third_party/solid-space` le intestazioni elencate sotto;
  `core/include/tg/core/arc.hpp`, `polyline.hpp`, `proximity.hpp`;
  `docs/verifica/domande.md` ("Intersezioni, lato e tolleranze")

## Intestazioni di solid-space che usa

Il contratto accettato con gli ADR 0009 e 0010 di solid-space:

- `solid_space/robust/point.hpp`, `result.hpp`, `diagnosis.hpp`,
  `tolerance.hpp` (`TolerancePolicy::forUnit`, `epsilon`, `coincident`);
- `solid_space/geom2d/diagnosis.hpp`, `curves.hpp` (`Segment`, `Arc`,
  `Circle`, `PolylineSegment`, `Curve`, `Polyline`, `pieces`, `magnitude`),
  `intersection.hpp` (`intersectAlong`, con i suoi tipi), `side.hpp` (`side`),
  `coincidence.hpp` (`isDegenerate`).

## Prima di cominciare

- Le funzioni di solid-space si usano solo quando il pacchetto di solid-space
  che le realizza è unito (AGENTS.md, "Se serve qualcosa a solid-space"). La
  issue dice a quale commit di `main` di solid-space spostare il sottomodulo: il
  commit che chiude l'ultimo dei pacchetti di solid-space che realizzano le
  intestazioni qui sopra. Se la issue non lo dice, chiedilo e non cominciare.
- Questa scheda è scritta sul contratto. Se qualcosa che serve qui manca dalle
  intestazioni o non si comporta come dichiarano, **fermati** come dice
  AGENTS.md ("Se serve qualcosa a solid-space"): non scriverlo tu.

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/intersections.hpp`, `core/src/intersections.cpp` (nuovi)
- `core/include/tg/core/tolerance.hpp`, `core/src/tolerance.cpp` (nuovi)
- `core/src/solid_space_bridge.hpp`, `core/src/solid_space_bridge.cpp` (nuovi,
  interni a `core`: la traduzione)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp` (nuovo), `script/src/question_table.cpp`,
  `script/CMakeLists.txt`
- `tests/core/intersections_test.cpp`, `tests/core/tolerance_test.cpp` (nuovi),
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R14.txt` (da cancellare)

## Cosa fare

La facciata dell'ADR 0035, punto 1: `core` parla con `solid_space::geom2d` e
`solid_space::robust`, e chi usa `core` vede solo i tipi di Tecnigrafo. Le
intestazioni di solid-space si includono solo nei `.cpp` di `core` e in
`solid_space_bridge.hpp`.

### La traduzione (`solid_space_bridge.hpp`, interna)

```cpp
namespace tg::core::bridge {
namespace g = solid_space::geom2d;

/// The curve of a Line, Arc or Circle (docs/adr/0035, point 1). An Arc gets
/// sweep = endAngle - startAngle, plus g::kFullTurn when not positive,
/// computed in double in this order. std::nullopt for a Polyline or a Text.
[[nodiscard]] std::optional<g::Curve> curveOf(const Entity& entity);

/// The pieces of a polyline (g::pieces), one per Polyline::segment(), in the
/// same order. A segment whose PolylineSegment::isArc() is false gets bulge 0
/// (docs/adr/0036, point 7).
[[nodiscard]] std::vector<g::PolylineSegment> piecesOf(const Polyline& polyline);

/// solid-space's tolerance rule for a ToleranceRule.
[[nodiscard]] solid_space::robust::TolerancePolicy policyOf(const ToleranceRule& rule);
}
```

Gli altri pacchetti della tappa che usano solid-space (P15, P17, P19, P21, P22)
riusano questa traduzione.

### Intersezioni e lato (`intersections.hpp`)

```cpp
/// Which part of an entity takes part (docs/adr/0035, points 1 and 6).
enum class Extension : std::uint8_t {
    None,      ///< the entity as it is
    /// A line: its whole line. An arc: its whole circle. An open polyline: the
    /// line or circle of its first segment, for the points before its start,
    /// and of its last segment, for the points after its end. Circles, closed
    /// polylines and texts: as None.
    Extended,
};

/// Where a point lies on a segment of an entity, decided exactly by
/// solid-space: never deduced from stations.
enum class Location : std::uint8_t { BeforeStart, AtStart, Inside, AtEnd, AfterEnd };

/// A position on the curve of an entity.
struct CurvePosition {
    /// For a polyline, the index of the segment (Polyline::segment); else 0.
    std::size_t segment{0};
    Location location{Location::Inside};
    /// Arc length from the start of the segment (for an arc, counter-clockwise
    /// from its start angle; for a circle, from angle 0), as solid-space gives
    /// it: rounded, for information only, never to order or compare points.
    double station{0.0};
    friend bool operator==(const CurvePosition&, const CurvePosition&) = default;
};

/// How two curves meet at a point, decided exactly.
enum class Contact : std::uint8_t {
    Crossing,   ///< different tangent directions
    Touching,   ///< the same tangent line, different curves
    SameCurve,  ///< on the same line or circle, meeting only there
};

struct Crossing {
    Vec2 point;  ///< within the error solid-space declares
    Contact contact{Contact::Crossing};
    CurvePosition onA;
    CurvePosition onB;
};

/// A stretch two curves share, from its end nearer the start of A.
struct SharedStretch {
    bool whole{false};  ///< the whole line or circle (extended lines, equal circles)
    Vec2 from;
    Vec2 to;
    CurvePosition fromOnA, toOnA, fromOnB, toOnB;
};

struct Intersections {
    /// In the exact order along A; no two are the same exact point.
    std::vector<Crossing> crossings;
    /// In the order of their start along A.
    std::vector<SharedStretch> shared;
    /// False when solid-space refused some data (beyond its domain, 2^50):
    /// what it refused is left out (docs/adr/0035, point 23).
    bool complete{true};
};

/// Where the curves of `a` and `b` meet (docs/adr/0035, point 1). A text has no
/// curve: empty. A line of zero length, or a polyline segment that is exactly a
/// point, meets nothing.
[[nodiscard]] Intersections intersect(const Entity& a, Extension extendA, const Entity& b,
                                      Extension extendB);

/// Where a polyline meets itself: the common points of two segments that are
/// not consecutive, and those of two consecutive segments other than their
/// shared vertex. Both positions are on the polyline, onA before onB.
[[nodiscard]] Intersections selfIntersections(const Polyline& polyline);

/// A point of the host where one or more cutters meet it.
struct Cut {
    Vec2 point;
    CurvePosition onHost;
    /// The cutters through this exact point, by increasing index.
    std::vector<std::size_t> cutters;
};

struct HostCuts {
    /// In the exact order along the host; points of different cutters that are
    /// the same exact point form a single Cut.
    std::vector<Cut> cuts;
    /// Cutters that share a stretch with the host, by increasing index. They
    /// give no cuts (docs/adr/0035, point 4).
    std::vector<std::size_t> overlapping;
    bool complete{true};
};

/// The cuts that `cutters` make on `host`, as TRIM and EXTEND need them
/// (docs/adr/0035, points 4-6), from solid-space's intersectAlong. Texts among
/// the cutters cut nothing. With `hostCutsItself`, a polyline host also cuts
/// itself: every vertex is a cut, and so is every point of
/// selfIntersections().
[[nodiscard]] HostCuts cutsAlong(const Entity& host, Extension extendHost,
                                 std::span<const Entity> cutters, bool hostCutsItself = false);

enum class Side : std::uint8_t { Left, On, Right };

/// The side of `point` with respect to the line or circle of the entity
/// (solid-space side()), looking along the curve: for a circle and an arc,
/// left is inside. For a polyline, of its segment nearest to the point
/// (closestPoint; on a tie the first). std::nullopt for a text and for a curve
/// that is exactly a point.
[[nodiscard]] std::optional<Side> sideOf(const Entity& entity, const Vec2& point);

/// The position of the point of the entity nearest to `point`, with
/// Tecnigrafo's projection (closestPoint; on a tie the first segment): Inside,
/// or AtStart or AtEnd when the nearest point is that end. For choosing among
/// cuts by a pick, not an exact decision (docs/adr/0035, point 5).
/// Precondition: not a Text.
[[nodiscard]] CurvePosition nearestPosition(const Entity& entity, const Vec2& point);
```

Regole della traduzione (ADR 0035, punto 1):

- per una coppia di curve e per le polilinee la facciata chiama
  `intersectAlong`, un segmento dell'entità `a` (o della host) alla volta, con i
  pezzi dell'altra entità come taglienti; i punti vengono nell'ordine di
  solid-space, segmento dopo segmento;
- un punto `AtEnd` del segmento i e `AtStart` del segmento i + 1 è il vertice
  comune e compare una volta, come `AtStart` del segmento i + 1 (per una
  polilinea chiusa, l'ultimo e il primo: `AtStart` del segmento 0); lo stesso
  sull'altra entità, dove un `Cut` riunisce già i pezzi che passano per lo
  stesso punto esatto;
- con `Extension::Extended` su una polilinea aperta valgono, per il primo
  segmento, i punti `BeforeStart` (dritto) o del resto del suo cerchio (curvo,
  `AfterEnd` come in solid-space), per l'ultimo i punti `AfterEnd`; gli altri
  punti fuori dai segmenti si scartano;
- un pezzo o un tagliente che solid-space rifiuta perché è esattamente un punto
  si salta senza altro; uno rifiutato per i valori o per il dominio si salta e
  mette `complete` a falso;
- nessuna soglia nuova e nessun confronto di stazioni.

### Regola delle tolleranze (`tolerance.hpp`)

```cpp
/// The length in metres of the drawing unit INSUNITS `units` (docs/adr/0036,
/// point 1): the double nearest the exact value; 0, no unit, as millimetres.
/// Precondition: 0 <= units <= Document::kMaxUnits.
[[nodiscard]] double unitInMeters(int units);

/// solid-space's rule for coincidence and tolerance in the plane (its ADR
/// 0009), for the unit of a drawing (docs/adr/0036, point 2).
class ToleranceRule {
public:
    /// The rule of a drawing with INSUNITS `units`.
    /// Precondition: 0 <= units <= Document::kMaxUnits.
    explicit ToleranceRule(int units);
    /// The rule of `document`, for its units now.
    [[nodiscard]] static ToleranceRule of(const Document& document);
    [[nodiscard]] int units() const;
    /// epsilon(magnitude), in drawing units.
    [[nodiscard]] double epsilon(double magnitude) const;
    /// Whether two points coincide under the rule (exact comparison).
    [[nodiscard]] bool coincident(const Vec2& a, const Vec2& b) const;
};

/// Whether the curve of a line, arc or circle is degenerate under the rule
/// (geom2d isDegenerate: every two of its points within epsilon). A polyline
/// and a text: false.
[[nodiscard]] bool isDegenerate(const Entity& entity, const ToleranceRule& rule);
/// The same for one polyline segment.
[[nodiscard]] bool isDegenerate(const PolylineSegment& segment, const ToleranceRule& rule);
```

`ToleranceRule` non espone `TolerancePolicy`: la costruisce `bridge::policyOf`
da `unitInMeters` con `TolerancePolicy::forUnit`, che per le 25 unità dà
sempre un valore.

### Domande (`curve_questions.cpp`)

Le domande `intersections`, `self-intersections`, `side`, `coincident` e
`degenerate` del catalogo (`docs/verifica/domande.md`, "Intersezioni, lato e
tolleranze"), con le risposte nella forma scritta lì.

## Regole della tappa che valgono qui

- Il sottomodulo lo sposti solo al commit che indica la issue (ADR 0030, punto
  9); `cmake/` e `.github/` non si toccano (li ha già sistemati P0).
- `core/CMakeLists.txt`, `question_table.cpp`, `script/CMakeLists.txt`: solo le
  tue righe. `curve_questions.cpp` lo ampliano poi P17, P19, P21 e P22.
- Nessuna decisione geometrica propria: ogni "si incontrano", "dove", "in che
  ordine" e "da che parte" viene da solid-space.

## Test richiesti

- Ogni coppia tra linea, arco, cerchio e polilinea con archi: incroci,
  tangenze, nessun incontro, tratti in comune; con e senza `Extended`; arco con
  `endAngle` minore di `startAngle`.
- Polilinee: incrocio esattamente in un vertice (una sola volta, `AtStart` del
  segmento che segue), polilinea chiusa nel vertice 0, polilinea che si
  interseca, due segmenti consecutivi, segmento ripetuto (esattamente un punto),
  segmento con bulge sotto `1e-12`.
- `cutsAlong`: più tagli nello stesso punto (un `Cut` con più taglienti), un
  tagliente che si sovrappone, un testo tra i taglienti, `hostCutsItself` su un
  rettangolo attraversato da una linea, prolungamento di una polilinea aperta
  con il primo segmento curvo.
- I casi delle richieste Matleyx78/solid-space#20 e #23 (ADR 0010 e 0009 di
  solid-space, "I casi della richiesta"), passando dalle entità.
- `unitInMeters` per tutte le 25 unità; `ToleranceRule` in millimetri e in
  chilometri sui casi dell'ADR 0009 di solid-space; `isDegenerate` di una linea
  lunga `1e-14`.
- `sideOf` e `nearestPosition` su ogni tipo di curva.

## Fatto quando

- I casi di `[0.7-R14]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- Nessun file di Tecnigrafo fuori da `core/src` include un'intestazione di
  solid-space, salvo il test di P0.
- La CI è verde su tutti i sistemi.
