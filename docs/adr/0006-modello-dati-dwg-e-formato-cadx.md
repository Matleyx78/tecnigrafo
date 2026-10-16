# ADR 0006: Modello dati compatibile con DWG/DXF e formato di file `.cadx`

- **Stato:** Proposto
- **Data:** 2026-10-16

## Contesto

Chi usa un CAD 2D lavora con file DWG e DXF ricevuti da altri e li deve
restituire senza perdite. DWG è un formato chiuso; DXF ne è la versione
documentata, con lo stesso modello di dati (entità, tabelle, blocchi, oggetti,
handle), e la sua specifica è pubblicata gratuitamente da Autodesk. Per il disegno
2D non esiste uno standard aperto e indipendente diffuso: altri CAD liberi usano
direttamente DXF come formato proprio.

Tecnigrafo ha bisogno anche di un posto per le informazioni che DWG non prevede
(vincoli parametrici, legami con il modello 3D), e di un formato che resti
leggibile anche senza Tecnigrafo.

Questa decisione va presa prima della tappa 0.4, perché da lì in poi ogni nuova
entità deve nascere già compatibile.

## Decisione

### 1. Il modello dati segue quello di DWG/DXF

- Ogni entità di `tg::core` corrisponde a un'entità DXF, con gli stessi concetti e
  le stesse proprietà (per esempio `Polyline` ↔ `LWPOLYLINE`, ADR 0004). Una
  differenza è ammessa solo se la conversione resta **senza perdite in entrambe le
  direzioni**, e va documentata.
- Tabelle (layer, tipi di linea, stili di testo e di quota), blocchi e dizionari di
  oggetti seguono la struttura del DXF.
- `EntityId` corrisponde all'handle DXF: si conserva quando si apre e si salva un
  file, così i riferimenti tra oggetti restano validi.

### 2. Ciò che non capiamo si conserva intatto

Entità, oggetti, dati estesi (XDATA) e dizionari che Tecnigrafo non gestisce
vengono letti, conservati così come sono e riscritti identici al salvataggio. Non
si scartano mai in silenzio: se un'operazione non può conservarli, Tecnigrafo lo
dice prima di procedere.

### 3. Il formato nativo `.cadx`

Un file `.cadx` è un archivio ZIP con questa struttura:

```
mimetype            "application/vnd.tecnigrafo.cadx", primo file, non compresso
manifest.json       versione del formato, programma che l'ha scritto, elenco delle parti
drawing.dxf         l'intero disegno in DXF standard (versione R2018, ASCII, UTF-8)
extensions.json     solo i dati propri di Tecnigrafo (vincoli, parametri, legami 3D)
thumbnail.png       anteprima per i gestori di file (facoltativa)
```

- `drawing.dxf` è completo e autosufficiente: estratto dallo ZIP, si apre in
  qualunque CAD. Chi non conosce `.cadx` perde solo `extensions.json`.
- `extensions.json` fa riferimento alle entità tramite il loro handle.
- Il formato ha una versione intera in `manifest.json`. Tecnigrafo apre tutte le
  versioni precedenti e rifiuta con un messaggio chiaro le versioni più recenti.
- La specifica completa viene pubblicata in `docs/formati/cadx.md` (tappa 0.5),
  sotto licenza CC BY 4.0, perché chiunque possa implementarla.

### 4. DWG

DWG si leggerà e si scriverà come formato di scambio, convertendolo da e verso lo
stesso modello dati. La libreria da usare e il momento in cui introdurla (oggi
previsto dopo la 1.0, vedi la scheda della tappa 0.5) saranno decisi con un ADR
dedicato.

## Alternative considerate

- **DXF semplice come formato nativo** (come LibreCAD e QCAD): massima
  compatibilità, ma file molto grandi e nessuno spazio pulito per i nostri dati.
- **Formato del tutto nostro** (JSON o SQLite): libero da vincoli, ma un disegno
  diventerebbe illeggibile per chiunque non usi Tecnigrafo, e ogni scambio
  passerebbe da una conversione.
- **DWG come formato nativo:** formato chiuso, contro lo scopo del progetto.
- **Estensione con "dwg" nel nome:** il nome DWG è legato ad Autodesk; si evita per
  non creare confusione con il formato originale.

## Conseguenze

- Aprire e salvare DXF diventa quasi diretto, e i disegni restano leggibili in
  ogni caso.
- Ogni pacchetto che introduce un'entità o una proprietà (dalla tappa 0.4) deve
  indicare la corrispondenza DXF e avere un test di andata e ritorno.
- Il disegno sta in memoria e su disco nel modello DXF: alcune scelte interne
  (nomi, unità, angoli) seguono le convenzioni AutoCAD anche dove non sarebbero
  le più eleganti.
- Il DXF è controllato da Autodesk: le nuove versioni della specifica vanno
  seguite, ma il formato `.cadx` ne fissa una (R2018) finché un ADR non la cambia.
