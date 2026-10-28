# [0.7-P18] `cmd`: `OFFSET`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R18]` (#…), agente gemini
- **Dipende da:** P16, P17, R18
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 7, 9, 16 e 17; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti
  6 e 10; `core/include/tg/core/offset.hpp` (P17);
  `cmd/src/commands/trim_extend_commands.cpp` (P16: richieste di oggetto e
  *Undo*)

## File

- `cmd/include/tg/cmd/commands/offset_command.hpp`,
  `cmd/src/commands/offset_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults::offsetDistance`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/offset_command_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R18)

## Cosa fare

**`OFFSET` (`O`).** Prima il messaggio
`Current settings: Erase source=No  Layer=Source  OFFSETGAPTYPE=0`, poi:

```
Specify offset distance or [Through/Erase/Layer] <Through>:
Select object to offset or [Exit/Undo] <Exit>:
Specify point on side to offset or [Exit/Multiple/Undo] <Exit>:
Specify through point or [Exit/Multiple/Undo] <Exit>:
Erase source object after offsetting? [Yes/No] <No>:
Enter layer option for offset objects [Current/Source] <Source>:
```

- **Distanza:** una richiesta di distanza, ricordata per la sessione; *Through*
  diventa il valore predefinito dopo che lo si usa. Una distanza non maggiore di
  0 scrive `Value must be positive and nonzero.`
- **Oggetto:** `PromptKind::Object`; il testo dà `Cannot offset that object.`
- **Lato** (o punto di passaggio con *Through*): `offsetCurve`. Con *Through* la
  distanza è quella del punto dalla curva (`distance`).
  - `Done`: la copia con `addCopy` (layer corrente con *Layer Current*), e
    l'originale cancellato con *Erase Yes*.
  - `Vanished` e `Failed`: `Cannot offset that object.` e nessuna copia.
- *Multiple* ripete il lato sullo stesso oggetto, ogni volta dall'ultima copia,
  finché non si preme Invio. *Undo* torna all'ultimo punto di ripristino. *Exit*
  o Invio chiudono.
- Un passo di annulla `Offset`; `Esc` conserva le copie fatte.

## Regole della tappa che valgono qui

- I testi di `Current settings` e dei messaggi vanno controllati sulla versione
  inglese di AutoCAD (ADR 0035, punto 17); se differiscono, vale AutoCAD e lo
  scrivi nella pull request.
- `builtin_commands.cpp`, `command_descriptions.cpp`: prima di te P16, dopo di
  te P20.

## Test richiesti

- Distanza e *Through*, dai due lati, su ogni entità; arco che si annulla; testo.
- *Erase*, *Layer*, *Multiple*, *Undo*, `Esc` dopo due copie; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R18]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
