# [0.6-P4] `core`: tabelle dei layer e dei tipi di linea, proprietà delle entità

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: struttura dati senza effetti osservabili; la
  provano R9 e R10
- **Dipende da:** P2
- **Modulo:** `core`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) (tutto),
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punti 1–4;
  `core/include/tg/core/document.hpp`, `entity_extras.hpp`, `raw_drawing.hpp`,
  `entity_id.hpp`; le intestazioni di P2

## File

- `core/include/tg/core/layer.hpp` (nuovo: `LayerId`, `LinetypeId`, `Layer`,
  `Linetype`, `EntityProperties`, `CurrentProperties`, regole dei nomi)
- `core/src/layer.cpp` (nuovo)
- `core/include/tg/core/document.hpp`, `core/src/document.cpp`
- `core/CMakeLists.txt`
- `tests/core/layer_test.cpp` (nuovo), `tests/core/document_test.cpp`
- `tests/CMakeLists.txt`

## Cosa fare

### Tipi (`layer.hpp`)

```cpp
/// The handle of a LAYER or LTYPE table record (docs/adr/0023, point 1).
struct LayerId { std::uint64_t value{0}; /* == as EntityId */ };
struct LinetypeId { std::uint64_t value{0}; };

struct Linetype {
    LinetypeId id;
    std::string name;
    std::string description;
    LinetypePattern pattern;
    RawGroups extras;  ///< what the model does not represent, as read
};

struct Layer {
    LayerId id;
    std::string name;
    Color color{Color::fromIndex(7)};  ///< Index or Rgb only
    LinetypeId linetype;               ///< never ByLayer nor ByBlock
    int lineweight{lineweight::kDefault};  ///< allowed value or kDefault
    bool on{true};
    bool frozen{false};
    bool locked{false};
    bool plot{true};
    RawGroups extras;
};

struct EntityProperties {
    LayerId layer;
    Color color{Color::byLayer()};
    LinetypeId linetype;          ///< may be the ByLayer or ByBlock record
    int lineweight{lineweight::kByLayer};
    double linetypeScale{1.0};    ///< DXF 48
};

/// $CLAYER, $CECOLOR, $CELTYPE, $CELWEIGHT, $CELTSCALE (docs/adr/0023, point 3).
using CurrentProperties = EntityProperties;

/// docs/adr/0023, point 1: not empty, at most 255 characters, none of
/// < > / \ " : ; ? * | , = `, no leading or trailing blank.
[[nodiscard]] bool isValidTableName(std::string_view name);

/// Names compare without case for ASCII letters, as in AutoCAD.
[[nodiscard]] bool sameTableName(std::string_view a, std::string_view b);
```

Tutte le strutture hanno `operator==` predefinito.

### Documento

- `EntityRecord` riceve `EntityProperties properties{}` dopo `entity`.
- Un documento nuovo ha: il layer `0` (colore 7, `Continuous`, `kDefault`,
  acceso) e i tipi `ByBlock`, `ByLayer`, `Continuous`, con identificativi presi
  dal contatore degli handle, nell'ordine `ByBlock`, `ByLayer`, `Continuous`,
  `0`. I valori correnti sono layer `0`, `ByLayer`, tipo `ByLayer`,
  `kByLayer`, scala 1; `LTSCALE` 1; `LWDEFAULT` 25; `LWDISPLAY` spento.
- `add(Entity)` dà all'entità i valori correnti.

```cpp
[[nodiscard]] std::span<const Layer> layers() const;  ///< in table order
[[nodiscard]] const Layer* layer(LayerId id) const;
[[nodiscard]] const Layer* findLayer(std::string_view name) const;  ///< sameTableName
[[nodiscard]] std::span<const Linetype> linetypes() const;
[[nodiscard]] const Linetype* linetype(LinetypeId id) const;
[[nodiscard]] const Linetype* findLinetype(std::string_view name) const;
[[nodiscard]] LinetypeId byLayerLinetype() const;
[[nodiscard]] LinetypeId byBlockLinetype() const;
[[nodiscard]] LinetypeId continuousLinetype() const;
[[nodiscard]] LayerId layerZero() const;

[[nodiscard]] const CurrentProperties& current() const;
[[nodiscard]] double ltscale() const;
[[nodiscard]] int lwdefault() const;
[[nodiscard]] bool lwdisplay() const;

// Changed only by edit and io, like the entities.
LayerId addLayer(Layer layer);        ///< a new id if layer.id is 0; appended
void insertLayer(Layer layer, std::size_t position);  ///< undo: back at its place
std::optional<std::pair<Layer, std::size_t>> removeLayer(LayerId id);
void replaceLayer(const Layer& layer);  ///< same id, every other field
LinetypeId addLinetype(Linetype linetype);
void insertLinetype(Linetype linetype, std::size_t position);
std::optional<std::pair<Linetype, std::size_t>> removeLinetype(LinetypeId id);
void setProperties(EntityId id, const EntityProperties& properties);  ///< precondition: found
void setCurrent(const CurrentProperties& current);
void setLtscale(double scale);   ///< precondition: > 0
void setLwdefault(int value);    ///< precondition: an allowed value
void setLwdisplay(bool on);
```

- Gli identificativi dei record usano lo stesso contatore degli handle delle
  entità e non si riusano mai (come `reserveHandle`).
- Il documento **non** controlla le regole del punto 5 dell'ADR 0023
  (cancellare un layer usato, rinominare il layer `0`): le controlla `edit`
  (P6). Controlla solo le precondizioni: identificativi esistenti, nomi
  validi e non doppi, layer con colore indice o RGB.
- `layers()` non dipende dall'indice spaziale: l'indice non cambia.

## Regole della tappa che valgono qui

- I dati non gestiti si conservano: `extras` dei record viaggia con il record.
- `document.hpp/.cpp` li tocca poi P9; `core/CMakeLists.txt` anche P9 e P11.

## Test richiesti

- Documento nuovo: tabelle, identificativi e valori correnti come sopra.
- `add` dà alle entità i valori correnti; dopo `setCurrent`, i nuovi.
- Ricerca per nome senza maiuscole; nomi non validi (ogni carattere vietato,
  spazi ai capi, 255 e 256 caratteri).
- `removeLayer` e `insertLayer` alla stessa posizione; identificativi mai
  riusati, nemmeno dopo `remove` delle entità.
- `setProperties` e `remove`/`restore` di un'entità ne conservano le proprietà.

## Fatto quando

- I test qui sopra esistono e passano; i test esistenti di `core` non cambiano.
- La CI è verde.
