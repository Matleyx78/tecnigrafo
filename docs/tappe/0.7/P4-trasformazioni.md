# [0.7-P4] `core`: trasformazioni delle entità

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #340
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R4]` (#339), agente claude (calcoli delicati: testo
  riflesso, archi, bulge)
- **Dipende da:** R4
- **Modulo:** `core` (più la domanda `transform` in `script`)
- **Da leggere:** [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 1–4;
  [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md) punti 2–5;
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
  Per il testo riflesso fa fede il punto 3 completo (precisazione #406):
  - usare la direzione effettiva di `textFrame`, anche per *Aligned* e *Fit*,
    normalizzare gli angoli e applicare la tabella di `MIRRTEXT` 0/1;
  - ricostruire entrambi i punti di *Aligned* e *Fit* con direzione e distanza
    prescritte; per gli altri allineamenti ricollocare solo l'ancoraggio;
    usare il centro dei quattro vertici per collocare l'ingombro nel bersaglio;
  - conservare la giustificazione; invertire il segnale "all'indietro" solo
    con `Mirrored`, conservare "capovolto" e invertire l'obliquo solo con
    `Readable`; un testo già specchiato mantiene quei segnali con `Readable`;
  - conservare altezza memorizzata (scalata) e fattore di larghezza;
    trattare esplicitamente i casi degeneri dell'ADR 0022;
  - non copiare i punti grezzi di AutoCAD quando, con la semantica dell'ADR
    0022, darebbero un ingombro diverso dalla riflessione. Non usare lo scambio
    incondizionato dei punti né il solo gruppo 50 per orientare *Aligned/Fit*.
- Gli angoli degli archi si riportano in [0, 2π) e non diventano mai uguali.
- La domanda `transform` del catalogo, nel nuovo file `modify_questions.cpp`, con
  le risposte `invalid` dove il catalogo le prevede. Sostituire il segnaposto
  `nullptr` già registrato in `question_table.cpp`, senza duplicare la riga.

## Regole della tappa che valgono qui

- `core/CMakeLists.txt`, `question_table.cpp`, `script/CMakeLists.txt`: solo le
  tue righe. `modify_questions.cpp` lo ampliano poi P9 (grip); P14, P17, P19, P21
  e P22 aggiungono le loro in `curve_questions.cpp`.
- Tolleranze: quelle del confronto (1e-9), salvo `!tolerance` nei casi.

## Test richiesti

- Ogni entità con ognuna delle quattro trasformazioni e con una composizione.
- Arco riflesso (angoli scambiati, sempre antiorario); polilinea riflessa (bulge
  di segno opposto) e scalata (larghezze).
- Testo riflesso con `MIRRTEXT` 0 e 1, sui due assi e su un asse a 45°, almeno
  con le giustificazioni sinistra, centro, destra, *Aligned* e *Fit*.
- Con `Mirrored`, i quattro vertici di `textBox` devono essere le immagini
  degli originali nello stesso ordine; con `Readable`, lo stesso insieme di
  vertici, senza imporre l'ordine. Controllare anche direzione effettiva e
  lunghezza della linea di base di *Aligned/Fit* e i punti non d'ancoraggio.
- Testo obliquo positivo e negativo, già all'indietro o capovolto (anche con
  entrambi i segnali), *Aligned/Fit* con gruppo 50 diverso dalla direzione
  dei punti, punti coincidenti e testo vuoto. Confini 90° e 270° inclusi
  e valori appena prima e dopo; similitudine che riflette, scala e trasla.
- `scaling` con 0, negativo, infinito; `reflection` con punti coincidenti.

## Fatto quando

- R4 è aggiornato alla precisazione #406 dell'ADR 0031 (revisione della PR
  #404 da parte dell'autore dei riferimenti). Non modificare quei casi in P4.
- I casi di `[0.7-R4]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
