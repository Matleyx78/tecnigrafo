# [0.6-P13] `cmd`: `COLOR`, `-LINETYPE`, `LTSCALE`, `CELTSCALE`, `LWEIGHT`, `LWDISPLAY`

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #244
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.6-R13]` (#243), agente gemini
- **Dipende da:** P12, R13
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punto 3,
  [ADR 0024](../../adr/0024-colori.md) punto 6,
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti 2, 3 e 6;
  `cmd/src/commands/layer_commands.cpp` (P12), `cmd/src/value_format.cpp`

## File

- `cmd/include/tg/cmd/commands/property_commands.hpp`,
  `cmd/src/commands/property_commands.cpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`
- `cmd/CMakeLists.txt`
- `tests/cmd/property_commands_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R13)

## Cosa fare

| Comando (alias) | Domande | Effetto |
|---|---|---|
| `COLOR` (`COL`, `COLOUR`, `-COLOR`) | `Enter default object color [Truecolor] <BYLAYER>:` (il valore corrente in maiuscolo per `BYLAYER` e `BYBLOCK`, altrimenti `formatColor`); `Truecolor` → `Red, Green, Blue:` | colore corrente |
| `-LINETYPE` (`LINETYPE`, `LT`, `LTYPE`, `-LT`) | `Current line type: "<nome>"` poi `Enter an option [?/Load/Set]:`, finché Invio | — |
| … `?` | — | una riga per tipo caricato: `"<nome>" <descrizione>` |
| … `Load` | `Enter linetype(s) to load:` | carica dai tipi standard (P2); `Linetype "<nome>" not found.`, `Linetype "<nome>" is already loaded.` |
| … `Set` | `Specify linetype name or [?] <ByLayer>:` | tipo corrente (caricato, anche `ByLayer` o `ByBlock`) |
| `LTSCALE` (`LTS`) | `Enter new linetype scale factor <1.0000>:` | `LTSCALE`, > 0 |
| `CELTSCALE` | `Enter new value for CELTSCALE <1.0000>:` | scala corrente, > 0 |
| `LWEIGHT` (`LW`, `LINEWEIGHT`, `-LWEIGHT`) | `Current lineweight: <valore>` poi `Enter default lineweight for new objects or [?]:` | spessore corrente; `?` elenca i valori ammessi in mm |
| `LWDISPLAY` | `Enter new value for LWDISPLAY <OFF>:` | `ON`/`OFF` |

- I numeri tra `< >` con `formatLength` (quattro decimali); gli spessori in
  millimetri come `formatLineweight`, `ByLayer`, `ByBlock`, `Default`.
- Valori non validi: `Invalid color.`, `Requires a positive number.`,
  `Invalid lineweight.`, `Requires ON or OFF.`; la domanda resta.
- Ogni comando è una transazione con il suo nome (`COLOR`, `LINETYPE`,
  `LTSCALE`, `CELTSCALE`, `LWEIGHT`); `LWDISPLAY` usa `Editor::setLwdisplay` e
  non fa passi d'annullamento (P6).
- Descrizioni di tutti i comandi (ADR 0026).

## Regole della tappa che valgono qui

- Testi in inglese, interi, con le parole di AutoCAD; le opzioni non
  realizzate (`COlorbook`, `Create`) non compaiono.
- File comuni di `cmd`: solo le tue righe, in ordine alfabetico.

## Test richiesti

- Ogni comando con valori validi, non validi, Invio vuoto; ogni scrittura dei
  colori e degli spessori.
- `Load` di un tipo standard, già caricato, sconosciuto; `Set` di un tipo non
  caricato.
- Annulla e ripeti; le entità nuove ricevono i valori correnti.

## Fatto quando

- I casi di `[0.6-R13]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
