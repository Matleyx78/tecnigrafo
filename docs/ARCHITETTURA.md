# Architettura in breve

La mappa per orientarsi nel codice. Le motivazioni sono negli ADR, elencati in
fondo.

## Moduli

```
core  ←  render  ←──────────  app
core  ←  edit  ←  cmd  ←────  app      (dalla tappa 0.2)
                  cmd  ←────  script   (dalla tappa 0.4)
```

| Modulo | Namespace | Contiene | Può usare |
|---|---|---|---|
| `core` | `tg::core` | geometria (`Vec2`, `BBox`, `Transform2D`), entità (`Polyline`), `Document`, indice spaziale e interrogazioni (scelta, selezione con rettangolo, snap) | solo la libreria standard |
| `render` | `tg::render` | `Viewport` (zoom e pan), `Painter` astratto, disegno della scena | `core` |
| `edit` | `tg::edit` | `Editor`, transazioni, annulla/ripeti | `core` |
| `cmd` | `tg::cmd` | comandi, riga di comando senza interfaccia, coordinate, selezione corrente, snap e orto | `core`, `edit` |
| `script` | `tg::script` | modalità di controllo al livello 1: script `.tgs`, rapporto, domande al modello `.tgq`, confronto con i risultati attesi; l'eseguibile di sviluppo `tecnigrafo-script` | `core`, `edit`, `cmd` |
| `app` | `tg::app` | finestre Qt, input da mouse e tastiera, `QtPainter` | tutti, più Qt |

Solo `app` conosce Qt. Tutto il resto si prova con i test, senza finestre.

`script` non fa parte del programma distribuito: lo usano i test e
`tecnigrafo-script`, che esegue i casi di verifica scritti prima del lavoro
([ADR 0011](adr/0011-modalita-di-controllo.md), specifiche in
[verifica/](verifica/casi.md)).

## Dove si trova cosa

```
<modulo>/CMakeLists.txt                 target e dipendenze del modulo
<modulo>/include/tg/<modulo>/*.hpp      interfaccia pubblica
<modulo>/src/*.cpp                      implementazione
tests/<modulo>/*_test.cpp               test del modulo
tests/verifica/casi/<area>/<nome>/      casi di verifica (ADR 0011), etichetta ctest "verifica"
docs/verifica/                          linguaggio degli script, rapporto, domande, casi
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
   4. Sopra la scena `CadView` disegna, in quest'ordine, le entità selezionate
      (`renderHighlight`), l'anteprima del comando (`renderPreview`), il rettangolo
      di selezione e il marcatore dello snap attivo, chiedendo tutto al
      `CommandRunner`.

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
| [0009](adr/0009-interfaccia-grafica.md) | Impianto dell'interfaccia grafica | Proposto |
| [0010](adr/0010-segnalazione-errori.md) | Segnalazione di errori e crash | Proposto |
| [0011](adr/0011-modalita-di-controllo.md) | Modalità di controllo e verifica automatica | Accettato |
| [0012](adr/0012-linea-cerchio-arco.md) | Linea, cerchio e arco | Accettato |
| [0013](adr/0013-testo-e-font.md) | Testo su una riga e font incluso | Accettato |
| [0014](adr/0014-richieste-di-valori-e-testo.md) | Richieste di distanza, angolo e testo | Accettato |
| [0015](adr/0015-esc-conserva-il-lavoro.md) | Esc conserva il lavoro già completato | Accettato |
| [0016](adr/0016-sanitizer-in-ci.md) | AddressSanitizer e UBSan in CI | Accettato |
