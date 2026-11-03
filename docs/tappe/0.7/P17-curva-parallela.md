# [0.7-P17] `core`: curva parallela

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.7-R17]` (#…), agente gemini: la facciata (lato,
  esiti, polilinee, larghezze, unità); i valori geometrici dall'archivio dei
  casi di solid-space
- **Dipende da:** P14, Matleyx78/solid-space#130, P25, R17. La richiesta
  Matleyx78/solid-space#22 è diventata i pacchetti di solid-space che
  realizzano `offset.hpp` (ADR 0030, punto 13); si cita l'ultimo, 0.2-P19
  (Matleyx78/solid-space#130, riferimenti Matleyx78/solid-space#129), che
  comprende gli altri tra le sue dipendenze; perché serve lo dice il piano
  della tappa 0.2 di solid-space
- **Corretta:** 2026-11-01, con la decisione del responsabile sul buco B5 della
  #464 ([DECISIONI.md](../../DECISIONI.md)); 2026-11-03, operazione snellimento
  (#478): rimanda a `offset.hpp` invece di ripeterlo, e i casi `failed` sono
  quelli del contratto
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
`OffsetPiece`, `PieceOrigin`), con i tipi di `curves.hpp` e `side.hpp`, nella
versione del commit della issue (sotto). I commenti di `offset.hpp` dicono che
cosa è deciso esattamente e quali esiti sono liberi in una fascia
([ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
punto 4): la scheda li cita e non li ripete.

## Prima di cominciare

- La issue dice a quale commit di `main` di solid-space spostare il
  sottomodulo: quello che chiude Matleyx78/solid-space#130, l'ultimo
  pacchetto di solid-space che realizza `offset.hpp`. Se la issue non lo dice, chiedilo e non cominciare.
- Se qualcosa che serve qui manca dal contratto o non si comporta come dichiara,
  **fermati** come dice AGENTS.md ("Se serve qualcosa a solid-space").

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/offset.hpp`, `core/src/offset.cpp` (nuovi)
- `core/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/offset_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R17.txt` (da cancellare)

## Cosa fare

```cpp
enum class OffsetOutcome : std::uint8_t {
    Done,      ///< `entity` holds the copy
    Vanished,  ///< nothing is left at that distance
    Failed,    ///< solid-space could not make the copy (its outcome failed)
    NoSide,    ///< the side point lies exactly on the line or circle
};

struct OffsetResult {
    OffsetOutcome outcome{OffsetOutcome::Failed};
    std::optional<Entity> entity;  ///< with Done: the same type as the input
    /// With Done for a polyline: solid-space's PolylineOffset::simple.
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
  Quando `sideOf` non dà un lato per un'entità che non è un testo (una curva
  esattamente puntiforme, una polilinea senza segmenti o con soli segmenti
  puntiformi, dati che solid-space rifiuta), l'esito è `Failed`, senza
  chiamare solid-space (B5): `NoSide` resta solo per `Side::On`. La
  precondizione non cambia.
  Per un segmento di polilinea il lato è rispetto a quel segmento, e la
  distanza con segno vale per tutta la polilinea, che segue il verso dei
  vertici.
- **Linea, arco, cerchio:** `offsetCurve`. Il risultato torna dello stesso
  tipo, con la curva di `CurveOffset`: per un arco il nuovo raggio e **gli
  stessi angoli** dell'arco di partenza. `vanished` dà `Vanished`; `failed`
  dà `Failed`, nei casi che dice `CurveOffset`: per una linea solo quando i
  suoi estremi arrotondati coincidono, in una fascia del commento, che nessun
  test pretende; per un arco (che arriva come `Arc`, `bridge::curveOf`) e un
  cerchio mai.
- **Polilinea:** `offsetPolyline` con la regola del documento
  (`bridge::policyOf`). Il risultato torna una polilinea aperta o chiusa come
  quella di partenza, con le posizioni e i bulge di solid-space e, per ogni
  segmento, grazie a `origins` (una voce per pezzo del risultato,
  `PolylineOffset`; mai per posizione) (ADR 0035, punto 11):
  - un segmento che viene da un segmento di partenza (`source`) prende le sue
    larghezze;
  - un arco di giunzione (`gap`) ha larghezze 0;
  - larghezza costante, generazione del tipo di linea e ogni altra proprietà
    restano quelle della polilinea di partenza.

  `done` dà `Done` con `simple`; `vanished` dà `Vanished`; `failed` dà
  `Failed`, qualunque sia il motivo (`OffsetFailure`).
- Una diagnosi di `offsetCurve` o di `offsetPolyline` dà `Failed`.
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
  uguale al raggio); punto esattamente sulla curva (`NoSide`);
  `Line[(0,0),(0,0)]` con distanza 1 e lato (0,1), e una polilinea senza
  segmenti (`Failed`); un cerchio di centro (0,0) e raggio 2^50 con distanza 1
  verso l'esterno (`Failed`: la diagnosi di `offsetCurve` per il raggio
  costruito oltre il dominio).
- I casi di `autocad-offset.dxf` e quelli della richiesta
  Matleyx78/solid-space#22 (ADR 0012 di solid-space, "I casi della
  richiesta"), passando dalle entità.
- Polilinee semplici aperte e chiuse con archi; larghezze per segmento e
  costante; un caso non semplice che dà `Failed`; coordinate 1e6.

## Fatto quando

- I casi di `[0.7-R17]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
