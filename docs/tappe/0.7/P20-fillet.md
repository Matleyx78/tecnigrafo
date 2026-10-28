# [0.7-P20] `cmd`: `FILLET`, `CIRCLE` *Ttr*

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R20]` (#…), agente gemini
- **Dipende da:** P18, P19, R20
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 10, 12, 16 e 17; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 8; `core/include/tg/core/fillet.hpp` (P19);
  `cmd/src/commands/circle_command.cpp`; [dxf-di-prova.md](dxf-di-prova.md)
  ("Che cosa resta da controllare", punto 2)

## File

- `cmd/include/tg/cmd/commands/fillet_command.hpp`,
  `cmd/src/commands/fillet_command.cpp` (nuovi)
- `cmd/src/commands/circle_command.cpp`,
  `cmd/include/tg/cmd/commands/circle_command.hpp`
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults::filletRadius`,
  `trimMode`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/fillet_command_test.cpp` (nuovo), `tests/cmd/circle_command_test.cpp`,
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R20)

## Cosa fare

**`FILLET` (`F`).** Prima il messaggio
`Current settings: Mode = TRIM, Radius = <r>`, poi:

```
Select first object or [Undo/Polyline/Radius/Trim/Multiple]:
Select second object or shift-select to apply corner or [Radius]:
Specify fillet radius <{default}>:
Enter Trim mode option [Trim/No trim] <Trim>:
Select 2D polyline or [Radius]:
```

- Gli oggetti sono richieste di oggetto (`PromptKind::Object`). `Shift` sul
  secondo vale raggio 0.
- Il risultato viene da `fillet` o `filletPolyline`:
  - le entità cambiano con `replace`;
  - l'arco nuovo con `addCopy` dalla prima entità se le due hanno le stesse
    proprietà, altrimenti con le proprietà correnti (ADR 0035, punto 12);
  - impossibile: un messaggio inglese di AutoCAD (da controllare) e nessuna
    modifica.
- *Multiple* ripete finché non si preme Invio; *Undo* torna all'ultimo punto di
  ripristino.
- **Sequenze:**
  - interattiva: dopo *Radius* e *Trim* torna la prima domanda;
  - degli script: dopo *Radius* e *Trim* il comando finisce (ADR 0032, punto 8).
- Un passo di annulla `Fillet`.

**`CIRCLE` *Ttr*:** l'opzione `Ttr` nella prima domanda di `CIRCLE`, poi:

```
Specify point on object for first tangent of circle:
Specify point on object for second tangent of circle:
Specify radius of circle <{default}>:
```

Le due domande sono richieste di oggetto; il cerchio viene da
`circleTangentTangentRadius`; senza soluzione, `Circle does not exist.` e il
comando finisce senza disegnare.

## Regole della tappa che valgono qui

- Prima di cominciare controlla nella issue l'esito della prova a mano sul layer
  dell'arco ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta da
  controllare", punto 2). Se smentisce l'ADR 0035, punto 12, fermati.
- `builtin_commands.cpp`, `command_descriptions.cpp`: prima di te P18, dopo di
  te P21.

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `FILLET` e *Ttr*, nelle due sequenze.
- Raggio 0, `Shift`, parallele, *Polyline*, *Trim No*, *Multiple*, *Undo*,
  impossibile, proprietà dell'arco; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R20]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
