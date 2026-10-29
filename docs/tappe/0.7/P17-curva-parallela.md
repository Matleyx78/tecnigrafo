# [0.7-P17] `core`: curva parallela

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R17]` (#…), agente gemini: la facciata (lato,
  esiti, polilinee, larghezze, unità); i valori geometrici dall'archivio dei
  casi di solid-space
- **Dipende da:** P14, Matleyx78/solid-space#22, R17. La richiesta diventa il
  pacchetto di solid-space che realizza `offset.hpp`, quando la tappa 0.2 di
  solid-space lo avrà (ADR 0030, punto 13)
- **Modulo:** `core` (più la domanda `offset` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 2, 10–12; l'[ADR 0012 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0012-curva-parallela-nel-piano.md)
  (Decisione); in `third_party/solid-space` l'intestazione qui sotto;
  [dxf-di-prova.md](dxf-di-prova.md) (`autocad-offset.dxf`);
  `core/include/tg/core/intersections.hpp`, `tolerance.hpp` e
  `core/src/solid_space_bridge.hpp` (P14)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0012 di solid-space:
`solid_space/geom2d/offset.hpp` (`offsetCurve`, `offsetPolyline`,
`CurveOffset`, `PolylineOffset`, `OffsetOutcome`, `OffsetFailure`,
`OffsetPiece`, `PieceOrigin`), con i tipi di `curves.hpp` e `side.hpp`.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude il pacchetto di solid-space che realizza
  `offset.hpp`. Se la issue non lo dice, chiedilo e non cominciare.
- Se qualcosa che serve qui manca dal contratto o non si comporta come dichiara,
  **fermati** come dice AGENTS.md ("Se serve qualcosa a solid-space").

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/offset.hpp`, `core/src/offset.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/offset_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R17)

## Cosa fare

```cpp
enum class OffsetOutcome : std::uint8_t {
    Done,      ///< `entity` holds the copy
    Vanished,  ///< nothing is left at that distance
    Failed,    ///< a polyline copy that cannot be made without self-intersections
    NoSide,    ///< the side point lies exactly on the line or circle
};

struct OffsetResult {
    OffsetOutcome outcome{OffsetOutcome::Failed};
    std::optional<Entity> entity;  ///< with Done: the same type as the input
    /// With Done for a polyline: solid-space's simple case (its ADR 0012, point 6).
    bool simple{false};
};

/// OFFSET (docs/adr/0035, points 10-12): the curve at `distance` (> 0) from
/// `entity`, on the side of `sidePoint`.
/// Precondition: not a Text; distance finite and > 0.
[[nodiscard]] OffsetResult offsetEntity(const Entity& entity, double distance,
                                        const Vec2& sidePoint, const ToleranceRule& rule);
```

- **Lato** (ADR 0035, punto 10): `sideOf(entity, sidePoint)` (P14), che per una
  polilinea usa il segmento più vicino al punto. `Left` dà la distanza
  positiva, `Right` negativa, `On` dà `NoSide` senza chiamare solid-space.
  Per un segmento di polilinea il lato è rispetto a quel segmento, e la
  distanza con segno vale per tutta la polilinea, che segue il verso dei
  vertici.
- **Linea, arco, cerchio:** `offsetCurve`. Il risultato torna dello stesso
  tipo: la linea con i suoi due estremi nuovi; il cerchio con il nuovo raggio;
  l'arco con il nuovo raggio e **gli stessi angoli** dell'arco di partenza
  (solid-space tiene angolo iniziale e ampiezza, ADR 0012 di solid-space,
  punto 3). `vanished` dà `Vanished`.
- **Polilinea:** `offsetPolyline` con la regola del documento
  (`bridge::policyOf`). Il risultato torna una polilinea aperta o chiusa come
  quella di partenza, con le posizioni e i bulge di solid-space e, per ogni
  segmento, grazie a `origins` (ADR 0035, punto 11):
  - un segmento che viene da un segmento di partenza (`source`) prende le sue
    larghezze;
  - un arco di giunzione (`gap`) ha larghezze 0;
  - larghezza costante, generazione del tipo di linea e ogni altra proprietà
    restano quelle della polilinea di partenza.

  `done` dà `Done` con `simple`; `vanished` dà `Vanished`; `failed` dà
  `Failed`, qualunque sia il motivo.
- Una diagnosi di solid-space (dati fuori dal dominio, polilinea senza pezzi)
  dà `Failed`.
- La domanda `offset` del catalogo, con le risposte scritte lì.

## Regole della tappa che valgono qui

- Il sottomodulo lo sposti solo al commit che indica la issue (ADR 0030, punto
  9).
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P19, P21 e P22).
- Nessuna soglia propria per "si annulla" o "si interseca": le decisioni vengono
  da solid-space.

## Test richiesti

- Linea, cerchio e arco dai due lati; arco e cerchio che si annullano (distanza
  uguale al raggio); punto esattamente sulla curva (`NoSide`).
- I casi di `autocad-offset.dxf` e quelli della richiesta
  Matleyx78/solid-space#22 (ADR 0012 di solid-space, "I casi della
  richiesta"), passando dalle entità.
- Polilinee semplici aperte e chiuse con archi; larghezze per segmento e
  costante; un caso non semplice che dà `Failed`; coordinate 1e6.

## Fatto quando

- I casi di `[0.7-R17]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
