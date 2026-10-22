# ADR 0018: Conservazione dei dati non gestiti

- **Stato:** Proposto
- **Data:** 2026-10-22

## Contesto

L'ADR 0006, punto 2, chiede che entità, oggetti, dati estesi e dizionari che
Tecnigrafo non gestisce si leggano, si conservino così come sono e si
riscrivano identici. Nella tappa 0.5 Tecnigrafo gestisce cinque entità (`LINE`,
`CIRCLE`, `ARC`, `LWPOLYLINE`, `TEXT`); un DXF reale contiene molto altro:
tabelle, blocchi, retini, quote, layout, dizionari, dati di altre applicazioni.

Restano anche due punti aperti da ADR precedenti:

- l'ADR 0012, punto 2, rimanda alla tappa 0.5 come conservare quota Z,
  spessore e direzione d'estrusione diversi dai valori predefiniti;
- l'ADR 0012, punto 1, rimanda alla tappa 0.5 cosa fare delle entità degeneri
  (raggio nullo o negativo) che arrivano da un file.

Vincoli: il modello resta 2D (ADR 0012); `core` non dipende da `io` né da
librerie esterne (ADR 0001); ogni modifica al documento passa da
`edit::Editor` e si può annullare (ADR 0005).

## Decisione

### 1. Gruppi grezzi in `core`

- `core::RawGroup` è un gruppo DXF così come è stato letto: codice (intero) e
  valore come testo UTF-8, **senza conversioni** (un numero resta la stringa
  scritta nel file). `core::RawGroups` è una sequenza ordinata di gruppi.
- I gruppi grezzi stanno in `core` perché devono viaggiare con il documento e
  con l'annullamento; `core` li conserva ma non li interpreta.

### 2. Il documento

- **Sezioni non gestite.** `HEADER` (tranne le variabili che il modello
  rappresenta: `$ACADVER`, `$DWGCODEPAGE`, `$HANDSEED`, `$INSUNITS`),
  `CLASSES`, `TABLES`, `BLOCKS`, `OBJECTS`, `THUMBNAILIMAGE`, `ACDSDATA` e
  ogni sezione sconosciuta si conservano come gruppi grezzi, nel loro ordine, e
  si riscrivono uguali. Layer, tipi di linea e stili (tappa 0.6) passeranno da
  qui al modello senza cambiare il formato.
- **Unità.** `$INSUNITS` diventa una proprietà del documento (ADR 0021).
- **Ordine delle entità.** Ogni entità, gestita o no, ha una chiave d'ordine:
  quella del file per le entità lette, la successiva per le nuove. Un'entità
  tolta e rimessa con l'annullamento conserva la sua chiave. Lo scrittore
  scrive le entità in ordine di chiave, così l'ordine di disegno di un file
  aperto e salvato non cambia.

### 3. Entità non gestite

Restano **non gestite**, conservate come gruppi grezzi con il loro handle e la
loro chiave d'ordine, non disegnate, non selezionabili e fuori dall'indice
spaziale:

- ogni tipo di entità diverso dai cinque della tappa;
- le entità dello spazio carta (gruppo 67 uguale a 1) o di un layout diverso
  dallo spazio modello (gruppo 330 diverso da `*Model_Space`);
- le entità con una direzione d'estrusione diversa da `(0,0,1)` e da
  `(0,0,-1)` (punto 5);
- le entità **degeneri**: raggio ≤ 0, altezza del testo ≤ 0, fattore di
  larghezza ≤ 0, arco con angolo iniziale uguale al finale, polilinea senza
  vertici. Chiude il punto lasciato aperto dall'ADR 0012.

All'apertura il messaggio dice quante entità non sono gestite, per tipo
(`3 entities not supported: HATCH 2, INSERT 1`). Il numero si legge anche nel
rapporto della modalità di controllo.

### 4. Ciò che un'entità gestita porta in più

Un'entità gestita conserva, insieme al record del documento, i gruppi che il
modello non rappresenta ancora:

- i gruppi tra l'handle e la sottoclasse `AcDbEntity`: reattori e dizionario
  d'estensione (`102 {ACAD_REACTORS …}`, `102 {ACAD_XDICTIONARY …}`) e il
  proprietario (330);
- i gruppi comuni di `AcDbEntity` non ancora nel modello: layer (8), colore
  (62, 420, 430, 440), tipo e scala di linea (6, 48), spessore di linea (370),
  visibilità (60) e gli altri;
