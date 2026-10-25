# [0.6-P12] `cmd`: `-LAYER`, `CLAYER`

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.6-R12]` (#…), agente gemini
- **Dipende da:** P1, P6, P9, R12
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punti 1, 4,
  5; [ADR 0024](../../adr/0024-colori.md) punto 6;
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md) punto 1;
  [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md);
  `cmd/src/commands/file_commands.cpp` (`INSUNITS`, domanda di valore con
  `< >`), `cmd/src/command_descriptions.cpp` (P1),
  `edit/include/tg/edit/layer_rules.hpp` ed `editor.hpp` (P6)

## File

- `cmd/include/tg/cmd/commands/layer_commands.hpp`,
  `cmd/src/commands/layer_commands.cpp` (nuovi)
- `cmd/src/name_list.hpp`, `cmd/src/name_list.cpp` (nuovi: elenchi di nomi
  con `,` e caratteri jolly)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`
- `cmd/CMakeLists.txt`
- `tests/cmd/layer_commands_test.cpp`, `tests/cmd/name_list_test.cpp` (nuovi)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R12)

## Cosa fare

### `-LAYER`

Scrive `Current layer:  "<nome>"` (due spazi dopo i due punti, come AutoCAD)
e chiede, finché si risponde con Invio:

`Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/Plot/Freeze/Thaw/LOck/Unlock]:`

| Opzione | Domande | Effetto |
|---|---|---|
| `?` | `Enter layer name(s) to list <*>:` | una riga per layer: `<nome>  <stati> <colore> "<tipo>" <spessore>` (stati `on`/`off`, `frozen`/`thawed`, `locked`/`unlocked`); ultima riga `Current layer: "<nome>"` |
| `Make` | `Enter name for new layer (becomes the current layer) <nome corrente>:` | crea se non c'è, poi lo rende corrente |
| `Set` | `Enter layer name to make current:` | rende corrente |
| `New` | `Enter name list for new layer(s):` | crea ogni nome dell'elenco |
| `Rename` | `Enter old layer name:` poi `Enter new layer name:` | rinomina (avviso dell'ADR 0023, punto 5, prima) |
| `ON`, `OFF`, `Freeze`, `Thaw`, `LOck`, `Unlock` | `Enter name list of layer(s) to turn on:` (`to turn off`, `to freeze`, `to thaw`, `to lock`, `to unlock`) | cambia lo stato |
| `Color` | `New color [Truecolor]:`; con `Truecolor`, `Red, Green, Blue:`; poi `Enter name list of layer(s) for color <colore> <nome corrente>:` | cambia il colore (solo indice o RGB) |
| `Ltype` | `Enter a loaded linetype name <Continuous>:` poi `Enter name list of layer(s) for linetype "<tipo>" <nome corrente>:` | cambia il tipo (solo un tipo caricato, non `ByLayer` né `ByBlock`) |
| `LWeight` | `Enter lineweight (0.0mm - 2.11mm):` poi `Enter name list of layer(s) for lineweight <mm> <nome corrente>:` | cambia lo spessore (un valore ammesso o `Default`) |
| `Plot` | `Enter a plotting preference [Plot/No plot] <Plot>:` poi `Enter name list of layer(s) for this plot preference <nome corrente>:` | cambia la stampa |

- **Elenchi di nomi:** separati da `,`, spazi ai capi tolti, caratteri jolly
  `*` e `?` senza distinguere maiuscole. Un elenco che non trova layer:
  `No matching layer names found.`
- **Messaggi:**
  - nome non valido: `Invalid layer name.` (la domanda resta);
  - `New` o `Rename` verso un nome usato: `Layer "<nome>" already exists.`;
  - layer `0` in `Rename`: `Cannot rename layer "0".`;
  - congelare il corrente: `Cannot freeze layer "<nome>".  It is the CURRENT layer.`;
  - spegnere il corrente: domanda `Really want layer <nome> (the CURRENT layer) off? [Yes/No] <No>:`;
  - `Set`/`Make` su un layer congelato: `Cannot set current to a frozen layer.`;
  - layer o tipo sconosciuto: `Layer "<nome>" not found.`, `Linetype "<nome>" is not loaded.`;
  - rinomina con il nome in altri dati non gestiti:
    `Layer "<vecchio>" is also named in data Tecnigrafo does not handle; that data keeps the old name.`
- Tutto il comando è **una transazione** `Layer`; `Esc` conserva ciò che è già
  fatto (ADR 0015).

### `CLAYER`

`Enter new value for CLAYER <"<nome>">:` — un layer esistente e non
congelato diventa corrente (transazione `CLAYER`); altrimenti i messaggi di
sopra e la domanda resta.

### Descrizioni

Aggiungi le descrizioni di `-LAYER` (alias `-LA`) e `CLAYER` (ADR 0026). Il
comando `LAYER` (alias `LA`) lo registra l'applicazione (P22), che apre il
pannello.

## Regole della tappa che valgono qui

- Testi in inglese, interi, con le parole di AutoCAD; le opzioni non
  realizzate non compaiono.
- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt` li
  toccano poi P13, P14 e P23: solo le tue righe, in ordine alfabetico.

## Test richiesti

- Ogni opzione, con elenchi e caratteri jolly; ogni messaggio.
- Annulla e ripeti di un `-LAYER` con più opzioni; `Esc` a metà.
- `CLAYER` valido, sconosciuto, congelato.
- Le entità nuove dopo `Make` vanno sul nuovo layer.

## Fatto quando

- I casi di `[0.6-R12]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
