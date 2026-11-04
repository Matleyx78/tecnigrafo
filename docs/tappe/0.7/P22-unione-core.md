# [0.7-P22] `core`: unione di entità contigue

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R22]` (#…), agente gemini: la facciata (catena,
  ordine, entità che risultano, unità); i confini della regola dall'archivio
  dei casi di solid-space
- **Dipende da:** P14, Matleyx78/solid-space#23, P25, R22, R22b,
  Matleyx78/solid-space#167. La richiesta diventa il
  pacchetto di solid-space che realizza `coincidence.hpp`, quando la tappa 0.2
  di solid-space lo avrà (ADR 0030, punto 13). Matleyx78/solid-space#167
  (0.2-P21) realizza `joinAligned`, la funzione degli estremi delle linee
  allineate unite (punto 3 della #373): la decisione è nella issue `contratto`
  Matleyx78/solid-space#158, e l'ha scritta nel contratto la pull request
  Matleyx78/solid-space#159, unita. La issue di P22 dice a quale commit
  spostare il sottomodulo: quello che chiude la Matleyx78/solid-space#167
- **Corretta:** 2026-11-01, con le decisioni del responsabile sui buchi B10–B13,
  B16 e B17 della #464 ([DECISIONI.md](../../DECISIONI.md)); 2026-11-02, con la
  revisione di B11 dopo le prove in AutoCAD (#468); 2026-11-02, con le
  decisioni sui quattro buchi trovati nella #373 (verso dall'inizio della
  catena, polilinee chiuse, ordine degli estremi delle linee allineate, arco
  con angoli uguali); 2026-11-03, operazione snellimento (#478): rimanda alla
  decisione della Matleyx78/solid-space#158 e alla
  Matleyx78/solid-space#159, e la fascia di B13 all'ADR 0013 di solid-space;
  2026-11-03, con la decisione del responsabile su B13: gli estremi degli archi
  vengono da `endpoints()` di solid-space (DECISIONI.md, 2026-11-03), e la
  fascia comprende gli errori di tutti e due gli estremi (revisione della #480,
  R480-2);
  2026-11-03, con il responsabile: che cosa fa `JOIN` con una diagnosi di
  `endpoints()`, per l'ADR 0035, punto 23; 2026-11-03, decisione del
  responsabile su R480-1 (revisione della #480; DECISIONI.md, 2026-11-03): un
  estremo restituito oltre `kMaxCoordinate` vale come una diagnosi, con la
  sua fascia al confine del dominio (R480-4); 2026-11-03, la dipendenza
  Matleyx78/solid-space#158 diventa Matleyx78/solid-space#167 (0.2-P21), e
  `joinAligned` è il nome definitivo, perché la Matleyx78/solid-space#159 è
  unita; 2026-11-04, decisione del responsabile sul buco della #373 trovato da
  Codex: `isStraight` decide sul segmento com'è nella sua entità, prima che un
  suo estremo si sposti sul vertice comune (DECISIONI.md, 2026-11-04), con i
  casi in R22b (#493)
- **Modulo:** `core` (più la domanda `join` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2, 17–20; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punti 1, 2 e 5; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punto 7;
  l'[ADR 0009 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0009-regola-delle-tolleranze-del-piano.md)
  (Decisione); in `third_party/solid-space` l'intestazione qui sotto;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-unione.dxf`, e "Che cosa resta
  da controllare", punti 5, 6 e 7);
  `core/include/tg/core/arc_geometry.hpp` (`normalizeAngle`);
  `core/include/tg/core/tolerance.hpp` e `core/src/solid_space_bridge.hpp` (P14)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0009 di solid-space:
