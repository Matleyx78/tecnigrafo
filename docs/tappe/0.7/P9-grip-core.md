# [0.7-P9] `core`: grip

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R9]` (#…), agente gemini, fascia forte
  (calcoli delicati; valori da `autocad-grip.dxf`)
- **Dipende da:** P4, R9
- **Modulo:** `core` (più le domande `grips` e `grip` in `script`)
- **Da leggere:** [ADR 0034](../../adr/0034-grip.md) punti 2, 4 e 5;
  [ADR 0012](../../adr/0012-linea-cerchio-arco.md) punto 4;
  [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md) punto 5;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-grip.dxf`);
  `core/include/tg/core/arc.hpp` (`arcThroughPoints`), `polyline.hpp`, `text.hpp`;
  `core/include/tg/core/similarity.hpp` (P4)

## File

- `core/include/tg/core/grips.hpp`, `core/src/grips.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/modify_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/grips_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R9)

## Cosa fare

```cpp
/// The grips of an entity, in this order (docs/adr/0034, point 2):
/// - Line: start, end, midpoint;
/// - Arc: start, end, midpoint on the arc, centre;
/// - Circle: centre, then the quadrants at 0, 90, 180 and 270 degrees;
/// - Polyline: for each vertex, the vertex and then the midpoint of the
///   segment that starts there (on the arc for a bulged segment); an open
///   polyline has no segment after its last vertex;
/// - Text: the insertion snap point (snap_points.hpp); for Aligned and Fit,
///   position and then alignment point.
[[nodiscard]] std::vector<Vec2> gripPoints(const Entity& entity);

enum class GripOutcome : std::uint8_t { Moved, Invalid };

struct GripResult {
    GripOutcome outcome{GripOutcome::Invalid};
    Entity entity;  ///< the moved entity when outcome is Moved, else the input
};

/// The entity after its grip number `index` (into gripPoints()) is moved to
/// `target` (docs/adr/0034, points 4 and 5). Precondition: index is valid.
[[nodiscard]] GripResult moveGrip(const Entity& entity, std::size_t index, const Vec2& target);
```

- Regole dell'ADR 0034, punto 4, con la costruzione per tre punti di
  `arcThroughPoints`:
  - l'estremo di un arco porta l'arco per il nuovo punto, l'altro estremo e il
    medio di prima;
  - il medio di un arco porta l'arco per i due estremi e il nuovo punto;
  - un vertice di polilinea si sposta, e ogni segmento curvo che vi arriva passa
    per il suo medio di prima, con il bulge ricalcolato;
  - il medio di un segmento dritto sposta i suoi due vertici, e i segmenti
    curvi accanto seguono la regola del vertice;
  - il medio di un segmento curvo dà il bulge per il nuovo punto.
- Un *Aligned* con il secondo punto spostato prende l'altezza che l'allineamento
  dà (`textFrame`); un *Fit* tiene l'altezza.
- Risultato degenere (tre punti allineati per la regola di oggi, raggio nullo,
  estremi coincidenti): `Invalid`.
- Le domande `grips` e `grip` del catalogo. `grip` cerca il grip che sta in
  `x1,y1` entro 1e-9; se non c'è, risponde `none`.

## Regole della tappa che valgono qui

- `core/CMakeLists.txt`, `question_table.cpp`: solo le tue righe;
  `modify_questions.cpp` l'ha creato P4.
- La decisione "allineati" è quella di `areCollinear` (ADR 0012): nessuna soglia
  nuova.

## Test richiesti

- `gripPoints` di ogni entità, polilinee aperte e chiuse, con archi; testi
  *Left*, *Aligned*, *Fit*.
- `moveGrip` per ogni riga della tabella dell'ADR 0034, punto 4, con i casi di
  `autocad-grip.dxf`.
- Risultati degeneri: arco con il medio portato sulla corda, cerchio con il
  quadrante sul centro, estremo di linea sull'altro estremo.

## Fatto quando

- I casi di `[0.7-R9]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
