# ADR 0047: Quote e stili di quota

- **Stato:** Proposto
- **Data:** 2026-11-11

## Contesto

La tappa 0.8 porta le quote lineari, allineate, angolari, radiali e diametrali,
con gli stili di quota essenziali (testo, frecce, unità, precisione). Il modello
della quota tiene separati presentazione, misura e valore (decisione del
2026-11-07; [ADR 0044](0044-entita-composte-e-geometria-generata.md), punto 5),
perché la quota possa diventare associativa e, negli schizzi, un vincolo. La
quota della 0.8 resta un'annotazione: non segue la geometria.

In un DXF la quota è un'entità `DIMENSION` con i punti di definizione, il nome
del suo stile, le variabili sovrascritte (XDATA `ACAD`, `DSTYLE`) e il nome di un
blocco anonimo `*D…` che contiene ciò che si vede: linee, frecce (`SOLID` o
blocchi), testo (`MTEXT`), punti. Le variabili correnti stanno nell'intestazione
(`$DIMSTYLE`, `$DIMTXT`, …) e gli stili nella tabella `DIMSTYLE`.

## Decisione

### 1. Tipi

- Nella 0.8: **lineare** (orizzontale, verticale o ruotata; tipo 0),
  **allineata** (1), **angolare fra due linee** (2), **diametrale** (3),
  **radiale** (4).
- Gli altri tipi (angolare a tre punti, coordinata, lunghezza d'arco, raggio
  spezzato) sono `Dimension` di tipo *altro*: si mostrano con la loro grafica,
  si spostano e si copiano, e ogni altra modifica si rifiuta (punto 4).

### 2. Stili di quota

- `core::DimStyleTable` (ADR 0044, punto 3) con i record `DIMSTYLE`. Ogni
  record ha le **variabili essenziali** nel modello e tutte le altre in
  `extras`. Lo stesso per le variabili correnti dell'intestazione.
- Le variabili essenziali: `DIMSCALE`, `DIMASZ`, `DIMEXO`, `DIMEXE`, `DIMTXT`,
  `DIMGAP`, `DIMTAD`, `DIMTIH`, `DIMTOH`, `DIMDEC`, `DIMADEC`, `DIMDSEP`,
  `DIMZIN`, `DIMAZIN`, `DIMLFAC`, `DIMRND`, `DIMPOST`, `DIMTXSTY`, `DIMCEN`,
  `DIMSE1`, `DIMSE2`, `DIMSD1`, `DIMSD2`, `DIMCLRD`, `DIMCLRE`, `DIMCLRT`,
  `DIMLWD`, `DIMLWE`.
- **Rigenerabile.** Tecnigrafo sa rigenerare una quota quando, nel suo stile
  con le sovrascritture, queste variabili hanno il valore dello stile `ISO-25`:
  `DIMBLK`, `DIMBLK1`, `DIMBLK2` e `DIMLDRBLK` (freccia piena chiusa),
  `DIMSAH` 0, `DIMTOL` 0, `DIMLIM` 0, `DIMALT` 0, `DIMLUNIT` 2 (decimale),
  `DIMAUNIT` 0 (gradi decimali), `DIMJUST` 0, `DIMTVP` 0, `DIMFRAC` 0,
  `DIMTSZ` 0 (frecce, non tratti obliqui), e le variabili dell'adattamento,
  `DIMATFIT` 3, `DIMTMOVE` 0, `DIMTIX` 0, `DIMSOXD` 0 e `DIMTOFL` con il valore
  di `ISO-25` (punto 4). Altrimenti la quota non è rigenerabile.
- **Disegno nuovo:** gli stili `Standard` e `ISO-25`, corrente `ISO-25`, con i
  valori del modello `acadiso.dwt` di AutoCAD, che il responsabile conferma con
  `-DIMSTYLE` → *STatus* in AutoCAD LT (piano della tappa, «Prerequisiti»).
- **`-DIMSTYLE`** (alias `DIMSTYLE`): le opzioni *Save*, *Restore*, *STatus* e
  `?`; *ANnotative*, *Variables* e *Apply* restano fuori. Ogni variabile
  essenziale si legge e si cambia scrivendone il nome, come in AutoCAD: il
  valore nuovo è una sovrascrittura dello stile corrente finché non si salva
  con *Save*.

### 3. Valore e testo

- **Misura:** distanza dei due punti lungo la direzione della quota (lineare),
  distanza dei due punti (allineata), angolo fra le due semirette (angolare),
  raggio o diametro.
- **Testo:** la misura moltiplicata per `DIMLFAC`, arrotondata a `DIMRND` se è
  maggiore di 0, scritta con `DIMDEC` cifre decimali (`DIMADEC` per gli angoli,
  `DIMDEC` se `DIMADEC` è −1) e il separatore `DIMDSEP`, senza gli zeri finali
  se `DIMZIN` ha il bit 8 (`DIMAZIN` per gli angoli). La conversione in cifre
  usa il valore binario esatto del numero (`std::to_chars`), così è uguale su
  tutti i sistemi; può differire da AutoCAD solo sui numeri a metà strada.
