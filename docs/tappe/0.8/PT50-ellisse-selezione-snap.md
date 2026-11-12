# [0.8-PT50] `core`: ellisse nella selezione e nello snap *Intersezione*

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT50]` (#…), agente codex, fascia forte: la
  facciata (traduzione dell'ellisse, posizioni, vertici delle polilinee,
  scelta con un clic e a intersezione, snap, dati fuori dal dominio); i punti,
  le distanze e le intersezioni dall'archivio dei casi di solid-space per
  `ellipse.hpp`, citando il caso
  ([ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 14),
  e da `autocad-ellissi.dxf`
- **Dipende da:** PT48, PT40 (revisione delle interfacce),
  Matleyx78/solid-space#217, RT50. La richiesta diventa i pacchetti di
  solid-space che realizzano `ellipse.hpp`, quando la tappa `ss-0.3` li avrà
  (ADR 0030, punto 13)
- **Modulo:** `core`
- **Da leggere:** [ADR 0052](../../adr/0052-ellisse.md), punti 3, 4 e 6;
  [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md), punti 1 e 23;
  l'[ADR 0017 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0017-ellisse-nel-piano.md)
  (Decisione); in `third_party/solid-space` l'intestazione qui sotto; la
  scheda [PT48](PT48-ellisse.md) (l'entità, ingombro, snap e grip);
  `core/include/tg/core/intersections.hpp`, `proximity.hpp`,
  `document_query.hpp` (`pickEntity`, `selectInRectangle`, `findSnap`);
  `core/src/solid_space_bridge.hpp`; l'idea
  [#663](https://github.com/Matleyx78/tecnigrafo/issues/663)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0017 di solid-space e rivisto in
[`[0.3-CS]` #222](https://github.com/Matleyx78/solid-space/issues/222), al
commit [`fd69e80`](https://github.com/Matleyx78/solid-space/commit/fd69e807d4ebafe12f73f46ba26072a46ff72809)
di `main` di solid-space (2026-11-12, #231):

- `solid_space/geom2d/ellipse.hpp` (`Ellipse`, `magnitude`, `endpoints`,
  `intersect` con una curva e con un'ellisse, `EllipseIntersections`,
  `EllipseIntersectionPoint`, `EllipseOverlap`, `nearest`, `EllipseNearest`,
  `meetsRectangle`, `kRelativeNearestError`);
- con i tipi di `curves.hpp` e `intersection.hpp` (`Contact`, `Location`,
  `Extension`) che la facciata usa già.

I commenti di `ellipse.hpp` dicono che cosa è deciso esattamente, gli errori e
la fascia di `nearest` (più candidati quasi alla stessa distanza: ne torna uno
qualsiasi, ADR 0013 di solid-space, punto 4): la scheda li cita e non li
ripete.

## Prima di cominciare

- **Sottomodulo.** Il pacchetto sposta `third_party/solid-space` al commit
  della riga «Sottomodulo di solid-space» della issue: quello che chiude
  l'ultimo dei pacchetti di solid-space che realizzano `ellipse.hpp` (oggi la
  richiesta Matleyx78/solid-space#217; il piano lo sostituisce con il
  pacchetto quando la tappa `ss-0.3` lo avrà). Se la riga manca, fermati
  ([contratto.md](../../agenti/contratto.md)).
- Prima della pull request confronta `ellipse.hpp` al commit di destinazione
  con quello di `fd69e80` ([contratto.md](../../agenti/contratto.md), «Prima
  della pull request, se sposti il sottomodulo»).

## Che cosa resta fuori

- **Snap *Vicino*.** L'ADR 0052, punto 4, lo nomina, ma Tecnigrafo non ha lo
  snap *Vicino* per nessuna entità: resta fuori dalla 0.8, per tutte le entità
  insieme, nell'idea [#665](https://github.com/Matleyx78/tecnigrafo/issues/665)
  (decisione del 2026-11-12, [DECISIONI.md](../../DECISIONI.md)). `nearest`
  serve qui alla scelta con un clic e alla domanda `closest`.
- **L'ellisse nei comandi di modifica**, anche come limite di taglio o di
  prolungamento (ADR 0052, punto 6; idea #663): `cutsAlong` non la usa.
- Snap *Perpendicolare* e *Tangente* su un'ellisse: l'ADR 0052, punto 3, non li
  prevede.

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/ellipse.hpp`, `core/src/ellipse.cpp` (di PT48)
- `core/include/tg/core/intersections.hpp`, `core/src/intersections.cpp`
- `core/src/proximity.cpp`, `core/src/snap_points.cpp`
- `core/src/solid_space_bridge.hpp`, `core/src/solid_space_bridge.cpp` (file
  comune con PT42 e PT43: una funzione)
