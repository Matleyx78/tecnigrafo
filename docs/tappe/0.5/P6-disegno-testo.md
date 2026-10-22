# [0.5-P6] `render`: disegno del testo allineato, obliquo e specchiato

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: la geometria (ancoraggio, riquadro) la
  verifica R2 con le domande al modello; le immagini con il testo non si
  possono ricavare senza eseguire Tecnigrafo, quindi il disegno si prova con i
  test del painter, come nella tappa 0.4 (P12)
- **Dipende da:** P2
- **Modulo:** `render`, più l'implementazione Qt del painter in `app`
- **Da leggere:** [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md)
  punto 4; `core/include/tg/core/text.hpp` (dopo P2: `TextFrame`,
  `textFrame()`), `render/include/tg/render/painter.hpp`,
  `render/src/scene_renderer.cpp` (il disegno del testo),
  `app/src/qt_painter.cpp` (`drawText`)

## File

- `render/include/tg/render/painter.hpp`
- `render/src/scene_renderer.cpp`
- `app/src/qt_painter.cpp`
- `tests/app/qt_painter_test.cpp`
- `tests/render/scene_renderer_test.cpp` (nuovo, con un painter che registra
  le chiamate)
- `tests/CMakeLists.txt` (una riga)

## Cosa fare

Il disegno del testo usa `core::textFrame()` (P2) invece di posizione e
rotazione grezze.

### `TextRun`

```cpp
struct TextRun {
    std::u32string text;      ///< Control codes already converted.
    core::Vec2 origin;        ///< Start of the baseline, screen pixels.
    double capHeightPx{0.0};  ///< Height of capital letters (the used height).
    double angle{0.0};        ///< Radians, counter-clockwise as seen on screen.
    double widthFactor{1.0};  ///< The used width factor.
    double oblique{0.0};      ///< Radians, positive leans right (DXF 51).
    bool backward{false};     ///< Mirrored along the baseline (DXF 71, bit 2).
    bool upsideDown{false};   ///< Mirrored across the baseline (DXF 71, bit 4).
};
```

I campi nuovi hanno valori predefiniti: i `TextRun` costruiti oggi restano
validi.

### `scene_renderer.cpp`

- `origin` è `worldToScreen(frame.origin)`; altezza, rotazione e fattore di
  larghezza sono quelli **usati** di `TextFrame` (per `Aligned` e `Fit` sono
  ricavati); obliquo e specchiatura vengono dal testo.
- Sotto l'altezza minima in pixel si disegna, come oggi, il contorno di
  `core::textBox()`, che dopo P2 è il parallelogramma giusto.

### `qt_painter.cpp`

La trasformazione di `drawText` diventa: traslazione all'origine, rotazione,
specchiatura (`scale(−1, 1)` all'indietro, `scale(1, −1)` capovolto, nel
sistema del testo prima della rotazione), inclinazione dell'obliquo. Va tenuto
conto che sullo schermo Y punta in basso: la stessa accortezza che il codice
già usa per la rotazione. Il riquadro disegnato per i testi piccolissimi
(`kOutlineOnlyHeightPx`) riceve la stessa trasformazione.

## Regole della tappa che valgono qui

- `render` non usa Qt; tutto ciò che è Qt resta in `qt_painter.cpp`.
- `tests/CMakeLists.txt` è un file comune: solo la tua riga.

## Test richiesti

- `scene_renderer_test`: un painter di prova registra i `TextRun`; per un
  testo `MC`, uno `Aligned`, uno `Fit`, uno obliquo, uno all'indietro, uno
  capovolto, controlla origine, angolo, altezza, fattore di larghezza e
  indicatori, calcolati da `textFrame()` e dalla vista.
- Sotto l'altezza minima: il contorno disegnato coincide con `textBox()`
  portato sullo schermo.
- `qt_painter_test`: un testo all'indietro occupa i pixel a sinistra
  dell'origine e non quelli a destra; uno capovolto quelli sotto la linea di
  base; uno obliquo di 30° sposta a destra la cima della `H` di circa
  `tan(30°) · altezza`, entro un pixel.
- Un testo `Left` normale produce gli stessi pixel di prima (confronto con
  l'immagine del test esistente).

## Fatto quando

- I test nuovi passano e quelli della tappa 0.4 sul disegno del testo passano
  ancora.
- I casi di verifica della tappa 0.4 con immagini (`tests/verifica/casi/`)
  passano ancora.
- La CI è verde sui tre sistemi.
