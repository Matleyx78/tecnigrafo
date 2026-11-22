# [0.8.1-PT31] `cmd` e `script`: snap *Prolungamento* e *Parallelo*, anche nel tracciamento

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT31]` (#…), agente codex, fascia forte
  (guide, incroci, pareggi)
- **Dipende da:** PT2, PT5, PT28, PT30, RT31
- **Modulo:** `cmd` (più `!osnap ext,par` e le righe del tracciamento in
  `script`)
- **Da leggere:** [ADR 0057](../../adr/0057-snap-vicino-prolungamento-parallelo.md),
  punti 3–5;
  [ADR 0049](../../adr/0049-tracciamento-degli-snap-a-oggetto.md);
  [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 3, punto 1, e
  sezione 4, «Tracciamento degli snap»; `cmd/src/object_tracking.hpp`,
  `cmd/src/object_tracking.cpp`, `cmd/src/command_runner.cpp`
  (`pointerHeld`, `trackedPoint`, `aidedPoint`); la scheda [PT30](PT30-snap-vicino.md)

## File

- `cmd/src/object_tracking.hpp`, `cmd/src/object_tracking.cpp`
- `cmd/include/tg/cmd/command_runner.hpp` (`TrackingState`: acquisizioni e
  guide nuove), `cmd/src/command_runner.cpp`
- `script/src/script.cpp` (`ext`, `par`), `script/src/report.cpp`
  (`acquired:`, `active tracking:`)
- `tests/cmd/object_tracking_test.cpp`, `tests/cmd/command_runner_test.cpp`,
  `tests/script/report_test.cpp`

## Cosa fare

1. **Acquisizione** (ADR 0057, punto 3) con `pointerHeld`, anche con il
   tracciamento spento ma con lo snap a oggetto e il modo acceso: prolungamenti
   di `LINE`, `ARC` e dei segmenti estremi di `LWPOLYLINE` aperte; direzioni di
   `LINE` e segmenti diritti (solo con un punto base). Le acquisizioni stanno
   nella stessa lista dei punti del tracciamento, al più 7. **Con più
   candidati** (revisione PT5, [decisione] 4, regole esistenti): il
   prolungamento acquisito è quello dell'estremo che vince lo snap *Estremo*,
   quindi a pari distanza l'id più alto, e in una polilinea il suo segmento
   estremo (REGOLE-NUMERICHE.md, sezione 4, «Selezione e snap a oggetto»,
   punto 2); la direzione di *Parallelo* è quella del segmento che vince il
   clic, l'id più alto a pari distanza (punto 1). Una seconda sosta sullo
   stesso punto rilascia quell'acquisizione, punto e prolungamento insieme
   (ADR 0049, punto 2). Per questo l'acquisizione tiene id e segmento, che
   `SnapPoint` oggi non dà: chi realizza li ricava nello stesso passaggio di
   `findSnap`. **`setDrawingAids`** azzera le acquisizioni solo quando non
   può più esisterne nessuna: con lo snap a oggetto spento, o con il
   tracciamento spento e *Prolungamento* e *Parallelo* spenti; con OTRACK
   spento e *Prolungamento* acceso restano ([bloccante] 9; ADR 0057, punto
   3).
2. **Guide** (punto 4): prolungamento di una linea (semiretta), di un arco
   (tratto del cerchio oltre l'estremo), parallela per il punto base; incroci
   fra due guide qualsiasi con l'intersezione di solid-space (rette estese), e
   di un prolungamento d'arco con una guida diritta sul tratto oltre l'estremo;
   la priorità dell'ADR 0049, punto 5. **Pareggio rimasto** ([decisione] 5):
   quando due incroci alla stessa distanza usano tutti e due l'acquisizione
   più recente, vince quello la cui altra guida è stata acquisita più di
   recente; se la coppia di guide è la stessa (un prolungamento d'arco che
   incrocia una retta in due punti), vince il primo incrocio lungo il
   prolungamento, partendo dall'estremo acquisito e allontanandosi dall'arco
   (REGOLE-NUMERICHE.md, sezione 4, «Tracciamento degli snap», punto 6).
   Le guide partono dal punto dopo lo snap alla griglia, lo stesso per clic,
   preview, rapporto e vista (PT28, punto 2; [bloccante] 8).
   **Le guide nella vista** ([bloccante] 9): `TrackingState` dà, oltre al
   punto, l'elenco delle guide attive con la loro identità (l'acquisizione) e
   la geometria da disegnare (semiretta con origine e direzione, tratto di
   cerchio con centro, raggio e angoli, parallela con il punto base e la
   direzione), e lo stesso per l'incrocio; quando una guida si rilascia
   sparisce dall'elenco. La vista (PT33) disegna solo da qui, senza
   ricostruire niente.
3. **Distanza scritta** lungo una sola guida diritta, e **lungo l'arco** sul
   prolungamento di un arco: angolo `d / r` dall'estremo, con seno e coseno
   deterministici di `robust` (punto 4, ultimo). Sulla parallela la distanza
   si misura dal **punto base del comando** (`Command::basePoint()`), non da
   `lastPoint` come fa oggi il ramo della distanza scritta ([bloccante] 8).
   **Oltre il resto del cerchio** ([decisione] 5): se `d / r` supera
   l'ampiezza del tratto oltre l'estremo (`2π −` l'ampiezza dell'arco), la
   guida non vale e il numero è una distanza diretta verso il puntatore,
   come senza guide (ADR 0049, punto 6); il confine è compreso (una distanza
   che arriva esattamente all'altro estremo usa ancora la guida).
4. Il rapporto (punto 5; campi annunciati da PT2).

## Regole della tappa che valgono qui

- Distanze al quadrato, pareggi del tracciamento:
  [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 3, punto 1, e
  sezione 4, «Tracciamento degli snap», punti 1–5, e punto 6 per le guide
  nuove.
- `command_runner.cpp` lo cambiano prima PT34 e PT28.

## Test richiesti

- Le dodici `LINE` di `autocad-prolungamento-parallelo` (linea, arco e
  polilinea oltre i due estremi, con la distanza lungo l'arco; due incroci di
  prolungamenti; prolungamento e guida del tracciamento; le due parallele; il
  rilascio); la parallela senza punto base (nessuna acquisizione); l'ottava
  acquisizione.
- Revisione PT5:
  - due `LINE`, una verso destra e una verso l'alto, con l'estremo `(0,0)` in
    comune, *Prolungamento* ed *Estremo* accesi: la sosta su `(0,0)`
    acquisisce il punto e il prolungamento della linea con l'id più alto; una
    seconda sosta li rilascia tutti e due; due segmenti diritti che si
    incrociano lontano dagli estremi, *Parallelo*: la direzione di quello con
    l'id più alto ([decisione] 4);
  - prolungamenti verticali a `x=-1` (acquisito per primo) e `x=1`
    (secondo), poi la guida orizzontale `y=0` (ultima), puntatore
    `(0,0.25)` entro l'apertura da tutte e tre: `(1,0)`; un arco prolungato
    che incrocia una retta in due punti alla stessa distanza dal puntatore:
    il primo lungo il prolungamento ([decisione] 5);
  - `ARC` con centro `0,0`, raggio 10, da 0° a 90°, acquisito all'estremo
    `(0,10)`: `50` non usa la guida e dà il punto a 50 dal punto base verso il
    puntatore; la distanza che arriva esattamente a `(10,0)` lo dà sulla
    guida ([decisione] 5);
  - distanza scritta sulla parallela con il punto base diverso dall'ultimo
    punto ([bloccante] 8);
  - OTRACK spento e *Prolungamento* acceso: l'acquisizione resta dopo
    `setDrawingAids` e la guida c'è; il rilascio la toglie dall'elenco delle
    guide di `TrackingState` ([bloccante] 9).

## Fatto quando

- I casi di RT31 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/snap/prolungamento-parallelo/script.tgs
!view 0,-60 0.05 800x600
!osnap ext,par
LINE
0,-50
30,-50

LINE
0,-80
30,-70

LINE
!hover 30,-50
!move 38,-50.2
!report
10

LINE
0,-60
!hover 15,-75
!move 17,-54
!report
20

```

