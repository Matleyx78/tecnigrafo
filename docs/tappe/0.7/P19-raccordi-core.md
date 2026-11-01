# [0.7-P19] `core`: raccordi, tangenze, snap a tangente

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R19]` (#…), agente gemini: la facciata (scelta
  dei segmenti, entità che risultano, esiti, snap); i valori geometrici
  dall'archivio dei casi di solid-space
- **Dipende da:** P15, Matleyx78/solid-space#21, R19. La richiesta diventa il
  pacchetto di solid-space che realizza `tangency.hpp`, quando la tappa 0.2 di
  solid-space lo avrà (ADR 0030, punto 13)
- **Modulo:** `core` (più le domande in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2, 13, 14 e 23; [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 4 e 5 (tangente); [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punto 5; l'[ADR 0011 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0011-raccordi-e-tangenze-nel-piano.md)
  (Decisione e righe **Corretto:**); in `third_party/solid-space` l'intestazione qui sotto;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-raccordi.dxf`);
  `core/include/tg/core/intersections.hpp`, `tolerance.hpp`,
  `core/src/solid_space_bridge.hpp` (P14); `core/include/tg/core/curve_parts.hpp`
  e `core/src/document_query.cpp` (P15)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0011 di solid-space:
`solid_space/geom2d/tangency.hpp` (`tangentsFrom`, `tangentCircleNear`,
`fillet`, con i loro tipi), con i tipi di `curves.hpp`, `intersection.hpp` e
`side.hpp`.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude il pacchetto di solid-space che realizza
  `tangency.hpp`. Se la issue non lo dice, chiedilo e non cominciare.
- Se qualcosa che serve qui manca dal contratto o non si comporta come dichiara,
  **fermati** come dice AGENTS.md ("Se serve qualcosa a solid-space").

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/fillet.hpp`, `core/src/fillet.cpp` (nuovi: raccordo,
  cerchio tangente, tangenti da un punto)
- `core/CMakeLists.txt`
- `core/src/document_query.cpp` (lo snap a tangente)
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/fillet_test.cpp` (nuovo), `tests/core/document_query_test.cpp`,
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R19.txt` (da cancellare)

## Cosa fare

```cpp
/// How a corner construction (FILLET, CHAMFER) ended (docs/adr/0035).
enum class CornerOutcome : std::uint8_t {
    Done,
    None,            ///< it does not exist (solid-space: none)
    InfinitelyMany,  ///< the curves lie on one line or circle, or similar
    AmbiguousPick,   ///< a pick lies exactly on the other curve's line or circle
    NotAllowed,      ///< an entity the command does not take
    /// It exists, but beyond solid-space's domain: a centre or a corner
    /// farther than robust::kMaxCoordinate (two almost parallel lines that
    /// meet very far away).
    OutOfDomain,
};

/// The result of FILLET and CHAMFER on two entities.
struct CornerResult {
    CornerOutcome outcome{CornerOutcome::None};
    /// With Done: the two entities as they become (unchanged with trim off,
    /// and a circle is never cut), and the new arc or line, none for a
    /// corner of radius 0.
    std::optional<Entity> first;
    std::optional<Entity> second;
    std::optional<Entity> added;
};

/// FILLET of two different entities, each a Line, an Arc or a Circle, picked
/// at the given points (docs/adr/0035, point 13). Radius 0 gives the corner.
/// Anything else: NotAllowed. A radius beyond robust::kMaxCoordinate: None
/// (docs/adr/0035, point 23). Precondition: radius finite and >= 0.
[[nodiscard]] CornerResult fillet(const Entity& first, const Vec2& pickFirst,
                                  const Entity& second, const Vec2& pickSecond,
                                  double radius, bool trim, const ToleranceRule& rule);

/// FILLET of two segments of one polyline, each the segment nearest its pick:
/// `first` of the result is the new polyline; `second` and `added` are empty.
/// Two segments that are not consecutive (also across the closing segment):
/// NotAllowed.
[[nodiscard]] CornerResult filletSegments(const Polyline& polyline, const Vec2& pickFirst,
                                          const Vec2& pickSecond, double radius,
                                          const ToleranceRule& rule);

/// FILLET Polyline: every vertex, segment pair by segment pair along the
/// polyline (docs/adr/0035, point 13).
struct PolylineCorners {
    Polyline polyline;
    std::size_t done{0};      ///< vertices filleted
    std::size_t tooShort{0};  ///< vertices whose segments are too short
};
[[nodiscard]] PolylineCorners filletPolyline(const Polyline& polyline, double radius,
                                             const ToleranceRule& rule);

/// CIRCLE Ttr (docs/adr/0035, point 14): the circle of `radius` tangent to
/// both, nearest the picks; for a polyline its segment nearest the pick. A
/// text: NotAllowed. A radius beyond robust::kMaxCoordinate: None
/// (docs/adr/0035, point 23).
struct TangentCircleResult {
    CornerOutcome outcome{CornerOutcome::None};
    std::optional<Circle> circle;  ///< with Done
};
[[nodiscard]] TangentCircleResult circleTangentTangentRadius(const Entity& first,
                                                            const Vec2& pickFirst,
                                                            const Entity& second,
                                                            const Vec2& pickSecond,
                                                            double radius);

