# [0.5-P1] `core`: handle, unità e dati conservati

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #153
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: struttura dati senza effetti osservabili; la
  provano i casi di R10–R12, quando `io` la userà
- **Dipende da:** —
- **Modulo:** `core`
- **Da leggere:** [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md)
  (tutto), [ADR 0017](../../adr/0017-modulo-io-e-dxf.md) punto 5 (handle),
  [ADR 0006](../../adr/0006-modello-dati-dwg-e-formato-cadx.md) punti 1–2;
  `core/include/tg/core/document.hpp` e `polyline.hpp`

## File

- `core/include/tg/core/raw_group.hpp` (nuovo)
- `core/include/tg/core/entity_extras.hpp` (nuovo)
- `core/include/tg/core/raw_drawing.hpp` (nuovo)
- `core/include/tg/core/document.hpp`
- `core/src/document.cpp`
- `core/include/tg/core/polyline.hpp`
- `core/src/polyline.cpp`
- `tests/core/document_test.cpp`
- `tests/core/polyline_test.cpp`
- `tests/core/raw_data_test.cpp` (nuovo)
- `tests/CMakeLists.txt` (una riga)

## Cosa fare

`core` impara a portare con sé ciò che il modello non rappresenta, senza
interpretarlo. Nessun altro modulo cambia: i dati nuovi hanno valori
predefiniti, e il codice che costruisce `EntityRecord{id, entity}` continua a
compilare.

### Gruppi grezzi (`raw_group.hpp`)

```cpp
/// A DXF group as read, never converted (docs/adr/0018, point 1).
struct RawGroup {
    std::int16_t code{0};
    std::string value;  ///< UTF-8, exactly as written (numbers stay text)
    friend bool operator==(const RawGroup&, const RawGroup&) = default;
};
using RawGroups = std::vector<RawGroup>;
```

### Dati in più di un'entità gestita (`entity_extras.hpp`)

```cpp
/// The DXF data of a handled entity that the model does not represent
/// (docs/adr/0018, points 4 and 5). Empty for entities made by commands.
struct EntityExtras {
    RawGroups beforeEntity;   ///< between handle and AcDbEntity: 102 {…}, 330
    RawGroups common;         ///< AcDbEntity groups not in the model (8, 62, 6, …)
    RawGroups specific;       ///< groups of the entity subclass not in the model
    RawGroups placement;      ///< 30, 31, 38, 39, 210, 220, 230 as read
    RawGroups extended;       ///< XDATA, from the first 1001 on
    bool flippedExtrusion{false};  ///< extrusion (0,0,-1): geometry is in WCS

    [[nodiscard]] bool empty() const;
    friend bool operator==(const EntityExtras&, const EntityExtras&) = default;
};
```

### Dati del disegno (`raw_drawing.hpp`)

```cpp
/// A section kept as read; the ENTITIES section is a placeholder with no
/// groups, which marks where the entities go (docs/adr/0018, point 2).
struct RawSection {
    std::string name;  ///< "HEADER", "TABLES", …
    RawGroups groups;  ///< without the 0 SECTION / 2 name / 0 ENDSEC groups
    friend bool operator==(const RawSection&, const RawSection&) = default;
};

/// An entity Tecnigrafo does not handle (docs/adr/0018, point 3).
struct RawEntity {
    EntityId id;           ///< its handle
    std::uint64_t order{0};
    std::string type;      ///< group 0, "HATCH"
    RawGroups groups;      ///< every group after group 0, handle included
    friend bool operator==(const RawEntity&, const RawEntity&) = default;
};

/// A part of a .cadx archive other than mimetype, manifest.json and
/// drawing.dxf, kept byte for byte (extensions.json, thumbnail.png, …).
struct NativePart {
    std::string name;
    std::string bytes;
    friend bool operator==(const NativePart&, const NativePart&) = default;
};

/// What a file brought that the model does not represent.
struct RawDrawing {
    std::string version;   ///< $ACADVER as read ("AC1015"), empty for a new drawing
    std::string codePage;  ///< $DWGCODEPAGE as read, empty if absent
    std::vector<RawSection> sections;  ///< in file order; empty for a new drawing
    std::vector<NativePart> nativeParts;
    friend bool operator==(const RawDrawing&, const RawDrawing&) = default;
};
```

### Documento

- `EntityRecord` riceve `EntityExtras extras;` e `std::uint64_t order{0};`
  (chiave d'ordine, ADR 0018 punto 2).
- `entities()` resta in ordine di chiave. `add()` assegna la chiave
  successiva alla più alta presente (entità gestite e non gestite);
  `restore()` rimette il record **al suo posto** secondo la chiave, non in
  fondo.
- Nuove funzioni (pubbliche per motivi tecnici, come `add`: fuori da `edit` e
  `io` non si usano):

```cpp
/// Adds an entity read from a file, with its handle, order and extras.
/// Precondition: record.id is not 0 and not used (handled, raw or reserved).
void insert(EntityRecord record);
/// Adds an unhandled entity. Same precondition on raw.id.
void insertRaw(RawEntity raw);
[[nodiscard]] std::span<const RawEntity> rawEntities() const;  ///< in order

/// Marks a handle as used by a table, block or object, so that add()
/// never takes it.
void reserveHandle(EntityId id);
/// The identifier add() will give next; setNextId() only moves it forward.
[[nodiscard]] std::uint64_t nextId() const;
void setNextId(std::uint64_t next);

/// INSUNITS, 0 to 24 (docs/adr/0021); 4, millimetres, for a new document.
[[nodiscard]] int units() const;
void setUnits(int units);  ///< precondition: 0 <= units <= 24

[[nodiscard]] const RawDrawing& raw() const;
void setRaw(RawDrawing raw);
```

- Le entità non gestite non entrano nell'indice spaziale né in `entities()`,
  `extents()` e `query()`.
- `remove()` e `restore()` portano con sé `extras` e `order`: l'annullamento
  conserva tutto.

### Polilinea

Le larghezze sono proprietà del DXF (ADR 0018, conseguenze):

- `PolylineVertex` riceve `double startWidth{0.0};` (40) e
  `double endWidth{0.0};` (41);
- `Polyline` riceve `constantWidth()` / `setConstantWidth(double)` (43,
  predefinita 0).

Non cambiano lunghezza, riquadro, tassellazione né disegno.

## Regole della tappa che valgono qui

- `core` non dipende da `io` né da librerie esterne; i gruppi grezzi non si
  interpretano qui.
- Gli identificativi non si riusano mai, nemmeno quelli riservati.

## Test richiesti

- `insert` con handle sparsi (`0x2F`, `0x100`), poi `add`: il nuovo
  identificativo è maggiore di tutti e non uguale a uno riservato.
- `setNextId` minore del valore attuale non cambia niente.
- Ordine: entità inserite con chiavi 10, 30, 20 escono in ordine 10, 20, 30;
  `remove` e `restore` di quella in mezzo la rimettono in mezzo.
- `extras` e chiave sopravvivono a `remove` + `restore`.
- Le entità non gestite non compaiono in `entities()`, `query()`,
  `extents()`; `rawEntities()` le dà in ordine.
- `units()` vale 4 in un documento nuovo; `setUnits` lo cambia.
- Polilinea con larghezze: uguaglianza, copia, nessun effetto su lunghezza e
  riquadro.

## Fatto quando

- Tutti i test di `core` passano, vecchi e nuovi, e la CI è verde.
- Nessun file fuori da quelli elencati è cambiato.
