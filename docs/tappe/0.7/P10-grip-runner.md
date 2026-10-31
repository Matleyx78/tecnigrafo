# [0.7-P10] `cmd`: modifica con i grip nel runner

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R10]` (#…), agente gemini
- **Dipende da:** P1, P8, P9, R10
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0034](../../adr/0034-grip.md) punti 1, 3 e 6;
  [ADR 0008](../../adr/0008-selezione-e-snap.md);
  [ADR 0015](../../adr/0015-esc-conserva-il-lavoro.md);
  `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`;
  `core/include/tg/core/grips.hpp` (P9); `edit/include/tg/edit/editor.hpp` (P1)

## File

- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
- `cmd/src/commands/grip_command.hpp`, `cmd/src/commands/grip_command.cpp`
  (nuovi, interni: non nel registro)
- `cmd/CMakeLists.txt`
- `tests/cmd/grip_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R10.txt` (da cancellare)

## Cosa fare

```cpp
/// The grips to draw (docs/adr/0034, point 1): those of the idle selection,
/// when no command runs and it holds at most kMaxGripEntities entities.
[[nodiscard]] std::vector<core::Vec2> grips() const;
/// The hot grip, while a grip is being stretched.
[[nodiscard]] std::optional<core::Vec2> hotGrip() const;

static constexpr std::size_t kMaxGripEntities = 100;
static constexpr double kGripSizePixels = 10.0;  ///< side of the grip square
```

- **Clic senza comando in corso:**
  - se cade nel quadrato di un grip (metà lato, `kGripSizePixels / 2 * pixelSize`,
    in ogni direzione), il runner avvia il comando interno del grip. A pari
    distanza vince l'entità con l'identificativo più alto;
  - altrimenti il clic è un gesto di selezione, come oggi.
- **Comando interno:**
  - scrive `** STRETCH **` e chiede `Specify stretch point:` (`PromptKind::Point`,
    punto base il grip caldo, quindi snap, orto e polare partono da lì);
  - l'anteprima mostra l'entità modificata;
  - con il punto, `moveGrip` e `Editor::replace` in una transazione `Grip edit`;
    se `moveGrip` dà `Invalid`, nessuna modifica e il messaggio
    `Invalid grip edit.`;
  - `Esc` esce senza modifiche;
  - in ogni caso la selezione resta.
- `grips()` è vuoto con un comando in corso, mentre si chiede una selezione per
  un comando, e con più di `kMaxGripEntities` entità selezionate.
- `U`, `UNDO` e `REDO` annullano e ripetono `Grip edit` come ogni altro passo.
- Il comando interno non è nel registro e non ha descrizione: le sue domande
  sono del runner (ADR 0026, punto 3).

## Regole della tappa che valgono qui

- `command_runner.cpp` e `.hpp`: prima di te P2, P3, P7, P8.
- `cmd/CMakeLists.txt`: solo le tue righe.

## Test richiesti

- Clic su un grip di ogni entità, poi un punto con il clic o scritto.
- Grip con snap, orto e polare.
- `Esc` sul grip caldo; annulla e ripeti; modifica non valida.
- Due entità selezionate con un grip nello stesso punto.
- Clic fuori dai grip: la selezione funziona come prima.
- 101 entità selezionate: nessun grip.
- Entità su un layer bloccato: non è selezionata, quindi non ha grip.

## Fatto quando

- I casi di `[0.7-R10]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`; i casi di selezione esistenti passano ancora.
- La CI è verde.
