# [0.7-P8] `cmd`: tracciamento polare, `POLARANG`, `!polar`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R8]` (#…), agente gemini
- **Dipende da:** P6, P7, R8
- **Modulo:** `cmd` (più la direttiva in `script` e `app`)
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 8–11; [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md)
  (distanza scritta da sola); `cmd/include/tg/cmd/drawing_aids.hpp`,
  `cmd/src/drawing_aids.cpp`, `cmd/src/command_runner.cpp`;
  `docs/verifica/script.md` (`!polar`)

## File

- `cmd/include/tg/cmd/drawing_aids.hpp`, `cmd/src/drawing_aids.cpp`
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults::polarAngle`)
- `cmd/src/command_runner.cpp`
- `cmd/include/tg/cmd/commands/aid_commands.hpp`,
  `cmd/src/commands/aid_commands.cpp` (nuovi: `POLARANG`)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `script/include/tg/script/script.hpp`, `script/src/script.cpp`,
  `script/src/level1.cpp`
- `app/src/script_session.cpp` (`!polar` al livello 2)
- `tests/cmd/drawing_aids_test.cpp`, `tests/cmd/command_runner_test.cpp`
- `tests/verifica/in-attesa.txt` (togliere le righe di R8)

## Cosa fare

```cpp
struct DrawingAids {
    bool objectSnap{true};
    core::SnapModes snapModes;
    bool ortho{false};  ///< F8
    bool polar{false};  ///< F10; ortho and polar are never both true
    // ...
};

/// The point on the polar ray from `base` nearest to `pointer`, among the
/// rays at the multiples of `increment` (radians), if `pointer` is within
/// `tolerance` of it; otherwise std::nullopt (docs/adr/0033, point 9).
[[nodiscard]] std::optional<core::Vec2> polarPoint(const core::Vec2& base,
                                                   const core::Vec2& pointer,
                                                   double increment, double tolerance);
```

- `CommandDefaults::polarAngle`: π/2 all'avvio.
- **Runner:**
  - con il polare acceso e un punto base, dopo lo snap e al posto dell'orto, il
    punto è `polarPoint(base, pointer, polarAngle, 10 * pixelSize)` se esiste;
  - una distanza scritta da sola segue la semiretta polare su cui sta il
    puntatore;
  - `setDrawingAids` con orto e polare insieme tiene l'orto: l'esclusione la fa
    chi chiama (l'interfaccia in P11, le direttive qui).
- **`POLARANG`:** `Enter new value for POLARANG <90>:`, un angolo maggiore di 0 e
  al più di 180 gradi; altro dà `Value must be between 0 and 180.` e la domanda
  resta.
- **`!polar on|off`:** accende o spegne il polare senza scrivere nella
  cronologia. `!polar on` spegne l'orto e `!ortho on` spegne il polare.

## Regole della tappa che valgono qui

- `command_runner.cpp`: prima di te P7, dopo di te P10.
- `builtin_commands.cpp`, `command_descriptions.cpp`: prima di te P6, dopo di te
  P16.
- `script.*`, `level1.cpp`, `script_session.cpp`: prima di te P7.

## Test richiesti

- `polarPoint`: con 90°, 45° e 7°; puntatore sul raggio, a 9 e a 11 pixel, vicino
  al punto base, a metà tra due raggi.
- Runner: snap che vince sul polare; polare con distanza scritta; nessun punto
  base (primo punto di un comando); `POLARANG` cambiato.
- `!polar` e `!ortho` che si escludono.

## Fatto quando

- I casi di `[0.7-R8]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
