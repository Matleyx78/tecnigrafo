# [0.6-P6] `edit`: modifiche annullabili a layer e proprietà, valori correnti

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #232
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: si vede solo con i comandi; i casi sono in
  R12–R14
- **Dipende da:** P4
- **Modulo:** `edit`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punti 3–5,
  [ADR 0005](../../adr/0005-comandi-e-annullamento.md) (transazioni);
  `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`;
  `core/include/tg/core/layer.hpp` e `document.hpp` (P4),
  `core/include/tg/core/raw_drawing.hpp`

## File

- `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`
- `edit/include/tg/edit/layer_rules.hpp`, `edit/src/layer_rules.cpp` (nuovi)
- `edit/CMakeLists.txt`
- `tests/edit/editor_test.cpp`, `tests/edit/layer_rules_test.cpp` (nuovo)
- `tests/CMakeLists.txt`

## Cosa fare

### Regole (`layer_rules.hpp`)

```cpp
enum class LayerProblem : std::uint8_t {
    None,
    InvalidName,  ///< core::isValidTableName fails
    NameInUse,    ///< another layer has the name (core::sameTableName)
    LayerZero,    ///< layer 0 cannot be renamed or removed
    Current,      ///< the current layer cannot be removed nor frozen
    InUse,        ///< an entity, handled or not, or a block entity uses it
    NotFound,
};

[[nodiscard]] LayerProblem checkNewLayer(const core::Document& doc, std::string_view name);
[[nodiscard]] LayerProblem checkRename(const core::Document& doc, core::LayerId id,
                                       std::string_view newName);
[[nodiscard]] LayerProblem checkRemove(const core::Document& doc, core::LayerId id);
[[nodiscard]] LayerProblem checkFreeze(const core::Document& doc, core::LayerId id);

/// True if the layer's name appears in unhandled data other than the group 8
/// of unhandled entities and of entities in the raw BLOCKS section (layer
/// states, filters, XDATA, any other raw group equal to the name, ignoring
/// case): a rename leaves that data with the old name (docs/adr/0023, point 5).
[[nodiscard]] bool namedInOtherUnhandledData(const core::Document& doc, core::LayerId id);
```

"Usato" per `checkRemove`: un'entità gestita con quel layer; un'entità non
gestita o un'entità della sezione `BLOCKS` grezza con il gruppo 8 uguale al
nome (senza maiuscole).

### Editor

Tutte richiedono una transazione aperta, salvo dove scritto; restituiscono
`false` senza cambiare nulla se non c'è niente da cambiare o se la regola non
lo permette (chi chiama controlla prima con le funzioni qui sopra).

```cpp
std::optional<core::LayerId> addLayer(core::Layer layer);  ///< checkNewLayer
bool renameLayer(core::LayerId id, std::string name);      ///< checkRename
bool removeLayer(core::LayerId id);                        ///< checkRemove
/// Colour, linetype, lineweight and states; name and id stay. Refuses to
/// freeze the current layer.
bool changeLayer(const core::Layer& layer);
std::optional<core::LinetypeId> addLinetype(core::Linetype linetype);
bool setProperties(core::EntityId id, const core::EntityProperties& properties);
bool setCurrent(const core::CurrentProperties& current);  ///< refuses a frozen layer
bool setLtscale(double scale);                            ///< > 0
/// LWDISPLAY is a view setting saved with the drawing: it marks the drawing
/// modified but makes no undo step, as in AutoCAD. No transaction needed.
void setLwdisplay(bool on);
```

- `renameLayer` aggiorna il gruppo 8 delle entità non gestite e delle entità
  nella sezione `BLOCKS` grezza che hanno il vecchio nome; l'annullamento li
  rimette com'erano.
- Cambiare il colore di un'entità che ha il gruppo 430 in
  `extras.common` lo toglie (ADR 0023, punto 2); l'annullamento lo rimette.
- Ogni operazione è un `Change` annullabile e ripetibile; i passi conservano
  la posizione dei record nelle tabelle.
- `isModified()` tiene conto di tutto, `setLwdisplay` compreso.

## Regole della tappa che valgono qui

- Fuori da `edit` e `io` il documento si cambia solo con l'editor.
- I dati non gestiti non si scartano mai in silenzio.

## Test richiesti

- Ogni regola di `layer_rules`, con un documento che ha entità non gestite e
  una sezione `BLOCKS` grezza.
- Ogni operazione, annullata e ripetuta, con lo stato identico (`==`) prima e
  dopo; più operazioni nella stessa transazione.
- `renameLayer` con entità non gestite e blocchi; `namedInOtherUnhandledData`
  con un nome in `OBJECTS` e in XDATA.
- Gruppo 430 tolto e rimesso.
- `setLwdisplay`: niente passo d'annullamento, disegno modificato.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