Righe da controllare: in `[report 1]` `acquired:` con `extension 30,-50` e
`active tracking: Extension 38,-50 from 30,-50`, la linea che parte da
`40,-50`; in `[report 2]` `parallel 0,-80 30,-70` e `Parallel … from 0,-60`, la
linea fino a `18.9737,-53.6754` (seduta della #920, punto 6).

- **Strumenti:** `!osnap ext,par` e i campi annunciati da PT2; `!hover` (0.8),
  `!view`, `!move`, `!report` su `main`.
- **Dipendenze:** `LINE` su `main`; PT30 per i modi, PT28 per il runner.
- **Fonte:** la seduta della #920, punto 6, e `autocad-prolungamento-parallelo.dxf`
  con la sua cronologia (seduta del 2026-11-22,
  [dxf-di-prova.md](dxf-di-prova.md)): le righe `[a mano]` dicono che cosa è
  stato acquisito in ogni `LINE`; i punti attesi sono i punti iniziali delle
  dodici `LINE`, letti con l'oracolo. Il rilascio non si vede nel DXF: la seduta
  dice che la croce sparisce e il prolungamento non si mostra più. RT31 aggiunge
  `autocad-prolungamento-parallelo` a `sorgenti.json`.
- **Risposte condivise:** nessuna: con *Prolungamento* e *Parallelo* spenti
  per difetto i casi del tracciamento di oggi non acquisiscono nulla di
  nuovo.
