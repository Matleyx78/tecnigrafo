# [0.7-P14] `core`: intersezioni e lato di un punto; sottomodulo solid-space

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R14]` (#…), agente gemini 4 Argon (calcoli
  delicati)
- **Dipende da:** Matleyx78/solid-space#20 (la sua tappa chiusa), R14
- **Modulo:** `core` (più il sottomodulo, la CI e le domande in `script`)
- **Da leggere:** [ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md)
  punti 2–10; la richiesta
  [#326](https://github.com/Matleyx78/tecnigrafo/issues/326) e l'ADR di
  valutazione di solid-space che le risponde; le intestazioni pubbliche di
  `solid_space::robust` alla versione da fissare;
  [ADR 0012](../../adr/0012-linea-cerchio-arco.md);
  `core/include/tg/core/arc_geometry.hpp`, `polyline.hpp`;
  `docs/verifica/domande.md` (`intersections`, `side`)

## Prima di cominciare

Questa scheda fissa l'interfaccia di Tecnigrafo, non quella di solid-space.
Controlla che ciò che solid-space offre alla versione da fissare permetta di
realizzarla: decisioni di intersezione, tangenza, sovrapposizione e lato, con
punti entro un errore dichiarato. Se non lo permette, **fermati** come dice
AGENTS.md ("Se serve qualcosa a solid-space"): scrivi nella issue che cosa
manca, senza scriverlo tu.

## File

- `.gitmodules`, `third_party/solid-space` (sottomodulo, fissato al commit o
  al tag indicato nella issue), `third_party/CMakeLists.txt`
- `.github/workflows/ci.yml` (`submodules: true` in ogni `actions/checkout` che
  compila, **non** `recursive`: i sottomoduli di solid-space non servono)
- `core/CMakeLists.txt` (`solid_space::robust`, privato)
- `core/include/tg/core/intersections.hpp`, `core/src/intersections.cpp` (nuovi)
- `script/src/curve_questions.cpp` (nuovo), `script/src/question_table.cpp`,
  `script/CMakeLists.txt`
- `tests/core/intersections_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `docs/AMBIENTE-SVILUPPO.md` (clonare con il sottomodulo)
- `tests/verifica/in-attesa.txt` (togliere le righe di R14)

## Cosa fare

**Sottomodulo** (ADR 0030, punti 6–8):

- `third_party/solid-space`, aggiunto con
  `add_subdirectory(... EXCLUDE_FROM_ALL)` e `SOLID_SPACE_BUILD_KERNEL` spento,
  così si compila solo ciò che `core` collega e il 2D non richiede OCCT;
- le intestazioni di solid-space non come `SYSTEM`;
- i test di solid-space non si compilano: li esegue la sua CI;
- un clone senza il sottomodulo inizializzato dà un errore di configurazione
  chiaro, con il comando per inizializzarlo.

**Facciata** (`intersections.hpp`):

```cpp
/// Where two curves meet. Stations are lengths along an entity from its
/// start: a line from start, an arc from its start angle counter-clockwise, a
/// circle from angle 0, a polyline along its segments in order. On an
/// extended curve (Extension::Full) a station may be negative or beyond the
/// length: before the start or after the end.
struct Crossing {
    Vec2 point;
    double stationA{0.0};
    double stationB{0.0};
    bool tangent{false};
};
/// A stretch the two curves share.
struct SharedStretch {
    Vec2 from;
    Vec2 to;
    double fromA{0.0}, toA{0.0}, fromB{0.0}, toB{0.0};
};
struct Intersections {
    std::vector<Crossing> crossings;     ///< by stationA, then stationB
    std::vector<SharedStretch> shared;   ///< by fromA
};
enum class Extension : std::uint8_t { None, Full };  ///< Full: line → infinite line, arc → circle

/// docs/adr/0033 and 0035. A text has no curve: empty. `a` and `b` may be
/// the same polyline: then its self-crossings, without the shared vertex of
/// two consecutive segments. Extension applies to Line and Arc only.
[[nodiscard]] Intersections intersect(const Entity& a, const Entity& b,
                                      Extension extendA = Extension::None,
                                      Extension extendB = Extension::None);

enum class Side : std::uint8_t { Left, Right, On };
/// The side of `point` looking along the curve at its nearest point; for a
/// polyline the first nearest segment. Precondition: not a Text.
[[nodiscard]] Side sideOf(const Entity& entity, const Vec2& point);

/// The station of the point of the curve nearest to `point` (closestPoint).
[[nodiscard]] double stationOf(const Entity& entity, const Vec2& point);
```

- Ogni decisione (si intersecano, sono tangenti, si sovrappongono, da che parte
  sta un punto) e ogni punto vengono da `solid_space::robust`. La facciata
  traduce soltanto: dalle entità alle curve di solid-space e dalle risposte alle
  stazioni. Nessuna soglia nuova in `core`.
- Le domande `intersections` e `side` del catalogo, nel nuovo
  `curve_questions.cpp`.

## Regole della tappa che valgono qui

- Le modifiche a `.gitmodules`, `third_party/`, `cmake/` se servono e `.github/`
  sono autorizzate da questa scheda (ADR 0030, punto 8) e da nessun'altra.
- `core/CMakeLists.txt`, `question_table.cpp`, `script/CMakeLists.txt`: solo le
  tue righe.

## Test richiesti

- Ogni coppia tra linea, arco, cerchio e segmenti di polilinea: incroci,
  tangenze, nessun incontro, sovrapposizioni; con e senza `Extension::Full`.
- Polilinea che si interseca, polilinea con due segmenti consecutivi.
- I casi concreti della richiesta #326, anche a coordinate 1e6.
- `sideOf` e `stationOf` su ogni tipo di curva.

## Fatto quando

- I casi di `[0.7-R14]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde su tutti i sistemi, con il sottomodulo.
