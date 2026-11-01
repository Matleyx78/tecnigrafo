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
- **Modulo:** `core`
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 3, 5 e 6; [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 5 e 6; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 9 e
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

La costruzione che serve a `TRIM`, `EXTEND` e `FILLET` (ADR 0031, punto 10):
dove tagliare lo dice solid-space, il pezzo lo costruisce Tecnigrafo.

```cpp
/// A place on the curve of an entity, as intersect(), cutsAlong() and the
/// constructions of solid-space give it: the point and its position.
struct CurvePoint {
    Vec2 point;
    CurvePosition position;
};

/// The part of `entity` from `from` to `to`, each std::nullopt for its start
/// or its end (docs/adr/0035, points 5 and 6):
/// - Line: from `from` to `to`; a place BeforeStart or AfterEnd extends it;
/// - Arc: counter-clockwise from `from` to `to`; AfterEnd extends it along
///   its circle;
/// - Circle: the arc counter-clockwise from `from` to `to` (both required);
/// - Polyline: the vertices between the two places, with the cut segments
///   ending at the given points; a closed polyline becomes open, running from
///   `from` to `to` across its vertex 0 if needed; BeforeStart on the first
///   segment and AfterEnd on the last extend it.
/// The new end points are the given points, exactly. New arc angles come from
/// solid_space::robust::atan2 of (point - centre), brought into [0, 2π) with
/// normalizeAngle; a cut polyline arc gets the bulge of its new included angle;
/// the widths of a cut polyline segment are interpolated by length
/// (docs/adr/0031, point 9). Every other property of the entity stays.
/// std::nullopt for a Text, or when `from` and `to` are the same place.
/// Precondition: `from` does not come after `to` along the entity (in the
/// order of cutsAlong), except across vertex 0 of a closed polyline and
/// around a circle.
[[nodiscard]] std::optional<Entity> partOf(const Entity& entity,
                                           const std::optional<CurvePoint>& from,
                                           const std::optional<CurvePoint>& to);
```

"Lo stesso posto" è lo stesso `CurvePosition` per segmento e posizione e lo
stesso punto: due tagli diversi di `cutsAlong` non sono mai lo stesso punto
esatto (P14), quindi non serve confrontare stazioni.

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
- `partOf`: linea accorciata ai due lati e prolungata; arco tagliato, arco
  prolungato oltre `2π`; cerchio che diventa arco; polilinea aperta tagliata in
  un segmento dritto e in uno curvo (bulge e larghezze), polilinea chiusa
  aperta attraverso il vertice 0, polilinea prolungata al primo e all'ultimo
  segmento; testo; stesso posto.

## Fatto quando

- I casi di `[0.7-R15]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`; i casi esistenti (rivisti da R15) passano.
- I test di `partOf` esistono e passano.
- La CI è verde.
