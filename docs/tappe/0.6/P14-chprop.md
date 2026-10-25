# [0.6-P14] `cmd`: `CHPROP` e layer bloccati nella selezione

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #246
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.6-R14]` (#245), agente gemini
- **Dipende da:** P10, P13, R14
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punti 2 e 4,
  [ADR 0008](../../adr/0008-selezione-e-snap.md) (selezione);
  `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`,
  `cmd/src/selection_set.cpp`, `cmd/src/commands/erase_command.cpp`,
  `cmd/src/commands/property_commands.cpp` (P13)

## File

- `cmd/include/tg/cmd/commands/chprop_command.hpp`,
  `cmd/src/commands/chprop_command.cpp` (nuovi)
- `cmd/src/command_runner.cpp`, `cmd/include/tg/cmd/command_runner.hpp`
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`
- `cmd/CMakeLists.txt`
- `tests/cmd/chprop_command_test.cpp` (nuovo), `tests/cmd/command_runner_test.cpp`
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R14)

## Cosa fare

### Layer bloccati

- Ogni gesto di selezione (clic, finestra, intersezione), con un comando o
  senza, scarta le entità dei layer bloccati.
- Mentre il runner chiede `Select objects:`, se il gesto ne ha scartate scrive
  prima `<n> found, <t> total` e poi `1 was on a locked layer.` o
  `<n> were on a locked layer.`; senza comando non scrive nulla.

### `CHPROP`

Selezione prima (`selectsFirst`), poi, finché Invio:

`Enter property to change [Color/LAyer/LType/ltScale/LWeight]:`

| Opzione | Domanda | Valore tra `< >` |
|---|---|---|
| `Color` | `New color [Truecolor] <…>:`; `Truecolor` → `Red, Green, Blue:` | il colore comune, o `varies` |
| `LAyer` | `Enter new layer name <…>:` | il layer comune, o `varies` |
| `LType` | `Enter new linetype name <…>:` | il tipo comune, o `varies` |
| `ltScale` | `Specify new linetype scale <…>:` | la scala comune (`formatLength`), o `varies` |
| `LWeight` | `Enter new lineweight <…>:` | lo spessore comune, o `varies` |

- Invio vuoto a una domanda di valore lascia le entità come sono.
- Layer inesistente: `Layer "<nome>" not found.`; congelato: si può assegnare
  (l'entità sparisce), come in AutoCAD. Tipo non caricato:
  `Linetype "<nome>" is not loaded.`
- Tutto il comando è una transazione `CHPROP`; `Esc` conserva i cambi già
  fatti (ADR 0015).
- Descrizione di `CHPROP` (ADR 0026).

## Regole della tappa che valgono qui

- Testi in inglese con le parole di AutoCAD; le opzioni non realizzate
  (`Thickness`, `TRansparency`, `Material`, `PLotstyle`) non compaiono.
- `command_runner.cpp` e i file comuni di `cmd`: solo le tue righe.

## Test richiesti

- Selezione con layer bloccati: clic, finestra, intersezione, con e senza
  comando, messaggi al singolare e al plurale.
- `ERASE` non cancella entità di layer bloccati.
- `CHPROP` di ogni proprietà, selezione mista (`varies`), Invio vuoto, `Esc` a
  metà, annulla e ripeti.

## Fatto quando

- I casi di `[0.6-R14]` passano, compresi quelli che salvano e riaprono il
  disegno (per questo P14 dipende anche da P10), e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
