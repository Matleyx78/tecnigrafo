# [0.8.1-PT21] `app`: finestre degli attributi per `TEXTEDIT` e il doppio clic

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: test Qt e prova manuale
- **Dipende da:** PT11, PT12, PT20
- **Modulo:** `app`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 4; [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md), punto 6;
  le schede [PT11](PT11-textedit.md), [PT12](PT12-editor.md) e
  [PT20](PT20-battman.md)

## File

- `app/src/attribute_dialogs.hpp`, `app/src/attribute_dialogs.cpp` (nuovi)
- `app/src/app_commands.cpp` (le richieste di `TEXTEDIT` su `ATTDEF` e
  `INSERT` si rispondono con la finestra)
- `app/CMakeLists.txt` (file comune)
- `tests/app/attribute_dialogs_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. Quando `TEXTEDIT` (dalla riga di comando o dal doppio clic di PT12) chiede
   gli attributi di un inserimento, il programma apre una finestra con
   etichetta e valore di ogni attributo non costante, nell'ordine; *OK* risponde
   alle richieste del comando con i valori, *Annulla* dà `Esc`.
2. Su un `ATTDEF`, una finestra con etichetta, richiesta e predefinito, allo
   stesso modo.
3. Negli script e al livello 1 restano le richieste di PT11.

## Regole della tappa che valgono qui

- File comuni: `app/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- Doppio clic su un inserimento con tre attributi, un valore cambiato, *OK*:
  l'inserimento ha il valore nuovo e `U` lo annulla; *Annulla* non cambia nulla.

## Fatto quando

- I test passano; la CI è verde.