- Prefissi: `R` per il raggio, `%%c` per il diametro; gli angoli finiscono con
  `%%d`. `DIMPOST` aggiunge prefisso e suffisso (con `<>` al posto del valore).
- **Testo sostituito** (gruppo 1): vuoto = il valore; con `<>` il valore al
  suo posto; uno spazio = nessun testo.

### 4. Grafica e rigenerazione

- La grafica (ADR 0044, punto 5) contiene linee e archi della quota, frecce
  come `SOLID` (punta, base lunga `DIMASZ·DIMSCALE` e larga un terzo) e il testo
  come `MTEXT`, sul layer `0` con colori e spessori *DaBlocco* salvo le
  variabili di colore e spessore. Non contiene `POINT` ai punti di definizione.
- **Le regole geometriche** (dove vanno linee d'estensione, frecce e testo per
  ogni tipo) le scrivono le schede delle quote in `core`.
- **Quando non ci stanno** (decisione del 2026-11-11, punto 6 del piano), come
  AutoCAD con `DIMATFIT` 3 e `DIMTMOVE` 0, in quest'ordine: testo e frecce
  dentro; solo il testo dentro e le frecce fuori; solo le frecce dentro e il
  testo fuori, accanto a una linea d'estensione con la linea di quota
  prolungata fino a lui; tutto fuori. AutoCAD non pubblica le soglie né le
  posizioni esatte: le schede le ricavano dai disegni di AutoCAD LT del
  responsabile (quote strette in `autocad-quote.dxf`), e dove un caso
  contraddice la scheda vale il disegno di AutoCAD.
- **Quando si rigenera:** alla creazione; e a ogni modifica, tranne la
  traslazione (`MOVE`, `COPY`, grip sul testo che si sposta soltanto), che
  sposta punti e grafica senza ricalcolare. Rotazione, scala, specchiatura e
  grip sui punti di definizione rigenerano la quota; con la specchiatura il
  testo resta leggibile.
- **Non rigenerabile** (tipo *altro*, o variabili del punto 2): la modifica si
  rifiuta con `Dimension cannot be regenerated by Tecnigrafo.` e la quota resta
  com'era; `MOVE` e `COPY` funzionano sempre.
- `EXPLODE` di una quota mette nel disegno le entità della sua grafica, come
  AutoCAD.

### 5. Comandi

Nella forma da riga di comando di AutoCAD, con i messaggi che il responsabile
copia dalla cronologia di AutoCAD LT (piano, «Prerequisiti»):

- `DIMLINEAR`: due origini delle linee d'estensione (o `Invio` per scegliere
  una linea), posizione della linea di quota con le opzioni *Text*, *Angle*,
  *Horizontal*, *Vertical* e *Rotated*;
- `DIMALIGNED`: lo stesso, con *Text* e *Angle*;
- `DIMANGULAR`: due linee, posizione dell'arco con *Text* e *Angle*; archi,
  cerchi e vertice indicato restano fuori;
- `DIMRADIUS`, `DIMDIAMETER`: un arco o un cerchio, posizione con *Text* e
  *Angle*.

L'opzione *Mtext* (l'editor nella finestra) resta fuori, come per `MTEXT`
([ADR 0046](0046-stili-di-testo-e-testo-multiriga.md); idea
[#662](https://github.com/Matleyx78/tecnigrafo/issues/662)). Ogni comando scrive alla
fine `Dimension text = <testo>`.

### 6. Associatività

Le quote associative dei file (oggetti `ACAD_DIMASSOC`) restano com'erano:
reattori e oggetti si conservano, e Tecnigrafo non segue la geometria. Una quota
spostata da sola in Tecnigrafo può tornare al suo oggetto se AutoCAD, riaprendo
il file, ricalcola l'associazione: è un rischio annotato nel piano, da provare
con un file della tappa.

## Alternative considerate

- **Rigenerare sempre le quote lette:** cambierebbe l'aspetto di ogni quota con
  opzioni che Tecnigrafo non ha.
- **Testo sempre in mezzo** quando non ci sta (la prima proposta): più
  semplice, ma con le quote strette, frequenti nel disegno meccanico, il testo
  si sovrapporrebbe alle linee d'estensione, e le quote di AutoCAD cambierebbero
  aspetto alla prima rigenerazione.
- **Tutte le varianti di `DIMATFIT` e `DIMTMOVE`:** molte, poco documentate; si
  rimandano. Con valori diversi da `ISO-25` la quota si conserva e si sposta.
- **Tutte le variabili di quota nel modello:** sono più di settanta; le altre
  si conservano e bloccano la rigenerazione solo se cambiano la grafica.
- **Quote associative già nella 0.8:** la scheda le mette dopo la 1.0.

## Conseguenze

- Le quote dipendono dal `SOLID` e dal `MTEXT` della tappa, e la loro scrittura
  nel DXF dalla sezione `BLOCKS` nel modello ([ADR 0045](0045-blocchi-e-attributi.md)).
- Il catalogo di parità registra le opzioni rimaste fuori.
