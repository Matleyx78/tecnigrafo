# [0.7-P3] `cmd` e `script`: sequenze degli script di AutoCAD

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R3]` (#…), agente gemini
- **Dipende da:** P2, R3
- **Modulo:** `script` (più il contesto dei comandi e `TEXT` in `cmd`, e il
  livello 2 in `app`)
- **Da leggere:** [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punti 6–9; `docs/verifica/script.md` ("Direttive", "Sequenze degli script di
  AutoCAD"); `cmd/src/commands/text_command.cpp`; `script/src/script.cpp`,
  `script/src/level1.cpp`, `script/tool/main.cpp`; `app/src/script_session.cpp`,
  `app/src/main.cpp`

## File

- `cmd/include/tg/cmd/command.hpp` (`CommandContext::scriptSequences`)
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
  (`setScriptSequences`)
- `cmd/src/commands/text_command.cpp`
- `script/include/tg/script/script.hpp`, `script/src/script.cpp`,
  `script/src/level1.cpp`, `script/tool/main.cpp`
- `app/src/script_session.cpp`, `app/src/main.cpp` se serve
- `tests/cmd/text_command_test.cpp`, `tests/script/level1_test.cpp`
- `tests/verifica/in-attesa.txt` (togliere le righe di R3)

## Cosa fare

```cpp
// command.hpp
struct CommandContext {
    // ...
    bool scriptSequences{false};  ///< docs/adr/0032, points 6-8
};

// command_runner.hpp
/// The AutoCAD script sequences for the commands that have them
/// (docs/adr/0032, point 7). Default false.
void setScriptSequences(bool on);
```

- **`TEXT`**, con `scriptSequences`: dopo la prima riga di testo il comando
  finisce. Senza, nulla cambia.
- **Direttiva `!scr on` / `!scr off`**, con la sintassi e gli errori di `!snap`,
  ai livelli 1 e 2.
- **`tecnigrafo-script run`** con un file che finisce in `.scr`, in qualunque
  maiuscola, accende le sequenze degli script dall'inizio. Lo stesso fa il
  programma con `--script` e un file `.scr`.
- Gli altri comandi della tabella dell'ADR 0032, punto 8, leggono
  `scriptSequences` quando arrivano (P5, P16, P20, P21, P23).

## Regole della tappa che valgono qui

- `command.hpp`, `command_runner.*`: prima di te P2, dopo di te P7.
- `script.*`, `level1.cpp`, `script_session.cpp`: dopo di te P7 (`!osnap`) e P8
  (`!polar`).

## Test richiesti

- `TEXT` con e senza sequenze degli script.
- `!scr on`, `!scr off`, `!scr` senza valore o con un valore sbagliato.
- Un `.scr` eseguito con `run`; lo stesso contenuto in un `.tgs`, che resta
  interattivo.

## Fatto quando

- I casi di `[0.7-R3]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
