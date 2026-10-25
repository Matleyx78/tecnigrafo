# [0.6-P24] `app`: pannello delle proprietà e finestra `QSELECT`

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: interfaccia sopra operazioni verificate da R14
  e R23; test Qt (decisione 5 del piano)
- **Dipende da:** P22, P23
- **Modulo:** `app`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punti 2–4;
  `cmd/include/tg/cmd/quick_select.hpp` (P23); `app/src/property_widgets.hpp`
  (P21), `app/src/layer_panel.cpp` (P22, come è fatto un pannello)

## File

- `app/src/properties_panel.hpp`, `app/src/properties_panel.cpp` (nuovi)
- `app/src/quick_select_dialog.hpp`, `app/src/quick_select_dialog.cpp` (nuovi)
- `app/src/app_commands.cpp` (comandi `PROPERTIES` e `QSELECT`)
- `app/src/builtin_actions.cpp` (azioni `properties.panel`,
  `edit.quick-select`, con icona)
- `app/src/ui_builder.cpp` (controlli delle proprietà nel gruppo
  `home/properties` e nella barra `Properties`)
- `app/src/main_window.cpp`
- `app/CMakeLists.txt`
- `tests/app/properties_panel_test.cpp`, `tests/app/quick_select_dialog_test.cpp`
  (nuovi), `tests/app/icons_test.cpp`
- `tests/CMakeLists.txt`

## Cosa fare

### Pannello delle proprietà

- `QDockWidget`, `objectName` `propertiesPanel`. In alto il tipo della
  selezione (`No selection`, `Line`, `All (3)` con un filtro per tipo).
- Sezione **General**: colore, layer, tipo di linea, scala del tipo,
  spessore, con i controlli di P21. Un valore diverso tra le entità mostra
  `*VARIES*`.
- Senza selezione mostra e cambia i **valori correnti** (come AutoCAD).
- Ogni cambio è una transazione `Properties` (annullabile); le entità dei
  layer bloccati non sono mai nella selezione (P14).
- Sezione **Geometry** in sola lettura nella 0.6: le coordinate principali
  dell'entità singola, come nel rapporto.

### Barra delle proprietà

I controlli colore, tipo e spessore nel gruppo `home/properties` della ribbon
e nella barra `Properties`: con una selezione cambiano le entità, senza
selezione i valori correnti.

### `QSELECT`

Finestra con: dove cercare (tutto il disegno o la selezione corrente), tipo
di oggetto, proprietà, operatore (`=` o `≠`), valore (con il controllo
adatto), "Includi" o "Escludi". **OK** chiama `cmd::quickSelect` e
`CommandRunner::replaceSelection`, e scrive nella cronologia
`<n> item(s) selected.`

### Comandi

`PROPERTIES` (`PR`, `CH`, `MO`, `PROPS`) mostra il pannello; `QSELECT` apre
la finestra; con le loro descrizioni senza domande (ADR 0026).

## Regole della tappa che valgono qui

- Le icone esistono già (P19): collegale e includile nel test delle icone.
- File comuni di `app`: solo le tue righe.

## Test richiesti

- Pannello: selezione vuota, singola, mista con `*VARIES*`; ogni proprietà
  cambiata e annullata; filtro per tipo.
- Barra delle proprietà con e senza selezione.
- `QSELECT`: un criterio per proprietà, Escludi, dentro la selezione.
- I tre comandi aprono ciò che devono.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
