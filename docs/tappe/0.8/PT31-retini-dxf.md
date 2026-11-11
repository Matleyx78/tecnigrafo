# [0.8-PT31] `io`: `HATCH` nel DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT31]` (#…), agente claude
- **Dipende da:** PT30, RT31
- **Modulo:** `io`
- **Da leggere:** [ADR 0048](../../adr/0048-retini.md), punto 1;
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md), punti 3–5;
  `io/src/dxf_lwpolyline.cpp`; la scheda [PT30](PT30-retini-core.md)

## File

- `io/src/dxf_hatch.cpp` (nuovo), `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp` (file comuni)
- `tests/io/dxf_hatch_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. Codec `HATCH` con i gruppi dell'ADR 0048, punto 1, **nell'ordine del DXF**,
   che è rigido: 10/20/30 (punto di quota), 210/220/230, 2, 70, 71, 91 e i
   contorni (92; per la polilinea 72, 73, 93, vertici 10/20 e 42; per i lati
   93, e per ogni lato 72 con 10/20/11/21 o 10/20/40/50/51/73; dopo ogni
   contorno 97 e gli handle 330 in `extras` del contorno), 75, 76, 52, 41, 77,
   78 e le righe (53, 43, 44, 45, 46, 79, 49…), 47, 98 e i punti interni, e il
   resto in coda (`extras`).
2. **Grezzi** (ADR 0048, punto 1): lati d'ellisse (72 = 3) o spline (4), sfumatura
   (450 diverso da 0), contorni non chiusi (PT30, `supported`), estrusione
   diversa da `(0,0,1)` e `(0,0,-1)`.
3. Un `HATCH` letto e scritto senza modifiche resta identico (ADR 0018, punto 6).

## Regole della tappa che valgono qui

- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `dxf_hatch_test.cpp`: giro di un `SOLID` con una polilinea e un cerchio
  (lato d'arco intero), di un `ANSI31` con contorno a lati e due punti interni;
  un retino con un lato d'ellisse resta grezzo; estrusione `(0,0,-1)`.

## Fatto quando

- I casi di RT31 passano, compresi quelli di `autocad-retini.dxf`.
- La CI è verde.
