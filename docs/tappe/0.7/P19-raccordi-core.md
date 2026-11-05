# [0.7-P19] `core`: raccordi, tangenze, snap a tangente

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R19]` (#…), agente gemini: la facciata (scelta
  dei segmenti, entità che risultano, esiti, snap); i valori geometrici
  dall'archivio dei casi di solid-space
- **Dipende da:** P15, Matleyx78/solid-space#145,
  Matleyx78/solid-space#190, P25, R19. Matleyx78/solid-space#190 è 0.2-P22
  di solid-space ([scheda](https://github.com/Matleyx78/solid-space/blob/main/docs/tappe/0.2/P22-parte-tenuta-raccordo.md), parte di
  ciascuna curva tenuta dal raccordo, riferimenti Matleyx78/solid-space#189,
  0.2-R22): realizza la correzione del contratto di `fillet` per B2 della
  #367, issue `contratto` Matleyx78/solid-space#184, scritta nel contratto
  dalla Matleyx78/solid-space#185. La richiesta Matleyx78/solid-space#21 è
  diventata i pacchetti di solid-space che realizzano `tangency.hpp` (ADR
  0030, punto 13); si citano 0.2-P20 (Matleyx78/solid-space#145, dominio
  delle tangenze, riferimenti Matleyx78/solid-space#146) e l'ultimo, 0.2-P22
  (Matleyx78/solid-space#190): gli altri (0.2-P15 e 0.2-P16,
  Matleyx78/solid-space#66 e #70) sono già uniti
- **Corretta:** 2026-11-01, con le decisioni del responsabile sulla #359 e sui
  buchi B1, B2, B6, B7, B14 e B15 della #464 ([DECISIONI.md](../../DECISIONI.md));
  2026-11-02, con la revisione di B6 dopo le prove in AutoCAD (#468);
  2026-11-03, operazione snellimento (#478): rimanda a `tangency.hpp` invece di
  ripeterlo, aspetta Matleyx78/solid-space#145 invece di #21, tratta il bulge 0
  come arco degenere e ha un caso in più (due parallele lontane);
  2026-11-04, B2 della #367 con le Matleyx78/solid-space#184 e #185 e
  l'esito `RadiusTooLarge` della Matleyx78/solid-space#187 (#494); 2026-11-05,
  l'esito `DistanceTooLarge`, che dà solo lo smusso di P21 (decisione del
  responsabile sulla #507)
- **Modulo:** `core` (più le domande in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2, 13, 14 e 23; [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 4 e 5 (tangente); [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punto 5; l'[ADR 0011 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0011-raccordi-e-tangenze-nel-piano.md)
  (Decisione e righe **Corretto:**); in `third_party/solid-space` l'intestazione qui sotto;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-raccordi.dxf`, e "Che cosa resta
  da controllare", punto 4);
  `core/include/tg/core/intersections.hpp`, `tolerance.hpp`,
  `core/src/solid_space_bridge.hpp` (P14); `core/include/tg/core/curve_parts.hpp`
  e `core/src/document_query.cpp` (P15)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0011 di solid-space:
