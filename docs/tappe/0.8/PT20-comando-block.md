# [0.8-PT20] `cmd`: `-BLOCK`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT20]` (#…), agente claude
- **Dipende da:** PT7, PT17, RT20
- **Modulo:** `cmd` (più le operazioni sui blocchi in `edit`)
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punto 5;
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  (richieste di oggetti); `cmd/src/commands/layer_commands.cpp` (come è fatto
  `-LAYER`); la scheda [PT7](PT7-tabelle-edit.md); la cronologia di `-BLOCK`
  copiata dal responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `edit/include/tg/edit/blocks.hpp`, `edit/src/blocks.cpp` (nuovi)
- `cmd/include/tg/cmd/commands/block_command.hpp`,
  `cmd/src/commands/block_command.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `edit/CMakeLists.txt`, `cmd/CMakeLists.txt`
  (file comuni)
- `tests/cmd/block_command_test.cpp`, `tests/edit/blocks_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `edit/blocks.*`: `defineBlock(name, base, ids)` toglie dal disegno le entità
   `ids` e le mette, con i loro handle, nella definizione nuova o ridefinita,
   in un solo cambiamento annullabile (`DocumentTables::replaceBlock` più le
   rimozioni); rifiuta una definizione che crea un ciclo (`wouldCycle`). Con
   `ATTDEF` fra le entità, la definizione ha `hasAttributes`.
2. **`-BLOCK`** (alias `BLOCK`), con le richieste di AutoCAD LT dalla cronologia
   del responsabile; quelle attese:
   - `Enter block name or [?]:` — `?` elenca i blocchi; un nome non valido dà il
     messaggio dei nomi delle tabelle;
   - se esiste: `Block "<nome>" already exists. Redefine it? [Yes/No] <N>:`;
     con *No* il comando finisce;
   - `Specify insertion base point:` (senza *Annotative*);
   - `Select objects:` (richiesta di oggetti dell'ADR 0032), `Invio` termina;
     senza oggetti: `No objects selected.` e il comando finisce senza blocco;
   - un blocco che conterrebbe sé stesso: `Block "<nome>" references itself.`
   - fine: gli oggetti spariscono dal disegno, come `-BLOCK` di AutoCAD.
   Un passo di annullamento: `Block`.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `edit/CMakeLists.txt`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `blocks_test.cpp`: definire e annullare; ridefinire un blocco con un
  inserimento (che cambia ingombro); un ciclo rifiutato.
- `block_command_test.cpp`: sequenza completa; *Redefine* *No*; `?`; senza
  oggetti; `U`.

## Fatto quando

- I casi di RT20 passano; la CI è verde.
