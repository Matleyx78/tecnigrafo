# [0.8.1-PT29] `render`: griglia a passo fisso e adattiva, icona dell'UCS

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nelle immagini; test con il
  pittore di prova, prova manuale
- **Dipende da:** PT5, PT27, PT30 (`scene_renderer.cpp`: prima il simbolo di
  *Vicino*)
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
   passo `gridUnit` asse per asse (un asse a 0 prende il passo dello snap di
   quell'asse: `0,10` con snap 10,10 dà 10 e 10; PT27, punto 1), principale
   ogni `gridMajor`,
   adattiva e suddivisione come l'ADR 0055, punto 5; con `gridMajor` 1 il
   passo si dirada per 5 e non si suddivide
   ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 3, punto 7).
   `gridSpacing` e il passo
   automatico spariscono. Con maglie diverse nei due assi (per esempio 8×20
   pixel con soglia 12) chi realizza sceglie se diradare per asse o tutta la
   griglia, lo scrive nel commento dell'interfaccia e lo fissa con un test: è
   solo aspetto, non un buco visibile (verbale della revisione PT5,
   [decisione] 2; [pacchetto.md](../../agenti/pacchetto.md), «Buchi visibili
   e no»).
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

- `render/src/scene_renderer.cpp` lo cambia prima PT30 (il simbolo di
  *Vicino* in `renderSnapMarker`): questo pacchetto si aggiorna.
- Misura e forma dell'icona dalla schermata di AutoCAD; colori `axisX` e
  `axisY` di oggi.

## Test richiesti

- Pittore di prova: griglia spenta; passi 5,10; `GRIDUNIT` 0,10 con snap 5,5
  (X a 5, Y a 10); maglie diverse nei due assi sotto la soglia, con la scelta
  scritta; adattiva a due zoom;
  suddivisione; `GRIDMAJOR` 10 e `GRIDMAJOR` 1 a due zoom (con 1 il passo
  diradato per 5, mai la griglia sparita); icona nell'origine, nell'angolo con
  l'origine fuori vista e sul bordo; `UCSICON` 0 e 1; `AxesStyle::Lines`.

## Fatto quando

- I test passano; la CI è verde; la prova manuale confronta l'icona con la
  schermata.
