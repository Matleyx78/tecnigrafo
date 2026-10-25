# [0.6-P4] `core`: tabelle dei layer e dei tipi di linea, proprietà delle entità

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #230
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.6-R4]` (da creare), agente gemini, per gli
  identificativi di un disegno nuovo; le tabelle non hanno altri effetti
  osservabili e le provano R9 e R10
- **Dipende da:** P2, R4
- **Modulo:** `core`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) (tutto,
  in particolare "Handle di un disegno nuovo" nel punto 1),
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punti 1–4;
  `core/include/tg/core/document.hpp`, `entity_extras.hpp`, `raw_drawing.hpp`,
  `entity_id.hpp`; le intestazioni di P2

## File

- `core/include/tg/core/layer.hpp` (nuovo: `LayerId`, `LinetypeId`, `Layer`,
  `Linetype`, `EntityProperties`, `CurrentProperties`, regole dei nomi)
- `core/include/tg/core/start_handles.hpp` (nuovo: la mappa degli handle di
  partenza)
- `core/src/layer.cpp` (nuovo)
- `core/include/tg/core/document.hpp`, `core/src/document.cpp`
- `core/CMakeLists.txt`
- Dove nasce un disegno nuovo: `cmd/src/commands/file_commands.cpp` (`NEW`),
  `script/src/level1.cpp` (sessione di livello 1), `script/src/questions.cpp`
  e `script/src/question_table.cpp` (domande al modello),
  `app/src/main_window.cpp` e `app/src/sample_drawing.cpp` (finestra
  principale, quindi anche il livello 2)
- `tests/core/layer_test.cpp` (nuovo), `tests/core/document_test.cpp`,
  `tests/cmd/file_commands_test.cpp` (solo il test nuovo su `NEW`)
- `tests/script/level1_test.cpp`, `tests/script/case_check_test.cpp`,
  `tests/script/dati/pline-atteso.txt`, `tests/app/script_session_test.cpp`
  (solo gli identificativi, vedi sotto)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R4)

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

### Handle di partenza (`start_handles.hpp`)

La mappa dell'ADR 0023, punto 1 ("Handle di un disegno nuovo"):

```cpp
/// Handles of the objects a new drawing starts with (docs/adr/0023, point 1).
/// Fixed for ever, given downwards from kLastReserved; a new object takes the
/// first free handle below kLowest and becomes the new kLowest.
namespace tg::core::start_handles {
inline constexpr std::uint64_t kLastReserved = 256;
inline constexpr std::uint64_t kFirstEntity = kLastReserved + 1;  // 257
inline constexpr std::uint64_t kByBlock = 256;
inline constexpr std::uint64_t kByLayer = 255;
inline constexpr std::uint64_t kContinuous = 254;
inline constexpr std::uint64_t kLayerZero = 253;
inline constexpr std::uint64_t kLowest = kLayerZero;  ///< P10 goes below
static_assert(kLowest >= 1);
}  // namespace tg::core::start_handles
```

### Documento

- `EntityRecord` riceve `EntityProperties properties{}` come **ultimo**
  campo, dopo `order`: così le inizializzazioni per posizione che esistono in
  `io` e nei test (`EntityRecord{id, entity, extras, order}`) restano valide
  (scelta del responsabile).
- Due modi di costruire un documento:
  - `Document()`, cioè `Document{}`: un documento **vuoto**, senza record
    nelle tabelle e con `nextId()` uguale a 1. Lo usano il lettore (un file
    porta i suoi handle, ADR 0023 punto 1) e i test che costruiscono
    documenti a mano. È il comportamento di oggi: i test esistenti di
    `core`, `io`, `edit` e `cmd` non cambiano.
  - `static Document newDrawing()`: un **disegno nuovo**, con i tipi
    `ByBlock`, `ByLayer`, `Continuous` (in quest'ordine nella tabella) e il
    layer `0` (colore 7, `Continuous`, `kDefault`, acceso), con gli handle
    della mappa; nessun altro handle tra 1 e 256 è occupato, e `nextId()` è
    257, quindi la prima entità aggiunta ha l'identificativo 257.
- Valori correnti di un disegno nuovo: layer `0`, colore `ByLayer`, tipo
  `ByLayer`, `kByLayer`, scala 1; `LTSCALE` 1; `LWDEFAULT` 25; `LWDISPLAY`
  spento. Un documento vuoto ha gli stessi valori, ma layer e tipo correnti
  con identificativo 0 finché il lettore (P10) non li imposta.
- `add(Entity)` dà all'entità i valori correnti.

```cpp
[[nodiscard]] std::span<const Layer> layers() const;  ///< in table order
[[nodiscard]] const Layer* layer(LayerId id) const;
[[nodiscard]] const Layer* findLayer(std::string_view name) const;  ///< sameTableName
[[nodiscard]] std::span<const Linetype> linetypes() const;
[[nodiscard]] const Linetype* linetype(LinetypeId id) const;
[[nodiscard]] const Linetype* findLinetype(std::string_view name) const;
/// The record with that name (sameTableName), or the id 0 if the document
/// has none, as an empty document.
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

