# [0.8.1-PT29] `render`: griglia a passo fisso e adattiva, icona dell'UCS

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nelle immagini; test con il
  pittore di prova, prova manuale
- **Dipende da:** PT5, PT27
- **Modulo:** `render`
- **Da leggere:** [ADR 0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  punti 5 e 6; `render/include/tg/render/scene_renderer.hpp`,
  `render/src/scene_renderer.cpp`; la schermata dell'icona dell'UCS nella
  [#920](https://github.com/Matleyx78/tecnigrafo/issues/920#issuecomment-7067255469)

## File

- `render/include/tg/render/scene_renderer.hpp`, `render/src/scene_renderer.cpp`
- `tests/render/scene_renderer_test.cpp`

## Cosa fare

1. La griglia legge `document.grid()`: niente griglia con `gridOn` spento;
   passo `gridUnit` (o `snapUnit` con 0,0), principale ogni `gridMajor`,
   adattiva e suddivisione come l'ADR 0055, punto 5. `gridSpacing` e il passo
   automatico spariscono.
2. ```cpp
   struct AxisFrame {
       core::Vec2 origin{0, 0};
       core::Vec2 xDirection{1, 0}, yDirection{0, 1};  // unit vectors
       std::string xLabel{"X"}, yLabel{"Y"};
   };
   enum class AxesStyle : std::uint8_t { UcsIcon, Lines, None };  // a preference of the program
   ```
   `SceneStyle` ha `axes` (predefinito `UcsIcon`), `axisFrame` e la misura
   dell'icona in pixel; l'icona segue `ucsIcon` del disegno (bit 1 e 2) e il
   punto 6 dell'ADR (nell'origine solo se ci sta intera, altrimenti in basso a
   sinistra con un margine fisso). `Lines` disegna le rette di oggi; `None`
   niente.

## Regole della tappa che valgono qui

- Misura e forma dell'icona dalla schermata di AutoCAD; colori `axisX` e
  `axisY` di oggi.

## Test richiesti

- Pittore di prova: griglia spenta; passi 5,10; adattiva a due zoom;
  suddivisione; `GRIDMAJOR` 10; icona nell'origine, nell'angolo con l'origine
  fuori vista e sul bordo; `UCSICON` 0 e 1; `AxesStyle::Lines`.

## Fatto quando

- I test passano; la CI è verde; la prova manuale confronta l'icona con la
  schermata.
