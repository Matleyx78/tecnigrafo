# [0.7-P15] `core`: snap a intersezione

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R15]` (#…), agente gemini (anche la revisione dei
  casi esistenti, ADR 0033 punto 6)
- **Dipende da:** P7, P14, R15
- **Modulo:** `core`
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 3, 5 e 6; `core/include/tg/core/intersections.hpp` (P14);
  `core/src/document_query.cpp` (`findSnap`, P7)

## File

- `core/src/document_query.cpp`, `core/include/tg/core/document_query.hpp` se
  serve
- `tests/core/document_query_test.cpp`
- `tests/verifica/in-attesa.txt` (togliere le righe di R15)

## Cosa fare

- In `findSnap`, con `modes.intersection`:
  - si prendono le entità visibili la cui curva passa entro l'apertura (indice
    spaziale, poi `distance`);
  - si calcola `intersect` per ogni coppia, e per ogni polilinea con se stessa;
  - i punti entro l'apertura sono candidati `Intersection`, con le priorità
    dell'ADR 0033, punto 5.
- I tratti condivisi non danno punti di intersezione: i loro estremi sono già
  estremi delle entità.
- Prestazioni: al più qualche decina di entità vicine al puntatore; il test di
  prestazioni esistente (`tests/bench`, se c'è un caso dello snap) non deve
  peggiorare in modo visibile.

## Regole della tappa che valgono qui

- `document_query.cpp`: prima di te P7, dopo di te P19.

## Test richiesti

- Due linee che si incrociano, linea e arco, due cerchi, polilinea che si
  interseca; incrocio fuori dall'apertura; tangenza.
- Priorità tra intersezione ed estremo alla stessa distanza (vince l'estremo).
- Layer spento: nessuna intersezione con le sue entità.

## Fatto quando

- I casi di `[0.7-R15]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`; i casi esistenti (rivisti da R15) passano.
- La CI è verde.
