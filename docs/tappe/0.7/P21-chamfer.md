# [0.7-P21] `cmd`: `CHAMFER`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R21]` (#…), agente gemini
- **Dipende da:** P20, R21
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 11, 12, 16 e 17; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 8; `core/include/tg/core/fillet.hpp` (P19);
  `cmd/src/commands/fillet_command.cpp` (P20, lo stesso schema)

## File

- `cmd/include/tg/cmd/commands/chamfer_command.hpp`,
  `cmd/src/commands/chamfer_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults`: distanze, lunghezza,
  angolo, metodo)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/chamfer_command_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R21)

## Cosa fare

**`CHAMFER` (`CHA`).** Prima il messaggio
`(TRIM mode) Current chamfer Dist1 = <d1>, Dist2 = <d2>` (o
`Length = <l>, Angle = <a>` con il metodo dell'angolo), poi:

```
Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]:
Select second line or shift-select to apply corner or [Distance/Angle/Method]:
Specify first chamfer distance <{default}>:
Specify second chamfer distance <{default}>:
Specify chamfer length on the first line <{default}>:
Specify chamfer angle from the first line <{default}>:
Enter trim method [Distance/Angle] <{default}>:
Select 2D polyline or [Distance/Angle/Method]:
```

- Stesso schema di `FILLET` (P20): richieste di oggetto, `Shift` per lo
  spigolo, *Multiple*, *Undo*, le due sequenze (negli script il comando finisce
  dopo *Distance*, *Angle*, *mEthod* e *Trim*), le proprietà della linea nuova.
- Solo linee e segmenti dritti di polilinee; un arco dà il messaggio inglese di
  AutoCAD (da controllare) e la domanda resta.
- Un passo di annulla `Chamfer`.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`: prima di te P20, dopo di
  te P23.

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `CHAMFER`, nelle due sequenze.
- Distanze, angolo, *mEthod*, *Polyline*, *Trim No*, `Shift`, un arco,
  *Multiple*, *Undo*; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R21]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