- `docs/verifica/domande.md` (la riga `ELLIPSE` della tabella dei tipi)
- `tests/core/ellipse_query_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)
- `tests/verifica/in-attesa/0.8-RT50.txt` (da cancellare)

## Cosa fare

### 1. Il ponte

```cpp
/// The ellipse of solid-space for an Ellipse of Tecnigrafo (docs/adr/0052,
/// point 1): the same doubles, field by field (end parameter 0 and start 0
/// are the whole ellipse there too: equal parameters, ellipse.hpp).
[[nodiscard]] g::Ellipse ellipseOf(const Ellipse& ellipse);
```

### 2. Distanza, punto più vicino, rettangolo

In `ellipse.hpp` (se PT48 le ha già dichiarate con un corpo provvisorio, qui
cambia il corpo):

```cpp
/// Distance from `point` to the ellipse as given, never its extension
/// (docs/adr/0052, point 4): solid-space's nearest(), within its declared
/// error. Precondition: supported(ellipse).
[[nodiscard]] double distance(const Ellipse& ellipse, const Vec2& point);
/// The point of the ellipse nearest to `point` (nearest()); an end of an
/// arc is the end of endpoints(), bit for bit. Among candidates almost at the
/// same distance any may come back (the band of nearest()). std::nullopt for
/// data solid-space refuses. Precondition: supported(ellipse).
[[nodiscard]] std::optional<Vec2> closestPoint(const Ellipse& ellipse, const Vec2& point);
/// Crossing selection, borders included: solid-space's meetsRectangle(). A
/// whole ellipse around the rectangle without touching it is false.
/// Precondition: supported(ellipse); an empty rectangle is false.
[[nodiscard]] bool intersects(const Ellipse& ellipse, const BBox& rectangle);
```

- La finestra (`isInside`) resta quella di PT48, con l'ingombro.
- **Dati che solid-space rifiuta.** Con `outOfDomain` (una coordinata oltre
  2^50, o un parametro oltre `kMaxTrigArgument`): distanza infinita, nessun
  punto vicino (`closest` risponde `none`), `intersects` falso; è la regola dell'ADR
  0035, punto 23 (ciò che solid-space rifiuta resta fuori). Con
  `degenerateCurve` (un arco ridotto a una sola direzione, `ellipse.hpp`): la
  curva è il suo punto iniziale, `pointAt(ellipse, startParameter)` di PT48;
  distanza e punto vicino sono quelli di quel punto, `intersects` dice se il
  punto sta nel rettangolo.
- Le forme generate dagli inserimenti con scala non uniforme sono `Ellipse`
  come le altre: con queste funzioni diventano sceglibili con un clic e a
  intersezione (PT6, «Geometria generata»).

### 3. Estremi

Lo snap *Estremo* di un arco d'ellisse (PT48, punto 5) usa `endpoints()` di
solid-space invece del coseno e del seno di Tecnigrafo, come chiede
`ellipse.hpp`: così coincide bit per bit con il punto che dà `nearest()`
quando il più vicino è un estremo. Centro, quadranti e grip restano di PT48.

### 4. Intersezioni

`intersect(a, extendA, b, extendB)` di `intersections.hpp` accetta anche
l'ellisse:

- **ellisse e linea, arco, cerchio o polilinea:** `g::intersect(ellisse,
  curva)` del contratto; una polilinea pezzo per pezzo (`bridge::piecesOf`), i
  pezzi in ordine. Un vertice comune a due pezzi consecutivi (`atEnd` su uno e
  `atStart` sull'altro, deciso esattamente) si elenca una volta, sul pezzo che
  segue, come oggi; per una polilinea chiusa il vertice 0 sul pezzo 0. Punti
  di pezzi diversi che non sono un vertice comune non si uniscono: il
  contratto non dice quando due punti di chiamate diverse sono lo stesso
  punto esatto.
- **due ellissi:** `g::intersect(prima, seconda)`.
- **Ordine.** Con l'ellisse come `A`, l'ordine del contratto (lungo
  l'ellisse), pezzo per pezzo per una polilinea. Con l'ellisse come `B` e una
  curva come `A`, gli stessi punti di `intersect(B, A)`, con le posizioni
  scambiate e nello stesso ordine: **non** nell'ordine lungo `A`, perché il
  contratto non dà le stazioni sull'altra curva. Lo dice il commento di
  `Intersections::crossings`. Nessun chiamante di oggi ne dipende: lo snap
  sceglie il punto più vicino al puntatore, e `cutsAlong` non usa le ellissi.
- **Posizioni** sull'ellisse: segmento 0, la `Location` del contratto,
  stazione 0 (il contratto non la dà: la stazione è solo un'informazione,
  `intersections.hpp`). Contatto e posizioni sull'altra curva come oggi.
- **Tratti comuni** (un'ellisse di rapporto 1 e un cerchio o un arco sullo
  stesso cerchio, due ellissi uguali): `SharedStretch` dagli estremi di
  `EllipseOverlap`, `whole` come il contratto, stazioni 0.
- `Extension::Extended` per un'ellisse è l'ellisse intera
  (`Extension::extended` del contratto); per l'altra curva vale quello di oggi.
- `omittedBeyondDomain` maggiore di 0, o una diagnosi `outOfDomain`:
  `complete` falso, come oggi.
- `cutsAlong` con un'ellisse come ospite o fra i limiti: l'ellisse non dà e non
  riceve tagli (idea #663), come un testo. `selfIntersections`, `sideOf` e
  `nearestPosition`: l'ellisse non c'è (`std::nullopt` per `sideOf`;
  precondizione «non un'ellisse» per `nearestPosition`, che usano solo `TRIM`
  ed `EXTEND`).

Lo snap *Intersezione* (`findSnap`) prende così anche le intersezioni con le
ellissi, con le regole di oggi (`document_query.hpp`); i tratti comuni non
danno punti.

### 5. La riga `ELLIPSE` delle domande

Nella tabella «Come ogni tipo di entità risponde» di domande.md:

| Tipo | `length` | `distance`, `closest` | `snaps` |
|---|---|---|---|
| `ELLIPSE` | `none` | dalla curva, mai dal resto dell'ellisse di un arco; a pari distanza (entro l'errore di solid-space) uno qualsiasi dei punti più vicini: i casi non ne usano | centro, quadranti sulla curva, estremi di un arco |

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` (ADR 0044, punto 8).
- File comuni: `core/src/solid_space_bridge.*` (PT42, PT43 e PT50: ognuno la
  sua funzione, in ordine alfabetico), `tests/CMakeLists.txt`.
