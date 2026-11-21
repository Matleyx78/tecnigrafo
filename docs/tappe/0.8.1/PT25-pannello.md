# [0.8.1-PT25] `app`: pannello delle proprietà con le proprietà proprie, e `QSELECT`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: la logica è nel modello di `core`, già
  verificato da RT22–RT24; test Qt e prova manuale
- **Dipende da:** PT22, PT23, PT24
- **Modulo:** `app`
- **Da leggere:** [#913](https://github.com/Matleyx78/tecnigrafo/issues/913);
  [ADR 0023](../../adr/0023-layer-e-proprieta.md), punti 2–4;
  [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), area 8;
  `app/src/properties_panel.cpp`, `app/src/property_grid.cpp`,
  `app/src/property_widgets.cpp`, `app/src/quick_select_dialog.cpp`; la scheda
  [PT22](PT22-proprieta-core.md)

## File

- `app/src/properties_panel.hpp`, `app/src/properties_panel.cpp`
- `app/src/property_grid.hpp`, `app/src/property_grid.cpp`
- `app/src/property_widgets.hpp`, `app/src/property_widgets.cpp`
- `app/src/quick_select_dialog.cpp`
- `tests/app/properties_panel_test.cpp`, `tests/app/quick_select_dialog_test.cpp`

## Cosa fare

1. Il tipo della selezione e il filtro per tipo usano `entityTypeName` (#913):
   un test fallisce se un tipo di `core::Entity` non ha il suo nome nel
   pannello (`Ellipse`, `Solid`, `MText`, `Attribute Definition`, `Block
   Reference`, `Dimension`, `Hatch`, come AutoCAD).
2. Per una sola entità della 0.8, sotto *General*, un gruppo con le proprietà
   di `ownProperties`: numeri, angoli in gradi, punti (X e Y), testi, scelte,
   segni; le derivate in sola lettura. Ogni cambiamento chiama
   `withOwnProperty` in una transazione `Properties`, come oggi; un rifiuto lo
   scrive nella riga di comando e lascia il valore di prima.
3. `SOLID` con *Current Vertex* come la polilinea (frecce e coordinate del
   vertice corrente).
4. `INSERT`: un gruppo *Attributes* con un valore per attributo; il valore
   di `MTEXT` si apre con un pulsante nell'editor di PT12, se c'è, o in un campo
   di testo.
5. `QSELECT`: il filtro per tipo con tutti i tipi.

## Regole della tappa che valgono qui

- Dopo PT25 `properties_panel.cpp` lo cambia anche [PT33](PT33-segni-nella-vista.md)
  (il segno sul vertice corrente), che dipende da questo pacchetto.

## Test richiesti

- Il nome di ogni tipo; un retino con scala cambiata dal pannello e `U`; un
  rifiuto; un `INSERT` con un attributo cambiato; `QSELECT` con `Hatch`.

## Fatto quando

- I test passano; la CI è verde.
