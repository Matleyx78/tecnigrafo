# [0.6-P9] `core`: proprietà effettive e stati dei layer nelle domande del documento

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.6-R9]` (#…), agente gemini
- **Dipende da:** P5, P8, R9
- **Modulo:** `core` (più le due domande in `script`)
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punto 4,
  [ADR 0024](../../adr/0024-colori.md) punti 3 e 5,
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti 3 e 6;
  `docs/verifica/domande.md` (`aci`, `effective`, domande sul documento);
  `core/include/tg/core/document_query.hpp`, `core/src/document_query.cpp`,
  `core/src/snap_points.cpp`; `script/src/property_questions.cpp` (P5)

## File

- `core/include/tg/core/effective_properties.hpp`,
  `core/src/effective_properties.cpp` (nuovi)
- `core/include/tg/core/document_query.hpp`, `core/src/document_query.cpp`
- `core/CMakeLists.txt`
- `script/src/property_questions.cpp`, `script/src/question_table.cpp` (le
  righe di `aci` ed `effective`)
- `tests/core/effective_properties_test.cpp` (nuovo),
  `tests/core/document_query_test.cpp`
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R9)

## Cosa fare

```cpp
/// What the drawing shows of an entity (docs/adr/0023-0025).
struct EffectiveProperties {
    Rgb color;                  ///< colour 7 as {255,255,255}: the renderer swaps it
    bool colorIsSeven{false};   ///< true for index 7, also through ByLayer or ByBlock
    const Linetype* linetype{nullptr};  ///< never null; ByBlock gives Continuous
    double linetypeScale{1.0};  ///< LTSCALE x the entity's scale
    int lineweight{25};         ///< hundredths of a mm, Default resolved by LWDEFAULT
    bool visible{true};         ///< layer on and thawed
    bool locked{false};
};

[[nodiscard]] EffectiveProperties effectiveProperties(const Document& document,
                                                      const EntityRecord& record);

/// True when the entity's layer is on and thawed.
[[nodiscard]] bool isVisible(const Document& document, const EntityRecord& record);
```

- `ByLayer` prende colore, tipo e spessore dal layer; `ByBlock` vale colore
  7, `Continuous` e `Default` finché non ci sono blocchi.
- `pickEntity`, `windowSelect`, `crossingSelect` e la ricerca dello snap
  (`findSnap`) ignorano le entità invisibili. I layer bloccati non cambiano
  nulla qui (lo fa la selezione dei comandi, P14).
- Domande: `aci <n>` con `aciToRgb`; `effective <E>` nel formato di
  `domande.md`.

## Regole della tappa che valgono qui

- La tabella dei colori è quella di P2: se R9 (copiato dall'oracolo) dice
  altro, **la correzione è in `core/src/color.cpp`**, in questo pacchetto;
  aggiungi il file all'elenco e scrivilo nella pull request.
- `core/CMakeLists.txt`, `question_table.cpp`: solo le tue righe.

## Test richiesti

- `effectiveProperties` per ogni combinazione di `ByLayer`, `ByBlock`, valore
  proprio; `Default` con `LWDEFAULT` cambiato; `LTSCALE` × scala.
- Scelta, finestra, intersezione e snap con layer spento, congelato,
  bloccato.

## Fatto quando

- I casi di `[0.6-R9]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
