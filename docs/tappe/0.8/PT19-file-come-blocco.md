# [0.8-PT19] `io`: un file DXF come definizione di blocco

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT19]` (#…), agente codex
- **Dipende da:** PT18, PT22, RT19
- **Modulo:** `io`
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punto 7;
  [ADR 0023](../../adr/0023-layer-e-proprieta.md), punto 1;
  `io/include/tg/io/io.hpp`, `io/src/dxf_read.cpp`, `io/src/dxf_blocks.cpp`
  (PT18); le schede [PT11](PT11-stili-di-testo.md), [PT16](PT16-blocchi-core.md),
  [PT22](PT22-stili-di-quota.md)

## File

- `io/include/tg/io/block_import.hpp`, `io/src/block_import.cpp` (nuovi)
- `io/CMakeLists.txt` (file comune)
- `tests/io/block_import_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

```cpp
/// What inserting `source` into `target` as a block brings (docs/adr/0045,
/// point 7). Nothing changes in `target`: the command applies it.
struct BlockImport {
    core::BlockDefinition definition;               // name of the file, base (0,0) or $INSBASE
    std::vector<core::BlockDefinition> nested;      // the file's blocks that the drawing lacks
    std::vector<core::Layer> layers; std::vector<core::Linetype> linetypes;
    std::vector<core::TextStyle> textStyles; std::vector<core::DimStyle> dimStyles;
    double unitFactor{1.0};                         // INSUNITS conversion, else 1
    std::uint64_t nextId{0};                        // target's handle counter after it
};
[[nodiscard]] io::Result<BlockImport> importBlock(const std::filesystem::path& source,
                                                  const core::Document& target,
                                                  io::FileAccess& files);
```

1. Legge il file con il lettore di sempre (stessi limiti, stessi messaggi
   d'errore).
2. La definizione ha come nome il nome del file senza estensione (come
   l'utente lo ha scritto, maiuscole comprese), come punto base **(0,0)**, le
   entità dello spazio modello del file nel loro ordine **spostate di
   −`$INSBASE`** (nessuno spostamento se manca), come le scrive AutoCAD
   (decisione del 2026-11-13, `autocad-inserisci-file.dxf`), e unità =
   `$INSUNITS` del file; lo spazio carta non entra. Se `$INSBASE` non è nullo
   e lo **spazio modello** del file ha un'entità che non si sa spostare
   (un'entità non gestita, conservata com'è, ADR 0006), la definizione tiene
   come punto base
   `$INSBASE` e **tutte** le entità intatte: il disegno è lo stesso e niente si
   perde, solo i dati differiscono da quelli di AutoCAD (decisione del
   2026-11-13, «Buchi della revisione della #756»). Contano solo le entità
   dello spazio modello, le sole da spostare: lo spazio carta non entra, e un
   blocco annidato tiene le sue coordinate (si sposta il suo inserimento).
3. **Tabelle** (ADR 0045, punto 7): entrano solo i record il cui nome il
   disegno non ha (`sameTableName`); a parità di nome vince quello del disegno,
   e le entità che lo usavano usano quello del disegno. Lo stesso per le
   definizioni dei blocchi del file (in `nested`).
4. **Handle.** Ogni handle del file (entità, record, blocchi, attributi) si
   rinumera da `target.nextId()` in su, nell'ordine del file; i gruppi che
   nominano handle (330, 340, 360, i 102 dei reattori) si aggiornano se
   l'handle è stato rinumerato, e si lasciano com'erano altrimenti.
5. **Unità:** se file e disegno hanno `$INSUNITS` diverso da zero e diversi,
   `unitFactor` è il rapporto delle due unità; altrimenti 1.
6. Un file che contiene un inserimento di un blocco con il proprio nome (che
   farebbe un ciclo) si rifiuta con `Block "<nome>" references itself.`

## Regole della tappa che valgono qui

- Il lettore non cambia: `importBlock` lo usa.
- File comuni: `io/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `block_import_test.cpp`: un file con `$INSBASE` (100, 120): base (0,0) ed
  entità spostate di (−100, −120), attributi compresi; lo stesso file con
  una `SPLINE`: base (100, 120), entità e gruppi della `SPLINE` intatti; con
  `$INSBASE` (0, 0) e la `SPLINE`: base (0,0), gruppi intatti; la `SPLINE` solo
  nello spazio carta, e poi solo in un blocco annidato: base (0,0) ed entità
  dello spazio modello spostate (la definizione annidata intatta); un layer e un tipo di linea
  che il disegno ha con altre proprietà (vince il disegno) e uno che non ha
  (entra); handle rinumerati e proprietari aggiornati; unità 4 nel file e 6 nel
  disegno (fattore 0,001); un file che si inserisce in sé stesso.

## Fatto quando

- I casi di RT19 passano; la CI è verde.
