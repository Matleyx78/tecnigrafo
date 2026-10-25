# [0.6-P21] `app`: controlli delle proprietà e finestra dei colori

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: interfaccia sopra operazioni verificate da
  R12–R14; test Qt (decisione 5 del piano)
- **Dipende da:** P6, P20
- **Modulo:** `app`
- **Da leggere:** [ADR 0024](../../adr/0024-colori.md) (tutto),
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punto 6;
  `core/include/tg/core/color.hpp`, `lineweight.hpp`, `layer.hpp`;
  `app/src/theme.hpp` (P17)

## File

- `app/src/color_dialog.hpp`, `app/src/color_dialog.cpp` (nuovi)
- `app/src/property_widgets.hpp`, `app/src/property_widgets.cpp` (nuovi)
- `app/CMakeLists.txt`
- `tests/app/color_dialog_test.cpp`, `tests/app/property_widgets_test.cpp` (nuovi)
- `tests/CMakeLists.txt`

## Cosa fare

Controlli riusati dal pannello dei layer (P22), dal pannello delle proprietà
e dalla finestra `QSELECT` (P24).

### Finestra dei colori

```cpp
class ColorDialog final : public QDialog {
public:
    enum class Allow : std::uint8_t { All, IndexAndRgb };  ///< layers: no ByLayer/ByBlock
    ColorDialog(core::Color initial, Allow allow, QWidget* parent = nullptr);
    [[nodiscard]] core::Color color() const;
};
```

- Scheda **Indice**: i colori 1–255 in griglia (10–249 per tinta e
  luminosità come AutoCAD, poi 1–9 e 250–255), con `aciToRgb`; pulsanti
  `ByLayer` e `ByBlock` se ammessi; campo con il numero.
- Scheda **RGB**: tre campi 0–255 e un'anteprima; mostra anche l'indice più
  vicino (`nearestAci`).
- Il colore 7 si mostra metà bianco e metà nero, come in AutoCAD.

### Controlli

```cpp
/// Combo boxes that show and edit one property. A "varies" entry appears
/// only while showing a mixed selection.
class ColorCombo;      ///< ByLayer, ByBlock, 1-7 by name, recent colours, "Select Colour…"
class LayerCombo;      ///< every layer, with its colour and states; sets a LayerId
class LinetypeCombo;   ///< loaded linetypes with a short preview of the pattern
class LineweightCombo; ///< ByLayer, ByBlock, Default and kAllowed, with a preview
```

Ogni controllo ha `setDocument(const core::Document*)`, `setValue(…)`,
`setVaries()`, il segnale `valueChosen(…)` e non cambia il documento da sé:
chi lo usa chiama l'editor.

## Regole della tappa che valgono qui

- Testi con `tr()`, in inglese; nomi dei colori come AutoCAD (`Red`,
  `Yellow`, `Green`, `Cyan`, `Blue`, `Magenta`, `White`).
- `app/CMakeLists.txt`: solo le tue righe.

## Test richiesti

- Finestra: scelta di un indice, di un RGB, di `ByLayer`; `IndexAndRgb` non
  offre `ByLayer` né `ByBlock`; indice più vicino mostrato.
- Ogni controllo: valori del documento, `varies`, segnale alla scelta.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
