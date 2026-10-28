# [0.7-P17] `core`: curva parallela

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R17]` (#…), agente gemini 4 Argon (calcoli
  delicati; valori da `autocad-offset.dxf`)
- **Dipende da:** Matleyx78/solid-space#22 (la sua tappa chiusa), P14, R17
- **Modulo:** `core` (più la domanda `offset` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 8 e 9; la richiesta
  [#328](https://github.com/Matleyx78/tecnigrafo/issues/328) e l'ADR di
  valutazione di solid-space che le risponde; [dxf-di-prova.md](dxf-di-prova.md)
  (`autocad-offset.dxf`); `core/include/tg/core/intersections.hpp` (P14)

## Prima di cominciare

Come per P14: questa scheda fissa l'interfaccia di Tecnigrafo. Se solid-space ha
risposto alla richiesta #328 con l'esito 4 (la curva parallela spetta a
Tecnigrafo), l'algoritmo si scrive qui, sopra le decisioni di
`solid_space::robust`, e il piano lo annota in "Modifiche al piano" prima che il
pacchetto parta. Se manca una decisione che serve, fermati come dice AGENTS.md.

## File

- `core/include/tg/core/offset.hpp`, `core/src/offset.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/offset_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R17)

## Cosa fare

```cpp
enum class OffsetOutcome : std::uint8_t { Done, Vanished, Failed };

struct OffsetResult {
    OffsetOutcome outcome{OffsetOutcome::Failed};
    std::optional<Entity> entity;  ///< with Done: same type as the input
};

/// The curve at `distance` (> 0) from `entity` on the side of `sidePoint`
/// (docs/adr/0035, points 8-9). Vanished: an arc or circle that would have a
/// radius <= 0, or a polyline with nothing left. Failed: a polyline outside
/// the simple cases that cannot be made without self-intersections.
/// Precondition: not a Text; distance finite and > 0.
[[nodiscard]] OffsetResult offsetCurve(const Entity& entity, double distance,
                                       const Vec2& sidePoint);
```

- Il lato si decide con `sideOf` (P14).
- Giunzioni con `OFFSETGAPTYPE` = 0, come in `autocad-offset.dxf`:
  - segmenti prolungati fino a incontrarsi;
  - dove i prolungamenti non si incontrano, un arco di raggio `distance` con il
    centro nel vertice;
  - un segmento curvo che si annulla sparisce, e le giunzioni accanto si chiudono
    allo stesso modo.
- Garanzie dell'ADR 0035, punto 9: nei casi semplici sempre `Done`, senza
  autointersezioni; negli altri `Done` senza autointersezioni oppure `Failed`.
- La polilinea risultato tiene chiusura e larghezza costante; le larghezze dei
  segmenti si conservano per i segmenti che restano.
- La domanda `offset` del catalogo (`cannot` per `Vanished` e `Failed`).

## Regole della tappa che valgono qui

- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: solo le tue
  righe.
- Nessuna soglia propria per "si annulla" o "si interseca": le decisioni vengono
  da solid-space.

## Test richiesti

- Linea, cerchio e arco dai due lati; arco e cerchio che si annullano.
- I casi di `autocad-offset.dxf`.
- Polilinee semplici aperte e chiuse con archi; un caso non semplice che dà
  `Failed`; coordinate 1e6.

## Fatto quando

- I casi di `[0.7-R17]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
