# [0.8.1-PT31] `cmd` e `script`: snap *Prolungamento* e *Parallelo*, anche nel tracciamento

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT31]` (#…), agente codex, fascia forte
  (guide, incroci, pareggi)
- **Dipende da:** PT2, PT5, PT8, PT28, PT30, RT31 (PT8 per
  `core/src/document_query.cpp`)
- **Modulo:** `cmd` (più l'identità dello snap trovato in `core`, `!osnap
  ext,par` e le righe del tracciamento in `script`)
- **Da leggere:** [ADR 0057](../../adr/0057-snap-vicino-prolungamento-parallelo.md),
  punti 3–5;
  [ADR 0049](../../adr/0049-tracciamento-degli-snap-a-oggetto.md);
  [REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 3, punto 1, e
  sezione 4, «Tracciamento degli snap»; `cmd/src/object_tracking.hpp`,
  `cmd/src/object_tracking.cpp`, `cmd/src/command_runner.cpp`
  (`pointerHeld`, `trackedPoint`, `aidedPoint`); la scheda [PT30](PT30-snap-vicino.md)

## File

- `core/include/tg/core/document_query.hpp`, `core/src/document_query.cpp`
  (`findSnapHit`; verbale della revisione della correzione #992,
  [bloccante] 5), `tests/core/document_query_test.cpp`
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
   candidati** (revisione PT5, [decisione] 4, regole esistenti): la sosta
   acquisisce lo snap che vince (`findSnapHit`, con le priorità e i pareggi
   di sempre: a pari distanza l'id più alto, REGOLE-NUMERICHE.md, sezione 4,
   «Selezione e snap a oggetto», punto 2), e il prolungamento **solo se**
   quello snap è l'*Estremo* di una `LINE`, di un `ARC` o del segmento
   estremo di una `LWPOLYLINE` aperta. Per la sosta, con *Prolungamento*
   acceso, quegli estremi entrano in gara come *Estremo* **anche con
   *Estremo* spento**: se vincono si acquisisce il solo prolungamento (il
   punto, come punto del tracciamento, solo con OTRACK ed *Estremo* accesi);
   per il clic e la preview restano fuori (#992, secondo giro, [bloccante]
   1). Se vince un altro snap di punto (un *Medio* più vicino), si acquisisce
   solo quello, anche con un estremo dentro l'apertura (#992, [decisione] 4,
   scelta del responsabile). Per lo stesso motivo una sosta che acquisisce un
   estremo non acquisisce anche una direzione di *Parallelo*: la direzione
   si acquisisce solo quando nessuno snap di punto vince, estremi del
   *Prolungamento* compresi (#992, secondo giro, [decisione] 2, applicazione
   della stessa scelta); la direzione di *Parallelo* è quella del segmento che vince il
   clic, l'id più alto a pari distanza (punto 1). Una seconda sosta sullo
   stesso punto rilascia quell'acquisizione, punto e prolungamento insieme
   (ADR 0049, punto 2). Per questo l'acquisizione tiene id, segmento ed
   estremo, che `SnapPoint` non dà: `core` li restituisce con una funzione
   nuova, e `findSnap` resta com'è per i chiamanti di oggi (#992,
   [bloccante] 5):

   ```cpp
   struct SnapHit {
       SnapPoint snap;
       EntityId entity;                    // the entity that gives the point
       std::optional<std::size_t> segment; // for a polyline, the segment
       bool atStart{false};                // an endpoint: the start of the curve or segment
   };
   // findSnap() with where the point comes from; findSnap() returns its
   // `snap`. The same candidates, priorities and ties.
   [[nodiscard]] std::optional<SnapHit> findSnapHit(const Document& document,
       const Vec2& pointer, double aperture, const SnapModes& modes,
       std::optional<Vec2> from = std::nullopt);
   ```

   Due `LINE` con l'estremo `(0,0)` in comune danno le stesse coordinate ma
   un'identità diversa, e quindi prolungamenti diversi. Con due segmenti
   diritti della **stessa** polilinea alla stessa distanza (una polilinea
   che si incrocia), *Parallelo* prende il **primo segmento**, come `closest`
   e D7 (REGOLE-NUMERICHE.md, sezione 4, punto 2; #992, [decisione] 3,
   regola esistente). **`setDrawingAids`**: spegnere un modo toglie la parte
   delle acquisizioni che ne dipende, e riaccenderlo non la riporta (serve
   una nuova sosta); i posti liberati non contano più fra i 7 (#992,
   [decisione] 5, scelta del responsabile, come fa già oggi lo spegnimento
   di OTRACK). OTRACK spento toglie i punti del tracciamento,
   *Prolungamento* spento i prolungamenti, *Parallelo* spento le direzioni,
   lo snap a oggetto spento tutto; ciò che resta di un'acquisizione (il
   prolungamento, con OTRACK spento e *Prolungamento* acceso) continua a
   valere e si rilascia con una sosta ([bloccante] 9; ADR 0057, punto 3).
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
   Se le due coppie hanno le stesse acquisizioni ma guide diverse, si
   confrontano le guide dalla acquisizione più recente: prolungamento, poi
   orizzontale, poi verticale (REGOLE-NUMERICHE.md, sezione 4, «Tracciamento
   degli snap», punto 6, con i due esempi: `(1,1)` e `(-1,0)`; #992, primo giro [decisione] 1 e
   secondo giro [decisione] 1, regole esistenti).
   Con lo snap alla griglia, il nodo è il punto con cui si controlla
   l'apertura delle guide e che le guide proiettano, lo stesso per clic,
   preview, rapporto e vista; le guide restano dove sono (PT28, punto 2;
   [bloccante] 8).
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
   che arriva esattamente all'altro estremo usa ancora la guida). Lo stesso
   **dall'altra parte**: una distanza negativa porterebbe prima
   dell'estremo acquisito (sopra l'arco, o dietro la semiretta di una
   linea), fuori dal tratto, quindi la guida non vale e il numero è una
   distanza diretta, con il suo segno, come oggi senza guide; `0` dà
   l'estremo (#992, [decisione] 2, applicazione della stessa regola).
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
  - una `LINE` da `(-10,0)` a `(0,0)` e una da `(0.5,-5)` a `(0.5,5)`,
    *Estremo*, *Medio* e *Prolungamento* accesi, sosta in `(0.5,0)` con
    apertura 0,75: `acquired:` ha solo il punto `0.5,0`, nessun
    prolungamento (#992, [decisione] 4); lo stesso con *Estremo* spento;
  - con il solo *Prolungamento* (e *Parallelo*), senza *Estremo*, la sosta su
    un estremo acquisisce il prolungamento, come nella prova a secco
    (`extension 30,-50`); con OTRACK ed *Estremo* accesi anche il punto
    (#992, secondo giro, [bloccante] 1);
  - una `LINE` da `(-10,0)` a `(0,0)` e una da `(0,-5)` a `(0,5)`, *Estremo*,
    *Prolungamento* e *Parallelo* accesi, punto base `(0,5)`, sosta in
    `(0,0)` con apertura 0,75: si acquisisce l'estremo con il prolungamento,
    nessuna direzione (#992, secondo giro, [decisione] 2);
  - due punti acquisiti con il loro prolungamento, `(-1,-4)` lungo `y=2x-2`
    e poi `(1,4)` lungo `y=2x+2`, puntatore `(0,0)`, apertura 1,1: `(-1,0)`
    (#992, secondo giro, [decisione] 1);
  - `-5` sullo stesso arco e sul prolungamento di una linea: distanza
    diretta con il segno, non un punto della guida; `0`: l'estremo
    (#992, [decisione] 2);
  - il punto `(-1,1)` e il prolungamento lungo `y=x` dell'esempio del punto
    2: `(1,1)` (#992, [decisione] 1);
  - una `LWPOLYLINE` aperta `(-2,-2)`, `(2,2)`, `(-2,2)`, `(2,-2)`, punto base
    `(0,5)`, sosta in `(0,0)` con apertura 0,25: la direzione del primo
    segmento, quindi la parallela `y=5+x` (#992, [decisione] 3);
  - `findSnapHit` sulle due `LINE` con l'estremo in comune: id e estremo di
    quella con l'id più alto; `findSnap` dà lo stesso punto e tipo di oggi
    (#992, [bloccante] 5);
  - distanza scritta sulla parallela con il punto base diverso dall'ultimo
    punto ([bloccante] 8);
  - OTRACK spento e *Prolungamento* acceso: l'acquisizione resta dopo
    `setDrawingAids` e la guida c'è; il rilascio la toglie dall'elenco delle
    guide di `TrackingState` ([bloccante] 9);
  - un estremo acquisito con il suo prolungamento, poi *Prolungamento*
    spento e riacceso senza sosta: il prolungamento non c'è più, il punto sì;
    lo stesso con OTRACK spento e riacceso: il punto non c'è più, il
    prolungamento sì; sette acquisizioni, poi *Prolungamento* spento e una
    sosta nuova: nessuna acquisizione vecchia esce per far posto a quelle
    tolte (#992, [decisione] 5).

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
