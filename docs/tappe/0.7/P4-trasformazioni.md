# [0.7-P4] `core`: trasformazioni delle entità

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R4]` (#…), agente claude (calcoli delicati: testo
  riflesso, archi, bulge)
- **Dipende da:** R4
- **Modulo:** `core` (più la domanda `transform` in `script`)
- **Da leggere:** [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 1–4;
  [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md) punti 4 e 5;
  [ADR 0012](../../adr/0012-linea-cerchio-arco.md);
  `docs/verifica/domande.md` ("Modifiche (tappa 0.7)", `transform`);
  `core/include/tg/core/transform2d.hpp`, `text.hpp`, `arc.hpp`, `polyline.hpp`

## File

- `core/include/tg/core/similarity.hpp`, `core/src/similarity.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/modify_questions.cpp` (nuovo), `script/src/question_table.cpp`,
  `script/src/question_table.hpp` se serve, `script/CMakeLists.txt`
- `tests/core/similarity_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R4)

## Cosa fare

```cpp
/// A similarity of the plane (docs/adr/0031, point 1): p' = s·R(θ)·F·p + t,
/// with F the identity or the reflection across the X axis.
class Similarity {
public:
    Similarity() = default;  ///< the identity
    [[nodiscard]] static Similarity translation(const Vec2& offset);
    [[nodiscard]] static Similarity rotation(double radians, const Vec2& center);
    /// std::nullopt unless factor is finite and > 0.
    [[nodiscard]] static std::optional<Similarity> scaling(double factor, const Vec2& center);
    /// Reflection across the line through a and b; std::nullopt if they coincide.
    [[nodiscard]] static std::optional<Similarity> reflection(const Vec2& a, const Vec2& b);

    [[nodiscard]] Vec2 apply(const Vec2& p) const;
    [[nodiscard]] double scale() const;        ///< s
    [[nodiscard]] double rotationAngle() const;  ///< θ, radians
    [[nodiscard]] bool reflects() const;        ///< F is the reflection
    [[nodiscard]] Transform2D matrix() const;

    /// (a * b).apply(p) == a.apply(b.apply(p))
    friend Similarity operator*(const Similarity& a, const Similarity& b);
};

/// MIRRTEXT (docs/adr/0031, point 3).
enum class MirrorText : std::uint8_t { Readable, Mirrored };  ///< 0, 1

/// `entity` transformed by `similarity` (docs/adr/0031, points 2-4).
[[nodiscard]] Entity transformed(const Entity& entity, const Similarity& similarity,
                                 MirrorText mirrorText = MirrorText::Readable);
```

- Le regole per ogni entità sono quelle della tabella dell'ADR 0031, punto 2.
  Il testo riflesso segue il punto 3:
  - con `Mirrored` i punti si riflettono, il bit "all'indietro" si inverte e la
    rotazione diventa `2φ − r + π`;
  - con `Readable` si riflette il parallelogramma di `textBox` e il testo si
    riscrive dentro di esso, leggibile, con rotazione `2φ − r`, a cui si tolgono
    `π` quando cade in (π/2, 3π/2]. La giustificazione resta; per *Aligned* e
    *Fit* i due punti riflessi si scambiano.
- Gli angoli degli archi si riportano in [0, 2π) e non diventano mai uguali.
- La domanda `transform` del catalogo, nel nuovo file `modify_questions.cpp`, con
  le risposte `invalid` dove il catalogo le prevede.

## Regole della tappa che valgono qui

- `core/CMakeLists.txt`, `question_table.cpp`, `script/CMakeLists.txt`: solo le
  tue righe. `modify_questions.cpp` lo ampliano poi P9 (grip); P14, P17, P19 e
  P22 aggiungono le loro in `curve_questions.cpp`.
- Tolleranze: quelle del confronto (1e-9), salvo `!tolerance` nei casi.

## Test richiesti

- Ogni entità con ognuna delle quattro trasformazioni e con una composizione.
- Arco riflesso (angoli scambiati, sempre antiorario); polilinea riflessa (bulge
  di segno opposto) e scalata (larghezze).
- Testo riflesso con `MIRRTEXT` 0 e 1, sui due assi e su un asse a 45°, almeno
  con le giustificazioni sinistra, centro, destra, *Aligned* e *Fit*.
- `scaling` con 0, negativo, infinito; `reflection` con punti coincidenti.

## Fatto quando

- I casi di `[0.7-R4]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
