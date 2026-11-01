# [0.7-P22] `core`: unione di entità contigue

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R22]` (#…), agente gemini: la facciata (catena,
  ordine, entità che risultano, unità); i confini della regola dall'archivio
  dei casi di solid-space
- **Dipende da:** P14, Matleyx78/solid-space#23, P25, R22. La richiesta diventa il
  pacchetto di solid-space che realizza `coincidence.hpp`, quando la tappa 0.2
  di solid-space lo avrà (ADR 0030, punto 13)
- **Corretta:** 2026-11-01, con le decisioni del responsabile sui buchi B10–B13,
  B16 e B17 della #464 ([DECISIONI.md](../../DECISIONI.md))
- **Modulo:** `core` (più la domanda `join` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2, 17–20; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punti 1, 2 e 5; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punto 7;
  l'[ADR 0009 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0009-regola-delle-tolleranze-del-piano.md)
  (Decisione); in `third_party/solid-space` l'intestazione qui sotto;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-unione.dxf`, e "Che cosa resta
  da controllare", punti 5 e 6);
  `core/include/tg/core/tolerance.hpp` e `core/src/solid_space_bridge.hpp` (P14)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0009 di solid-space:
`solid_space/geom2d/coincidence.hpp` (`areAligned`, `onSameCircle`,
`isStraight`) e `solid_space/robust/tolerance.hpp` (`coincident`), con i tipi
di `curves.hpp`.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude il pacchetto di solid-space che realizza
  `coincidence.hpp`. Se P14 l'ha già portato lì o oltre, il sottomodulo non si
  sposta. Se la issue non lo dice, chiedilo e non cominciare.
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
};

/// JOIN of `entities`, entities[0] the source, the others in the order they
/// were chosen: aligned lines into a line, when the source and every other
/// entity are lines aligned with the source; arcs of one circle into an arc,
/// when the source and every other entity are arcs on the source's circle;
/// otherwise a chain of contiguous lines, arcs and polylines into a polyline.
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
- **Gli estremi degli archi** (B13): la coincidenza si decide sugli estremi
  come li calcola Tecnigrafo, con la stessa funzione usata altrove per gli
  estremi di un arco. Quando la distanza esatta fra due estremi sta entro
  l'errore di quel calcolo dal confine della tolleranza, l'esito è libero
  (punto 10 di [CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md)),
  e i riferimenti R22 e R23 non mettono casi in quella fascia. Nessun obbligo
  di estremi arrotondati correttamente e nessuna richiesta a solid-space: è la
  soluzione di Matleyx78/solid-space#152.
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
  ed e1).
- **Archi dello stesso cerchio** (B11), in `join` e in `joinToSource`:
  - il risultato è tutto il cerchio **meno il vuoto più grande** fra gli archi
    uniti, con centro e raggio della sorgente. I vuoti si confrontano
    esattamente sugli **angoli memorizzati** degli archi, che sono dati e non
    costruzioni;
  - se non resta nessun vuoto, o se tutti i vuoti hanno gli estremi
    coincidenti per la regola (`coincident`, con gli estremi di B13), il
    risultato è un cerchio; con `close` di `joinToSource` è un cerchio
    comunque;
  - un arco contenuto in quello che risulta si consuma;
  - a pari ampiezza vince il primo vuoto in senso antiorario dopo la fine
    della sorgente.

  Per esempio, sorgente 0°→90° e candidato 270°→45° sullo stesso cerchio:
  l'arco 270°→90°.
- **Catena** (ADR 0035, punto 18), per tutto il resto: parte dalla sorgente,
  cresce prima dalla sua fine e poi dal suo inizio; a ogni passo si aggiunge la
  prima entità, nell'ordine di `entities`, con un estremo coincidente con
  l'estremo libero della catena, girata se serve; ci si ferma quando nessuna si
  aggiunge. Due estremi che coincidono diventano un vertice, nel punto
  dell'entità che segue. In `join` e in `joinToSource`:
  - **verso** (B10): quando tutti e due gli estremi di un'entità coincidono
    con l'estremo libero della catena vince il suo inizio, e l'entità si
    aggiunge com'è; si gira solo quando coincide soltanto la sua fine. Per
    esempio la candidata [(2e-7,0),(10,10),(−2e-7,0)] si aggiunge in
    quest'ordine dopo la sorgente [(0,−10),(0,0)];
  - **chiusura** (B17): quando l'estremo libero della catena coincide con il
    suo primo estremo, la catena è chiusa e la crescita si ferma da tutti e
    due i lati; le entità non ancora aggiunte non si consumano. Per esempio
    (0,0)→(10,0), (10,0)→(10,10), (10,10)→(0,0) e (0,0)→(−10,0): resta il
    triangolo chiuso, e l'ultima si scarta.

  La polilinea risultante:
  - è chiusa se l'ultimo estremo coincide con il primo;
  - ha segmenti dritti dove `isStraight` dice che il bulge vale zero;
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
- Prima di cominciare controlla nella issue l'esito delle prove a mano di
  `JOIN` su due archi dello stesso cerchio e su una scelta mista
  ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da controllare", punti
  5 e 6). Se AutoCAD si comporta diversamente, fermati: le decisioni B11 e B16
  si rivedono prima di questo pacchetto.

## Test richiesti

- I casi di `autocad-unione.dxf`.
- Polilinee aperte unite tra loro e con linee in testa e in coda; una catena che
  si chiude; entità in ordine sparso; archi che coprono il cerchio intero;
  estremi a `1e-10` in millimetri (coincidono) e in un disegno in metri;
  tre estremi dove a ~ b e b ~ c ma non a ~ c (l'ordine scritto decide).
- Gli esempi di B10, B11 (anche due vuoti di pari ampiezza e un arco
  contenuto), B12, B16 e B17, in `join` e, dove si applicano, in
  `joinToSource`.

## Fatto quando

- I casi di `[0.7-R22]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
