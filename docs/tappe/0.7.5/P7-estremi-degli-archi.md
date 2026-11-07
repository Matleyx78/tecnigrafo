# [0.7.5-P7] `core`: estremi degli archi da solid-space, commento dello snap alle intersezioni

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: il comportamento non cambia oltre gli errori
  dichiarati da solid-space; i casi di verifica esistenti, invariati, sono la
  prova
- **Dipende da:** —
- **Modulo:** `core`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md),
  punto 1 (la traduzione verso solid-space);
  [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-07, «Proposte di metodo» (le
  proposte che sono codice); in solid-space, solo
  `geom2d/include/solid_space/geom2d/curves.hpp` (`endpoints()`, `Endpoints`,
  `kRelativePointError`) e `intersection.hpp` (che cosa promette sui punti);
  `core/src/solid_space_bridge.hpp`, `core/src/join.cpp` (`arcEnds`, che usa già
  `endpoints()`), `core/src/snap_points.cpp`, `core/src/grips.cpp`,
  `core/src/document_query.cpp`

## Perché

L'intestazione di solid-space dice che Tecnigrafo prende gli estremi di un arco
da `endpoints()`, non da coseno e seno propri, perché concordino con il
significato esatto dell'arco che usano intersezioni e unione (0.7-P22, B13).
Snap e grip fuori dagli assi li calcolano ancora con `std::cos` e `std::sin`
(`pointAt` in `core/src/arc_geometry.cpp`, `arcEndpoint` in `grips.cpp`), quindi
il punto agganciato all'estremo di un arco può differire di qualche ulp da
quello che `TRIM` o `JOIN` considerano l'estremo. Sugli assi resta invece
l'eccezione già fissata dalla [#422](https://github.com/Matleyx78/tecnigrafo/issues/422):
gli estremi sono esattamente `centro ± raggio`, anche per lo snap. Non devono
coincidere bit per bit con `endpoints()` sugli assi: questa correzione conserva
i casi esistenti e resta entro l'errore dichiarato dal kernel per i dati nel
dominio di quella garanzia.

## File

- `core/src/solid_space_bridge.hpp`, `core/src/solid_space_bridge.cpp` (la
  funzione interna nuova)
- `core/src/snap_points.cpp`, `core/src/grips.cpp`
- `core/src/document_query.cpp` (solo il commento)
- `tests/core/snap_points_test.cpp`, `tests/core/grips_test.cpp`

## Cosa fare

1. Una funzione **interna** di `core` (in `solid_space_bridge.*`, non nelle
   intestazioni pubbliche): gli estremi di un'entità `Arc` presi da
   `g::endpoints()` sulla curva di `bridge::curveOf()`, tradotti con
   `fromPoint()`. Se solid-space risponde con una diagnosi (arco degenere, fuori
   dominio), la funzione restituisce gli estremi di oggi (`startPoint` e
   `endPoint` della sua `ArcGeometry`): così nessun arco che oggi ha estremi resta
   senza. Sia con un risultato sia con una diagnosi, conserva gli estremi
   esatti sugli assi della #422: usa le stesse condizioni di `arcEndpoint`,
   `normalizeAngle(angle)` uguale a `0.0`, `kPi / 2.0`, `kPi` o
   `3.0 * kPi / 2.0`, sugli angoli `geometry.startAngle` e
   `geometry.startAngle + geometry.sweep`.
2. La usano lo snap agli estremi di un `Arc` (`snap_points.cpp`) e i grip degli
   estremi di un `Arc` (`grips.cpp`, al posto di `arcEndpoint` per quei due
   punti). Gli archi dei segmenti di polilinea non cambiano: i loro estremi sono
   i vertici.
3. Cerca in `core` e in `io` gli altri punti che calcolano l'estremo di un
   `Arc` con coseno e seno ed elencali nella pull request: quelli di `core` li
   cambi allo stesso modo se il loro risultato è un estremo; quelli fuori da
   `core` (per esempio `cmd/src/commands/trim_extend_commands.cpp`) li elenchi
   soltanto. Il disegno (`render`) non cambia.
4. Il commento di `core/src/document_query.cpp` dove lo snap ordina gli id e
   chiama `intersect` una coppia alla volta: oggi dice che l'ordine stabile
   «risolve» le intersezioni di più coppie nello stesso punto. Deve dire che
   l'ordine è solo uno spareggio deterministico: i punti di coppie diverse
   possono differire di un ulp, senza effetto visibile, perché lo snap sceglie
   comunque il più vicino al puntatore; `intersection.hpp` promette gli stessi
   bit scambiando gli argomenti, non tra coppie diverse.
5. Se la scheda 0.7-P15 o un ADR di Tecnigrafo chiede che i punti di coppie
   diverse siano identici, non scegliere tu: fermati e scrivilo nella issue.

## Regole della tappa che valgono qui

- Nessuna funzione nuova nelle intestazioni pubbliche di `core`: per questo il
  pacchetto non passa la revisione delle interfacce.
- Se un caso di verifica cambia risultato, non toccarlo: fermati e scrivilo
  nella issue (AGENTS.md, «Quando fermarsi»).

## Test richiesti

- Un arco con estremi non esatti in virgola mobile (per esempio da 10° a 110°):
  lo snap all'estremo e il grip dell'estremo coincidono, bit per bit, con
  `g::endpoints()`.
- Un arco degenere (raggio zero, se il documento lo ammette): snap e grip danno
  gli estremi di oggi.
- Ripiego con diagnosi sugli assi: un arco di raggio zero da 0 a π/2 dà il
  centro per entrambi gli estremi; un arco con centro `(0, 2^51)`, raggio 13 e
  gli stessi angoli è fuori dominio e dà esattamente `(13, 2^51)` e
  `(0, 2^51 + 13)`, per snap e grip, senza residui trigonometrici.

## Fatto quando

- Snap agli estremi e grip degli estremi di un `Arc` vengono da `endpoints()`,
  salvo il ripiego con diagnosi e gli estremi esatti sugli assi della #422.
- Il commento di `document_query.cpp` dice che l'ordine è uno spareggio.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
