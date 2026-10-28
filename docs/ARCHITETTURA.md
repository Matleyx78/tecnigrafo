# Architettura in breve

La mappa per orientarsi nel codice. Le motivazioni sono negli ADR, elencati in
fondo.

## Moduli

```
core  ←  render  ←──────────  app
core  ←  edit  ←  cmd  ←────  app      (dalla tappa 0.2)
core  ←  io    ←  cmd                  (dalla tappa 0.5)
                  cmd  ←────  script   (dalla tappa 0.4)
                  cmd, io, script  ←  mcp  (dalla tappa 0.6)
```

| Modulo | Namespace | Contiene | Può usare |
|---|---|---|---|
| `core` | `tg::core` | geometria (`Vec2`, `BBox`, `Transform2D`), entità (`Polyline`, `Line`, `Circle`, `Arc`, `Text`), misure del font incluso, `Document`, tabelle dei layer e dei tipi di linea, proprietà delle entità (`color`, `linetype`, `ltscale`, `lineweight`), valori correnti, indice spaziale e interrogazioni (scelta, selezione con rettangolo, snap) | solo la libreria standard |
| `render` | `tg::render` | `Viewport` (zoom e pan), `Painter` astratto (`drawPolyline`, `drawText`), disegno della scena | `core` |
| `edit` | `tg::edit` | `Editor`, transazioni, annulla/ripeti, regole per i layer | `core` |
| `io` | `tg::io` | lettura e scrittura DXF e formato nativo `.cadx`, astrazione per l'accesso ai file (`FileAccess`) | `core`, zlib (in `third_party/`, collegamento privato) |
| `cmd` | `tg::cmd` | comandi, riga di comando senza interfaccia, coordinate, selezione corrente, selezione rapida, snap e orto | `core`, `edit`, `io` |
| `script` | `tg::script` | modalità di controllo al livello 1: script `.tgs`, rapporto, domande al modello `.tgq`, confronto con i risultati attesi; l'eseguibile di sviluppo `tecnigrafo-script` | `core`, `edit`, `cmd` |
| `mcp` | `tg::mcp` | server MCP di sviluppo su stdio per assistenti IA (protocollo JSON-RPC, strumenti per eseguire comandi, script e casi di verifica); l'eseguibile di sviluppo `tecnigrafo-mcp` | `script`, `cmd`, `io`, nlohmann/json (in `third_party/`) |
| `tools` | — | `fontmetrics`, generatore delle misure del font TrueType durante la build | solo la libreria standard |
| `app` | `tg::app` | finestre Qt, registro delle azioni (`ActionRegistry`), interfaccia ribbon e classica (`UiBuilder`), temi e tavolozze (`theme`), pannelli agganciabili (`layer_panel`, `properties_panel`), finestre di dialogo (`settings`, `shortcuts`, `quick_select`), input da mouse e tastiera, `QtPainter` | tutti, più Qt e SARibbon (in `third_party/`) |

Solo `app` conosce Qt. Tutto il resto si prova con i test, senza finestre.

`script` e `mcp` non fanno parte del programma distribuito: li usano i test,
`tecnigrafo-script` e `tecnigrafo-mcp`, che eseguono i casi di verifica scritti prima del lavoro
([ADR 0011](adr/0011-modalita-di-controllo.md), [ADR 0027](adr/0027-server-mcp-di-sviluppo.md), specifiche in
[verifica/](verifica/casi.md)).

## Il kernel: solid-space

La geometria robusta, la politica delle tolleranze geometriche, il solver dei
vincoli, il naming topologico e i solidi 3D stanno in
[solid-space](https://github.com/Matleyx78/solid-space), il kernel di Tecnigrafo,
in un repository separato ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md)).
Entra come sottomodulo in `third_party/solid-space` con il primo pacchetto che lo
usa; `core` potrà usare solo `solid_space::robust`, e il futuro `modeling` gli
altri componenti. Nessun modulo collega OCCT direttamente. Ciò che serve si chiede
con le regole di [RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md).

## Dove si trova cosa

```
<modulo>/CMakeLists.txt                 target e dipendenze del modulo
<modulo>/include/tg/<modulo>/*.hpp      interfaccia pubblica
<modulo>/src/*.cpp                      implementazione
third_party/                            librerie esterne: zlib per gli archivi .cadx (ADR 0019), SARibbon per la ribbon (ADR 0009), nlohmann/json per il server MCP (ADR 0027)
tests/<modulo>/*_test.cpp               test del modulo
tests/fuzz/                             bersagli di fuzzing per lettori DXF, CADX e archivi ZIP (ADR 0020)
tests/verifica/casi/<area>/<nome>/      casi di verifica (ADR 0011), etichetta ctest "verifica"
tests/verifica/oracolo/                 oracolo ezdxf per ricavare i valori attesi dei DXF (ADR 0020)
docs/verifica/                          linguaggio degli script, rapporto, domande, casi
docs/adr/                               decisioni architetturali
docs/tappe/                             roadmap: schede e piani delle tappe
```

