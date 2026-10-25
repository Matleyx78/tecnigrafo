# [0.6-P23] `cmd`: selezione rapida

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #257
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.6-R23]` (#256), agente gemini
- **Dipende da:** P14, R23
- **Modulo:** `cmd` (più la domanda `quickselect` in `script`)
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punto 4;
  `docs/verifica/domande.md` (`quickselect`, pubblicata da P5);
  `cmd/include/tg/cmd/command_runner.hpp`, `selection_set.hpp`;
  `core/include/tg/core/effective_properties.hpp` (P9)

## File

- `cmd/include/tg/cmd/quick_select.hpp`, `cmd/src/quick_select.cpp` (nuovi)
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
  (`replaceSelection`)
- `cmd/CMakeLists.txt`
- `script/src/property_questions.cpp`, `script/src/question_table.cpp` (la
  riga di `quickselect`)
- `tests/cmd/quick_select_test.cpp` (nuovo), `tests/cmd/command_runner_test.cpp`
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R23)

## Cosa fare

```cpp
struct QuickSelectCriteria {
    enum class Property : std::uint8_t { None, Layer, Color, Linetype, Lineweight };
    enum class Operator : std::uint8_t { Equal, NotEqual };

    std::optional<std::string> type;  ///< DXF type, "LINE"; none: every type
    Property property{Property::None};
    Operator op{Operator::Equal};
    std::string value;                ///< as written in the report: "Muri", "1", "bylayer", "0.25"
    bool exclude{false};              ///< select what does NOT match
};

/// The entities of `within` (every entity if empty) that match, in id order.
/// Entities on layers off, frozen or locked are never selected, as by the
/// selection gestures. The comparison uses the entity's own property, not
/// the effective one: Color = bylayer finds the entities with ByLayer, as
/// in AutoCAD. Names compare without case.
[[nodiscard]] std::vector<core::EntityId> quickSelect(const core::Document& document,
                                                      std::span<const core::EntityId> within,
                                                      const QuickSelectCriteria& criteria);
```

- `CommandRunner::replaceSelection(std::vector<core::EntityId> ids)`: con
  nessun comando in corso, la selezione corrente diventa `ids` (senza
  messaggi); altrimenti non fa nulla e restituisce `false`.
- La domanda `quickselect` traduce i suoi argomenti nei criteri, come scritto
  in `domande.md`.
- Un valore che non si legge (colore o spessore non validi) non trova nulla:
  nessun errore.

## Regole della tappa che valgono qui

- La finestra `QSELECT` è di P24: qui solo la logica.
- File comuni di `cmd` e `script`: solo le tue righe.

## Test richiesti

- Per tipo, per ogni proprietà, `=` e `!=`, `exclude`, `within`, combinazioni;
  layer spenti, congelati e bloccati esclusi; maiuscole nei nomi.
- `replaceSelection` con e senza comando in corso.

## Fatto quando

- I casi di `[0.6-R23]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
