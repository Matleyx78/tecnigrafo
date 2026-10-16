# Architettura in breve

La mappa per orientarsi nel codice. Le motivazioni sono negli ADR, elencati in
fondo.

## Moduli

```
core  ←  render  ←──────────  app
core  ←  edit  ←  cmd  ←────  app      (dalla tappa 0.2)
```

| Modulo | Namespace | Contiene | Può usare |
|---|---|---|---|
| `core` | `tg::core` | geometria (`Vec2`, `BBox`, `Transform2D`), entità (`Polyline`), `Document` | solo la libreria standard |
| `render` | `tg::render` | `Viewport` (zoom e pan), `Painter` astratto, disegno della scena | `core` |
| `edit` | `tg::edit` | `Editor`, transazioni, annulla/ripeti | `core` |
| `cmd` | `tg::cmd` | comandi, riga di comando senza interfaccia, coordinate | `core`, `edit` |
| `app` | `tg::app` | finestre Qt, input da mouse e tastiera, `QtPainter` | tutti, più Qt |

Solo `app` conosce Qt. Tutto il resto si prova con i test, senza finestre.

## Dove si trova cosa

```
<modulo>/CMakeLists.txt                 target e dipendenze del modulo
<modulo>/include/tg/<modulo>/*.hpp      interfaccia pubblica
<modulo>/src/*.cpp                      implementazione
tests/<modulo>/*_test.cpp               test del modulo
docs/adr/                               decisioni architetturali
docs/tappe/                             roadmap: schede e piani delle tappe
```

## Due sistemi di coordinate

- **Disegno (mondo):** `double`, unità di disegno, Y verso l'alto, angoli in
  radianti in senso antiorario. Tutta la geometria vive qui.
- **Schermo:** pixel logici, origine in alto a sinistra, Y verso il basso. Esiste
  solo nella vista e nel `Painter`.

Il passaggio dall'uno all'altro avviene sempre e solo con `render::Viewport`.

## Come si disegna un fotogramma

1. Qt chiede a `CadView` di ridisegnarsi (`paintEvent`).
2. `CadView` crea un `QtPainter` e chiama `render::renderScene`.
3. `renderScene` disegna sfondo, griglia e assi, poi per ogni entità visibile la
   approssima con segmenti (tolleranza in pixel), converte i punti in coordinate
   dello schermo e chiama `Painter::drawPolyline`.

## Decisioni architetturali

| ADR | Titolo | Stato |
|---|---|---|
| [0001](adr/0001-architettura-a-strati.md) | Architettura a strati | Accettato |
| [0002](adr/0002-linguaggio-e-strumenti.md) | Linguaggio, librerie e strumenti | Accettato |
| [0003](adr/0003-licenza.md) | Licenza GPLv3 o successiva | Accettato |
| [0004](adr/0004-modello-geometrico-2d.md) | Modello geometrico 2D | Accettato |
| [0005](adr/0005-comandi-e-annullamento.md) | Comandi, annullamento e input | Accettato |
| [0006](adr/0006-modello-dati-dwg-e-formato-cadx.md) | Modello dati compatibile con DWG/DXF e formato `.cadx` | Accettato |
| [0007](adr/0007-indice-spaziale.md) | Indice spaziale | Accettato |
| [0008](adr/0008-selezione-e-snap.md) | Selezione, snap a oggetto e orto | Accettato |
