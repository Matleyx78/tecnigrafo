# [0.8-PT33] `cmd`: `PEDIT` e `PEDITACCEPT`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT33]` (#…), agente claude
- **Dipende da:** PT32, RT33
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0050](../../adr/0050-pedit.md);
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md);
  [ADR 0031](../../adr/0031-modifiche-alle-entita.md), «Annullamento dentro un
  comando»; `cmd/src/commands/join_command.cpp`, `mirror_commands.cpp`
  (`MIRRTEXT`, una variabile come comando); la scheda [PT32](PT32-unione-con-distanza.md);
  la cronologia di `PEDIT` copiata dal responsabile in AutoCAD LT
  ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/pedit_command.hpp`,
  `cmd/src/commands/pedit_command.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/pedit_command_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

Richieste e messaggi dalla cronologia di AutoCAD LT; quelle attese:

1. `Select polyline or [Multiple]:` — una `LWPOLYLINE`, una `LINE` o un `ARC`.
   Per una linea o un arco: `Object selected is not a polyline` e, con
   `PEDITACCEPT` 0, `Do you want to turn it into one? <Y>`; *No* finisce il
   comando senza cambiare nulla.
2. Opzioni della polilinea:
   `Enter an option [Close/Join/Ltype gen/Reverse/Undo]:` (aperta) o
   `Enter an option [Open/Join/Ltype gen/Reverse/Undo]:` (chiusa); `Invio`
   termina. *Join*: `Select objects:`, poi `fuzz` 0 e unione (`<n> segments
   added to polyline`). *Ltype gen*: `Enter polyline linetype generation option
   [ON/OFF] <Off>:`. *Undo* annulla l'ultima opzione; senza opzioni da annullare
   `Nothing to undo.` (o il messaggio di AutoCAD).
3. ***Multiple***: `Select objects:`; se fra gli oggetti ci sono linee o archi,
   `Convert Lines, Arcs and Splines to polylines [Yes/No]? <Y>`; poi
   `Enter an option [Close/Open/Join/Ltype gen/Reverse/Undo]:`. *Join* in
   *Multiple*: `Join Type = <tipo>` e `Enter fuzz distance or [Jointype] <0>:`;
   *Jointype*: `Enter join type [Extend/Add/Both] <Extend>:`. Ogni polilinea
   scelta, nell'ordine della selezione, si unisce con gli oggetti rimasti
   (`joinWithGaps`, PT32); il messaggio di AutoCAD per il totale.
4. **`PEDITACCEPT`**: `Enter new value for PEDITACCEPT <0>:`, 0 o 1. In
   AutoCAD sta nel registro, non nel disegno: qui è un valore della sessione,
   come gli aiuti al disegno, e non entra nel file.
5. Identità e dati (ADR 0031, punto 7): una linea o un arco convertiti danno una
   polilinea con handle nuovo e i dati dell'origine; la polilinea che si unisce
   resta la stessa entità; gli oggetti usati si cancellano. Tutto il comando è
   un passo (`Pedit`).

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `pedit_command_test.cpp`: conversione con `PEDITACCEPT` 0 e 1; *Close*,
  *Open*, *Reverse*, *Ltype gen*; *Undo* dopo due opzioni; *Join* con distanza
  0; *Multiple* con distanza 2 e i tre tipi; `U` dopo il comando.

## Fatto quando

- I casi di RT33 passano; la CI è verde.
