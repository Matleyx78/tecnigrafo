# [0.7-P15] `core`: snap a intersezione; parti delle entità

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R15]` (#…), agente gemini, per lo snap (anche la
  revisione dei casi esistenti, ADR 0033 punto 6). Le parti delle entità non si
  raggiungono ancora da uno script: le provano i test di unità qui e i casi di
  `TRIM`, `EXTEND` e `FILLET` più avanti (R16, R20)
- **Dipende da:** P7, P14, P25, R15
- **Corretta:** 2026-11-01, con le decisioni del responsabile sulla #359 e sui
  buchi B1–B3 della #464 ([DECISIONI.md](../../DECISIONI.md))
- **Modulo:** `core`
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 3, 5 e 6; [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 5, 6 e 23; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 9 e
  10; `core/include/tg/core/intersections.hpp` (P14),
  `core/src/solid_space_bridge.hpp` (P14); `core/src/document_query.cpp`
  (`findSnap`, P7); `core/include/tg/core/arc_geometry.hpp`, `polyline.hpp`

## Intestazioni di solid-space che usa

Nessuna nuova: la facciata di P14 per lo snap, e `solid_space/robust/
trigonometry.hpp` (`atan2`) per gli angoli nuovi degli archi. Il sottomodulo
resta dove l'ha messo P14; se la issue indica un altro commit (perché `atan2`
arriva con un pacchetto di solid-space diverso), lo sposti a quello.

## File

- `core/src/document_query.cpp`, `core/include/tg/core/document_query.hpp` se
  serve
- `core/include/tg/core/curve_parts.hpp`, `core/src/curve_parts.cpp` (nuovi)
- `core/CMakeLists.txt`
- `tests/core/document_query_test.cpp`, `tests/core/curve_parts_test.cpp`
  (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R15.txt` (da cancellare)

## Cosa fare

### Snap a intersezione

In `findSnap`, con `modes.intersection`:

- si prendono le entità visibili la cui curva passa entro l'apertura (indice
  spaziale, poi `distance`);
- si calcola `intersect` per ogni coppia, e `selfIntersections` per ogni
  polilinea;
- i punti di `crossings` entro l'apertura sono candidati `Intersection`, con le
  priorità dell'ADR 0033, punto 5; i tratti in comune non danno punti (i loro
  estremi sono già estremi delle entità);
- un punto in cui si incontrano più coppie è uno solo: tra i candidati alla
  stessa distanza vale l'ordine di `findSnap`, e la facciata non deve dare due
  volte lo stesso punto per la stessa coppia.

Prestazioni: al più qualche decina di entità vicine al puntatore. Il test di
prestazioni esistente (`tests/bench`, se ha un caso dello snap) non deve
peggiorare in modo visibile; se lo fa, scrivilo nella pull request con le
misure.

### Parti delle entità (`curve_parts.hpp`)

La costruzione che serve a `TRIM`, `EXTEND`, `FILLET` e `CHAMFER` (ADR 0031,
punto 10): dove tagliare lo dice solid-space, il pezzo lo costruisce
Tecnigrafo.

```cpp
/// A place on the curve of an entity, as intersect(), cutsAlong() and the
/// constructions of solid-space give it: the point and its position.
struct CurvePoint {
    Vec2 point;
    CurvePosition position;
};

/// How the part runs from `from` to `to`. The caller knows it exactly, from
/// the order of HostCuts::cuts or from a construction; partOf never deduces it
/// from points or stations, which are rounded (#359).
enum class PartCourse : std::uint8_t {
    Direct,       ///< `from` comes before `to`, without passing the start
    AcrossStart,  ///< a circle or a closed polyline, passing its start
                  ///< (angle 0, vertex 0)
    SamePlace,    ///< `from` and `to` are the same place: nothing in between
};

enum class PartOutcome : std::uint8_t {
    Done,              ///< `entity` holds the part
    Empty,             ///< nothing is left: the part does not stay
    NotRepresentable,  ///< the exact part exists, but the model cannot hold it
};

struct PartResult {
    PartOutcome outcome{PartOutcome::Empty};
    std::optional<Entity> entity;  ///< with Done
};

/// The part of `entity` from `from` to `to`, each std::nullopt for its start
/// or its end (docs/adr/0035, point 5):
/// - Line: from `from` to `to`;
/// - Arc: counter-clockwise from `from` to `to`;
/// - Circle: the arc counter-clockwise from `from` to `to` (both required);
/// - Polyline: the vertices between the two places, with the cut segments
///   ending at the given points; a closed polyline becomes open, running from
///   `from` to `to`, across its vertex 0 with AcrossStart (both required).
/// The new end points of a line and the new vertices of a polyline are the
/// given points, exactly. New arc angles come from solid_space::robust::atan2
/// of (point - centre), brought into [0, 2π) with normalizeAngle: the ends of
/// an arc lie within the error, not exactly on the given points. A cut
/// polyline arc gets the bulge of its new included angle; the widths of a cut
/// polyline segment are interpolated by length (docs/adr/0031, point 9).
/// Every other property of the entity stays.
/// Empty with SamePlace, and when the part is a single arc (an Arc, a part of
/// a Circle, or a part of a polyline within one curved segment) whose two
/// angles, or two vertices, come out equal while the exact part is the short
/// one (Direct, on one segment); NotRepresentable when they come out equal
/// and the exact part is the long one (AcrossStart): it never becomes a
/// circle.
/// Precondition: not a Text; no place is BeforeStart or AfterEnd (extendTo
/// takes those); with Direct, `from` does not come after `to` in the order of
/// cutsAlong; AcrossStart only for a circle or a closed polyline, with `to`
/// not after `from`.
[[nodiscard]] PartResult partOf(const Entity& entity, const std::optional<CurvePoint>& from,
                                const std::optional<CurvePoint>& to, PartCourse course);

enum class EntityEnd : std::uint8_t { Start, End };

/// `entity` with its end `end` brought to `to`, beyond that end (EXTEND, and
/// FILLET and CHAMFER when the point is BeforeStart or AfterEnd; docs/adr/0035,
/// point 6):
/// - Line: along its line, `to` BeforeStart for the start, AfterEnd for the end;
/// - Arc: along its circle, `to` AfterEnd for both ends: for the start the
///   circle is run backwards from the start;
/// - open Polyline: its first or last segment that is not exactly a point
///   (as Extension::Extended, P14), the same way as a line or an arc; the
///   vertices of the segments exactly a point between that segment and the
///   end take the new point and keep all their other data (docs/adr/0006).
/// The new point of a line and of a polyline is `to`, exactly; a new arc
/// angle comes as in partOf. NotRepresentable when the new angle of an arc,
/// or the new vertex of a curved polyline segment, comes out equal to the
/// other end of that arc or segment: the arc would close into a circle.
/// Precondition: a Line, an Arc or an open Polyline; `to` is on the segment
/// above, BeforeStart or AfterEnd as said.
[[nodiscard]] PartResult extendTo(const Entity& entity, EntityEnd end, const CurvePoint& to);
```

