# [0.8-PT16] `core`: definizioni dei blocchi e inserimenti

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT16]` (#…), agente codex, fascia forte
- **Dipende da:** PT4 (revisione delle interfacce), PT6, PT8, PT9, PT48, RT16
- **Modulo:** `core`
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punti 1–4;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punti 2–4;
  [ADR 0031](../../adr/0031-modifiche-alle-entita.md), punti 1–6;
  `core/include/tg/core/effective_properties.hpp`, `similarity.hpp`,
  `transform2d.hpp`; le schede [PT6](PT6-scheletro-core.md) e
  [PT17](PT17-attributi-core.md) (sorella: gli attributi)

## File

- `core/include/tg/core/block_table.hpp`, `core/src/block_table.cpp`
- `core/include/tg/core/insert.hpp`, `core/src/insert.cpp`
- `core/src/generated.cpp` (la sua parte di `transformedShape`, dopo PT48)
- `core/src/effective_properties.cpp`, `core/include/tg/core/effective_properties.hpp`
  (le proprietà dentro un blocco)
- `tests/core/block_table_test.cpp`, `tests/core/insert_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Definizione** (`block_table.hpp`):

   ```cpp
   struct BlockDefinition {
       std::string name; Vec2 base; std::string description;
       int units{0}; bool explodable{true}; bool uniformScale{false};
       bool anonymous{false}; bool hasAttributes{false};
       std::vector<EntityRecord> entities;   // handled, with their handles
       std::vector<RawEntity> rawEntities;   // unhandled, in order with them
       std::uint64_t recordId{0};            // BLOCK_RECORD handle
       std::uint64_t blockId{0}; std::uint64_t endBlockId{0};
       RawGroups blockExtras, endBlockExtras, recordExtras;
   };
   class BlockTable {
   public:
       [[nodiscard]] std::span<const BlockDefinition> definitions() const;  // order
       [[nodiscard]] const BlockDefinition* find(std::string_view name) const;
       /// True when putting `definition` would make a block contain itself,
       /// directly or through nested inserts (docs/adr/0045, point 1).
       [[nodiscard]] bool wouldCycle(const BlockDefinition& definition) const;
       friend bool operator==(const BlockTable&, const BlockTable&) = default;
   private: friend class Document; /* storage */
   };
   ```

   `Document::replaceBlock` (PT6) ha la precondizione `!wouldCycle`.
2. **Inserimento.** `supported(const Insert&)` restituisce `true` se ogni
   attributo è valido (PT17); scala X e Y diverse da zero.
   - `Transform2D insertTransform(const Document&, const Insert&)`: togliere il
     punto base, scalare (X, Y), ruotare, portare al punto d'inserimento.
   - `bool isSimilarity(const Insert&)`: `|X| == |Y|`.
   - `dependsOnBlock(document, record, name)`: vero se l'inserimento usa `name`
     o un blocco che lo contiene, a qualunque profondità.
3. **Geometria generata** (ADR 0044, punto 2): per ogni entità della
   definizione, in ordine, la sua geometria generata (ricorsiva per gli
   inserimenti annidati) composta con la trasformazione dell'inserimento:
   similitudine → `transformed`; altrimenti `transformedShape` (PT6, punto 3),
   esatta per ogni forma: questo pacchetto realizza la sua parte per linee,
   polilinee (divise in linee e archi d'ellisse quando hanno archi), `Solid` e
   `FilledRegion` (che tiene l'affinità); ellissi e archi vengono da PT48, il
   testo da PT17. Poi gli attributi (PT17). Un blocco che non c'è dà
   nessun elemento (non succede: il lettore lo lascia non gestito).
4. **Proprietà** (ADR 0045, punto 3) in `effective_properties`: una funzione
   che, date le proprietà effettive dell'inserimento e un'entità del blocco,
   dà quelle dell'entità (layer `0` → layer dell'inserimento; *DaBlocco* →
   dell'inserimento; *DaLayer* → del layer su cui si mostra; visibilità: layer
   dell'entità e dell'inserimento), annidabile.
5. **Trasformazioni** (`transformedOf`): punto d'inserimento trasformato; scale
   per il fattore di scala; rotazione più l'angolo della similitudine. Con una
   riflessione rispetto a un asse d'angolo φ, da
   `T(p)·R(r)·S(sx, sy)` si ottiene `T(p')·R(2φ − r)·S(sx, −sy)`
   `= T(p')·R(2φ − r + π)·S(−sx, sy)`: si scrive la seconda forma, con la
   **scala X** di segno opposto e la rotazione `2φ − r + 180°` riportata in
   [0°, 360°), come AutoCAD (ADR 0045, punto 4); per una similitudine che
   riflette, φ viene dalla sua scomposizione (ADR 0031, punto 3). Gli
   attributi li trasforma PT17.
6. **Grip** sul punto d'inserimento; **snap** inserimento sul punto
   d'inserimento più quelli della geometria generata (PT6).

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` (`core::sinCos`, `core::atan2`).
- Gli handle delle entità di un blocco sono nello stesso spazio di quelle del
  disegno: `Document` non li riusa.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `block_table_test.cpp`: `find`; `wouldCycle` diretto e attraverso due
  livelli.
- `insert_test.cpp`: geometria generata con scala 1, 2, −1, (2, 1) e rotazione
  30°; blocco annidato; proprietà con layer `0` e *DaBlocco* su due livelli;
  layer congelato dell'inserimento; `MIRROR` di un inserimento; ingombro che
  cambia quando la definizione cambia (`Document::replaceBlock`).

## Fatto quando

- I casi di RT16 passano; la CI è verde.
