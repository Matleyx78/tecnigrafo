# [0.7-P22] `core`: unione di entità contigue

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R22]` (#…), agente claude (calcoli delicati:
  tolleranze)
- **Dipende da:** Matleyx78/solid-space#23 (la sua tappa chiusa), P14, R22; ADR
  0036 accettato
- **Modulo:** `core` (più la domanda `join` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 13–15; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md); la
  richiesta [#329](https://github.com/Matleyx78/tecnigrafo/issues/329) e l'ADR di
  valutazione di solid-space che le risponde; [dxf-di-prova.md](dxf-di-prova.md)
  (`autocad-unione.dxf`)

## Prima di cominciare

Come per P14: se solid-space non offre la regola di coincidenza, allineamento e
concentricità, fermati come dice AGENTS.md.

## File

- `core/include/tg/core/join.hpp`, `core/src/join.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/join_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R22)

## Cosa fare

```cpp
/// The result of joining (docs/adr/0035, point 13). `source` is the source
/// entity as it becomes, when its type does not change; otherwise empty and
/// `created` holds the new entity. `consumed` lists the indexes, into the
/// input, of the entities that the result absorbs (the source excluded).
struct JoinResult {
    std::optional<Entity> source;
    std::optional<Entity> created;
    std::vector<std::size_t> consumed;
};

/// JOIN of `entities`, with entities[0] as source: collinear lines into a
/// line, arcs of one circle into an arc, contiguous lines, arcs and
/// polylines into a polyline. std::nullopt when nothing joins.
[[nodiscard]] std::optional<JoinResult> join(std::span<const Entity> entities);

/// The script sequence (docs/adr/0035, point 14): only what the source's type
/// accepts (a line: collinear lines; an arc: arcs of its circle; a polyline:
/// contiguous lines, arcs and polylines).
[[nodiscard]] std::optional<JoinResult> joinToSource(std::span<const Entity> entities);
```

- Coincidenza degli estremi, allineamento e stesso cerchio vengono da
  solid-space, con la regola dell'ADR 0036. Il vertice comune prende il punto
  dell'entità che segue.
- Linee allineate e archi dello stesso cerchio si uniscono anche con spazi tra
  loro (`autocad-unione.dxf`, casi b ed e1).
- La domanda `join` del catalogo (la sequenza interattiva).

## Regole della tappa che valgono qui

- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: solo le tue
  righe.

## Test richiesti

- I casi di `autocad-unione.dxf`.
- Polilinee aperte unite tra loro e con linee in testa e in coda; una catena che
  si chiude; entità in ordine sparso; archi che coprono il cerchio intero.

## Fatto quando

- I casi di `[0.7-R22]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
