# [0.8.1-PT18] `cmd`: `ATTSYNC` e `ATTDISP`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8.1-RT18]` (#…), agente claude
- **Dipende da:** PT16, PT27, RT18
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punti 1 e 5; le schede [PT16](PT16-definizioni-edit.md) e
  [PT27](PT27-stato-del-disegno.md); le cronologie `autocad-attsync.txt` e
  `autocad-attdisp-on.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/attribute_commands.hpp`,
  `cmd/src/commands/attribute_commands.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/attribute_commands_test.cpp` (nuovo), `tests/CMakeLists.txt`
  (file comune)

## Cosa fare

1. **`ATTSYNC`**: in un disegno senza blocchi con attributi scrive subito
   `This drawing contains no blocks with attributes.` e finisce, senza domande;
   altrimenti `Enter an option [?/Name/Select] <Select>:`; *Name*: `Enter
   name of block to synchronize or [?]:`; *Select*: `Select a block:`; `?`
   elenca i blocchi con attributi. Un nome senza attributi, o che non esiste:
   `The block table contains no block with attributes named <nome>.` e il
   comando finisce (come AutoCAD, seduta del 2026-11-22). Poi
   `synchronizeInserts` (PT16), che aggiorna **ogni** inserimento del blocco,
   anche quelli dentro altre definizioni (ciascuno una volta: [bloccante] 12),
   e `ATTSYNC complete.`; un passo `Attsync`.
2. **`ATTDISP`**: `Enter attribute visibility setting [Normal/ON/OFF]
   <Normal>:`, che cambia l'`AttributeDisplay` del disegno con
   `edit::DocumentTables::setAttributeDisplay()` (PT27), senza annullamento,
   segnando il disegno modificato, e scrive `Regenerating model.` (verbale
   della revisione PT5, [bloccante] 1).

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `ATTSYNC` *Name* e *Select* sui blocchi di `autocad-attsync`; un blocco senza
  attributi e un nome che non esiste; un disegno senza blocchi con attributi;
  `U` dopo `ATTSYNC`; `ATTDISP` nei tre valori, che segna il disegno
  modificato senza un passo di annullamento ([bloccante] 1).

## Fatto quando

- I casi di RT18 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/attributi/attsync/script.tgs
OPEN
sincro.dxf
ATTSYNC
N
SINCRO_A
!report
U
ATTDISP
ON
```

`sincro.dxf`: un DXF del caso scritto a mano con lo stato di
`autocad-attsync.dxf` **dopo** le due ridefinizioni e **prima** di `ATTSYNC`
(si ricava da `SINCRO_B`, che `ATTSYNC` non ha toccato). Righe da controllare:
in `[report 1]` gli `ATTRIB` di `SINCRO_A` come quelli del DXF di AutoCAD
(valori tenuti, `SECONDO` tolto, `TERZO` spostato e alto 3,5, `QUARTO` con il
predefinito); dopo `U` di nuovo come prima; `attdisp: on` nel `[final]`.

- **Strumenti:** righe normali, `!report`, su `main`; il campo `attdisp:`
  annunciato da PT2.
- **Dipendenze:** PT16 e PT27.
- **Fonte:** `autocad-attsync.dxf` e la sua cronologia; `autocad-attdisp-on.txt`;
  i due messaggi dalla seduta del 2026-11-22 ([dxf-di-prova.md](dxf-di-prova.md)).
- **Risposte condivise:** nessuna.
