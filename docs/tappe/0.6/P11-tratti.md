# [0.6-P11] `core`: tratti dei tipi di linea

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.6-R11]` (#…), agente claude (calcoli delicati)
- **Dipende da:** P5, P9, P10, R11
- **Modulo:** `core` (più la domanda `dashes` in `script`)
- **Da leggere:** [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti
  1, 3, 4 e 5; `docs/verifica/domande.md` (`dashes`);
  `core/include/tg/core/arc_geometry.hpp`, `polyline.hpp`, `arc.hpp`,
  `circle.hpp`; `core/include/tg/core/effective_properties.hpp` (P9)

## File

- `core/include/tg/core/dashes.hpp`, `core/src/dashes.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/property_questions.cpp`, `script/src/question_table.cpp` (la
  riga di `dashes`)
- `tests/core/dashes_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R11)

## Cosa fare

```cpp
struct DashLine { Vec2 start; Vec2 end; };
struct DashArc { Vec2 center; double radius; double startAngle; double endAngle; };  ///< CCW, radians
struct DashPoint { Vec2 position; Vec2 direction; };  ///< unit tangent at the point
using DashPiece = std::variant<DashLine, DashArc, DashPoint>;

/// More pieces than this and the entity is drawn continuous (ADR 0025, point 5).
inline constexpr std::size_t kMaxDashPieces = 100'000;

/// The pieces of `entity` with `pattern` scaled by `scale` (docs/adr/0025,
/// point 4), in drawing coordinates, in order along the entity.
/// std::nullopt means "draw it continuous": continuous pattern, entity (or,
/// without continuous generation, every segment) shorter than one pattern,
/// more than kMaxDashPieces pieces, or a TEXT.
[[nodiscard]] std::optional<std::vector<DashPiece>> dashes(const Entity& entity,
                                                           const LinetypePattern& pattern,
                                                           double scale);
```

- Regole dell'ADR 0025, punto 4: il motivo parte dall'inizio; `LINE` dal
  primo punto; `ARC` dall'angolo iniziale in senso antiorario; `CIRCLE`
  dall'angolo 0; `LWPOLYLINE` riparte da ogni vertice, salvo
  `Polyline::continuousLinetype()` (bit 128 del gruppo 70, portato nel modello
  da P10), segue i bulge, e un segmento più corto
  del motivo si disegna continuo come pezzo intero.
- Un tratto che attraversa un vertice di una polilinea con generazione
  continua si spezza in due pezzi, uno per segmento.
- Un tratto di lunghezza 0 (dopo il troncamento) non si emette; un punto sì.
- La domanda `dashes <E>` usa il tipo e la scala effettivi (P9) e risponde
  nel formato di `domande.md`.

## Regole della tappa che valgono qui

- Tolleranze: quelle del confronto (1e-9) salvo `!tolerance` nei casi.
- `core/CMakeLists.txt`, `question_table.cpp`: solo le tue righe.

## Test richiesti

- Linea più lunga, uguale e più corta del motivo; motivo che finisce con uno
  spazio o con un punto; scala diversa da 1.
- Arco, cerchio, arco più corto del motivo.
- Polilinea con archi, con e senza generazione continua, segmento corto.
- Oltre `kMaxDashPieces`.

## Fatto quando

- I casi di `[0.6-R11]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
