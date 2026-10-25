# ADR 0025: Tipi di linea e spessori

- **Stato:** Proposto
- **Data:** 2026-10-25

## Contesto

La tappa 0.6 disegna le linee continue, tratteggiate e a punto e linea, con una
scala globale, e gli spessori di linea. Il DXF definisce i tipi di linea nella
tabella `LTYPE` e gli spessori con valori fissi in centesimi di millimetro. Il
disegno deve restare corretto e veloce a ogni zoom, e la geometria dei tratti
deve avere riferimenti scritti prima del lavoro (ADR 0011), quindi non può
nascere nel render.

## Decisione

### 1. Definizione di un tipo di linea

- Un tipo di linea ha nome, descrizione e un **motivo**: una sequenza di
  elementi, ciascuno un **tratto** (lunghezza > 0), uno **spazio** (< 0) o un
  **punto** (0), in unità del disegno. La lunghezza del motivo è la somma dei
  valori assoluti.
- Un tipo senza elementi è continuo.
- Gli elementi complessi (testi e forme, gruppo 74 diverso da 0) si conservano
  grezzi e si riscrivono uguali; nella 0.6 il tipo si disegna con i soli
  tratti, spazi e punti.

### 2. Tipi obbligatori e predefiniti

- Ogni disegno ha `ByBlock`, `ByLayer` e `Continuous`, come in AutoCAD; non si
  rinominano né si cancellano.
- Tecnigrafo offre due tipi da caricare con `-LINETYPE Load`, con le
  proporzioni della norma ISO 128 per linee da 1 mm e unità in millimetri:

  | Nome | Descrizione | Motivo |
  |---|---|---|
  | `DASHED` | `Dashed __ __ __` | 12, −3 |
  | `DASHDOT` | `Dash dot __ . __ .` | 24, −3, 0, −3 |

- Un DXF aperto porta le sue definizioni, che restano com'erano anche se
  hanno lo stesso nome dei nostri tipi. Nella 0.6 non si leggono file `.lin`.

### 3. Scala

La lunghezza di ogni elemento si moltiplica per `LTSCALE` (del disegno) e per
la scala dell'entità (gruppo 48, che prende il valore di `CELTSCALE` quando
l'entità nasce). La scala dello spazio carta (`PSLTSCALE`) arriva con i layout.

### 4. Tratti in coordinate del disegno

`core` calcola i tratti di un'entità: dato l'entità, il motivo e la scala,
restituisce i pezzi da disegnare (segmenti, archi e punti) in coordinate del
disegno. Regole:

- il motivo comincia dall'inizio dell'entità con il primo elemento, e si
  tronca alla fine: un tratto tagliato dalla fine si accorcia, uno spazio
  tagliato non lascia nulla;
- `LINE` dall'inizio alla fine; `ARC` dall'angolo iniziale, in senso
  antiorario; `CIRCLE` dall'angolo 0, in senso antiorario;
- `LWPOLYLINE`: il motivo riparte da ogni vertice, a meno che la polilinea
  abbia la generazione continua (gruppo 70, bit 128), che lo fa proseguire
  lungo tutta la polilinea; i tratti seguono gli archi dei bulge;
- un'entità (o, senza generazione continua, un segmento di polilinea) più
  corta della lunghezza del motivo si disegna continua;
- il testo non ha tipo di linea.

La domanda al modello `dashes` restituisce questi pezzi, così la geometria
ha i suoi riferimenti (pacchetti 0.6-P5 e 0.6-P11).

### 5. Disegno a schermo

- Se la lunghezza del motivo, vista sullo schermo, è minore di 4 pixel,
  l'entità si disegna continua: i tratti non si distinguerebbero.
- Un'entità che darebbe più di 100 000 pezzi si disegna continua.
- Un punto si disegna come un tratto di 1 pixel lungo la direzione della linea.

### 6. Spessori

- Valori ammessi, in centesimi di millimetro: 0, 5, 9, 13, 15, 18, 20, 25, 30,
  35, 40, 50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211; più
  `ByLayer` (−1), `ByBlock` (−2) e `Default` (−3). Un valore diverso letto da un
  file si arrotonda al valore ammesso più vicino e lo si conta nel messaggio
  d'apertura.
- `Default` vale `$LWDEFAULT` (25 per un disegno nuovo).
- `LWDISPLAY` (variabile del disegno, `$LWDISPLAY`, spenta per un disegno
  nuovo) decide se gli spessori si vedono. Si cambia anche con il pulsante
  `LWT` della barra di stato.
- Con `LWDISPLAY` acceso, la larghezza a schermo è
  `max(1, round(mm × 96 / 25,4 × devicePixelRatio))` pixel, indipendente dallo
  zoom, come nello spazio modello di AutoCAD; spento, ogni linea è larga 1
  pixel.

## Alternative considerate

- **Tratti calcolati nel render, in pixel:** più semplice, ma senza
  riferimenti ricavabili prima del lavoro, e diverso dal significato del DXF,
  dove il motivo è in unità del disegno.
- **Motivo centrato sull'entità** (tratti uguali ai due capi): più gradevole,
  ma non è ciò che chiede il DXF e complica le polilinee; si potrà valutare.
- **Definizioni di `acadiso.lin`** per `DASHED` e `DASHDOT`: i numeri
  coinciderebbero con AutoCAD, ma il file è di Autodesk; la norma ISO dà
  proporzioni pubbliche. In alternativa si possono adottare i nomi ISO
  (`ACAD_ISO02W100`, `ACAD_ISO04W100`).
- **Spessore che cresce con lo zoom:** è ciò che serve in stampa (tappa 0.9),
  non a schermo.

## Conseguenze

- `core` riceve un algoritmo nuovo con riferimenti propri; il render lo usa e
  aggiunge solo le soglie in pixel.
- La stampa (tappa 0.9) userà gli stessi tratti, con lo spessore in
  millimetri.
- I tipi di linea complessi e i file `.lin` restano per dopo la 1.0.
