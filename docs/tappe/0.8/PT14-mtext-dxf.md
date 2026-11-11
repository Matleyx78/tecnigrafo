# [0.8-PT14] `io`: `MTEXT` nel DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT14]` (#…), agente claude
- **Dipende da:** PT13, RT14
- **Modulo:** `io`
- **Da leggere:** [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  punto 2; [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md), punti
  3–5; [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto
  4; `io/src/dxf_text.cpp` come esempio; `core/include/tg/core/mtext.hpp`

## File

- `io/src/dxf_mtext.cpp` (nuovo), `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp` (file comuni: una riga ciascuno)
- `tests/io/dxf_mtext_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. Codec `MTEXT`: 10/20 inserimento, 40 altezza, 41 larghezza, 71
   allineamento, 72 direzione, 7 stile, 50 rotazione, 44 e 73 interlinea; il
   valore è l'unione dei gruppi 3 (nell'ordine) e del gruppo 1 finale.
2. **Direzione.** Se c'è il vettore 11/21 (direzione dell'asse X del testo), la
   rotazione è il suo angolo (`core::atan2`) e il vettore si conserva in `extras`
   per riscriverlo com'era quando la rotazione non cambia; se la rotazione
   cambia, si scrive il vettore nuovo `(cos r, sin r)` e non il 50.
3. **Scrittura.** Il valore si spezza in gruppi 3 da 250 caratteri (senza
   dividere una sequenza UTF-8) più il gruppo 1 finale, come fa AutoCAD.
4. **Non gestiti** (restano grezzi, ADR 0044, punto 4): colonne (gruppo 75 > 0
   o dati di colonna), direzione 72 diversa da 1 e 5, altezza o larghezza non
   valide (altezza ≤ 0, larghezza < 0), estrusione diversa da `(0,0,1)` e
   `(0,0,-1)`. Sfondo (90, 63, 421, 431, 45, 441) e il resto stanno in `extras`.

## Regole della tappa che valgono qui

- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `dxf_mtext_test.cpp`: giro di un `MTEXT` corto e di uno più lungo di 250
  caratteri con lettere accentate; vettore 11/21 conservato; un `MTEXT` con
  colonne resta non gestito.

## Fatto quando

- I casi di RT14 passano; i casi di conservazione esistenti sono invariati.
- La CI è verde.
