# [0.7.5-P14] `app`: pannello *Properties* a griglia

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: aspetto dell'interfaccia sopra operazioni già
  verificate (0.6-P24); test Qt, galleria e prova manuale
- **Dipende da:** P13
- **Modulo:** `app`
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punti 1 (area 8), 2 e 3; [ADR 0023](../../adr/0023-layer-e-proprieta.md),
  punti 2–4; `app/src/properties_panel.hpp` e `.cpp`,
  `app/src/property_widgets.hpp` e `.cpp`, `tests/app/properties_panel_test.cpp`

## File

- `app/src/properties_panel.hpp`, `app/src/properties_panel.cpp`
- `app/src/property_grid.hpp`, `app/src/property_grid.cpp` (nuovi: il modello e
  il delegato della griglia)
- `app/src/property_widgets.hpp`, `app/src/property_widgets.cpp`, solo se le
  combo servono come editori del delegato
- `app/CMakeLists.txt` (i sorgenti nuovi)
- `tests/app/properties_panel_test.cpp`, `tests/app/property_grid_test.cpp`
  (nuovo), `tests/CMakeLists.txt`

## Cosa fare

1. Il pannello diventa, sotto la combo del tipo della selezione e i suoi due
   pulsanti (filtro, selezione rapida, con le icone), una **griglia** a due
   colonne: un `QTreeView` con un modello e un delegato propri, gruppi che si
   chiudono (*General*, *Geometry*), righe da 20–22 px, nome a sinistra nel
   testo secondario e valore a destra.
2. I valori di *General* si cambiano nella cella, con lo stesso editore di oggi
   (combo del colore, del layer, del tipo di linea, dello spessore, campo della
   scala), aperto dal delegato; *VARIES* e i valori correnti senza selezione
   restano come oggi.
3. *Geometry* resta in sola lettura e mostra ciò che mostra oggi; per una
   polilinea, al posto del solo primo vertice, **lo scorrimento dei vertici**:
   *Current Vertex* con le frecce precedente e successivo, *Vertex X* e
   *Vertex Y* del vertice scelto, poi la lunghezza. L'area non si aggiunge:
   `core` non la calcola, e una funzione nuova di `core` non sta in questo
   pacchetto.
4. Ogni cambio resta una transazione «Properties» (`U` la annulla); gli oggetti
   sui layer bloccati non si mostrano né si cambiano, come oggi.

## Regole della tappa che valgono qui

- `property_widgets.*`: prima di te P13. `properties_panel.*`: dopo di te P18.
- Interfaccia in inglese.

## Test richiesti

- `property_grid_test.cpp`: gruppi, righe e valori del modello per una linea,
  un cerchio e una polilinea; lo scorrimento dei vertici, ai due estremi
  compresi.
- `properties_panel_test.cpp`: i test di oggi passano ancora (cambiano solo i
  widget che leggono); un cambio di colore dalla cella è una transazione
  annullabile.

## Fatto quando

- La galleria mostra la griglia come nell'ADR 0039, con una polilinea
  selezionata.
- I test qui sopra esistono e passano.
- La CI è verde.