- **Lo stesso posto e il giro** li decide il chiamante, che li ha esatti
  (decisione A della #359): per `TRIM` ed `EXTEND` dalla lista ordinata di
  `HostCuts::cuts`, dove lo stesso posto è lo stesso taglio della lista, mai
  due tagli con gli stessi dati arrotondati; per `FILLET` e `CHAMFER` dalla
  costruzione, che dà un solo posto per curva. Nessuna richiesta a
  solid-space: il contratto dà già identità e ordine esatti.
- **Archi** (B1): l'estremo nuovo di un arco sta entro l'errore, non sul
  punto ricevuto; quando i due angoli coincidono decide `course`, mai le
  stazioni. È la regola delle decisioni Matleyx78/solid-space#132 (arco quasi
  intero con estremi arrotondati coincidenti: `failed`) e #148 (pezzo dritto
  ridotto a un punto: si scarta). In una parte di polilinea di più segmenti un
  segmento curvo tagliato con i due vertici uguali è il pezzo corto: ampiezza
  0, bulge 0, un segmento esattamente puntiforme, che `TRIM` giudica con la
  regola dei pezzi degeneri (P16).
- **Nei comandi** `Empty` vuol dire che quel pezzo non resta;
  `NotRepresentable` che l'operazione non riesce e l'oggetto resta com'è, con
  il messaggio dell'operazione non riuscita (ADR 0035, punto 23).
- **Prolungare o tagliare** (B2): il chiamante sceglie fra `partOf` ed
  `extendTo` guardando la `Location`, che è esatta. Il significato di
  `AfterEnd` di solid-space non cambia.
- **Polilinee aperte** (B3): il «primo» e l'«ultimo» segmento sono il primo e
  l'ultimo non puntiforme, come in P14 (`Extension::Extended`). Per esempio
  [(0,0),(0,0),(10,0)] prolungata all'inizio fino a x=−5 diventa
  [(−5,0),(−5,0),(10,0)].

## Regole della tappa che valgono qui

- `document_query.cpp`: prima di te P7, dopo di te P19 (snap a tangente).
- `core/CMakeLists.txt`: solo le tue righe.
- Nessuna decisione geometrica propria: dove tagliare e in che ordine viene da
  `intersections.hpp`.

## Test richiesti

- Snap: due linee che si incrociano, linea e arco, due cerchi, polilinea che si
  interseca; tre linee nello stesso punto; incrocio fuori dall'apertura;
  tangenza; layer spento (nessuna intersezione con le sue entità); priorità tra
  intersezione ed estremo alla stessa distanza (vince l'estremo).
- `partOf`: linea accorciata ai due lati; arco tagliato; cerchio che diventa
  arco, con `Direct` e con `AcrossStart`; polilinea aperta tagliata in un
  segmento dritto e in uno curvo (bulge e larghezze), polilinea chiusa aperta
  attraverso il vertice 0; `SamePlace`; i due tagli distinti della #359 sulla
  linea da (0,0) a (2,0), con lo stesso punto arrotondato (una linea di
  lunghezza zero, `Done`); due posti di un arco con gli stessi angoli
  arrotondati, che danno `Empty` con `Direct` e, su un cerchio,
  `NotRepresentable` con `AcrossStart`; da (2,0) a (8,0) con `Direct` e da
  (8,0) a (2,0) con `AcrossStart` sul rettangolo chiuso della #359.
- `extendTo`: linea prolungata ai due estremi; arco prolungato alla fine oltre
  `2π` e all'inizio, all'indietro (l'arco 0°→90° fino al taglio a 270° dà
  270°→90°); arco che si chiuderebbe (`NotRepresentable`); polilinea aperta
  prolungata al primo e all'ultimo segmento, anche curvi, e con un segmento
  puntiforme all'estremo (l'esempio di B3).

## Fatto quando

- I casi di `[0.7-R15]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`; i casi esistenti (rivisti da R15) passano.
- I test di `partOf` esistono e passano.
- La CI è verde.
