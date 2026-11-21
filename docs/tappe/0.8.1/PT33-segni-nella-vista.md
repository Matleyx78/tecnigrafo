# [0.8.1-PT33] `app`: vertice corrente, contorni evidenziati e calamita nella vista

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
  `app/src/properties_panel.cpp`; le schede [PT25](PT25-pannello.md) e
  [PT26](PT26-hatchedit.md)

## File

- `app/src/cad_view.hpp`, `app/src/cad_view.cpp`
- `app/src/properties_panel.hpp`, `app/src/properties_panel.cpp` (il segnale del
  vertice corrente)
- `tests/app/cad_view_test.cpp`, `tests/app/properties_panel_test.cpp`

## Cosa fare

1. **Vertice corrente (#799):** appena si usano le frecce di *Current Vertex*
   nel pannello (polilinea o `SOLID`), un cerchietto attorno al grip di quel
   vertice; si sposta con le frecce e sparisce quando cambia la selezione.
2. **Contorni evidenziati:** finché `-HATCH` è in corso, le curve di
   `Command::highlighted()` (PT26) si disegnano evidenziate, come la selezione.
3. **Calamita:** con la preferenza accesa (PT32), il cursore a croce salta sul
   punto dello snap trovato; il punto del comando non cambia (lo decide già
   `cmd`).

## Regole della tappa che valgono qui

- Nessuna.

## Test richiesti

- Il segnale del vertice corrente e il segno nella vista; le curve evidenziate
  durante `-HATCH` e dopo `Invio`; la posizione del cursore con e senza
  calamita.

## Fatto quando

- I test passano; la CI è verde.
