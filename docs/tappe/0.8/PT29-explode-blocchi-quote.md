# [0.8-PT29] `core` e `cmd`: `EXPLODE` di inserimenti e quote

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT29]` (#…), agente codex
- **Dipende da:** PT4 (revisione delle interfacce), PT17, PT23, PT48, RT29
- **Modulo:** `core` (più il comando in `cmd`)
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punto 4;
  [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punto 4;
  [ADR 0031](../../adr/0031-modifiche-alle-entita.md), punti 6 e 8;
  `cmd/src/commands/explode_command.cpp`; le schede
  [PT16](PT16-blocchi-core.md), [PT17](PT17-attributi-core.md),
  [PT23](PT23-quote-lineari.md)

## File

- `core/include/tg/core/explode.hpp`, `core/src/explode.cpp` (nuovi)
- `cmd/src/commands/explode_command.cpp`, `cmd/include/tg/cmd/commands/explode_command.hpp`
- `tests/core/explode_test.cpp` (nuovo), `tests/cmd/explode_command_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. In `explode.hpp`:

   ```cpp
   struct Exploded {
       std::vector<EntityRecord> entities;  // ids 0; properties of their own
       std::vector<RawEntity> raw;          // unhandled entities of the block, transformed
       std::string refusal;                 // not empty: nothing explodes
   };
   [[nodiscard]] Exploded explodeInsert(const Document&, const Insert&);
   [[nodiscard]] Exploded explodeDimension(const Document&, const Dimension&);
   ```

   - **Inserimento** (ADR 0045, punto 4): le entità della definizione
     trasformate, ognuna con le **sue** proprietà ed `extras` senza i gruppi
     102: con una similitudine `transformed`; con scala non uniforme
     `transformedShape` (PT6), così cerchi e archi diventano `Ellipse` e i
     testi si deformano, come AutoCAD (ADR 0052, punto 2); i pezzi ellittici
     di una polilinea con larghezza escono senza larghezza, come EXPLODE di
     AutoCAD (PT4, punto P8). Con una scala non uniforme, un `MTEXT`, una
     quota o un retino nel blocco rifiutano l'esplosione con
     `Block "<nome>" contains objects Tecnigrafo cannot explode.`; un `ATTDEF`
     torna `ATTDEF`, trasformato come un testo (`transformedText`, PT17)
     (PT4, punto P25: [DECISIONI.md](../../DECISIONI.md), 2026-11-14,
     «Revisione delle interfacce PT4»). Entità e grezzi tornano
     nell'ordine della definizione. Gli inserimenti
     annidati restano inserimenti con la trasformazione composta, se è ancora
     scala più rotazione (le colonne della parte lineare sono ortogonali);
     altrimenti l'esplosione si rifiuta con
     `Block "<nome>" contains a nested block that cannot stay a block reference.`
     Al posto degli `ATTRIB`, gli `ATTDEF` della definizione trasformati (anche
     i costanti). Rifiuto anche per un blocco non esplodibile:
     `Block "<nome>" cannot be exploded.` (o il messaggio di AutoCAD). Le
     entità grezze del blocco: quelle senza coordinate da trasformare si
     copiano; le altre rifiutano l'esplosione con
     `Block "<nome>" contains objects Tecnigrafo cannot explode.`
   - **Quota:** le entità della grafica come sono (sono già nel disegno), con le
     loro proprietà risolte come in un blocco: layer `0` → layer della quota,
     *DaBlocco* → della quota (come AutoCAD). Se la grafica ha entità grezze
     (non gestite), l'esplosione si rifiuta con
     `Dimension contains objects Tecnigrafo cannot explode.` e la quota resta
     (PT4, punto P24).
   - Gli `ATTRIB` di un inserimento figlio che resta inserimento si
     trasformano con lui e prendono handle nuovi (PT17, punto 7).
2. **`EXPLODE`** usa queste funzioni per `INSERT` e `DIMENSION`, oltre alle
   polilinee di oggi: le entità nuove prendono handle nuovi e vanno in fondo
   all'ordine (ADR 0031, punto 6); un rifiuto si scrive nella cronologia e
   quell'entità resta; un solo passo di annullamento. `HATCH`, `MTEXT`,
   `SOLID` e `ATTDEF` non si esplodono nella 0.8: il messaggio è quello di oggi
   per le entità che non si esplodono.

## Regole della tappa che valgono qui

- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `explode_test.cpp`: un inserimento ruotato e scalato 2; un blocco annidato
  (resta un inserimento); un blocco con attributi (tornano gli `ATTDEF`); scala
  non uniforme `(2, 1)` con un cerchio e un arco (diventano ellissi) e con un
  blocco annidato ruotato di 30° (rifiutato); non esplodibile rifiutato; una
  quota lineare; una quota con un `POINT` grezzo nella grafica (rifiutata); un
  blocco con un `MTEXT` sotto scala `(2, 1)` (rifiutato) e un `ATTDEF` sotto
  la stessa scala (torna `ATTDEF`); un blocco con `Line A`, un grezzo e
  `Line B` (stesso ordine dopo).
- `explode_command_test.cpp`: selezione mista di una polilinea, un inserimento
  e una quota; `U`.

## Fatto quando

- I casi di RT29 passano; la CI è verde.