/// The tangent points from `point` to a circle, an arc or a polyline arc,
/// only those on the curve (tangentsFrom with Extension::none); the point
/// itself if it lies exactly on the circle; none from inside or for a straight
/// curve.
[[nodiscard]] std::vector<Vec2> tangentPoints(const Vec2& point, const Entity& entity);
```

- **Dalle entità alle curve** con la traduzione di P14; per una polilinea, il
  segmento più vicino al punto indicato (`nearestPosition`).
- **Il raccordo** viene da `fillet` di solid-space, con le sue regole
  (semicerchio tra parallele, raggio 0, lato dai punti, arco più corto). Gli
  esiti `none`, `infinitelyMany` e `ambiguousPick` danno l'esito omonimo.
- **Oltre il dominio** (ADR 0035, punti 13 e 14, corretti il 2026-11-01;
  ADR 0011 di solid-space, righe **Corretto:**): `fillet` e `tangentCircleNear`
  danno la diagnosi `outOfDomain` sul raggio (dato 2) anche quando il centro
  del cerchio, o lo spigolo del raggio 0, cade oltre `kMaxCoordinate`: due
  rette quasi parallele che si incontrano lontanissimo. Il raccordo esiste, ma
  non si rappresenta. Per non confonderlo con un raggio scritto oltre il
  dominio, che dà la stessa diagnosi:
  - un raggio oltre `solid_space::robust::kMaxCoordinate` (lo stesso
    confronto di solid-space: il dominio arriva a `kMaxCoordinate` compreso)
    dà `None` senza chiamare solid-space: è un dato fuori dal dominio (ADR
    0035, punto 23);
  - con un raggio nel dominio, `outOfDomain` sul dato 2 dà `OutOfDomain`;
  - ogni altra diagnosi (una curva o un punto fuori dal dominio, una linea di
    lunghezza zero) dà `None` (ADR 0035, punto 23).

  Vale per `fillet`, `filletSegments` e `circleTangentTangentRadius`.
- **Le entità che risultano:** ciascuna curva si porta al suo punto di
  tangenza con `partOf` (P15), tenendo la parte che il punto indicato mostra:
  - se il punto di tangenza è prima della fine nella parte tenuta, la curva si
    accorcia; se è oltre (`BeforeStart`, `AfterEnd`), si prolunga;
  - un cerchio non si taglia; con `trim` falso le due entità non cambiano.
- **L'arco nuovo** è `Fillet::arc` portato in un `Arc` di Tecnigrafo,
  antiorario: con ampiezza negativa, inizio e fine si scambiano; gli angoli si
  riportano in [0, 2π). Nei due segmenti di una polilinea il segmento nuovo è
  `Fillet::bulge`, tra i due punti di tangenza. Le proprietà dell'arco nuovo le
  decide il comando (P20).
- **Arco degenere** (ADR 0036, punto 5): se `isDegenerate` con la regola trova
  degenere l'arco di raccordo, il risultato è quello del raggio 0 (spigolo).
- **`filletPolyline`:** per ogni vertice, `fillet` sui due segmenti che vi
  arrivano (per una polilinea aperta, solo i vertici interni); un vertice i cui
  punti di tangenza non cadono dentro i due segmenti, un raccordo che non
  esiste, o uno che esiste oltre il dominio (`OutOfDomain`, un vertice a
  forcina quasi chiusa), resta com'è e conta in `tooShort`.
- **Snap a tangente** in `findSnap`: con `modes.tangent` e `from`, i punti di
  `tangentPoints(from, …)` entro l'apertura, con le priorità dell'ADR 0033,
  punto 5.
- Le domande `fillet`, `tangents` e `circle-ttr` del catalogo, con le risposte
  scritte lì.

## Regole della tappa che valgono qui

- Il sottomodulo lo sposti solo al commit che indica la issue (ADR 0030, punto
  9).
- `document_query.cpp`: prima di te P15.
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P17, P21 e P22).
- Nessuna decisione geometrica propria: esistenza, lato, punti di tangenza e
  verso dell'arco vengono da solid-space.

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `FILLET` e *Ttr*, e quelli della
  richiesta Matleyx78/solid-space#21 (ADR 0011 di solid-space, "I casi della
  richiesta"), passando dalle entità.
- Raccordo impossibile; archi concentrici (`InfinitelyMany`); punto sulla retta
  dell'altra curva (`AmbiguousPick`); raggio 0; parallele; *Trim* falso;
  cerchio; linea che si prolunga fino al punto di tangenza; arco degenere.
- Oltre il dominio: due rette quasi parallele, per esempio da 0,0 a 1,0 e da
  0,1 a 1,1.0000000000000002 (pendenza 2^-52, si incontrano a x = −2^52),
  danno `OutOfDomain` con raggio 1 e con raggio 0 in `fillet`, e con raggio 1
  in `circleTangentTangentRadius`; due segmenti consecutivi a forcina quasi
  chiusa, per esempio i vertici 10,0, 0,0 e 10,1e-15 con raggio 1 (centro a
  circa 2e16 dal vertice), danno `OutOfDomain` in `filletSegments` e contano
  in `tooShort` in `filletPolyline`; un raggio di 2e15, oltre `kMaxCoordinate`,
  tra due linee perpendicolari dà `None` in `fillet` e in
  `circleTangentTangentRadius`.
- Due segmenti consecutivi e non consecutivi di una polilinea; `filletPolyline`
  su un rettangolo e con un segmento troppo corto.
- Tangenti da fuori, sul cerchio, da dentro; snap a tangente senza `from`.

## Fatto quando

- I casi di `[0.7-R19]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
