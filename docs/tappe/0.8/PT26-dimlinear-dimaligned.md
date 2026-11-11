# [0.8-PT26] `cmd`: `DIMLINEAR`, `DIMALIGNED`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT26]` (#…), agente claude
- **Dipende da:** PT23, RT26
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punto 5;
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md);
  [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md); un comando con
  anteprima, per esempio `cmd/src/commands/line_command.cpp`; la scheda
  [PT23](PT23-quote-lineari.md) (`regenerated`, `dimensionText`); la cronologia
  dei comandi copiata dal responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/include/tg/cmd/commands/dimension_commands.hpp`,
  `cmd/src/commands/dimension_commands.cpp` (nuovi; `DIMLINEAR`, `DIMALIGNED`)
- `cmd/src/commands/dimension_input.cpp`, `cmd/src/commands/dimension_input.hpp`
  (nuovi: opzioni *Text* e *Angle* comuni a tutte le quote, che usa anche PT27)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/dimension_commands_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

Richieste e messaggi dalla cronologia di AutoCAD LT; quelle attese:

1. **`DIMLINEAR`**
   - `Specify first extension line origin or <select object>:` — `Invio` passa
     a `Select object to dimension:` (una `LINE` o un segmento di
     `LWPOLYLINE`: le origini sono i suoi estremi);
   - `Specify second extension line origin:`
   - `Specify dimension line location or [Mtext/Text/Angle/Horizontal/Vertical/Rotated]:`
     — senza *Mtext* (decisione 5 del piano): la richiesta lo omette. *Text*:
     `Enter dimension text <valore>:` (testo sostituito); *Angle*: `Specify
     angle of dimension text:` (rotazione del testo); *Horizontal* e *Vertical*
     fissano la direzione; *Rotated*: `Specify angle of dimension line <0>:`.
     Senza opzioni la direzione è orizzontale o verticale secondo dove sta il
     punto rispetto al rettangolo dei due punti, come AutoCAD: orizzontale se il
     punto è sopra o sotto, verticale se è a destra o a sinistra; dentro il
     rettangolo, l'asse con lo scarto maggiore.
   - fine: `Dimension text = <testo>`.
2. **`DIMALIGNED`**: le stesse prime due richieste, poi
   `Specify dimension line location or [Text/Angle]:`, e la fine uguale.
3. La quota nasce con lo stile corrente, le variabili correnti diverse dallo
   stile come sovrascritture, `regenerated` (PT23), layer e proprietà correnti.
   Anteprima della quota mentre si sceglie la posizione. Un passo di
   annullamento con il nome del comando.

## Regole della tappa che valgono qui

- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `dimension_commands_test.cpp`: le due quote con punti scritti; *select
  object* su una linea; *Text* con `<>`; *Rotated*; scelta automatica
  orizzontale e verticale; `U`.

## Fatto quando

- I casi di RT26 passano; la CI è verde.