- I record aggiunti con `addLayer` e `addLinetype` (identificativo 0) prendono
  l'identificativo dallo stesso contatore degli handle delle entità, e non si
  riusano mai (come `reserveHandle`). Solo `newDrawing()` usa la mappa.
- Il documento **non** controlla le regole del punto 5 dell'ADR 0023
  (cancellare un layer usato, rinominare il layer `0`): le controlla `edit`
  (P6). Controlla solo le precondizioni: identificativi esistenti, nomi
  validi e non doppi, layer con colore indice o RGB.
- `layers()` non dipende dall'indice spaziale: l'indice non cambia.

### Dove nasce un disegno nuovo

`Document::newDrawing()` sostituisce `Document{}` in ogni punto in cui
Tecnigrafo comincia un disegno, e solo lì:

- `NEW` (`file_commands.cpp`: `replaceDocument(core::Document::newDrawing(), "")`);
- la sessione di livello 1 (`level1.cpp`, membro `m_document`);
- il documento delle entità definite nelle domande (`questions.cpp`); in
  `question_table.cpp`, `QuestionModel::find` e `nameOf` contano gli
  identificativi da `start_handles::kFirstEntity` invece che da 1 (oggi
  `index + 1`). Il disegno letto da un file nelle domande (`fileDocument`)
  resta un documento vuoto;
- la finestra principale (`main_window.cpp`, disegno iniziale vuoto) e il
  disegno d'esempio (`sample_drawing.cpp`), così il livello 2 dà gli stessi
  identificativi del livello 1.

Il lettore (`io::read…`) **non** cambia: parte da un documento vuoto.

### Test esistenti da aggiornare

Solo gli identificativi delle entità create in una sessione nuova, che
crescono di 256 (`1 LWPOLYLINE` diventa `257 LWPOLYLINE`, `selection: 1 2`
diventa `selection: 257 258`), in `tests/script/level1_test.cpp`,
`tests/script/case_check_test.cpp`, `tests/script/dati/pline-atteso.txt` e
`tests/app/script_session_test.cpp`. Nient'altro di quei test cambia. Le
domande al modello rispondono per nome, quindi `tests/script/questions_test.cpp`
non cambia.

## Regole della tappa che valgono qui

- I dati non gestiti si conservano: `extras` dei record viaggia con il record.
- `document.hpp/.cpp` li tocca poi P9; `core/CMakeLists.txt` anche P9 e P11.
- Fuori da `core` P4 cambia una riga per file, dove nasce un disegno nuovo:
  `question_table.cpp` lo tocca poi P5, `main_window.cpp` P7 e i pacchetti di
  `app` che seguono.

## Test richiesti

- `newDrawing()`: tabelle, handle della mappa, valori correnti come sopra;
  `nextId()` 257 e prima entità 257; `NEW` dopo un disegno con entità
  riparte da 257.
- Documento vuoto: nessun record, `nextId()` 1, `layerZero()` e i tre tipi
  con identificativo 0; un record inserito con handle 0x10 (come il layer `0`
  di AutoCAD) o 253 non va in conflitto con nulla.
- `add` dà alle entità i valori correnti; dopo `setCurrent`, i nuovi.
- Ricerca per nome senza maiuscole; nomi non validi (ogni carattere vietato,
  spazi ai capi, 255 e 256 caratteri).
- `removeLayer` e `insertLayer` alla stessa posizione; identificativi mai
  riusati, nemmeno dopo `remove` delle entità.
- `setProperties` e `remove`/`restore` di un'entità ne conservano le proprietà.

## Fatto quando

- I test qui sopra esistono e passano; i test esistenti di `core`, `io`,
  `edit` e `cmd` non cambiano; quelli di `script` e `app` cambiano solo negli
  identificativi, come sopra.
- I casi di `[0.6-R4]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