`solid_space/geom2d/tangency.hpp` (`tangentsFrom`, `tangentCircleNear`,
`fillet`, con i loro tipi), con i tipi di `curves.hpp`, `intersection.hpp` e
`side.hpp`, nella versione del commit della issue (sotto). I commenti di
`tangency.hpp` dicono che cosa è deciso esattamente, in che ordine si danno le
diagnosi e quali esiti sono liberi in una fascia
([ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
punto 4): la scheda li cita e non li ripete.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude Matleyx78/solid-space#190 (0.2-P22), l'ultimo
  pacchetto di solid-space che realizza `tangency.hpp` (dopo
  Matleyx78/solid-space#145). Se la issue non lo dice, chiedilo e non
  cominciare.
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
    /// FILLET only (fillet, filletSegments): the kept part would turn a line
    /// back past the end it keeps (radiusTooLarge, tangency.hpp, rule 5).
    RadiusTooLarge,
    /// CHAMFER only (chamfer, chamferSegments; P21): a distance, or the
    /// length, longer than the kept part of its line, or with an angle a
    /// second point beyond the kept part of the second line.
    DistanceTooLarge,
    NotAllowed,      ///< an entity the command does not take
    /// It exists, but a construction is beyond solid-space's domain: the
    /// outOfDomain diagnosis on the radius of fillet and tangentCircleNear
    /// (tangency.hpp), or for CHAMFER a corner that intersect() omits.
    OutOfDomain,
};

/// The result of FILLET and CHAMFER on two entities.
struct CornerResult {
    CornerOutcome outcome{CornerOutcome::None};
    /// With Done: the two entities as they become (unchanged with trim off,
    /// and a circle is never cut), and the new arc or line, none for a
    /// corner of radius 0. With trim on, an empty `first` or `second` means
    /// that entity is erased: its kept part is empty (partOf gives Empty) or
    /// degenerate for the rule of the document (docs/adr/0036, point 5). In
    /// the results on one polyline, `first` is the polyline, never empty, and
    /// `second` is always empty without meaning "erased".
    std::optional<Entity> first;
    std::optional<Entity> second;
    std::optional<Entity> added;
};

/// FILLET of two different entities, each a Line, an Arc or a Circle, picked
/// at the given points (docs/adr/0035, point 13). Radius 0 gives the corner.
/// Anything else:
/// NotAllowed. A radius beyond robust::kMaxCoordinate: None
/// (docs/adr/0035, point 23). Precondition: radius finite and >= 0.
[[nodiscard]] CornerResult fillet(const Entity& first, const Vec2& pickFirst,
                                  const Entity& second, const Vec2& pickSecond,
                                  double radius, bool trim, const ToleranceRule& rule);

/// FILLET of two segments of one polyline, each the segment nearest its pick
/// (docs/adr/0035, point 13). With `trim`: `first` of the result is the new
/// polyline, with the curved segment in the corner, and `added` is empty.
/// Without: `first` is the polyline unchanged, and `added` the arc of the
/// corner (none for radius 0 or a degenerate arc, docs/adr/0036, point 5).
/// `second` is always empty. Two segments that are not consecutive (also
/// across the closing segment): NotAllowed.
[[nodiscard]] CornerResult filletSegments(const Polyline& polyline, const Vec2& pickFirst,
                                          const Vec2& pickSecond, double radius, bool trim,
                                          const ToleranceRule& rule);

/// FILLET Polyline: every original vertex in order (docs/adr/0035, point 13).
/// With `trim`, each vertex on the polyline as the vertices before it left
/// it, and `polyline` is the result. Without, the polyline does not change,
/// each vertex is judged on the original segments, and the new arcs go in
/// `added`, in the order of the visit.
struct PolylineCorners {
    Polyline polyline;
    /// With trim off: the new arcs (for CHAMFER, lines). Empty with trim on.
    std::vector<Entity> added;
    std::size_t done{0};      ///< vertices filleted
    std::size_t tooShort{0};  ///< vertices whose segments are too short
};
[[nodiscard]] PolylineCorners filletPolyline(const Polyline& polyline, double radius,
                                             bool trim, const ToleranceRule& rule);

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

/// The tangent points from `point` to a circle, an arc or the arcs of a
/// polyline, as tangentsFrom gives them with Extension::none (tangency.hpp);
/// none for a straight curve. For a polyline: by segment index, then in the
/// order tangentsFrom gives on the segment; a point AtEnd of a segment and
/// AtStart of the one that follows (for a closed polyline also the last and segment 0)
/// is listed once, on the segment that follows; a segment solid-space refuses,
/// or whose construction is beyond its domain, gives no points, and the
/// others keep theirs.
[[nodiscard]] std::vector<Vec2> tangentPoints(const Vec2& point, const Entity& entity);
```

- **Dalle entità alle curve** con la traduzione di P14; per una polilinea, il
  segmento più vicino al punto indicato (`nearestPosition`).
- **Il raccordo** viene da `fillet` di solid-space, con le sue regole
  (`tangency.hpp`, regole 1–5). Gli esiti `none`, `infinitelyMany`,
  `ambiguousPick` e `radiusTooLarge` danno l'esito omonimo, in `fillet` e in
  `filletSegments`; con `RadiusTooLarge` non c'è raccordo e le entità non
  cambiano (Matleyx78/solid-space#187).
- **Oltre il dominio** (ADR 0035, punti 13 e 14): `fillet` e
  `tangentCircleNear` danno la diagnosi `outOfDomain` sul raggio (dato 2) per
  una costruzione oltre il dominio, con le regole e l'ordine delle diagnosi del
  loro commento (`tangency.hpp`). Il raccordo esiste, ma non si rappresenta.
  Per non confonderlo con un raggio scritto oltre il
  dominio, che dà la stessa diagnosi:
  - un raggio oltre `solid_space::robust::kMaxCoordinate` (lo stesso
    confronto di solid-space: il dominio arriva a `kMaxCoordinate` compreso)
    dà `None` senza chiamare solid-space: è un dato fuori dal dominio (ADR
    0035, punto 23);
  - con un raggio nel dominio, `outOfDomain` sul dato 2 dà `OutOfDomain`;
  - ogni altra diagnosi (una curva o un punto fuori dal dominio, una linea di
    lunghezza zero) dà `None` (ADR 0035, punto 23).

  Vale per `fillet`, `filletSegments` e `circleTangentTangentRadius`.
- **Le entità che risultano:** con *Trim*, per ciascuna curva aperta la
  facciata tiene la parte che `fillet` indica in `Fillet::keepFirst` e
  `Fillet::keepSecond` (`KeptPart`), senza decisioni geometriche sue (B2 della
  #367). Le regole, anche per il punto di tangenza fuori dalla curva, lo
  spigolo con raggio zero, le tangenze coincidenti e `radiusTooLarge`, sono
  quelle del commento di `fillet` in `tangency.hpp` (regola 5), decise dalla
  Matleyx78/solid-space#184 e dalla #185: qui non si ripetono.
  - Per accorciare si usa `partOf` sul ramo indicato, con
    `PartCourse::Direct` (P15, decisione della #359). Per prolungare si usa
    `extendTo` sull'estremo indicato. La `Location` localizza il contatto
    sulla curva: per un arco `AfterEnd` comprende tutto il complemento
    dell'arco e da sola non individua l'estremo da spostare;
  - una parte tenuta `Empty`, o degenere per la regola del documento (ADR
    0036, punto 5), lascia vuoto `first` o `second`: l'entità si cancella
    (B14). Per esempio, raggio 1 sulle linee (0,0)→(1,0) e (0,0)→(0,1), scelte
    vicino a (1,0) e a (0,1): le due linee si cancellano e resta solo l'arco;
  - una parte `NotRepresentable` (B1) dà `None`: l'operazione non riesce e
    nessuna entità cambia;
  - un cerchio non si taglia; con `trim` falso le due entità ci sono sempre,
    invariate.
- ***Trim* vale anche per le polilinee** (B6, rivista il 2026-11-02 dopo la
  prova in AutoCAD: `autocad-prove-a-mano.dxf`, punto 4, in cui con `TRIMMODE`
  0 la polilinea resta com'è e il raccordo nasce come entità a sé).
  `filletSegments` e `filletPolyline` ricevono `trim`, come `fillet`:
  - con *Trim* la polilinea cambia come descritto sotto;
  - con *Trim No* la polilinea resta invariata (`first`, uguale a quella
    ricevuta) e la geometria nuova va in `added`: l'arco di raccordo come
    `Arc`, costruito come per due entità distinte (punto seguente). Nessun
    arco con raggio 0 o con un arco degenere.
- **L'arco nuovo** è `Fillet::arc` portato in un `Arc` di Tecnigrafo,
  antiorario: con ampiezza negativa, inizio e fine si scambiano; gli angoli si
  riportano in [0, 2π). Nei due segmenti di una polilinea il segmento nuovo è
  `Fillet::bulge`, tra i due punti di tangenza, solo con *Trim*; con *Trim No*
  l'arco è un `Arc` in `added`. Le proprietà dell'arco nuovo le decide il
  comando (P20).
- **Arco degenere** (ADR 0036, punto 5): se `isDegenerate` con la regola trova
  degenere l'arco di raccordo, il risultato è quello del raggio 0 (spigolo).
  Vale anche quando `Fillet::bulge` è 0 perché i punti di tangenza restituiti
  coincidono (`tangency.hpp`, `Fillet`): l'arco è degenere, e nessun segmento
  puntiforme entra nella polilinea. Che i punti coincidano sta in una fascia
  del commento: nessun test lo pretende.
- **`filletPolyline`** (B15): si visitano i **vertici originali in ordine**:
  per una polilinea aperta i vertici interni da 1 in su, per una chiusa da 0
  (il vertice di chiusura) in su. Ogni vertice si raccorda sulla polilinea già
  modificata dai vertici precedenti, con i suoi due segmenti come sono in quel
  momento. Un vertice i cui punti di tangenza non cadono dentro quei due
  segmenti, un raccordo che non esiste (anche `RadiusTooLarge`), o uno che
  esiste oltre il dominio (`OutOfDomain`, un vertice a forcina quasi chiusa),
  resta com'è e conta in `tooShort`. Per esempio [(0,2),(0,0),(1,0),(1,2)] con raggio 0.75: il primo
  vertice interno si raccorda e il secondo conta tra i troppo corti, `done` = 1
  e `tooShort` = 1.
  - **Con *Trim No*** la polilinea non cambia, quindi ogni vertice si giudica
    sui suoi due segmenti **originali**: è la visita di B15 su una polilinea
    che non si modifica. Un vertice i cui punti di tangenza non cadono dentro
    quei due segmenti, un raccordo che non esiste (anche `RadiusTooLarge`) o
    uno oltre il dominio resta senza arco e conta in `tooShort`; gli altri contano in `done` e il loro
    arco va in `added`, nell'ordine dei vertici. Non si controlla se gli archi
    si incontrano. Per esempio, lo stesso [(0,2),(0,0),(1,0),(1,2)] con raggio
    0.75: i due vertici interni si raccordano, perché il segmento da (0,0) a
    (1,0) è lungo 1, `done` = 2, `tooShort` = 0 e `added` ha due archi.
- **`tangentPoints` su una polilinea** (B7): l'ordine, il vertice comune e i
  segmenti rifiutati come nel commento sopra. Il vertice comune è lo stesso
  punto esatto perché la posizione la decide esattamente solid-space: è la
  convenzione di `intersect` (P14), e non si confrontano punti arrotondati. Per
  esempio, da (2,1) alla polilinea [(0,0),(2,0),(4,0)] con bulge 1 su
  tutti e due i segmenti, (2,0) compare una volta; un segmento rifiutato per
  dati oltre il dominio o una costruzione oltre il dominio
  (Matleyx78/solid-space#136 e #151) non toglie i punti degli altri.
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

- B2 della #367, *Trim* attivo: linee (0,0)→(30,0) e (0,0)→(0,30),
  scelte in (5,0) e (0,5), raggio 10. Restano (10,0)→(30,0) e
  (0,10)→(0,30). Verificare gli stessi tratti geometrici invertendo
  il verso delle linee o scambiando le due curve.
- B2 della #367, caso e di `autocad-raccordi.dxf`: arco di centro
  (230,0), raggio 10, da 0° a 180°, scelto in (238.6603,5); linea
  (215,-5)→(250,-5), scelta in (245,-5); raggio 3. Con *Trim* si
  prolunga l'inizio dell'arco a circa 351,15°, lasciando la fine a 180°.
  Invertendo il verso dell'arco si prolunga la fine e si conserva lo
  stesso tratto geometrico.
- `RadiusTooLarge` (Matleyx78/solid-space#187): L1 (2,0)→(5,0) e L2
  (0,0)→(0,30), scelte in (3,0) e (0,20), raggio 10. La tangenza su L1 è
  (10,0), oltre la fine, e tenere la parte che prosegue il raccordo
  richiederebbe di far tornare L1 indietro oltre (5,0): `fillet` dà
  `RadiusTooLarge`, senza arco, con le due linee invariate, anche invertendo
  L1 e con *Trim No*.

- I casi di `autocad-raccordi.dxf` per `FILLET` e *Ttr*, e quelli della
  richiesta Matleyx78/solid-space#21 (ADR 0011 di solid-space, "I casi della
  richiesta"), passando dalle entità.
- Raccordo impossibile; archi concentrici (`InfinitelyMany`); punto sulla retta
  dell'altra curva (`AmbiguousPick`); raggio 0; parallele; *Trim* falso;
  cerchio; linea che si prolunga fino al punto di tangenza; arco prolungato
  all'indietro dall'inizio; arco degenere; parti tenute vuote (l'esempio di
  B14: `first` e `second` vuoti con `Done`).
- Oltre il dominio: due rette quasi parallele, per esempio da 0,0 a 1,0 e da
  0,1 a 1,1.0000000000000002 (pendenza 2^-52, si incontrano a x = −2^52),
  danno `OutOfDomain` con raggio 1 e con raggio 0 in `fillet`, e con raggio 1
  in `circleTangentTangentRadius`; due linee esattamente parallele e lontane,
  da (2^50,−2^50) a (0,0) e da (0,2^50) a (2^50,0), indicate negli estremi
  (2^50,−2^50) e (0,2^50) con raggio 1, danno `OutOfDomain` in `fillet`
  (Matleyx78/solid-space#137); due segmenti consecutivi a forcina quasi
  chiusa, per esempio i vertici 10,0, 0,0 e 10,1e-15 con raggio 1 (centro a
  circa 2e16 dal vertice), danno `OutOfDomain` in `filletSegments` e contano
  in `tooShort` in `filletPolyline`; un raggio di 2e15, oltre `kMaxCoordinate`,
  tra due linee perpendicolari dà `None` in `fillet` e in
  `circleTangentTangentRadius`.
- Due segmenti consecutivi e non consecutivi di una polilinea; `filletPolyline`
  su un rettangolo, su una polilinea chiusa (la visita parte dal vertice 0) e
  con un segmento troppo corto; l'esempio di B15.
- *Trim No* (B6, 2026-11-02): `filletSegments` su [(0,10),(0,0),(10,0)] con
  raggio 1 (la polilinea invariata e un solo arco in `added`; raggio 0: nessun
  arco) e `filletPolyline` su un rettangolo e sull'esempio di B15 (i due
  archi, `done` = 2); con *Trim* `added` è sempre vuoto.
- Tangenti da fuori, sul cerchio, da dentro; su una polilinea, con il vertice
  comune (l'esempio di B7) e con un segmento rifiutato; snap a tangente senza
  `from`.

## Fatto quando

- I casi di `[0.7-R19]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
