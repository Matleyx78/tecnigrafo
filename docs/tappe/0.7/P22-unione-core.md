# [0.7-P22] `core`: unione di entità contigue

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R22]` (#…), agente gemini: la facciata (catena,
  ordine, entità che risultano, unità); i confini della regola dall'archivio
  dei casi di solid-space
- **Dipende da:** P14, Matleyx78/solid-space#23, R22. La richiesta diventa il
  pacchetto di solid-space che realizza `coincidence.hpp`, quando la tappa 0.2
  di solid-space lo avrà (ADR 0030, punto 13)
- **Modulo:** `core` (più la domanda `join` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2, 17–20; [ADR 0036](../../adr/0036-tolleranze-di-tecnigrafo.md)
  punti 1, 2 e 5; [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punto 7;
  l'[ADR 0009 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0009-regola-delle-tolleranze-del-piano.md)
  (Decisione); in `third_party/solid-space` l'intestazione qui sotto;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-unione.dxf`);
  `core/include/tg/core/tolerance.hpp` e `core/src/solid_space_bridge.hpp` (P14)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0009 di solid-space:
`solid_space/geom2d/coincidence.hpp` (`areAligned`, `onSameCircle`,
`isStraight`) e `solid_space/robust/tolerance.hpp` (`coincident`), con i tipi
di `curves.hpp`.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude il pacchetto di solid-space che realizza
  `coincidence.hpp`. Se P14 l'ha già portato lì o oltre, il sottomodulo non si
  sposta. Se la issue non lo dice, chiedilo e non cominciare.
- Se qualcosa che serve qui manca dal contratto o non si comporta come dichiara,
  **fermati** come dice AGENTS.md ("Se serve qualcosa a solid-space").

## File

- `third_party/solid-space` (il sottomodulo, se la issue lo dice)
- `core/include/tg/core/join.hpp`, `core/src/join.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/join_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R22)

## Cosa fare

```cpp
/// The result of JOIN (docs/adr/0035, points 17-20). `source` is the source
/// entity as it becomes, when its type does not change; otherwise empty, and
/// `created` holds the new entity. `consumed` lists the indexes, into the
/// input, of the entities the result absorbs (the source excluded).
struct JoinResult {
    std::optional<Entity> source;
    std::optional<Entity> created;
    std::vector<std::size_t> consumed;
};

/// JOIN of `entities`, entities[0] the source, the others in the order they
/// were chosen: aligned lines into a line, arcs of one circle into an arc,
/// contiguous lines, arcs and polylines into a polyline. std::nullopt when
/// nothing joins.
[[nodiscard]] std::optional<JoinResult> join(std::span<const Entity> entities,
                                             const ToleranceRule& rule);

/// The script sequence (docs/adr/0035, point 19): only what the source's type
/// accepts (a line: aligned lines; an arc: arcs of its circle; a polyline:
/// contiguous lines, arcs and polylines). `close` (arcs only) closes the arc
/// into a circle.
[[nodiscard]] std::optional<JoinResult> joinToSource(std::span<const Entity> entities,
                                                     const ToleranceRule& rule,
                                                     bool close = false);
```

- **La regola** è quella del documento (ADR 0036): "allineate" è `areAligned`
  delle due linee, "stesso cerchio" è `onSameCircle` dei due archi, "si
  toccano" è `coincident` dei due estremi. Ogni linea e ogni arco si confronta
  con la sorgente, non con l'ultima entità aggiunta (ADR 0035, punto 18).
- **Linee allineate:** la linea unita va dall'estremo della prima all'estremo
  dell'ultima lungo la sorgente, anche con spazi (`autocad-unione.dxf`, casi b
  ed e1). **Archi dello stesso cerchio:** l'arco unito ha centro e raggio della
  sorgente e copre gli archi in senso antiorario a partire dalla sorgente; se
  li copre tutti fino a richiudersi, è un cerchio.
- **Catena** (ADR 0035, punto 18), per tutto il resto: parte dalla sorgente,
  cresce prima dalla sua fine e poi dal suo inizio; a ogni passo si aggiunge la
  prima entità, nell'ordine di `entities`, con un estremo coincidente con
  l'estremo libero della catena, girata se serve; ci si ferma quando nessuna si
  aggiunge. Due estremi che coincidono diventano un vertice, nel punto
  dell'entità che segue. La polilinea risultante:
  - è chiusa se l'ultimo estremo coincide con il primo;
  - ha segmenti dritti dove `isStraight` dice che il bulge vale zero;
  - prende larghezze e proprietà delle polilinee unite per i loro segmenti, e
    larghezza 0 per linee e archi.
- La domanda `join` del catalogo (la sequenza interattiva), con le risposte
  scritte lì.

## Regole della tappa che valgono qui

- Il sottomodulo lo sposti solo al commit che indica la issue (ADR 0030, punto
  9).
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P17, P19 e P21).
- Nessuna soglia propria: coincidenze, allineamenti e stesso cerchio vengono da
  solid-space con la regola del documento.

## Test richiesti

- I casi di `autocad-unione.dxf`.
- Polilinee aperte unite tra loro e con linee in testa e in coda; una catena che
  si chiude; entità in ordine sparso; archi che coprono il cerchio intero;
  estremi a `1e-10` in millimetri (coincidono) e in un disegno in metri;
  tre estremi dove a ~ b e b ~ c ma non a ~ c (l'ordine scritto decide).

## Fatto quando

- I casi di `[0.7-R22]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
