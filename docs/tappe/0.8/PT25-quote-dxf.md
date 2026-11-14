# [0.8-PT25] `io`: quote nel DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT25]` (#…), agente codex
- **Dipende da:** PT18, PT23, RT25
- **Modulo:** `io`
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punti 1,
  4 e 6; [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punti 4 e 5; `io/src/dxf_blocks.cpp` (PT18); le schede
  [PT22](PT22-stili-di-quota.md) (sovrascritture) e [PT23](PT23-quote-lineari.md)

## File

- `io/src/dxf_dimension.cpp` (nuovo), `io/src/dxf_blocks.cpp` (i blocchi `*D…`)
- `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`, `io/src/dxf_entity_common.hpp`
  (file comuni)
- `tests/io/dxf_dimension_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Lettura.** Codec `DIMENSION`: 2 (blocco), 3 (stile), 10/20, 11/21, 13/23,
   14/24, 15/25, 16/26, 40, 42, 50, 53, 70, 1, le sottoclassi
   (`AcDbAlignedDimension`, `AcDbRotatedDimension`, `AcDb2LineAngularDimension`,
   `AcDbRadialDimension`, `AcDbDiametricDimension`) per il tipo; il resto
   (`DSTYLE`, reattori, 71, 72, 41, 51…) in `extras`. Un tipo diverso dai cinque
   della 0.8 è *altro* (ADR 0047, punto 1). La grafica viene dal blocco nominato
   in 2: le sue entità entrano in `graphics`/`rawGraphics`, con i loro handle, e
   il blocco esce dalla sezione `BLOCKS` del modello (resta associato alla
   quota, con il suo `BLOCK_RECORD`). Un blocco che manca: la quota resta grezza.
2. **Scrittura.** Ogni quota scrive il suo blocco anonimo: con il nome letto se
   ce l'ha, altrimenti `*D<n>` con `n` il primo numero libero; `BLOCK_RECORD`,
   `BLOCK` ed `ENDBLK` con handle nuovi se la quota è nuova; le entità della
   grafica nel blocco, con handle nuovi quelle con handle 0. Il gruppo 70 ha il
   bit 32 (blocco usato solo da questa quota) e il 128 se `userTextPosition`.
3. Una quota letta e scritta senza modifiche resta identica (ADR 0018, punto 6),
   blocco compreso.

## Regole della tappa che valgono qui

- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `dxf_dimension_test.cpp`: giro di ognuno dei cinque tipi e di una quota
  coordinata (*altro*); una quota nuova riceve il suo `*D` e i suoi handle; una
  quota senza blocco resta grezza; sovrascritture conservate.

## Fatto quando

- I casi di RT25 passano, compresi quelli di `autocad-quote.dxf`.
- La CI è verde.
