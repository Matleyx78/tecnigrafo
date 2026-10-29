# [0.7-P7] `core`: nuovi tipi di snap, quadrante e perpendicolare, `!osnap`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.7-R7]` (#…), agente gemini
- **Dipende da:** P3, R7
- **Modulo:** `core` (più i marcatori in `render`, il punto base nel runner, la
  direttiva in `script` e `app`)
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 1, 2, 4–7; [ADR 0008](../../adr/0008-selezione-e-snap.md) punto 7;
  `core/include/tg/core/snap_points.hpp`, `core/src/snap_points.cpp`,
  `core/src/document_query.cpp`; `render/src/scene_renderer.cpp`
  (`renderSnapMarker`); `docs/verifica/domande.md` (`snaps`, `snap`) e
  `docs/verifica/script.md` (`!osnap`)

## File

- `core/include/tg/core/snap_points.hpp`, `core/src/snap_points.cpp`
- `core/include/tg/core/document_query.hpp`, `core/src/document_query.cpp`
- `render/src/scene_renderer.cpp` (i marcatori)
- `cmd/src/command_runner.cpp` (il punto base a `findSnap`)
- `script/include/tg/script/script.hpp`, `script/src/script.cpp`,
  `script/src/level1.cpp`, `script/src/questions.cpp`
- `app/src/script_session.cpp` (`!osnap` al livello 2)
- `tests/core/snap_points_test.cpp`, `tests/core/document_query_test.cpp`,
  `tests/render/scene_renderer_test.cpp`, `tests/script/level1_test.cpp`
- `tests/verifica/in-attesa.txt` (togliere le righe di R7)

## Cosa fare

```cpp
enum class SnapKind : std::uint8_t {
    Endpoint, Midpoint, Center, Insertion, Intersection, Perpendicular, Tangent, Quadrant
};
struct SnapModes {
    bool endpoint{true};
    bool midpoint{true};
    bool center{true};
    bool insertion{true};
    bool intersection{true};   // docs/adr/0033, point 6
    bool perpendicular{false};
    bool tangent{false};
    bool quadrant{false};
    // ...
};

/// docs/adr/0033, point 5. `from` is the base point of the running command;
/// without it Perpendicular and Tangent give nothing.
[[nodiscard]] std::optional<SnapPoint> findSnap(const Document& document, const Vec2& pointer,
                                                double aperture, const SnapModes& modes,
                                                std::optional<Vec2> from = std::nullopt);
```

- **Quadrante** in `appendSnapPoints`: cerchi, archi e archi delle polilinee,
  solo i quadranti dentro l'arco (`containsAngle`).
- **Perpendicolare** in `findSnap`: il piede della perpendicolare da `from` alle
  curve vicine al puntatore, solo se cade sulla curva. Per gli archi è il punto
  sulla retta dal centro a `from`, se cade sull'arco.
- **Priorità** dell'ADR 0033, punto 5.
- **Intersezione e tangente:** i modi esistono, ma `findSnap` non dà ancora
  punti di questi tipi; li porteranno P15 e P19.
- **Runner:** passa `Command::basePoint()` come `from`.
- **Marcatori** in `renderSnapMarker` per i quattro tipi nuovi, come li descrive
  l'ADR 0033, punto 1.
- **`script`:**
  - la direttiva `!osnap` (sintassi in `script.md`), che imposta i modi senza
    toccare `!snap`;
  - le domande `snaps` e `snap` con i nomi nuovi di `modes=`, e `from=` per
    `snap`;
  - **senza `modes=` le domande usano i quattro modi della 0.4**, non i valori
    predefiniti di `SnapModes`.
- **`app`:** `!osnap` al livello 2.

## Regole della tappa che valgono qui

- `snap_points.*` e `document_query.cpp` li toccano poi P15 e P19;
  `command_runner.cpp` P8 e P10; `scene_renderer.cpp` P11; `script.*`,
  `level1.cpp` e `script_session.cpp` P8.
- La regola di "cade sulla curva" per il piede della perpendicolare usa
  `containsAngle` e il confronto dei parametri che `core` usa già: nessuna
  soglia nuova (AGENTS.md, "Se serve qualcosa a solid-space").

## Test richiesti

- Quadranti di un cerchio, di un arco che ne contiene due, di un arco di
  polilinea.
- Perpendicolare a linee, archi e segmenti di polilinea; piede fuori dalla curva;
  senza `from`.
- Priorità tra estremo, medio, quadrante e perpendicolare alla stessa distanza;
  il centro quando non c'è altro.
- Un marcatore per ogni tipo nuovo nel painter di prova.
- `!osnap` con modi validi, `none`, un nome sbagliato.

## Fatto quando

- I casi di `[0.7-R7]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`; i casi esistenti passano ancora.
- La CI è verde.
