# [0.7-P6] `cmd`: `ROTATE`, `SCALE`, `MIRROR`, `MIRRTEXT`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R6]` (#…), agente gemini
- **Dipende da:** P5, R6
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 1–3;
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 2; [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md);
  `cmd/src/commands/move_copy_commands.cpp` (P5, lo stesso schema);
  `cmd/include/tg/cmd/command.hpp`

## File

- `cmd/include/tg/cmd/commands/rotate_scale_commands.hpp`,
  `cmd/src/commands/rotate_scale_commands.cpp` (nuovi)
- `cmd/include/tg/cmd/commands/mirror_commands.hpp`,
  `cmd/src/commands/mirror_commands.cpp` (nuovi: `MIRROR` e `MIRRTEXT`)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `tests/cmd/rotate_scale_commands_test.cpp`, `tests/cmd/mirror_commands_test.cpp`
  (nuovi), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R6)

## Cosa fare

`CommandDefaults` riceve `rotation` (l'ultimo angolo di `ROTATE`, 0 all'avvio) e
`mirrorText` (`core::MirrorText::Readable` all'avvio).

**`ROTATE` (`RO`)**, con selezione. Prima il messaggio
`Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0`, poi:

```
Specify base point:
Specify rotation angle or [Copy/Reference] <{default}>:
Specify the reference angle <0>:
Specify the new angle or [Points] <0>:
```

- *Copy* scrive `Rotating a copy of the selected objects.` e ruota copie
  (`addCopy`) invece degli originali.
- *Reference* chiede l'angolo di riferimento (anche con due punti) e il nuovo
  angolo, oppure *Points* e due punti; la rotazione è la differenza.
- Un clic alla domanda dell'angolo dà l'angolo dal punto base.

**`SCALE` (`SC`)**, con selezione:

```
Specify base point:
Specify scale factor or [Copy/Reference]:
Specify reference length <1.0000>:
Specify new length or [Points] <1.0000>:
```

- Il fattore è una richiesta di numero (`PromptKind::Number`); un clic dà la
  distanza dal punto base.
- Un fattore non maggiore di 0 scrive `Value must be positive and nonzero.` e la
  domanda resta.
- *Copy* scrive `Scaling a copy of the selected objects.`.

**`MIRROR` (`MI`)**, con selezione:

```
Specify first point of mirror line:
Specify second point of mirror line:
Erase source objects? [Yes/No] <No>:
```

- Senza cancellare: copie riflesse (`addCopy`).
- Con *Yes*: le entità si riflettono sul posto (`replace`).
- Il testo segue `MIRRTEXT`.
- Due punti coincidenti: la seconda domanda resta.

**`MIRRTEXT`:** `Enter new value for MIRRTEXT <0>:`, accetta 0 o 1; altro dà
`Requires an integer between 0 and 1.` e la domanda resta.

Ogni comando è un passo di annulla (`Rotate`, `Scale`, `Mirror`).

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`: solo
  le tue righe; prima di te P5, dopo di te P8.
- `command.hpp` lo tocca anche P8 (`CommandDefaults::polarAngle`).

## Test richiesti

- `ROTATE`: angolo scritto, clic, *Copy*, *Reference* con valori e con punti;
  valore predefinito ricordato.
- `SCALE`: fattore, clic, *Copy*, *Reference*, fattore 0 e negativo.
- `MIRROR`: con e senza cancellare, con testi e `MIRRTEXT` 0 e 1, archi.
- Annulla e ripeti in ognuno; `Esc` a ogni domanda.

## Fatto quando

- I casi di `[0.7-R6]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
