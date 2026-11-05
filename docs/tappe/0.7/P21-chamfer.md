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
  buchi B2, B6, B8, B9, B14 e B15 della #464 ([DECISIONI.md](../../DECISIONI.md));
  2026-11-02, con la revisione di B6 dopo le prove in AutoCAD (#468);
  2026-11-03, operazione snellimento (#478): il messaggio di `OutOfDomain` (P20)
  e i rimandi a `intersection.hpp`; 2026-11-05, con le decisioni del
  responsabile sulla #507: il comando finisce dopo uno smusso non riuscito,
  l'esito `DistanceTooLarge` con `Chamfer distance too large.` e il valore
  predefinito della seconda distanza; con quelle sulla #508:
  `DistanceTooLarge` anche con *Angle* e, con una distanza zero e l'altra
  troppo lunga, `None` (B8 prevale)
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
/// A distance, or the length, longer than the kept part of its line, or with
/// an angle a second point beyond the kept part of the second line:
/// DistanceTooLarge.
/// With `trim` off the two lines are unchanged and the new line is in `added`.
/// Precondition (also for the two functions below): the values of `spec`
/// finite, distances and length >= 0.
[[nodiscard]] CornerResult chamfer(const Entity& first, const Vec2& pickFirst,
                                   const Entity& second, const Vec2& pickSecond,
                                   const ChamferSpec& spec, bool trim,
                                   const ToleranceRule& rule);
/// The same on two segments of one polyline (as filletSegments): with `trim`
/// the polyline changes; without, `first` is the polyline unchanged and the
/// new line is in `added`.
[[nodiscard]] CornerResult chamferSegments(const Polyline& polyline, const Vec2& pickFirst,
                                           const Vec2& pickSecond, const ChamferSpec& spec,
                                           bool trim, const ToleranceRule& rule);
/// CHAMFER Polyline: every original vertex between two straight segments, in
/// order, as filletPolyline; with `trim` the polyline changes; without, it is
/// unchanged and the new lines are in `added`.
[[nodiscard]] PolylineCorners chamferPolyline(const Polyline& polyline, const ChamferSpec& spec,
                                              bool trim, const ToleranceRule& rule);
```

- **Lo spigolo** è il punto comune delle due rette, deciso da solid-space:
  `intersect` di `intersection.hpp` sulle curve delle due linee, con la
  traduzione di P14 (`bridge::curveOf`, o il pezzo di `bridge::piecesOf` per
  un segmento di polilinea), tutte e due con `Extension::extended`. Non si
  passa da `intersect` della facciata (P14), che riassume in `complete` sia i
  dati rifiutati sia i punti omessi oltre il dominio: qui vanno distinti.
  I campi sono quelli di `PairIntersections` (`intersection.hpp`); qui si
  dice soltanto come diventano esiti:
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
    dalla parte tenuta della seconda linea a distanza positiva dallo spigolo
    (cade nello spigolo o dall'altra parte), l'esito è `None`;
  - se il punto d'incontro sta dalla parte tenuta ma più lontano dallo spigolo
    della fine della parte tenuta, l'esito è `DistanceTooLarge`, come per una
    seconda distanza scritta troppo lunga (decisione del 2026-11-05, #508); a
    distanza uguale la parte tenuta della seconda linea si cancella, come per
    le distanze (B14). Per esempio, prima linea da (0,0) a (10,0), seconda da
    (0,0) a (0,1), scelte in (8,0) e (0,0.5), lunghezza 5 e angolo 45°: lo
    smusso incontra la seconda retta in (0,5), oltre (0,1), e l'esito è
    `DistanceTooLarge`; con la seconda linea da (0,0) a (0,6) lo smusso va da
    (5,0) a (0,5) e della seconda linea resta il tratto da (0,5) a (0,6);
  - **la fascia al limite:** il secondo punto è costruito in `double`, e quando
    cade alla fine della parte tenuta della seconda linea, entro l'errore di
    arrotondamento della costruzione, l'esito è libero: la parte tenuta si
    cancella o l'esito è `DistanceTooLarge` (fascia dell'ADR 0013 di
    solid-space, punto 4; revisione della #508). Lo è per esempio la seconda
    linea da (0,0) a (0,5) con i dati sopra. Chi implementa scrive nel
    commento di `chamfer` l'errore della costruzione; nessun test pretende uno
    dei due esiti nella fascia, e i test fuori dalla fascia (la seconda linea
    fino a (0,1) e fino a (0,6)) distinguono i due lati. Il comando e la
    domanda `chamfer` usano l'esito che dà `core`, senza ricalcolarlo;
  - angolo 0, angoli troppo grandi e parallelismo non hanno regole a parte.

  Per esempio, prima linea sull'asse X, seconda sull'asse Y, parti tenute
  positive, lunghezza 1 e angolo 45°: lo smusso va da (1,0) a (0,1).
- **Distanze** (B8): una sola distanza zero dà `None`; tutte e due zero, lo
  spigolo senza linea nuova; una distanza (o la lunghezza) uguale alla parte
  tenuta, quella parte si cancella (B14); una distanza (o la lunghezza)
  maggiore della parte tenuta, `DistanceTooLarge`, in `chamfer` e in
  `chamferSegments`, anche con *Angle* come detto sopra (decisione del
  2026-11-05, #507: è il "messaggio di AutoCAD" dell'ADR 0035, punto 15,
  distinto da quello delle parallele). Per
  esempio, linee (200,0)→(210,0) e (200,0)→(200,10), scelte in (205,0) e
  (200,5), distanze 15 e 15, o 5 e 15: `DistanceTooLarge`, e nessuna entità
  cambia. In `chamferPolyline` un vertice così resta com'è e conta in
  `tooShort`, come gli altri smussi che non stanno nei due segmenti.
- **Una distanza zero e l'altra troppo lunga** (decisione del 2026-11-05 sulla
  #508, R508-1): prevale B8, quindi `None`, prima di guardare le lunghezze, in
  `chamfer` e in `chamferSegments`, in tutti e due gli ordini. Per esempio,
  le linee sopra con le distanze 0 e 15, e le stesse linee scambiate, con i
  clic scambiati e le distanze 15 e 0: `None` (`Lines are parallel.` in
  `CHAMFER`, `none` nella domanda), e nessuna entità cambia. In
  `chamferPolyline` il vertice conta in `tooShort` come prima.
- Le linee si portano agli estremi dello smusso con `partOf` o `extendTo`
  (P15, B2: sceglie la `Location`), con `PartCourse::Direct` perché la
  costruzione dà un solo posto per linea (#359). Come in P19 (B14), una parte
  tenuta `Empty` o degenere per la regola del documento lascia vuoto `first` o
  `second` (l'entità si cancella), e una parte `NotRepresentable` dà `None`.
  Con `trim` falso le linee ci sono sempre, invariate. La linea nuova va in
  `added`. Per giudicare degenere una parte tenuta le tre funzioni ricevono la
  regola del documento (`ToleranceRule::of`), come `fillet`: è l'unico
  parametro nuovo, aggiunto con B14.
- ***Trim* vale anche per le polilinee** (B6, rivista il 2026-11-02 dopo la
  prova in AutoCAD: `autocad-prove-a-mano.dxf`, punto 4, in cui con `TRIMMODE`
  0 la polilinea resta com'è e lo smusso nasce come entità a sé).
  `chamferSegments` e `chamferPolyline` ricevono `trim`, come `chamfer`: con
  *Trim No* la polilinea resta invariata (`first`, uguale a quella ricevuta) e
  le linee di smusso vanno in `added`, una per vertice smussato; nessuna con le
  due distanze zero.
- **`chamferPolyline`** (B15): la visita di `filletPolyline` (P19): i vertici
  originali in ordine, ognuno sulla polilinea già modificata dai precedenti;
  uno smusso che non sta nei due segmenti come sono in quel momento, che non
  esiste o che è oltre il dominio lascia il vertice com'è e conta in
  `tooShort`. Per esempio [(0,2),(0,0),(1,0),(1,2)] con distanze 0.75 e 0.75:
  `done` = 1 e `tooShort` = 1. Con *Trim No* la polilinea non cambia e ogni
  vertice si giudica sui suoi due segmenti **originali**, come in
  `filletPolyline` (P19): lo stesso esempio dà `done` = 2, `tooShort` = 0 e due
  linee in `added`.
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

- Il valore predefinito della seconda distanza è la prima distanza appena
  scritta, come in AutoCAD (decisione del 2026-11-05, #507): dopo
  `Specify first chamfer distance <0.0000>: 5` viene
  `Specify second chamfer distance <5.0000>:`. Gli altri valori predefiniti
  sono quelli correnti.
- Stesso schema di `FILLET` (P20): richieste di oggetto, `Shift` per lo
  spigolo, *Multiple*, *Undo*, le due sequenze (negli script il comando finisce
  dopo *Distance*, *Angle*, *mEthod* e *Trim*), le proprietà della linea nuova
  (ADR 0035, punto 16).
- `NotAllowed` (un arco) dà `Cannot chamfer those objects.`, `None` e
  `InfinitelyMany` danno `Lines are parallel.`, `DistanceTooLarge` dà
  `Chamfer distance too large.` (tabella di
  [dxf-di-prova.md](dxf-di-prova.md), punto 3); `OutOfDomain` dà il messaggio di `FILLET`
  per "oltre il dominio", `Result is beyond the drawing limits.` (P20), distinto
  da `Lines are parallel.`; `AmbiguousPick` dà il messaggio di `AmbiguousPick`
  di `FILLET` (B9). In tutti i casi nessuna entità cambia e, come in `FILLET`
  (P20), il comando finisce; con *Multiple* torna la prima domanda (decisione
  del 2026-11-05, #507).
- Una distanza, una lunghezza o un angolo negativo o non numerico dà il
  messaggio della tabella di [dxf-di-prova.md](dxf-di-prova.md), punto 3
  (`Value must be positive.` o `Requires numeric distance.`), e la
  domanda resta (B8): a `core` arrivano solo valori che rispettano la sua
  precondizione.
- Un `first` o un `second` vuoto cancella l'entità, come in `FILLET` (P20,
  B14).
- *Trim No* anche su due segmenti di una polilinea e con *Polyline*, come in
  `FILLET` (P20, B6): la polilinea resta com'è, senza `replace`, e le linee di
  `added` si aggiungono come entità a sé, con le proprietà della polilinea
  (ADR 0035, punto 16), nella stessa operazione.
- Un passo di annulla `Chamfer`.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P20, dopo di te P23.
- La prova a mano di `CHAMFER` con `TRIMMODE` = 0 su due segmenti di una
  stessa polilinea e con *Polyline* ([dxf-di-prova.md](dxf-di-prova.md), "Che
  cosa resta da controllare", punto 4) è fatta, e non c'è niente da
  controllare nella issue: AutoCAD lascia la polilinea com'è e lo smusso è
  un'entità a sé, e questo dà la decisione B6 di adesso (2026-11-02).
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P17, P19 e P22).
- Lo spigolo e la sua esistenza vengono da `intersect` di solid-space:
  nessuna soglia propria per "parallele" né per "quasi parallele".

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `CHAMFER`, nelle due sequenze.
- Distanze, angolo, *mEthod*, *Polyline*, *Trim No*, `Shift`, un arco,
  parallele, distanza troppo lunga, *Multiple*, *Undo*; annulla e ripeti.
- *Trim No* su due segmenti di una polilinea e con *Polyline* (i casi della
  prova a mano, punto 4: [(0,10),(0,0),(10,0)] con distanze 1 e 1, e *Polyline*
  sulla stessa polilinea spostata): la polilinea invariata, la linea come
  entità a sé con le proprietà della polilinea, un solo passo di annulla; e
  l'esempio di B15 con *Trim No*, che dà due linee.
- Gli esempi di B8 (angolo 45°; una sola distanza zero; distanza uguale alla
  parte tenuta, che cancella la linea), di B9 (clic proiettato sullo
  spigolo, `AmbiguousPick`) e di B15; angolo che dà una retta parallela alla
  seconda linea (`None`); valori negativi nel comando.
- `DistanceTooLarge` con l'esempio di "Distanze" (distanze 15 e 15, 5 e 15, e
  la sola prima troppo lunga, 15 e 5) e con quelli di *Angle*: il secondo
  punto in (0,5), oltre la fine (0,1) della seconda linea, anche con la
  seconda linea invertita, da (0,1) a (0,0); la sola lunghezza troppo lunga
  (lunghezza 15 sulla prima linea lunga 10); in `chamfer` e in
  `chamferSegments`. In `CHAMFER`: `Chamfer distance too large.`, nessuna
  modifica e il comando finito. Con la seconda linea fino a (0,6) lo smusso
  riesce; la seconda linea fino a (0,5) sta nella fascia e nessun test ne
  fissa l'esito.
- Dopo uno smusso non riuscito (le parallele, un arco) il comando finisce,
  senza passi di annulla; con *Multiple* torna la prima domanda.
- Il valore predefinito della seconda distanza: dopo una prima distanza 5,
  `<5.0000>`.
- Una distanza zero e l'altra troppo lunga (R508-1): le distanze 0 e 15, e
  le linee scambiate con le distanze 15 e 0, danno `None` in `chamfer` e in
  `chamferSegments`; in `CHAMFER`, `Lines are parallel.`, nessuna modifica e
  il comando finito.
- Rette quasi parallele (i dati di P19, da 0,0 a 1,0 e da 0,1 a
  1,1.0000000000000002): `OutOfDomain` in `chamfer` e il suo messaggio in
  `CHAMFER`, distinto da quello delle parallele; una linea di lunghezza zero:
  `None`.

## Fatto quando

- I casi di `[0.7-R21]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
