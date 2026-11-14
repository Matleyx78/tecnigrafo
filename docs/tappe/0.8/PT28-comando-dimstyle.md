# [0.8-PT28] `edit` e `cmd`: `-DIMSTYLE` e variabili di quota

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT28]` (#…), agente claude
- **Dipende da:** PT7, PT22, RT28
- **Modulo:** `cmd` (più le operazioni sulla tabella in `edit`)
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punto 2;
  `cmd/src/commands/mirror_commands.cpp` (`MIRRTEXT`, una variabile come
  comando); le schede [PT7](PT7-tabelle-edit.md), [PT12](PT12-comando-style.md)
  (lo stesso schema per gli stili di testo) e [PT22](PT22-stili-di-quota.md); la
  cronologia di `-DIMSTYLE` copiata dal responsabile in AutoCAD LT

## File

- `edit/include/tg/edit/dim_styles.hpp`, `edit/src/dim_styles.cpp` (nuovi)
- `cmd/include/tg/cmd/commands/dimstyle_commands.hpp`,
  `cmd/src/commands/dimstyle_commands.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `edit/CMakeLists.txt`, `cmd/CMakeLists.txt`
  (file comuni)
- `tests/cmd/dimstyle_commands_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `edit/dim_styles.*`: salvare le variabili correnti in uno stile (nuovo o
   esistente), rendere corrente uno stile (le variabili correnti diventano le
   sue), cambiare una variabile corrente; annullabili sopra
   `DocumentTables::replaceDimStyles`.
2. **`-DIMSTYLE`** (alias `DIMSTYLE`), dalla cronologia di AutoCAD LT; quelle
   attese:
   - `Current dimension style: <nome>` e, se ci sono, `Current dimension
     overrides:` con le variabili, una per riga;
   - `Enter a dimension style option [Save/Restore/STatus/?] <Restore>:`
     (senza *ANnotative*, *Variables*, *Apply*);
   - *Save*: `Enter name for new dimension style or [?]:`; un nome che c'è:
     `That name is already in use, redefine it? <N>:`;
   - *Restore*: `Enter a dimension style name, [?] or <select dimension>:`
     (`Invio` e una quota: il suo stile);
   - *STatus*: le variabili essenziali correnti, `NOME valore descrizione`, come
     la cronologia di AutoCAD;
   - `?`: gli stili.
3. **Variabili come comandi:** `DIMSCALE`, `DIMASZ`, `DIMEXO`, `DIMEXE`,
   `DIMTXT`, `DIMGAP`, `DIMTAD`, `DIMTIH`, `DIMTOH`, `DIMDEC`, `DIMADEC`,
   `DIMDSEP`, `DIMZIN`, `DIMAZIN`, `DIMLFAC`, `DIMRND`, `DIMPOST`, `DIMTXSTY`,
   `DIMCEN`, `DIMSE1`, `DIMSE2`, `DIMSD1`, `DIMSD2`: `Enter new value for <NOME>
   <valore>:`, con i controlli dei valori di AutoCAD (per esempio `DIMDEC` da 0 a
   8; `DIMTXSTY` uno stile che c'è). Il valore nuovo è una sovrascrittura dello
   stile corrente. Colori e spessori delle quote restano fuori dai comandi.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `edit/CMakeLists.txt`,
  `cmd/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `dimstyle_commands_test.cpp`: `DIMTXT 3.5` e poi *Save* con un nome nuovo;
  *Restore* di `ISO-25`; *STatus*; `?`; un valore fuori dai limiti; `U`.

## Fatto quando

- I casi di RT28 passano; la CI è verde.
