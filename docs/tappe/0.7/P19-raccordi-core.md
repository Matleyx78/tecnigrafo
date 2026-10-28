# [0.7-P19] `core`: raccordi, smussi, tangenze, snap a tangente

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R19]` (#…), agente gemini 4 Argon (calcoli
  delicati; valori da `autocad-raccordi.dxf`)
- **Dipende da:** Matleyx78/solid-space#21 (la sua tappa chiusa), P15, R19
- **Modulo:** `core` (più le domande in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 10–12; [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punto 4 (tangente); la richiesta
  [#327](https://github.com/Matleyx78/tecnigrafo/issues/327) e l'ADR di
  valutazione di solid-space che le risponde; [dxf-di-prova.md](dxf-di-prova.md)
  (`autocad-raccordi.dxf`); `core/include/tg/core/intersections.hpp` (P14),
  `core/src/document_query.cpp` (P15)

## Prima di cominciare

Come per P14: se solid-space non offre una costruzione che serve, fermati come
dice AGENTS.md.

## File

- `core/include/tg/core/fillet.hpp`, `core/src/fillet.cpp` (nuovi: raccordo,
  smusso, cerchio tangente, tangenti da un punto)
- `core/CMakeLists.txt`
- `core/src/snap_points.cpp`, `core/src/document_query.cpp` (lo snap a tangente)
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/fillet_test.cpp` (nuovo), `tests/core/document_query_test.cpp`,
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R19)

## Cosa fare

```cpp
/// One of the two objects of FILLET or CHAMFER, and where it was picked.
struct PickedEntity {
    Entity entity;
    Vec2 pick;
};

/// The result for FILLET and CHAMFER (docs/adr/0035, points 10-11): the two
/// entities as they become (unchanged with trim off, or when an entity is a
/// circle) and the new arc or line, if any. For two segments of one polyline,
/// `first` is the new polyline and `second` is empty.
struct CornerResult {
    Entity first;
    std::optional<Entity> second;
    std::optional<Entity> added;  ///< the fillet arc or the chamfer line
};

/// std::nullopt when the corner cannot be made (radius too large, concentric
/// arcs, entities that are not allowed). Radius 0 gives a sharp corner.
[[nodiscard]] std::optional<CornerResult> fillet(const PickedEntity& a, const PickedEntity& b,
                                                 double radius, bool trim);
/// FILLET with the same polyline picked twice, or Polyline: every vertex.
[[nodiscard]] std::optional<Polyline> filletPolyline(const Polyline& polyline, double radius);

struct ChamferDistances { double first; double second; };
struct ChamferAngle { double length; double angle; };  ///< length on the first, radians
using ChamferSpec = std::variant<ChamferDistances, ChamferAngle>;
[[nodiscard]] std::optional<CornerResult> chamfer(const PickedEntity& a, const PickedEntity& b,
                                                  const ChamferSpec& spec, bool trim);
[[nodiscard]] std::optional<Polyline> chamferPolyline(const Polyline& polyline,
                                                      const ChamferSpec& spec);

/// CIRCLE Ttr: the circle of `radius` tangent to both, nearest the picks.
[[nodiscard]] std::optional<Circle> circleTangentTangentRadius(const PickedEntity& a,
                                                               const PickedEntity& b,
                                                               double radius);
/// The tangent points from `point` to a circle, an arc or a polyline arc,
/// only those on the curve; the point itself if it lies on the curve.
[[nodiscard]] std::vector<Vec2> tangentPoints(const Vec2& point, const Entity& entity);
```

- Raccordi e tangenze vengono da `solid_space::robust`; il lato scelto con i
  punti usa `sideOf` e `stationOf` (P14).
- Per `chamfer` bastano le intersezioni di P14 (solo linee e segmenti dritti).
- Due parallele: il semicerchio, qualunque sia il raggio.
- **Snap a tangente** in `findSnap`: con `modes.tangent` e `from`, i punti di
  `tangentPoints(from, …)` entro l'apertura, con le priorità dell'ADR 0033.
- Le domande `fillet`, `chamfer`, `tangents`, `circle-ttr` del catalogo.

## Regole della tappa che valgono qui

- `snap_points.cpp`, `document_query.cpp`: prima di te P15.
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: solo le tue
  righe.

## Test richiesti

- I casi di `autocad-raccordi.dxf`, compresi i cerchi *Ttr*.
- Raccordo impossibile (raggio troppo grande, archi concentrici); raggio 0;
  parallele; *Trim No*; polilinea intera e due segmenti consecutivi.
- Smusso con distanze e con angolo; su una polilinea; con un arco (nessun
  risultato).
- Tangenti da fuori, sul cerchio, da dentro; snap a tangente senza `from`.

## Fatto quando

- I casi di `[0.7-R19]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
