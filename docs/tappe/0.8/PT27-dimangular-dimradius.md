# [0.8-PT27] `cmd`: `DIMANGULAR`, `DIMRADIUS`, `DIMDIAMETER`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT27]` (#…), agente claude
- **Dipende da:** PT24, PT26, RT27
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punto 5;
  le schede [PT24](PT24-quote-angolari-radiali.md) e
  [PT26](PT26-dimlinear-dimaligned.md) (sorella: `dimension_input`); la
  cronologia dei comandi copiata dal responsabile in AutoCAD LT
  ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `cmd/src/commands/dimension_angular_commands.cpp`,
  `cmd/include/tg/cmd/commands/dimension_angular_commands.hpp` (nuovi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt` (file comuni)
- `tests/cmd/dimension_angular_commands_test.cpp` (nuovo), `tests/CMakeLists.txt`
  (file comune)

## Cosa fare

Richieste e messaggi dalla cronologia di AutoCAD LT; quelle attese:

1. **`DIMANGULAR`**: `Select arc, circle, line, or <specify vertex>:` — nella
   0.8 solo linee (anche segmenti dritti di `LWPOLYLINE`): un arco, un cerchio o
   `Invio` danno `Object selected is not a line.` e la richiesta si ripete (la
   richiesta resta quella di AutoCAD, ADR 0047, punto 5); `Select second line:`;
   `Specify dimension arc line location or [Text/Angle/Quadrant]:` — *Quadrant*
   fissa l'angolo misurato con un punto, poi chiede la posizione dell'arco.
   Le linee parallele: `Lines are parallel.` e la seconda linea si richiede.
2. **`DIMRADIUS`** e **`DIMDIAMETER`**: `Select arc or circle:` (un `ARC`, un
   `CIRCLE` o un arco di `LWPOLYLINE`); `Specify dimension line location or
   [Text/Angle]:`. Il punto sulla curva è quello nella direzione del punto
   indicato dal centro.
3. Fine: `Dimension text = <testo>`. Stile, proprietà, anteprima e annullamento
   come PT26.

## Regole della tappa che valgono qui

- Le opzioni *Text* e *Angle* vengono da `dimension_input` (PT26), senza
  cambiarlo.
- File comuni: `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`,
  `tests/CMakeLists.txt`.

## Test richiesti

- `dimension_angular_commands_test.cpp`: angolare con il punto nei quattro
  quadranti e con *Quadrant*; linee parallele; un arco scelto per l'angolare;
  radiale e diametrale su un cerchio e su un arco di polilinea; `U`.

## Fatto quando

- I casi di RT27 passano; la CI è verde.
