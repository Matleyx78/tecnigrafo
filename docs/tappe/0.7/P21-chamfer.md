# [0.7-P21] `cmd`: `CHAMFER`, con lo smusso in `core`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R21]` (#…), agente gemini (valori da
  `autocad-raccordi.dxf`; lo smusso è un calcolo di Tecnigrafo)
- **Dipende da:** P20, P25, R21
- **Corretta:** 2026-11-01, con le decisioni del responsabile sulla #359 e sui
  buchi B2, B6, B8, B9, B14 e B15 della #464 ([DECISIONI.md](../../DECISIONI.md))
- **Modulo:** `cmd` (più lo smusso in `core` e la domanda `chamfer` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 15, 16, 21, 22 e 23; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 8; `core/include/tg/core/fillet.hpp` (P19: `CornerResult`),
  `core/include/tg/core/intersections.hpp`, `tolerance.hpp` e
  `core/src/solid_space_bridge.hpp` (P14); [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punto 5; in `third_party/solid-space` l'intestazione qui sotto;
  `core/include/tg/core/curve_parts.hpp` (P15);
  `cmd/src/commands/fillet_command.cpp` (P20, lo stesso schema);
  [dxf-di-prova.md](dxf-di-prova.md) ("Che cosa resta da controllare", punto 4)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0010 di solid-space:
`solid_space/geom2d/intersection.hpp` (`intersect`, `PairIntersections`,
`Extension`), con i tipi di `curves.hpp`, nella versione fissata da P19. Il
sottomodulo non si sposta.

## File

- `core/include/tg/core/chamfer.hpp`, `core/src/chamfer.cpp` (nuovi)
- `core/CMakeLists.txt`
- `cmd/include/tg/cmd/commands/chamfer_command.hpp`,
  `cmd/src/commands/chamfer_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults`: distanze, lunghezza,
  angolo, metodo)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/chamfer_test.cpp`, `tests/cmd/chamfer_command_test.cpp` (nuovi),
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R21.txt` (da cancellare)

## Cosa fare

### Lo smusso (`chamfer.hpp`)

```cpp
struct ChamferDistances { double first{0.0}; double second{0.0}; };
struct ChamferAngle { double length{0.0}; double angle{0.0}; };  ///< on the first; radians
using ChamferSpec = std::variant<ChamferDistances, ChamferAngle>;

/// CHAMFER of two lines, or two straight segments of one polyline, picked at
/// the given points (docs/adr/0035, point 15). An arc, a circle or a text:
/// NotAllowed. Parallel or collinear lines: None. Two almost parallel lines
/// whose corner lies beyond solid-space's domain: OutOfDomain. A pick whose
/// projection on the line of its entity is exactly the corner: AmbiguousPick.
/// `trim` applies only here, between distinct entities.
/// Precondition (also for the two functions below): the values of `spec`
/// finite, distances and length >= 0.
[[nodiscard]] CornerResult chamfer(const Entity& first, const Vec2& pickFirst,
                                   const Entity& second, const Vec2& pickSecond,
                                   const ChamferSpec& spec, bool trim,
                                   const ToleranceRule& rule);
/// The same on two segments of one polyline (as filletSegments): the polyline
/// always changes, Trim or not.
[[nodiscard]] CornerResult chamferSegments(const Polyline& polyline, const Vec2& pickFirst,
                                           const Vec2& pickSecond, const ChamferSpec& spec,
                                           const ToleranceRule& rule);
/// CHAMFER Polyline: every original vertex between two straight segments, in
/// order, as filletPolyline; the polyline always changes, Trim or not.
[[nodiscard]] PolylineCorners chamferPolyline(const Polyline& polyline, const ChamferSpec& spec,
                                              const ToleranceRule& rule);
```

- **Lo spigolo** è il punto comune delle due rette, deciso da solid-space:
  `intersect` di `intersection.hpp` sulle curve delle due linee, con la
  traduzione di P14 (`bridge::curveOf`, o il pezzo di `bridge::piecesOf` per
  un segmento di polilinea), tutte e due con `Extension::extended`. Non si
  passa da `intersect` della facciata (P14), che riassume in `complete` sia i
  dati rifiutati sia i punti omessi oltre il dominio: qui vanno distinti.
  - Un punto in `points`: lo spigolo.
  - Nessun punto e `omittedBeyondDomain` maggiore di zero: lo spigolo esiste,
    ma oltre il dominio (due rette quasi parallele), `OutOfDomain` (ADR 0035,
    punto 15, corretto il 2026-11-01). È lo stesso caso di `OutOfDomain` in
    `fillet` con raggio 0 (P19).
  - Nessun punto e niente di omesso (rette parallele), o un tratto in comune
    in `overlaps` (rette coincidenti): `None`.
  - Una diagnosi (una linea di lunghezza zero, dati fuori dal dominio, ADR
    0035, punto 23): `None`.

  Per due segmenti consecutivi di una polilinea lo spigolo è il loro vertice
  comune, che sta nel dominio: lì `OutOfDomain` non capita.
- **La parte da tenere** di ciascuna linea è quella dalla parte dello spigolo
  dove cade la proiezione del punto indicato (`nearestPosition` sulla retta):
  è la scelta dell'utente, non una decisione geometrica. Se la proiezione di
  uno dei due punti è **esattamente lo spigolo**, l'esito è `AmbiguousPick`
  (B9), in `chamfer` e in `chamferSegments`: per esempio A da (−10,0) a
  (10,0), B da (0,−10) a (0,10), il punto su A in (0,0) o in (0,2).
- **Gli estremi dello smusso** stanno alle distanze date dallo spigolo, lungo
  la parte tenuta. È una costruzione di Tecnigrafo (ADR 0035, punto 15).
- **Con *Angle*** (B8):
  - il primo punto P1 sta alla lunghezza data dallo spigolo, sulla parte
    tenuta della prima linea;
  - lo smusso parte da P1 nella direzione **da P1 verso lo spigolo**, ruotata
    dell'angolo **verso il lato in cui sta la parte tenuta della seconda
    linea**;
  - il secondo punto è dove questa retta incontra la seconda linea;
  - se la retta è parallela alla seconda linea, o il punto d'incontro non sta
    sulla parte tenuta della seconda linea a distanza positiva dallo spigolo,
    l'esito è `None`. Angolo 0, angoli troppo grandi e parallelismo non hanno
    regole a parte.

  Per esempio, prima linea sull'asse X, seconda sull'asse Y, parti tenute
  positive, lunghezza 1 e angolo 45°: lo smusso va da (1,0) a (0,1).
- **Distanze** (B8): una sola distanza zero dà `None`; tutte e due zero, lo
  spigolo senza linea nuova; una distanza (o la lunghezza) uguale alla parte
  tenuta, quella parte si cancella (B14); una distanza maggiore della parte
  tenuta, `None`.
- Le linee si portano agli estremi dello smusso con `partOf` o `extendTo`
  (P15, B2: sceglie la `Location`), con `PartCourse::Direct` perché la
  costruzione dà un solo posto per linea (#359). Come in P19 (B14), una parte
  tenuta `Empty` o degenere per la regola del documento lascia vuoto `first` o
  `second` (l'entità si cancella), e una parte `NotRepresentable` dà `None`.
  Con `trim` falso le linee ci sono sempre, invariate. La linea nuova va in
  `added`. Per giudicare degenere una parte tenuta le tre funzioni ricevono la
  regola del documento (`ToleranceRule::of`), come `fillet`: è l'unico
  parametro nuovo, aggiunto con B14.
- ***Trim* solo fra entità distinte** (B6): `chamferSegments` e
  `chamferPolyline` modificano sempre la polilinea.
- **`chamferPolyline`** (B15): la visita di `filletPolyline` (P19): i vertici
  originali in ordine, ognuno sulla polilinea già modificata dai precedenti;
  uno smusso che non sta nei due segmenti come sono in quel momento, che non
  esiste o che è oltre il dominio lascia il vertice com'è e conta in
  `tooShort`. Per esempio [(0,2),(0,0),(1,0),(1,2)] con distanze 0.75 e 0.75:
  `done` = 1 e `tooShort` = 1.
- La domanda `chamfer` del catalogo, con le risposte scritte lì.

### `CHAMFER` (`CHA`)

Prima il messaggio `(TRIM mode) Current chamfer Dist1 = <d1>, Dist2 = <d2>` (o
`Length = <l>, Angle = <a>` con il metodo dell'angolo), poi:

```
Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]:
Select second line or shift-select to apply corner or [Distance/Angle/Method]:
Specify first chamfer distance <{default}>:
Specify second chamfer distance <{default}>:
Specify chamfer length on the first line <{default}>:
Specify chamfer angle from the first line <{default}>:
Enter trim method [Distance/Angle] <{default}>:
Select 2D polyline or [Distance/Angle/Method]:
```

- Stesso schema di `FILLET` (P20): richieste di oggetto, `Shift` per lo
  spigolo, *Multiple*, *Undo*, le due sequenze (negli script il comando finisce
  dopo *Distance*, *Angle*, *mEthod* e *Trim*), le proprietà della linea nuova
  (ADR 0035, punto 16).
- `NotAllowed` (un arco) e `None` danno il messaggio inglese di AutoCAD (da
  controllare, ADR 0035 punto 22); `OutOfDomain` dà il messaggio di `FILLET`
  per le rette quasi parallele, `Lines are nearly parallel.` (P20), distinto
  da `Lines are parallel.`; `AmbiguousPick` dà il messaggio di `AmbiguousPick`
  di `FILLET` (B9); in tutti i casi la domanda resta.
- Una distanza, una lunghezza o un angolo negativo o non numerico dà il
  messaggio inglese di AutoCAD (da controllare, ADR 0035 punto 22) e la
  domanda resta (B8): a `core` arrivano solo valori che rispettano la sua
  precondizione.
- Un `first` o un `second` vuoto cancella l'entità, come in `FILLET` (P20,
  B14).
- Un passo di annulla `Chamfer`.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P20, dopo di te P23.
- Prima di cominciare controlla nella issue l'esito della prova di `CHAMFER`
  con `TRIMMODE` = 0 su due segmenti di una stessa polilinea e con *Polyline*
  ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da controllare", punto
  4). Se AutoCAD lascia la polilinea com'è e aggiunge lo smusso come entità a
  sé, fermati: la decisione B6 si rivede prima di questo pacchetto.
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P17, P19 e P22).
- Lo spigolo e la sua esistenza vengono da `intersect` di solid-space:
  nessuna soglia propria per "parallele" né per "quasi parallele".

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `CHAMFER`, nelle due sequenze.
- Distanze, angolo, *mEthod*, *Polyline*, *Trim No* (anche su due segmenti di
  una polilinea e con *Polyline*), `Shift`, un arco, parallele, distanza
  troppo lunga, *Multiple*, *Undo*; annulla e ripeti.
- Gli esempi di B8 (angolo 45°; una sola distanza zero; distanza uguale alla
  parte tenuta, che cancella la linea), di B9 (clic proiettato sullo
  spigolo, `AmbiguousPick`) e di B15; angolo che dà una retta parallela alla
  seconda linea (`None`); valori negativi nel comando.
- Rette quasi parallele (i dati di P19, da 0,0 a 1,0 e da 0,1 a
  1,1.0000000000000002): `OutOfDomain` in `chamfer` e il suo messaggio in
  `CHAMFER`, distinto da quello delle parallele; una linea di lunghezza zero:
  `None`.

## Fatto quando

- I casi di `[0.7-R21]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