`solid_space/geom2d/coincidence.hpp` (`areAligned`, `onSameCircle`,
`isStraight`) e `solid_space/robust/tolerance.hpp` (`coincident`), con i tipi
di `curves.hpp` ed `endpoints()` per gli estremi degli archi (B13). Per le linee allineate anche la funzione di
`coincidence.hpp` che dà gli estremi del segmento unito, decisa nella
[Matleyx78/solid-space#158](https://github.com/Matleyx78/solid-space/issues/158)
e scritta nella pull request
[Matleyx78/solid-space#159](https://github.com/Matleyx78/solid-space/pull/159),
unita: `joinAligned`, realizzata dalla
[Matleyx78/solid-space#167](https://github.com/Matleyx78/solid-space/issues/167)
(0.2-P21). Vale il suo commento in `coincidence.hpp`, non una sintesi qui. Il
commit del sottomodulo lo scrive la issue di P22.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude Matleyx78/solid-space#167 (0.2-P21), con
  `joinAligned` realizzata. Se P14 l'ha già portato lì o oltre, il
  sottomodulo non si sposta. Se la issue non lo dice, chiedilo e non
  cominciare.
- Se qualcosa che serve qui manca dal contratto o non si comporta come dichiara,
  **fermati** come dice AGENTS.md ("Se serve qualcosa a solid-space").

## File

- `third_party/solid-space` (il sottomodulo, se la issue lo dice)
- `core/include/tg/core/join.hpp`, `core/src/join.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/join_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R22.txt` (da cancellare)

## Cosa fare

```cpp
/// The result of JOIN (docs/adr/0035, points 17-20). `source` is the source
/// entity as it becomes, when its type does not change; otherwise empty, and
/// `created` holds the new entity. `consumed` lists the indexes, into the
/// input, of the entities the result absorbs (the source excluded).
struct JoinResult {
    std::optional<Entity> source;
    std::optional<Entity> created;
    std::vector<std::size_t> consumed;
    /// Arcs of the source's circle that reach the whole circle (docs/adr/0035,
    /// point 17): `created` is the circle, which the command applies only if
    /// the user answers Yes to "Convert to circle?"; with No nothing joins.
    /// Never set with `close` of joinToSource, which asks nothing.
    bool formsCircle{false};
};

/// JOIN of `entities`, entities[0] the source, the others in the order they
/// were chosen: aligned lines into a line, when the source and every other
/// entity are lines aligned with the source; arcs of one circle into an arc
/// (or, reaching the whole circle, into a circle: `formsCircle`), when the
/// source and every other entity are arcs on the source's circle; otherwise a
/// chain of contiguous lines, arcs and polylines into a polyline.
/// std::nullopt when nothing joins.
[[nodiscard]] std::optional<JoinResult> join(std::span<const Entity> entities,
                                             const ToleranceRule& rule);

/// The script sequence (docs/adr/0035, point 19): only what the source's type
/// accepts (a line: aligned lines; an arc: arcs of its circle; a polyline:
/// contiguous lines, arcs and polylines). `close` (arcs only) closes the arc
/// into a circle.
[[nodiscard]] std::optional<JoinResult> joinToSource(std::span<const Entity> entities,
                                                     const ToleranceRule& rule,
                                                     bool close = false);
```

- **La regola** è quella del documento (ADR 0036): "allineate" è `areAligned`
  delle due linee, "stesso cerchio" è `onSameCircle` dei due archi, "si
  toccano" è `coincident` dei due estremi. Ogni linea e ogni arco si confronta
  con la sorgente, non con l'ultima entità aggiunta (ADR 0035, punto 18).
- **Gli estremi degli archi** (B13), nella catena, che confronta gli estremi
  di archi e linee (gli archi di un solo cerchio non li confrontano, punto
  seguente): la coincidenza si decide sugli estremi che dà `endpoints()` di
  `curves.hpp` di solid-space, sulla curva della traduzione di P14
  (`bridge::curveOf`), come chiede il suo commento: non su un coseno e un seno
  di Tecnigrafo (decisione del responsabile, 2026-11-03). Gli estremi di linee
  e polilinee sono i loro dati. L'esito è libero dove due scelte lecite degli
  estremi costruiti, ciascuno entro il suo errore (`kRelativePointError` di
  `curves.hpp`, sulla sua grandezza), danno coincidenze diverse; fra due archi
  contano gli errori di tutti e due gli estremi, fra un arco e una linea solo
  quello dell'arco. È una fascia, come la definisce l'[ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md)
  (punto 4) e il punto 10 di [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
  e i riferimenti R22 e R23 non mettono casi in quella fascia. Nessun obbligo
  di estremi arrotondati correttamente e nessuna richiesta a solid-space.
- **Una diagnosi di `endpoints()`** (dati che solid-space rifiuta, per
  esempio oltre `kMaxCoordinate`): le curve che solid-space rifiuta non si
  uniscono (ADR 0035, punto 23). Vale lo stesso per un **estremo restituito
  oltre `kMaxCoordinate`**: `endpoints()` lo può dare per dati validi
  (`curves.hpp`, il dominio controlla i dati, non l'estensione della curva),
  e `coincident` non lo accetta (`tolerance.hpp`). Il controllo si fa su tutti
  e due gli estremi, per coordinate positive e negative, prima di ogni
  chiamata a `coincident` e in ogni verso di crescita della catena (decisione
  del responsabile su R480-1, 2026-11-03). In `join` e in `joinToSource`:
  - un'entità diversa dalla sorgente con una diagnosi non si unisce e non
    entra in `consumed`: conta tra le scartate del messaggio (P23);
  - con una diagnosi sulla sorgente non si unisce niente: il risultato è
    `std::nullopt`, e tutte le altre entità sono scartate.

  Il controllo guarda l'estremo **restituito**, che sta entro il suo errore
  (`kRelativePointError` di `curves.hpp`, sulla sua grandezza) e può superare
  `kMaxCoordinate` anche quando quello esatto non lo supera, o viceversa.
  Quando una scelta lecita lo porta oltre `kMaxCoordinate` e un'altra no, cioè
  quando una coordinata esatta dista da ±`kMaxCoordinate` al più quell'errore,
  accettare o scartare l'entità è libero: è una fascia (ADR 0013 di
  solid-space, punto 4), come quella delle coincidenze sopra, e nessun
  riferimento la impone. Fuori dalla fascia l'esito è determinato.
- **Quale forma** (B16): `join` fa una linea solo se la sorgente è una linea e
  ogni altra entità è una linea allineata alla sorgente, e un arco solo se la
  sorgente è un arco e ogni altra entità è un arco sul suo cerchio. In ogni
  altro caso vale la catena (ADR 0035, punto 18): le entità che non vi
  entrano non sono in `consumed` e contano tra le scartate del messaggio
  (P23). Per esempio, sorgente (0,0)→(10,0), poi (20,0)→(30,0), poi
  (10,0)→(10,10): la polilinea (0,0)→(10,0)→(10,10), e la linea staccata si
  scarta. `joinToSource` non cambia: accetta già solo i tipi che la sorgente
  ammette.
- **Linee allineate:** la linea unita va dall'estremo della prima all'estremo
  dell'ultima lungo la sorgente, anche con spazi (`autocad-unione.dxf`, casi b
  ed e1). I due estremi della linea unita, pareggi compresi, li dà
  solid-space, con `joinAligned` (sopra): la
  facciata non proietta e non confronta da sé (ADR 0035, punto 1). Per
  esempio, in millimetri, sorgente
  (0,0)→(10,0) e linea (0,5e-7)→(5,0): quale fra (0,0) e (0,5e-7) è il primo
  estremo lo dice solid-space.
- **Archi dello stesso cerchio** (B11, rivista il 2026-11-02 come AutoCAD:
  `autocad-prove-a-mano.dxf`, punto 5), in `join` e in `joinToSource`: ogni
  arco si copre in senso antiorario a partire dall'**inizio della sorgente**.
  L'ordine in cui gli archi sono scelti non conta. Sia σ l'angolo iniziale
  memorizzato della sorgente:
  - ogni arco, la sorgente compresa, va da `a` fino ad `a` + la sua ampiezza.
    `a` è (il suo angolo iniziale − σ) portato in [0, 2π) con `normalizeAngle`
    (0 per la sorgente); l'ampiezza è quella del punto 1 dell'ADR 0035
    (`endAngle − startAngle`, più `kFullTurn` se non è positiva). Si calcola in
    `double` in quest'ordine, sugli angoli memorizzati, che sono dati e non
    costruzioni, senza tolleranza;
  - il **più lontano** è il maggiore di questi estremi;
  - prima di tutto, almeno un altro arco deve unirsi: se `entities` ha la sola
    sorgente, o nessun altro arco del suo cerchio, il risultato è
    `std::nullopt`, anche quando le regole seguenti darebbero il cerchio sulla
    sola sorgente; *cLose* di `joinToSource` resta l'eccezione (decisione
    B476-1 sulla #476, 2026-11-02);
  - se il più lontano è almeno `kFullTurn`, **oppure** se
    `normalizeAngle(σ + il più lontano)`, calcolato in `double`, risulta
    uguale a σ (un arco con angoli uguali, che `Arc` non ammette: decisione sul
    punto 4 della #373, 2026-11-02), gli archi formano un cerchio:
    `created` è il cerchio con centro e raggio della sorgente, `formsCircle` è
    vero e tutti gli altri archi sono in `consumed`. Se convertire lo decide il
    comando (P23): con *No* niente si unisce e il disegno non cambia;
  - altrimenti il risultato è l'arco della sorgente (`source`) che va da σ a σ +
    il più lontano, con l'angolo finale portato in [0, 2π) con `normalizeAngle`;
    tutti gli altri archi sono in `consumed`, perché stanno in quello che
    risulta;
  - con `close` di `joinToSource` il risultato è sempre il cerchio e
    `formsCircle` resta falso: *cLose* non chiede niente, e resta com'è.

  Il vuoto più grande e il pareggio fra vuoti, di B11 prima del 2026-11-02,
  decadono. Per esempio, sullo stesso cerchio e con sorgente 0°→90°:
  - con l'arco 200°→300°: l'arco 0°→300° (prima 200°→90°);
  - con l'arco 300°→30°: il cerchio, perché 300° + 90° arriva oltre il giro
    intero (prima 300°→90°);
  - con l'arco 270°→45°, il caso osservato in AutoCAD: il cerchio;
  - con l'arco 90°→180°: l'arco 0°→180°;
  - sorgente con centro (0,0), raggio 10, angolo iniziale 6 rad e angolo
    finale il `double` precedente a 6 (5.999999999999999), con l'arco da 1 a 2
    rad: l'ampiezza della sorgente, 6.283185307179585, è il più lontano e sta
    sotto `kFullTurn`, ma `normalizeAngle(6 + 6.283185307179585)` dà 6, cioè
    σ: il cerchio di centro (0,0) e raggio 10, `formsCircle` vero, l'altro
    arco in `consumed`. La stessa sorgente da sola, senza l'arco da 1 a 2:
    `std::nullopt` in `join` e in `joinToSource` senza `close`, e il cerchio
    con `close` (B476-1).
- **Catena** (ADR 0035, punto 18), per tutto il resto: parte dalla sorgente,
  cresce prima dalla sua fine e poi dal suo inizio; a ogni passo si aggiunge la
  prima entità, nell'ordine di `entities`, con un estremo coincidente con
  l'estremo libero della catena, girata se serve; ci si ferma quando nessuna si
  aggiunge. Due estremi che coincidono diventano un vertice, nel punto
  dell'entità che segue. In `join` e in `joinToSource`:
  - **verso** (B10, precisata il 2026-11-02 con il punto 1 della #373):
    l'estremo della candidata che coincide con l'estremo libero della catena
    diventa il vertice comune; quando coincidono tutti e due vince il suo
    **inizio**. La candidata si orienta di conseguenza, da tutti e due i lati:
    - quando la catena cresce **dalla fine**, il vertice comune deve essere il
      primo punto della candidata: si aggiunge com'è se il vertice comune è il
      suo inizio, girata se è la sua fine;
    - quando la catena cresce **dall'inizio**, il vertice comune deve essere
      l'ultimo punto della candidata: si aggiunge girata se il vertice comune è
      il suo inizio, com'è se è la sua fine.

    Per esempio, dalla fine: la candidata [(2e-7,0),(10,10),(−2e-7,0)] si
    aggiunge in quest'ordine dopo la sorgente [(0,−10),(0,0)]. Dall'inizio, in
    millimetri: sorgente linea (0,0)→(10,0) e candidata polilinea aperta
    [(2e-7,0),(−10,10),(−2e-7,0)]; dalla fine (10,0) non si aggiunge niente,
    dall'inizio (0,0) coincidono tutti e due i suoi estremi, vince l'inizio e
    la candidata si gira: `new LWPOLYLINE closed=no vertex=-2e-7,0
    vertex=-10,10 vertex=0,0 vertex=10,0`. Girata, anche le larghezze dei suoi
    segmenti si scambiano e i bulge cambiano segno;
  - **polilinee chiuse** (punto 2 della #373, 2026-11-02): una polilinea
    chiusa che non è la sorgente non ha estremi liberi, quindi non entra mai
    nella catena: non è in `consumed` e conta tra le scartate del messaggio
    (P23). Per esempio, sorgente linea (0,0)→(10,0) e polilinea chiusa
    [(10,0),(20,0),(20,10)]: niente si unisce (`std::nullopt`; la domanda
    `join` dà `none`). Una sorgente chiusa non cresce (punto seguente);
  - **chiusura** (B17): quando l'estremo libero della catena coincide con il
    suo primo estremo, la catena è chiusa e la crescita si ferma da tutti e
    due i lati; le entità non ancora aggiunte non si consumano. Per esempio
    (0,0)→(10,0), (10,0)→(10,10), (10,10)→(0,0) e (0,0)→(−10,0): resta il
    triangolo chiuso, e l'ultima si scarta.

  La polilinea risultante:
  - è chiusa se l'ultimo estremo coincide con il primo;
  - ha segmenti dritti dove `isStraight` dice che il bulge vale zero. Si
    chiede a `isStraight` di **ogni segmento com'è nella sua entità**
    (2026-11-04, buco della #373 trovato da Codex): con gli estremi e il bulge
    con cui entra nella catena, girato se l'entità si gira, **prima** che un suo
    estremo si sposti sul vertice comune, anche alla chiusura. Vale per la
    sorgente e per le candidate, quando la catena cresce dalla fine e
    dall'inizio, in `join` e in `joinToSource`, in millimetri e in metri.
    Girare un segmento cambia il segno del bulge, non la freccia, quindi non
    cambia l'esito. Il segmento prende 0 o il suo bulge secondo quell'esito,
    anche se la corda che risulta, con il vertice comune, è diversa. Per
    esempio, in millimetri (epsilon 1e-6):
    - **dalla fine:** sorgente la polilinea aperta [(−10,0),(0,0),(1e-7,0)]
      con bulge 10 sul secondo vertice, candidata la linea
      (1.05e-6,0)→(1.05e-6,10). Gli estremi (1e-7,0) e (1.05e-6,0) distano
      9.5e-7 e coincidono; il vertice comune è (1.05e-6,0), il punto della
      linea. Il segmento d'ingresso (0,0)→(1e-7,0) ha freccia
      10 · 1e-7 / 2 = 5e-7, quindi è dritto, e la sorgente diventa
      `LWPOLYLINE closed=no vertex=-10,0 vertex=0,0 vertex=0.00000105,0
      vertex=0.00000105,10`. Sulla corda che risulta, (0,0)→(1.05e-6,0), la
      freccia sarebbe 5.25e-6: non conta;
    - **dall'inizio:** sorgente la linea (1.05e-6,0)→(1.05e-6,10), candidata
      la stessa polilinea, che si aggiunge com'è, oppure la polilinea
      [(1e-7,0),(0,0),(−10,0)] con bulge −10 sul primo vertice, che si
      aggiunge girata: in tutti e due i casi `new LWPOLYLINE closed=no
      vertex=-10,0 vertex=0,0 vertex=0.00000105,0 vertex=0.00000105,10`;
    - **in metri** (epsilon 1e-9): con le coordinate divise per 1000 e lo
      stesso bulge 10 il risultato è lo stesso, con le coordinate divise per
      1000 (freccia d'ingresso 5e-10, distanza 9.5e-10); i dati in millimetri
      danno `none`, perché 9.5e-7 > 1e-9;
  - **larghezze** (B12): ogni segmento conserva la sua larghezza effettiva,
    quella dei vertici se è impostata, altrimenti la larghezza costante della
    sua polilinea; linee e archi hanno larghezza 0. Queste larghezze si
    scrivono nei vertici. La larghezza costante è quella comune se tutti i
    segmenti hanno la stessa larghezza costante, altrimenti 0;
  - **generazione del tipo di linea** (`continuousLinetype`) e proprietà
    dell'entità: quelle della sorgente (ADR 0031, punto 7); se la sorgente
    non è una polilinea, la generazione è spenta. Per esempio, sorgente
    (0,0)→(10,0) con larghezza costante 2 e generazione spenta, e
    (10,0)→(10,10) con larghezza costante 5 e generazione accesa: larghezza 2
    sul primo segmento e 5 sul secondo, scritte nei vertici, larghezza
    costante 0 e generazione spenta.
- La domanda `join` del catalogo (la sequenza interattiva), con le risposte
  scritte lì.

## Regole della tappa che valgono qui

- Il sottomodulo lo sposti solo al commit che indica la issue (ADR 0030, punto
  9).
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P17, P19 e P21).
- Nessuna soglia propria: coincidenze, allineamenti e stesso cerchio vengono da
  solid-space con la regola del documento.
- Le prove a mano dei punti 5 e 6 di [dxf-di-prova.md](dxf-di-prova.md) ("Che
  cosa resta da controllare") sono fatte (2026-11-02): la 6 ha confermato B16;
  la 5 ha smentito B11, che è la regola di adesso, e la 7 l'ha confermata sui
  due casi che la distinguono dalla vecchia (0°→90° con 200°→300° dà 0°→300°;
  con 300°→30° dà il cerchio con la domanda). Non c'è niente da controllare
  nella issue.

## Test richiesti

- I casi di `autocad-unione.dxf`.
- Polilinee aperte unite tra loro e con linee in testa e in coda; una catena che
  si chiude; entità in ordine sparso; archi che coprono il cerchio intero;
  estremi a `1e-10` in millimetri (coincidono) e in un disegno in metri;
  tre estremi dove a ~ b e b ~ c ma non a ~ c (l'ordine scritto decide).
- Gli esempi di B10, B12, B16 e B17, in `join` e, dove si applicano, in
  `joinToSource`.
- B13: l'arco di centro (0,0), raggio 10, da 0 a `kQuarterTurn` e la linea da
  (0,10) a (−10,10) si uniscono in una polilinea, con la fine dell'arco presa
  da `endpoints()` e il vertice comune nel punto della linea (0,10).
- Diagnosi di `endpoints()` (ADR 0035, punto 23), con dati oltre
  `kMaxCoordinate`, perché un arco degenere Tecnigrafo non lo crea:
  - sorgente la linea (0,0)→(10,0), poi l'arco di centro (2^51,0) e raggio 1,
    poi la linea (10,0)→(10,10): la polilinea (0,0)→(10,0)→(10,10), con
    l'arco non in `consumed` e scartato;
  - sorgente l'arco di centro (2^51,0) e raggio 1, poi la linea (0,0)→(10,0):
    `std::nullopt` (la domanda `join` dà `none`), con la linea scartata.
- Estremo restituito oltre `kMaxCoordinate` (R480-1), con K = 2^50: sorgente
  la linea (K,K−1)→(K,K), poi l'arco di centro (K,0), raggio K, da 0 a
  `kQuarterTurn`, che ha dati validi ed estremi circa (2K,0) e (K,K): l'arco
  si scarta anche se il suo estremo (K,K) tocca la linea, e il risultato è
  `std::nullopt`. Con l'arco come sorgente e la linea come candidata:
  `std::nullopt`, con la linea scartata. Gli stessi due casi con le
  coordinate negative, la linea (−K,1−K)→(−K,−K) e l'arco di centro (−K,0),
  raggio K, da π a `kThreeQuarterTurn` (estremi circa (−2K,0) e (−K,−K)),
  danno lo stesso esito: un estremo resta vicino a ±2K, ben oltre il
  dominio per ogni arrotondamento lecito, quindi fuori dalla fascia.
- Gli esempi delle decisioni sulla #373: il verso quando la catena cresce
  dall'inizio (candidata girata, anche con larghezze diverse e un bulge, per
  controllare lo scambio e il segno); la polilinea chiusa scartata; l'arco
  con angoli uguali che dà il cerchio, in `join` e in `joinToSource`, e la
  stessa sorgente da sola, che dà `std::nullopt` (con `close` il cerchio);
  l'ordine
  degli estremi delle linee allineate con due proiezioni uguali, secondo la
  decisione in solid-space.
- Il segmento d'ingresso per `isStraight` (2026-11-04): gli esempi sopra, dalla
  fine e dall'inizio (con la candidata com'è e girata), in millimetri e in
  metri, in `join` e in `joinToSource`; e lo stesso dalla fine con un bulge
  curvo già in ingresso (per esempio 100: freccia 5e-6), che resta 100.
- Gli archi (B11 rivista): i quattro esempi sopra, in `join` e in
  `joinToSource`; un arco che arriva al giro intero partendo prima dell'inizio
  della sorgente; archi con spazi in mezzo e in ordine sparso (stesso
  risultato); un arco contenuto; `close` (il cerchio, `formsCircle` falso);
  senza `close` e con il cerchio, `formsCircle` vero e `created` il cerchio.

## Fatto quando

- I casi di `[0.7-R22]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