## Font e strumenti di build

`resources/fonts/` contiene Liberation Sans Regular 2.1.5, la licenza OFL 1.1
e i dati di provenienza. `tools/fontmetrics` legge il TTF e genera la tabella
inclusa da `core`: ascendente, discendente e avanzamento di ogni carattere,
normalizzati all’altezza delle maiuscole. CMake rigenera la tabella quando cambia
il font. Il generatore è uno strumento di build, non una dipendenza da Qt né un
componente del programma distribuito.

`core::textWidth`, `fontMetrics` e `textBox` permettono di misurare, selezionare
e verificare il testo senza finestre. `core::displayText` converte i codici di
controllo DXF; `core::fontCharacter` applica le sostituzioni dei caratteri
mancanti. Il valore originale di `Text` resta intatto. `QtPainter` usa lo stesso
TTF, incluso nelle risorse di `app`, e gli avanzamenti calcolati da `core`,
senza crenatura, hinting o font di riserva del sistema (ADR 0013).

## Due sistemi di coordinate

- **Disegno (mondo):** `double`, unità di disegno, Y verso l'alto, angoli in
  radianti in senso antiorario. Tutta la geometria vive qui.
- **Schermo:** pixel logici, origine in alto a sinistra, Y verso il basso. Esiste
  solo nella vista e nel `Painter`.

Il passaggio dall'uno all'altro avviene sempre e solo con `render::Viewport`.

## Come si disegna un fotogramma

   1. Qt chiede a `CadView` di ridisegnarsi (`paintEvent`).
   2. `CadView` crea un `QtPainter` e chiama `render::renderScene`.
   3. `renderScene` disegna sfondo, griglia e assi. Per polilinee, linee, cerchi
      e archi visibili converte i punti in coordinate dello schermo e chiama
      `Painter::drawPolyline`; le curve sono approssimate con segmenti secondo
      una tolleranza in pixel. Per `Text` prepara un `TextRun` con i caratteri
      convertiti, la linea di base sullo schermo, l’altezza delle maiuscole in
      pixel, la rotazione e il fattore di larghezza, poi chiama
      `Painter::drawText`. Sotto 2 pixel di altezza disegna invece il rettangolo
      d’ingombro. `QtPainter` disegna ogni carattere con il font incluso nelle
      posizioni determinate dalle misure di `core`.
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
| [0009](adr/0009-interfaccia-grafica.md) | Impianto dell'interfaccia grafica | Accettato |
| [0010](adr/0010-segnalazione-errori.md) | Segnalazione di errori e crash | Proposto |
| [0011](adr/0011-modalita-di-controllo.md) | Modalità di controllo e verifica automatica | Accettato |
| [0012](adr/0012-linea-cerchio-arco.md) | Linea, cerchio e arco | Accettato |
| [0013](adr/0013-testo-e-font.md) | Testo su una riga e font incluso | Accettato |
| [0014](adr/0014-richieste-di-valori-e-testo.md) | Richieste di distanza, angolo e testo | Accettato |
| [0015](adr/0015-esc-conserva-il-lavoro.md) | Esc conserva il lavoro già completato | Accettato |
| [0016](adr/0016-sanitizer-in-ci.md) | AddressSanitizer e UBSan in CI | Accettato |
| [0017](adr/0017-modulo-io-e-dxf.md) | Modulo `io` e lettura e scrittura del DXF | Accettato |
| [0018](adr/0018-conservazione-dati-non-gestiti.md) | Conservazione dei dati non gestiti | Accettato |
| [0019](adr/0019-archivi-zip.md) | Archivi ZIP per il formato `.cadx` | Accettato |
| [0020](adr/0020-fuzzing-e-oracolo-ezdxf.md) | Fuzzing dei lettori e oracolo ezdxf | Accettato |
| [0021](adr/0021-comandi-dei-file.md) | Comandi dei file | Accettato |
| [0022](adr/0022-allineamenti-obliquo-specchiatura-testo.md) | Allineamenti, obliquo e specchiatura del testo | Accettato |
| [0023](adr/0023-layer-e-proprieta.md) | Layer e proprietà delle entità | Accettato |
| [0024](adr/0024-colori.md) | Colori | Accettato |
| [0025](adr/0025-tipi-di-linea-e-spessori.md) | Tipi di linea e spessori | Accettato |
| [0026](adr/0026-descrizione-dei-comandi.md) | Descrizione dei comandi | Accettato |
| [0027](adr/0027-server-mcp-di-sviluppo.md) | Server MCP di sviluppo e libreria JSON | Accettato |
| [0028](adr/0028-versione-minima-di-qt.md) | Versione minima di Qt 6.8 | Accettato |
| [0029](adr/0029-verifica-dei-tipi-di-linea-iso.md) | Verifica dei tipi di linea ISO con un DXF di AutoCAD | Accettato |
| [0030](adr/0030-solid-space-kernel-di-tecnigrafo.md) | solid-space, il kernel geometrico di Tecnigrafo | Proposto |
