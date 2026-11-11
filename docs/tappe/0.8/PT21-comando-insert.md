# [0.8-PT21] `cmd`: `-INSERT` di un blocco o di un file DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT21]` (#…), agente codex
- **Dipende da:** PT19, PT20, RT21
- **Modulo:** `cmd` (più le operazioni in `edit/blocks.*`)
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punti 5–7;
  [ADR 0021](../../adr/0021-comandi-dei-file.md) (percorsi e accesso ai file
  negli script); `cmd/src/commands/file_commands.cpp`; le schede
  [PT17](PT17-attributi-core.md), [PT19](PT19-file-come-blocco.md),
  [PT20](PT20-comando-block.md); la cronologia di `-INSERT` copiata dal
  responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `edit/include/tg/edit/blocks.hpp`, `edit/src/blocks.cpp`
- `cmd/include/tg/cmd/commands/insert_command.hpp`,
  `cmd/src/commands/insert_command.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/insert_command_test.cpp` (nuovo), `tests/edit/blocks_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `edit/blocks.*`:
   - `addInsert(Insert)`: aggiunge l'inserimento con `defaultAttributes` (PT17)
     e dà handle nuovi all'inserimento e a ogni attributo;
   - `applyImport(BlockImport, bool redefine)`: aggiunge i record delle tabelle,
     i blocchi annidati e la definizione (o la ridefinisce), porta avanti il
     contatore degli handle (`nextId`), tutto nello stesso cambiamento.
2. **`-INSERT`** (alias `INSERT`), con le richieste di AutoCAD LT dalla
   cronologia del responsabile; quelle attese:
   - `Enter block name or [?] <ultimo>:` — un nome di blocco della tabella;
     oppure un **percorso** di un file DXF (finisce con `.dxf`, senza distinguere
     maiuscole, o contiene `/` o `\`): allora `importBlock` (PT19), e se il
     blocco esiste `Block "<nome>" already exists. Redefine it? [Yes/No] <N>:`;
     un blocco che non c'è e non è un file: `Unknown block name: "<nome>".`;
   - `Specify insertion point or [Basepoint/Scale/Rotate]:` — *Basepoint* cambia
     il punto base solo per questo inserimento; *Scale* e *Rotate* li chiedono
     prima del punto;
   - `Enter X scale factor, specify opposite corner, or [Corner/XYZ] <1>:`
     (l'angolo opposto dà le scale dalla diagonale; *XYZ* chiede X e Y; Z si
     accetta e si conserva); `Enter Y scale factor <use X scale factor>:`;
   - `Specify rotation angle <0>:`.
   La scala proposta è moltiplicata per `unitFactor` (PT19). Gli attributi
   hanno i valori predefiniti, senza domande (`ATTREQ` = 0). Un passo di
   annullamento: `Insert`. Scala 0: `Value must be nonzero.` e la richiesta
   si ripete.
3. **File negli script:** il percorso segue le regole dei comandi dei file
   (ADR 0021: in modalità di controllo solo dentro la cartella del caso o
   `@sources/…`).
4. Durante l'inserimento l'anteprima mostra l'inserimento al puntatore.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `insert_command_test.cpp`: un blocco per nome, con scala `(2, 1)` e rotazione;
  un file con un nome nuovo; un file con un nome che esiste, *Redefine* *Yes* e
  *No*; *Basepoint*; scala 0; `U`.
- `blocks_test.cpp`: `addInsert` dà handle nuovi agli attributi; `applyImport`
  si annulla in un passo.

## Fatto quando

- I casi di RT21 passano; la CI è verde.
