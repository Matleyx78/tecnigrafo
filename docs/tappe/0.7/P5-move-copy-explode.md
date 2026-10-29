# [0.7-P5] `cmd`: `MOVE`, `COPY`, `EXPLODE`, `LINE` con `Invio`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R5]` (#…), agente gemini
- **Dipende da:** P1, P3, P4, R5
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 5, 6,
  8 e 11; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 6; [ADR 0015](../../adr/0015-esc-conserva-il-lavoro.md);
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md);
  `cmd/src/commands/erase_command.cpp` (un comando con selezione),
  `cmd/src/commands/line_command.cpp`; `core/include/tg/core/similarity.hpp` (P4);
  `edit/include/tg/edit/editor.hpp` (P1)

## File

- `cmd/include/tg/cmd/commands/move_copy_commands.hpp`,
  `cmd/src/commands/move_copy_commands.cpp` (nuovi)
- `cmd/include/tg/cmd/commands/explode_command.hpp`,
  `cmd/src/commands/explode_command.cpp` (nuovi)
- `cmd/src/commands/line_command.cpp`
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/move_copy_commands_test.cpp`, `tests/cmd/explode_command_test.cpp`
  (nuovi), `tests/cmd/line_command_test.cpp`, `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R5)

## Cosa fare

**`MOVE` (`M`)**, con selezione (`needsSelection`):

```
Specify base point or [Displacement] <Displacement>:
Specify second point or <use first point as displacement>:
Specify displacement <{default}>:
```

- Dopo il punto base, il secondo punto sposta le entità di (secondo − base);
  Invio usa il punto base come spostamento.
- *Displacement*, o Invio alla prima domanda, chiede lo spostamento.
- Ogni entità si sposta con `Editor::replace` e `transformed`; un passo di
  annulla `Move`.
- L'anteprima mostra le entità spostate mentre il puntatore si muove.

**`COPY` (`CO`, `CP`):** prima il messaggio `Current settings:  Copy mode = Multiple`,
poi le domande:

```
Specify base point or [Displacement] <Displacement>:
Specify second point or <use first point as displacement>:
Specify second point or [Exit/Undo] <Exit>:
```

- Ogni secondo punto aggiunge una copia (`Editor::addCopy`).
- *Undo* toglie l'ultima copia (`savepoint`, `rollbackTo`); *Exit* o Invio
  chiudono.
- `Esc` conserva le copie fatte (ADR 0015).
- Nella **sequenza degli script** (ADR 0032) il comando finisce dopo la prima
  copia.
- *mOde* e *Array* restano fuori.

**`EXPLODE` (`X`)**, con selezione:

- ogni polilinea diventa linee e archi (`Editor::addCopy` per ogni pezzo, poi la
  polilinea si cancella);
- le larghezze si perdono, con il messaggio
  `Exploding this polyline has lost width information.` seguito da
  `The UNDO command will restore it.`, una volta per comando;
- una polilinea con quota o spessore (gruppi 38 o 39 nei dati di posizione) non
  si scompone, e conta tra quelle non scomposte;
- le altre entità non si scompongono: `1 was not able to be exploded.` oppure
  `<n> were not able to be exploded.`;
- un passo di annulla `Explode`.

**`LINE`:** alla prima domanda, Invio fa partire la linea da
`CommandContext::lastPoint`. Senza ultimo punto la domanda resta. La
continuazione tangente dopo un arco (`Length of line:`) resta fuori.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`: solo
  le tue righe, in ordine alfabetico; dopo di te li tocca P6.
- Le descrizioni dei comandi elencano le domande di entrambe le sequenze
  (ADR 0032, punto 9).
- Il testo dei messaggi di `EXPLODE` va controllato sulla versione inglese di
  AutoCAD prima di scriverlo ([dxf-di-prova.md](dxf-di-prova.md), "Che cosa resta
  da controllare"): se differisce, vale l'inglese di AutoCAD e lo scrivi nella
  pull request.

## Test richiesti

- `MOVE` e `COPY` con punti, con *Displacement*, con Invio; ogni entità;
  annulla e ripeti; `Esc` a metà.
- `COPY` con tre copie, *Undo*, `Esc`; con la sequenza degli script.
- `EXPLODE`: polilinea con archi, con larghezze, chiusa, con quota; selezione
  mista; dati non gestiti sulle entità nuove.
- `LINE` con Invio alla prima domanda, con e senza ultimo punto.

## Fatto quando

- I casi di `[0.7-R5]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
