# [0.6-P15] `render`: stile dalle proprietà

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nelle immagini, che non si
  possono ricavare senza Tecnigrafo; si prova con il painter di prova (come
  0.5-P6), sulla geometria verificata da R9 e R11
- **Dipende da:** P9, P11
- **Modulo:** `render`
- **Da leggere:** [ADR 0024](../../adr/0024-colori.md) punto 5,
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti 5 e 6;
  `render/include/tg/render/painter.hpp`, `scene_renderer.hpp`,
  `render/src/scene_renderer.cpp`, `tests/render/scene_renderer_test.cpp`;
  `core/include/tg/core/effective_properties.hpp` (P9), `dashes.hpp` (P11)

## File

- `render/include/tg/render/scene_renderer.hpp`, `render/src/scene_renderer.cpp`
- `render/include/tg/render/painter.hpp` (solo se serve un metodo per i punti)
- `render/src/linework.hpp`, `render/src/linework.cpp` (nuovi: tratti e spessori
  a schermo)
- `render/CMakeLists.txt`
- `tests/render/scene_renderer_test.cpp`, `tests/render/linework_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `app/src/qt_painter.cpp` solo se cambia `Painter`

## Cosa fare

- `renderScene` disegna solo le entità visibili (`isVisible`, P9).
- **Colore:** l'RGB effettivo; il colore 7 con `SceneStyle::entity` (il colore
  principale del tema).
- **Spessore:** con `document.lwdisplay()` acceso,
  `max(1, round(mm × 96 / 25.4 × devicePixelRatio))` pixel; spento, 0
  (cosmetico, 1 pixel). `SceneStyle` riceve `double devicePixelRatio{1.0}`.
- **Tipo di linea:** `core::dashes()` con tipo e scala effettivi; continuo se
  restituisce `std::nullopt` o se la lunghezza del motivo × scala, a schermo, è
  minore di 4 pixel. Ogni pezzo si disegna con `drawLine` o `drawPolyline`
  (gli archi tassellati come oggi); un punto è un tratto di 1 pixel lungo la
  direzione.
- Testo: colore effettivo, nessun tipo di linea, nessuno spessore.
- Anteprima, evidenziazione e rettangolo di selezione non cambiano.

## Regole della tappa che valgono qui

- `render` non usa Qt.
- `SceneStyle::entity` lo riempiono i temi in P17; qui basta usarlo.

## Test richiesti

Con il painter di prova che registra le chiamate:

- entità su layer spento e congelato non disegnate; bloccato sì;
- colore indice, RGB, 7, `ByLayer`, `ByBlock`;
- spessore con `LWDISPLAY` acceso e spento, `devicePixelRatio` 2;
- linea tratteggiata: i segmenti di `dashes()` trasformati in pixel; sotto i
  4 pixel, un segmento solo;
- `kMaxDashPieces` superato: un segmento solo.

## Fatto quando

- I test qui sopra esistono e passano; i casi con immagini esistenti passano
  ancora.
- La CI è verde.
