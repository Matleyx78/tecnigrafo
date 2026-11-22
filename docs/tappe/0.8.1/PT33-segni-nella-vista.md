# [0.8.1-PT33] `app`: vertice corrente, contorni evidenziati, calamita e editor dal pannello

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nella vista; test Qt e prova
  manuale
- **Dipende da:** PT12, PT25, PT26, PT32
- **Modulo:** `app`
- **Da leggere:** [#799](https://github.com/Matleyx78/tecnigrafo/issues/799);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-21, «Testo formattato, griglia,
  snap e attributi della 0.8.1», punto 9; `app/src/cad_view.cpp`,
  `app/src/properties_panel.cpp`; le schede [PT12](PT12-editor.md),
  [PT25](PT25-pannello.md), [PT26](PT26-hatchedit.md) e
  [PT31](PT31-prolungamento-parallelo.md) (le guide di `TrackingState`)

## File

- `app/src/cad_view.hpp`, `app/src/cad_view.cpp`
- `app/src/properties_panel.hpp`, `app/src/properties_panel.cpp` (il segnale del
  vertice corrente; il pulsante del valore di `MTEXT`)
- `app/src/text_editor_overlay.hpp`, `app/src/text_editor_overlay.cpp` (solo:
  aprire l'editor su un `MTEXT` scelto, senza una richiesta del runner)
- `tests/app/cad_view_test.cpp`, `tests/app/properties_panel_test.cpp`

## Cosa fare

1. **Vertice corrente (#799):** appena si usano le frecce di *Current Vertex*
   nel pannello (polilinea o `SOLID`), un cerchietto attorno al grip di quel
   vertice; si sposta con le frecce e sparisce quando cambia la selezione.
2. **Contorni evidenziati:** finché `-HATCH` è in corso, le curve di
   `Command::highlighted()` (PT26) si disegnano evidenziate, come la selezione.
3. **Guide del tracciamento** (verbale della revisione PT5, [bloccante] 9):
   la vista disegna le guide attive dall'elenco di `TrackingState` (PT31),
   con la loro geometria (semiretta, tratto di cerchio oltre l'estremo,
   parallela per il punto base) e l'incrocio, invece di ricostruire
   orizzontale, verticale e polare da ogni punto acquisito come oggi
   (`cad_view.cpp`); una guida rilasciata sparisce. Le croci dei punti
   acquisiti restano.
4. **Calamita:** con la preferenza accesa (PT32), il cursore a croce salta sul
   punto dello snap trovato; il punto del comando non cambia (lo decide già
   `cmd`).
5. **Editor dal pannello** (verbale della revisione del piano, [bloccante] 5):
   nel pannello, accanto al valore di un `MTEXT`, un pulsante apre l'editor di
   PT12 sul testo nel disegno; *Conferma* cambia il valore in una transazione
   `Properties` (un passo di annullamento), *Annulla* non cambia nulla.

## Regole della tappa che valgono qui

- Nessuna.

## Test richiesti

- Le guide di `TrackingState` nella vista: un prolungamento d'arco e una
  guida diritta che si incrociano si disegnano tutti e due con l'incrocio, con
  OTRACK spento e *Prolungamento* acceso; dopo il rilascio non si disegnano
  più ([bloccante] 9).
- Il segnale del vertice corrente e il segno nella vista; le curve evidenziate
  durante `-HATCH` e dopo `Invio`; la posizione del cursore con e senza
  calamita; l'editor aperto dal pannello, confermato (valore nuovo, un `U` lo
  annulla) e annullato (nessun cambiamento).

## Fatto quando

- I test passano; la CI è verde.