- i gruppi della sottoclasse dell'entità che il modello non usa;
- i dati estesi (XDATA, dal gruppo 1001 in poi).

Questi gruppi fanno parte del record: si tolgono e si rimettono con
l'entità, quindi l'annullamento li conserva. Un'entità nuova, creata da un
comando, non ne ha: lo scrittore usa i valori predefiniti (layer `0`,
proprietario `*Model_Space`).

Un'entità cancellata può lasciare oggetti che la nominavano (per esempio un
dizionario d'estensione). Si conservano anche quelli: è ciò che fa AutoCAD, e
il suo comando `AUDIT` li ripulisce. Non è una perdita di dati.

### 5. Quota, spessore ed estrusione

- **Quota Z** (30, 31; 38 per `LWPOLYLINE`), **spessore** (39) e
  **direzione d'estrusione** (210/220/230) si conservano come gruppi grezzi e
  si riscrivono com'erano. Non si disegnano: il modello è 2D e mostra la
  proiezione sul piano XY.
- **Estrusione `(0,0,1)`** (con le prime due componenti entro 1e-12 da zero):
  è il caso normale.
- **Estrusione `(0,0,-1)`**: capita spesso nei disegni specchiati in AutoCAD.
  `LINE` ha i punti già nel sistema globale e non cambia. Per le entità nel
  sistema dell'oggetto, il modello riceve la geometria convertita nel sistema
  globale, `x → −x`:
  - `CIRCLE`: centro con x cambiata di segno;
  - `ARC`: centro come sopra; angolo iniziale `180° − fine`, angolo finale
    `180° − inizio`, sempre in senso antiorario;
  - `LWPOLYLINE`: vertici con x cambiata di segno, bulge cambiati di segno;
  - `TEXT`: posizione e punto d'allineamento con x cambiata di segno,
    rotazione `−r`, specchiatura orizzontale (71, bit 2) invertita.

  L'entità ricorda la direzione e lo scrittore la riporta nel sistema
  dell'oggetto con la conversione inversa.
- **Altre direzioni**: l'entità resta non gestita (punto 3).

### 6. Cosa vuol dire "identico"

Un file aperto e salvato senza modifiche è identico all'originale quando:

- le sezioni non gestite, le entità non gestite e i gruppi in più delle entità
  gestite sono uguali **carattere per carattere** dopo la decodifica (punto 3
  dell'ADR 0017), nello stesso ordine;
- le entità gestite hanno gli stessi valori del modello entro la tolleranza
  della modalità di controllo (per difetto 1e-9), perché i numeri si
  riscrivono dal modello (ADR 0017, punto 4);
- l'ordine delle entità è lo stesso;
- cambiano solo `$HANDSEED` e, se si converte la versione, `$ACADVER` e la
  codifica.

La domanda `roundtrip` della modalità di controllo applica questa definizione.

## Alternative considerate

- **Un tipo `Unknown` nella `std::variant` delle entità:** ogni `std::visit`
  del progetto (disegno, snap, selezione, rapporto) dovrebbe trattarlo, per
  un'entità che non si disegna e non si seleziona. Le entità non gestite
  stanno accanto alle altre, con la loro chiave d'ordine.
- **Conservare tutto il file e riscriverne solo le parti cambiate:** semplice
  per un file aperto e salvato, ma fragile non appena il modello cambia
  qualcosa, e non dice cosa il modello rappresenta davvero.
- **Scartare quota, spessore ed estrusione non predefiniti:** viola l'ADR
  0006.
- **Rendere non gestite anche le entità con estrusione `(0,0,-1)`:** molti
  disegni reali perderebbero archi e cerchi a schermo.
- **Rappresentare il 3D nel modello 2D:** il 3D avrà il suo modello (fase 2);
  qui servirebbe solo a conservare file insoliti.

## Conseguenze

- `core::Document` cresce: gruppi grezzi delle sezioni, entità non gestite,
  chiave d'ordine, unità, handle già usati.
- `Polyline` riceve le larghezze (43; 40 e 41 per vertice), che sono
  proprietà del DXF e non dati estranei (ADR 0006, punto 1): si leggono, si
  conservano e si scrivono; il disegno con la larghezza arriva più avanti.
- La tappa 0.6 sposta layer, colori e tipi di linea dai gruppi grezzi al
  modello, con la stessa regola: ciò che non è ancora rappresentato resta
  grezzo.
- Il messaggio d'apertura rende visibile, a chi usa il programma, quanta
  parte del disegno Tecnigrafo non mostra ancora.
