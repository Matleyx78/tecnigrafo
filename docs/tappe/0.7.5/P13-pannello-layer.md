# [0.7.5-P13] `app`: combo dei layer a icone e pannello *Layers* rifatto

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: aspetto dell'interfaccia sopra operazioni già
  verificate (0.6-P22); test Qt, galleria e prova manuale
- **Dipende da:** P12
- **Modulo:** `app`
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punti 1 (aree 3 e 9), 2 e 3; [ADR 0023](../../adr/0023-layer-e-proprieta.md);
  `app/src/property_widgets.hpp` e `.cpp` (`LayerCombo`), `app/src/layer_panel.hpp`
  e `.cpp`, `app/src/layer_table_model.hpp` e `.cpp`, `app/src/ui_builder.cpp`
  (dove il gruppo *Layers* del ribbon usa la combo)

## File

- `app/src/property_widgets.hpp`, `app/src/property_widgets.cpp`
- `app/src/layer_panel.hpp`, `app/src/layer_panel.cpp`
- `app/src/layer_table_model.hpp`, `app/src/layer_table_model.cpp`
- `resources/icons/`: le icone a ruoli che il pannello chiede e non esistono
  (per esempio la ricerca), 16 px, con le regole di ICONE.md
- `tests/app/layer_panel_test.cpp`, `tests/app/property_widgets_test.cpp`

## Cosa fare

1. **Combo dei layer** (`LayerCombo`, nel ribbon e nel pannello *Properties*):
   ogni voce mostra le icone di stato (acceso/spento, scongelato/congelato,
   sbloccato/bloccato), il campione del colore e il nome, al posto di
   `0 [On, Thawed, Unlocked, Plot]`. Il testo per esteso resta nel suggerimento
   e nel nome accessibile.
2. **Pannello *Layers*** (ADR 0039, area 9):
   - in alto una barra di icone (nuovo, elimina, rendi corrente) al posto dei
     pulsanti di testo, e un campo di ricerca che filtra le righe per nome;
   - le colonne di stato hanno un'icona nell'intestazione e un'icona nella riga,
     che si cambia con un clic; la colonna del colore mostra il campione e il
     nome del colore; il layer corrente ha la spunta;
   - righe da 20–22 px, nessuna barra di scorrimento orizzontale finché le
     colonne entrano nella larghezza.
3. Le operazioni restano quelle di oggi, una transazione «Layer» ciascuna
   (`U` le annulla), con le regole di `edit::layer_rules`.

## Regole della tappa che valgono qui

- `layer_panel.*`, `property_widgets.*`: dopo di te P14 (`property_widgets.*`) e
  P18. `main_window.*`: prima di te P12.
- Interfaccia in inglese.

## Test richiesti

- `layer_panel_test.cpp`: la ricerca filtra; un clic sull'icona di stato cambia
  lo stato in una transazione annullabile; il layer corrente ha la spunta; con
  un solo layer non c'è barra di scorrimento orizzontale.
- `property_widgets_test.cpp`: le voci della combo hanno le icone di stato
  giuste e il suggerimento con il testo per esteso.

## Fatto quando

- La galleria mostra la combo e il pannello *Layers* come nell'ADR 0039.
- I test qui sopra esistono e passano.
- La CI è verde.
