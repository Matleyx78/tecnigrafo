# [0.8.1-PT20] `app`: `BATTMAN`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: una finestra sopra operazioni di `edit` già
  provate; test Qt e prova manuale
- **Dipende da:** PT16
- **Modulo:** `app`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 5; `app/src/app_commands.cpp` (come il programma registra un comando che
  apre una finestra, per esempio `QSELECT`), `app/src/quick_select_dialog.cpp`;
  la scheda [PT16](PT16-definizioni-edit.md); la cronologia
  `autocad-battman.txt` ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `app/src/battman_dialog.hpp`, `app/src/battman_dialog.cpp` (nuovi)
- `app/src/app_commands.cpp`, `app/src/app_commands.hpp` (il comando
  `BATTMAN`)
- `app/CMakeLists.txt` (file comune)
- `tests/app/battman_dialog_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. `BATTMAN` apre la finestra sul blocco scelto (un elenco dei blocchi con
   attributi; con un inserimento scelto, il suo): una riga per `ATTDEF`
   nell'ordine, con etichetta, richiesta, predefinito e modi.
2. *Su*, *Giù*, *Modifica* (una finestra con etichetta, richiesta, predefinito,
   modi, altezza, rotazione, stile), *Rimuovi*: ognuno chiama la funzione di
   PT16 in una transazione `Battman`, così `U` e `Ctrl+Z` lo annullano.
3. *Sincronizza* chiama `synchronizeInserts`; alla chiusura, se ci sono
   modifiche non sincronizzate, la finestra chiede se sincronizzare, come
   AutoCAD (*Applica* e *OK* sincronizzano gli inserimenti esistenti).
4. Nell'AutoCAD italiano il campo della richiesta si chiama *Messaggio*; qui
   le parole inglesi del programma (`Prompt`).

## Regole della tappa che valgono qui

- File comuni: `app/CMakeLists.txt`, `tests/CMakeLists.txt`.
- Dopo PT20 `app_commands.cpp` lo cambia anche [PT21](PT21-finestre-attributi.md).

## Test richiesti

- Il riordino e la modifica di `autocad-battman` con *Sincronizza*: gli
  inserimenti hanno gli `ATTRIB` nel nuovo ordine e tengono i valori; `U` dopo
  ciascuna operazione.

## Fatto quando

- I test passano; la CI è verde.
