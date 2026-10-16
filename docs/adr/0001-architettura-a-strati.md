# ADR 0001: Architettura a strati

- **Stato:** Accettato
- **Data:** 2026-10-15

## Contesto

Tecnigrafo dovrà offrire un CAD 2D e, sulla stessa base, un modellatore 3D
parametrico. L'interfaccia grafica e le librerie esterne cambieranno nel tempo
(backend di disegno, versioni di Qt, kernel geometrico); il modello dei dati e la
geometria devono sopravvivere a questi cambiamenti.

## Decisione

Il codice è diviso in moduli con dipendenze a senso unico:

```
core  ←  render  ←  app
```

- **core** (`tg::core`): geometria e modello del disegno. C++20 standard, nessuna
  dipendenza esterna.
- **render** (`tg::render`): viewport (zoom/pan) e disegno della scena attraverso
  l'interfaccia astratta `Painter`. Dipende solo da `core`.
- **app** (`tg::app`): applicazione Qt. Unico modulo che conosce Qt; contiene il
  backend `QtPainter`.

Le dipendenze sono imposte dai target CMake: `tg_core` e `tg_render` non hanno Qt
fra le librerie collegate, quindi un `#include <Q...>` lì non compila.

Moduli futuri previsti (ognuno richiederà il suo ADR): `io` (DXF e formato nativo),
`commands` (comandi e annullamento), `modeling` (3D su Open CASCADE), `drawing`
(tavole dal modello 3D).

## Alternative considerate

- **Tutto in un'unica applicazione Qt.** Più rapido all'inizio, ma lega il modello
  dati all'interfaccia ed è la causa principale dei progetti impossibili da
  rifattorizzare.
- **Usare i tipi Qt (QPointF, QList) anche nel modello.** Comodo, ma renderebbe
  impossibile testare e riusare la geometria senza Qt.

## Conseguenze

- La geometria e la viewport si testano senza interfaccia grafica.
- Il backend di disegno si può sostituire (OpenGL, Vulkan) toccando solo `app`.
- Serve una piccola conversione tra tipi `tg::core` e tipi Qt al confine.
