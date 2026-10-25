# [0.6-P22] `app`: pannello dei layer

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #255
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: interfaccia sopra operazioni verificate da
  R12; test Qt (decisione 5 del piano)
- **Dipende da:** P12, P21
- **Modulo:** `app`
- **Da leggere:** [ADR 0023](../../adr/0023-layer-e-proprieta.md) punti 4 e 5,
  [ADR 0009](../../adr/0009-interfaccia-grafica.md) punto 3;
  `edit/include/tg/edit/editor.hpp`, `layer_rules.hpp` (P6);
  `app/src/property_widgets.hpp`, `color_dialog.hpp` (P21);
  `app/src/main_window.cpp` (`addPanel`, P17), `app/src/app_commands.cpp` (P16)

## File

- `app/src/layer_panel.hpp`, `app/src/layer_panel.cpp` (nuovi)
- `app/src/layer_table_model.hpp`, `app/src/layer_table_model.cpp` (nuovi)
- `app/src/app_commands.cpp` (comando `LAYER`, alias `LA`)
- `app/src/builtin_actions.cpp` (azioni `layer.panel`, `layer.new`,
  `layer.delete`, `layer.current`, con icona)
- `app/src/ui_builder.cpp` (il menu a tendina dei layer nel gruppo
  `home/layers` e nella barra `Layers`)
- `app/src/main_window.cpp`
- `app/CMakeLists.txt`
- `tests/app/layer_panel_test.cpp` (nuovo), `tests/app/icons_test.cpp`
- `tests/CMakeLists.txt`

## Cosa fare

- **Pannello** (`QDockWidget`, `objectName` `layerPanel`): una tabella con
  colonne Stato (corrente), Nome, Acceso, Congelato, Bloccato, Colore, Tipo di
  linea, Spessore, Stampa. Clic sulle icone di stato per cambiarle; doppio
  clic sul nome per rinominare; colore, tipo e spessore con i controlli di
  P21.
- Pulsanti **Nuovo**, **Cancella**, **Rendi corrente**. Il nome di un layer
  nuovo è `Layer1`, `Layer2`… (il primo libero).
- Ogni cambio è una **transazione** `Layer` dell'editor, quindi si annulla con
  `U` e `Ctrl+Z`. Le regole di `layer_rules` decidono: un'operazione rifiutata
  mostra il motivo (gli stessi testi dei messaggi di `-LAYER`, P12) e non
  cambia nulla. Prima di rinominare un layer nominato in altri dati non
  gestiti, una domanda di conferma con il testo dell'ADR 0023, punto 5.
- Il pannello si aggiorna dopo ogni modifica del documento, annulla e ripeti
  compresi.
- **Comando `LAYER`** (`LA`): mostra il pannello e gli dà il fuoco, senza
  messaggi né domande; la sua descrizione ha la sintesi e nessuna domanda
  (ADR 0026, punto 3).
- **Menu a tendina** dei layer (`LayerCombo`) nella ribbon e nella barra
  `Layers`: sceglie il layer corrente; accanto, i pulsanti di stato del layer
  corrente.

## Regole della tappa che valgono qui

- Le icone di layer esistono già (P19): collegale alle azioni e includile nel
  test delle icone.
- File comuni di `app`: solo le tue righe.

## Test richiesti

- Nuovo, rinomina (anche con avviso), cancella (permesso e rifiutato),
  corrente, ogni stato, colore, tipo, spessore; annulla e ripeti di ognuno.
- Il pannello segue un `-LAYER` scritto nella riga di comando.
- Menu a tendina: cambia il layer corrente; segue i cambi del pannello.
- `LAYER` apre il pannello.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