- `docs/verifica/domande.md` lo toccano PT9, PT34, PT43 e PT50: solo le tue
  righe; chi arriva dopo si aggiorna.

## Test richiesti

- `ellipse_query_test.cpp`:
  - distanza e punto vicino da fuori, da dentro, da un punto sull'ellisse, da
    un punto oltre l'estremo di un arco (l'estremo, bit per bit come
    `endpoints()`);
  - `intersects`: un rettangolo che taglia l'ellisse, uno dentro l'ellisse
    intera senza toccarla (falso), uno che contiene solo l'estremo di un
    arco;
  - `pickEntity` e `selectInRectangle` (finestra e intersezione) con un'ellisse
    e con un inserimento stirato che genera un'ellisse;
  - intersezioni con una linea (due punti), un cerchio tangente (`touching`),
    una polilinea chiusa con un vertice sull'ellisse (una volta, sul pezzo che
    segue), un'altra ellisse, un cerchio uguale a un'ellisse di rapporto 1
    (tratto comune intero); l'ellisse come `B`;
  - `findSnap` con *Intersezione* fra una linea e un'ellisse; *Estremo* di un
    arco d'ellisse;
  - un'ellisse oltre il dominio e un arco ridotto a un punto.

## Fatto quando

- I casi di RT50 passano, compresi quelli di `autocad-ellissi.dxf`.
- La CI è verde.
