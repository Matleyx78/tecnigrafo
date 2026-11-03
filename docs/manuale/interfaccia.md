# Manuale dell'interfaccia

Questo manuale descrive l'interfaccia grafica del programma: la barra multifunzione
(*ribbon*) e l'interfaccia classica, i pannelli agganciabili per i layer e le
proprietà, la selezione rapida `QSELECT`, la finestra di selezione chiusa
trascinando, i grip per la modifica diretta delle entità, i nuovi snap a oggetto
con i loro marcatori, il tracciamento polare (`POLAR`), i temi chiaro e scuro, la
personalizzazione delle scorciatoie da tastiera e la visualizzazione dello spessore
delle linee (`LWT`).

Per i comandi da riga di comando vedi il [manuale dei comandi](comandi.md).

## Ribbon e interfaccia classica

Tecnigrafo offre due modalità di interfaccia per accedere agli strumenti di disegno,
modifica, visualizzazione e gestione del documento:

- **Barra multifunzione (Ribbon)** (predefinita): organizza comandi e controlli in schede
  e gruppi logici (`Home` con i gruppi `Draw`, `Modify`, `Layers`, `Properties`; `View`
  con `Navigate` e `Aids`). I comandi di disegno principali dispongono di pulsanti grandi
  per una scelta visiva immediata; gli altri strumenti sono raccolti in pulsanti compatti o
  in controlli dedicati (come i menu a tendina per la selezione rapida di layer, colori e
  tipi di linea). Il pulsante del menu dell'applicazione nell'angolo superiore dà accesso
  alle operazioni sui file (*New*, *Open*, *Save*, *Save As*) e consente di aprire il menu
  classico completo.
- **Interfaccia classica**: barra dei menu tradizionale (*File*, *Edit*, *View*,
  *Draw*, *Modify*, *Tools*, *Help*) affiancata da barre degli strumenti dedicate
  (*Standard*, *Draw*, *Modify*, *Layers*, *Properties*), ciascuna corrispondente a un
  gruppo della barra multifunzione.

Entrambe le modalità condividono lo stesso registro centrale delle azioni: lo stato delle
azioni (abilitato o disabilitato, selezionato o deselezionato) e le combinazioni di scelta
rapida da tastiera rimangono identici nel passaggio tra le due interfacce. La riga di
comando sotto l'area di disegno è sempre presente e pienamente attiva in entrambe le
configurazioni.

### Passare da un'interfaccia all'altra

Il cambio di interfaccia è istantaneo e non richiede il riavvio del programma:

| Metodo | Come fare |
|---|---|
| Pulsante nella barra di stato | Clic sul pulsante `RIBBON` / `MENU` all'estrema destra della barra di stato |
| Riga di comando | Scrivi `RIBBON` per passare alla barra multifunzione, oppure `RIBBONCLOSE` per l'interfaccia classica |
| Finestra Impostazioni | Scegli *Tools → Settings* e imposta la voce *Interface* su *Ribbon* o *Classic* |

L'interfaccia selezionata viene memorizzata automaticamente nelle impostazioni utente
all'uscita e ripristinata al successivo avvio. Durante l'esecuzione in modalità di controllo
automatica con script da riga di comando (`--script`), viene sempre forzata la barra
multifunzione predefinita.

## Pannelli agganciabili

I pannelli degli strumenti (`QDockWidget`) offrono controlli interattivi avanzati e
possono essere disposti liberamente nell'area di lavoro:
- **agganciati** a uno qualsiasi dei quattro lati della finestra principale (sinistra, destra,
  sopra, sotto);
- **raggruppati a schede** sovrapponendo un pannello a un altro già agganciato, risparmiando spazio;
- **staccati** come finestre mobili indipendenti (*floating*), trascinando la barra del titolo.

La posizione, le dimensioni e la disposizione a schede dei pannelli vengono salvate nelle
impostazioni dell'applicazione alla chiusura e ripristinate fedelmente all'avvio.

### Il pannello dei layer (`LAYER`)

Si apre digitando `LAYER` (o l'alias `LA`) nella riga di comando, oppure premendo il pulsante
*Layer Properties* nel gruppo *Layers* della barra multifunzione o nella barra *Layers*.

Il pannello presenta una tabella interattiva con tutti i layer definiti nel disegno:

| Colonna | Significato e interazione |
|---|---|
| **Stato** | Indica con un segno di spunta il layer attualmente corrente |
| **Nome** | Nome del layer; doppio clic per rinominare (il layer `0` non può essere rinominato) |
| **Acceso** | Clic sull'icona della lampadina per accendere o spegnere il layer |
| **Congelato** | Clic sull'icona fiocco/sole per congelare o scongelare il layer (il layer corrente non può essere congelato) |
| **Bloccato** | Clic sull'icona del lucchetto per bloccare o sbloccare il layer |
| **Colore** | Campione del colore associato; clic per aprire la finestra di scelta colore (indice ACI o Truecolor RGB) |
| **Tipo di linea** | Nome del tipo di linea assegnato; menu a tendina con i tipi attualmente caricati |
| **Spessore** | Spessore di linea associato; menu a tendina con gli spessori standard o `Default` |
| **Stampa** | Clic sull'icona della stampante per abilitare o disabilitare la stampa del layer |

**Pulsanti della barra degli strumenti del pannello:**
- **Nuovo**: crea un nuovo layer (con nome sequenziale `Layer1`, `Layer2`, ecc.) assegnandogli le proprietà standard del layer `0`.
- **Cancella**: elimina il layer selezionato. L'eliminazione viene rifiutata per il layer `0`, per il layer corrente e per qualsiasi layer contenente entità (anche in blocchi o dati non gestiti).
- **Rendi corrente**: imposta il layer selezionato come layer attivo di disegno. Non è possibile rendere corrente un layer congelato.

Ogni operazione eseguita nel pannello costituisce una transazione `Layer` dell'editor,
annullabile con `U` o `Ctrl+Z` e ripristinabile con `REDO` o `Ctrl+Y`. Se un'operazione non
rispetta le regole (es. cancellazione di un layer contenente oggetti o congelamento del layer
corrente), il programma mostra un messaggio descrittivo e lascia il disegno inalterato.

Nella ribbon e nella barra *Layers* è inoltre disponibile il **menu a tendina rapido dei layer**
(`LayerCombo`): permette di selezionare al volo il layer corrente senza aprire il pannello,
e include i pulsanti rapidi per accendere/spegnere, congelare o bloccare il layer corrente.

### Il pannello delle proprietà (`PROPERTIES`)

Si apre con il comando `PROPERTIES` (o gli alias `PR`, `CH`, `MO`, `PROPS`), dal pulsante
dedicato nella ribbon o dal menu *Modify*.

Il pannello si adatta automaticamente allo stato della selezione:

- **Nessuna selezione attiva (`No selection`)**: visualizza e consente di modificare i
  **valori correnti** del documento per le entità future (colore corrente, layer corrente,
  tipo di linea corrente, scala corrente `CELTSCALE`, spessore corrente).
- **Una o più entità selezionate**:
  - In alto, una casella combinata indica il tipo di entità selezionate (es. `Line`, `Circle`,
    oppure `All (n)`) e permette di filtrare la visualizzazione per un tipo specifico.
  - Sezione **General**:
    - **Color**: colore dell'entità (`ByLayer`, `ByBlock`, indice numerico o RGB Truecolor);
    - **Layer**: layer a cui appartiene l'entità;
    - **Linetype**: tipo di linea assegnato;
    - **Linetype scale**: fattore di scala individuale dell'entità;
    - **Lineweight**: spessore di linea assegnato.
    Se la selezione include entità con valori disomogenei per una determinata proprietà, il
    campo corrispondente mostra `*VARIES*`. La modifica di un valore si applica a tutte le
    entità selezionate in un'unica transazione `Properties` annullabile.
  - Sezione **Geometry** (in sola lettura): mostra le coordinate e i parametri geometrici
    essenziali dell'entità singola selezionata (es. punti di inizio e fine per una linea,
    centro e raggio per un cerchio).

I controlli Colore, Tipo di linea e Spessore presenti nella barra multifunzione (gruppo
*Properties*) e nella barra *Properties* classica operano in modo coordinato con il pannello:
modificano la selezione attiva se presente, oppure i valori correnti in assenza di selezione.

## Selezione rapida (`QSELECT`)

La finestra di dialogo **Quick Select** consente di comporre selezioni di oggetti basate
su criteri e filtri di proprietà. Si apre con il comando `QSELECT`, dal pulsante nella sezione
di selezione della ribbon o dal menu *Edit*.

Opzioni della finestra:
- **Apply to**: definisce il dominio di ricerca:
  - *Entire drawing*: applica il filtro a tutti gli oggetti del disegno;
  - *Current selection*: restringe la ricerca alle sole entità già selezionate.
- **Object type**: filtra per tipo di oggetto (*All*, *Line*, *Circle*, *Arc*, *Polyline*, *Text*).
- **Properties**: proprietà da verificare (*Layer*, *Color*, *Linetype*, *Lineweight*).
- **Operator**: operatore di confronto:
  - `= Equals`: seleziona le entità con valore uguale a quello specificato;
  - `≠ Not Equal`: seleziona le entità con valore diverso da quello specificato.
- **Value**: valore da ricercare (inseribile con il controllo dedicato a ciascun tipo di proprietà).
- **How to apply**:
  - *Include in new selection set*: inserisce nella selezione le entità che soddisfano la condizione;
  - *Exclude from new selection set*: inserisce nella selezione le entità che non soddisfano la condizione.

Alla conferma con **OK**, la selezione attiva nel disegno viene aggiornata e la riga di
comando segnala il totale degli elementi selezionati (`<n> item(s) selected.`). Le entità
su layer spenti, congelati o bloccati vengono escluse automaticamente.

## Finestra di selezione chiusa trascinando

Nella tappa 0.7 il rettangolo di selezione nell'area di disegno si può creare sia con
la tradizionale tecnica a due clic separati, sia tenendo premuto e trascinando il mouse
(comportamento analogo alla variabile `PICKDRAG` = 2 di AutoCAD, sempre attivo):

- **Clic fermo (spostamento inferiore a 5 pixel):** premendo e rilasciando il tasto
  sinistro del mouse nella stessa posizione, il rettangolo di selezione si apre e
  rimane attivo seguendo il puntatore; un secondo clic nella vista fissa l'angolo
  opposto e conclude la selezione.
- **Pressione, trascinamento e rilascio:** premendo il tasto sinistro nel vuoto e
  trascinando il puntatore oltre la soglia di 5 pixel, il rilascio del tasto vale come
  secondo clic e chiude immediatamente il rettangolo di selezione.

In entrambe le modalità valgono le regole standard della selezione a rettangolo:

- **Verso destra (finestra):** sfondo azzurro trasparente con bordo continuo; seleziona
  solo le entità interamente contenute all'interno dell'area.
- **Verso sinistra (intersezione):** sfondo verde trasparente con bordo tratteggiato;
  seleziona tutte le entità contenute o attraversate dal contorno del rettangolo.
- **Tasto `Shift`:** tenendo premuto `Shift` al momento del rilascio (o del secondo clic),
  gli oggetti individuati vengono rimossi dalla selezione anziché aggiunti.
- **Tasto `Esc`:** premuto con un rettangolo aperto, chiude e annulla il rettangolo
  senza modificare la selezione precedente.

Il gesto di trascinamento chiude **esclusivamente** un rettangolo di selezione: durante
la richiesta di un punto o di un oggetto per un comando il rilascio dopo un trascinamento
non ha effetto, e non sposta un grip.

## Grip: modifica diretta delle entità

I grip sono quadratini blu (10 pixel di lato a schermo) che compaiono sui vertici e
sui punti notevoli delle entità quando queste sono selezionate nell'area di disegno
senza alcun comando in corso (*selezione a riposo*).

Se la selezione contiene più di 100 entità (come la variabile `GRIPOBJLIMIT`
in AutoCAD), i grip non vengono disegnati a schermo per preservare la fluidità grafica.

### Dove si trovano

| Entità | Posizione dei grip |
|---|---|
| `LINE` | I due estremi e il punto medio |
| `ARC` | I due estremi, il punto medio (sull'arco) e il centro |
| `CIRCLE` | Il centro e i quattro quadranti (a 0°, 90°, 180° e 270°) |
| `LWPOLYLINE` | Ciascun vertice e il punto medio di ogni segmento (sull'arco per i segmenti curvi) |
| `TEXT` | Il punto d'inserimento (sulla linea di base); per allineamenti *Aligned* e *Fit*, anche il secondo punto di allineamento |

### Come si usano

1. Fai clic con il tasto sinistro entro il quadrato di un grip (con tolleranza di 5 pixel
   dal centro): il grip diventa **caldo** (*hot grip*) e si colora di rosso.
2. Sulla riga di comando compare l'intestazione `** STRETCH **` e la domanda
   `Specify stretch point:`.
3. Gli aiuti al disegno (snap a oggetto, modalità orto e tracciamento polare) usano la
   posizione iniziale del grip caldo come punto base.
4. L'anteprima dinamica mostra l'entità deformata o spostata mentre muovi il puntatore.
5. Indica il nuovo punto cliccando nella vista o digitando le coordinate sulla riga di
   comando: l'entità viene aggiornata. La modifica produce un passo di annullamento
   `Grip edit` dell'editor.
6. Se la nuova posizione genera una geometria degenere o non valida (ad esempio tre punti
   allineati per un arco, raggio nullo, o estremi coincidenti), compare il messaggio
   `Invalid grip edit.` e l'entità resta invariata.
7. Premendo `Esc` sul grip caldo si abbandona l'operazione senza modificare l'entità.
8. Sia dopo aver completato la modifica, sia dopo aver premuto `Esc`, la selezione e i
   suoi grip rimangono attivi sul disegno. Un ulteriore tasto `Esc` a riposo svuota la
   selezione e nasconde i grip.

Se due o più entità selezionate condividono un grip nello stesso punto, il clic seleziona
il grip dell'entità con l'identificativo più alto (quella creata più di recente).

### Effetto di ciascun grip

| Grip | Effetto del nuovo punto |
|---|---|
| Estremo di `LINE` | Sposta quell'estremo nella nuova posizione |
| Medio di `LINE`, centro di `ARC` e `CIRCLE`, inserimento di `TEXT` | Sposta l'intera entità (traslazione rigida) |
| Quadrante di `CIRCLE` | Modifica il raggio del cerchio, che diventa pari alla distanza tra il nuovo punto e il centro |
| Estremo di `ARC` | L'arco viene ricalcolato affinché passi per il nuovo punto, per l'altro estremo e per il punto medio originario |
| Medio di `ARC` | L'arco passa per i due estremi invariati e per il nuovo punto (può diventare l'arco maggiore se supera 180°) |
| Secondo punto di `TEXT` *Aligned* o *Fit* | Sposta quel punto: per *Aligned* adatta anche l'altezza del testo, per *Fit* varia il fattore di larghezza |
| Vertice di `LWPOLYLINE` | Sposta il vertice; i segmenti curvi collegati continuano a passare per il loro punto medio originario |
| Medio di un segmento rettilineo di `LWPOLYLINE` | Sposta il segmento parallelamente a se stesso: i due vertici estremi del segmento si traslano, e gli eventuali tratti curvi adiacenti seguono la regola del vertice |
| Medio di un segmento curvo di `LWPOLYLINE` | Il segmento curvo viene ricalcolato affinché passi per i suoi due vertici e per il nuovo punto indicato |

## Temi dell'interfaccia

Tecnigrafo supporta due temi grafici completi basati sullo stile Qt Fusion, con tavolozza
coordinata e coerente su Windows, Linux e macOS:

- **Chiaro (Light)**: area di disegno con sfondo bianco, controlli chiari. Il colore entità 7
  (bianco/nero convenzionale del CAD) viene disegnato in nero.
- **Scuro (Dark)**: area di disegno con sfondo scuro, controlli scuri a basso riverbero. Il
  colore entità 7 viene disegnato in bianco.

### Cambiare tema

- Dal menu *View → Theme → Light* o *Dark*.
- Dalla finestra *Tools → Settings*, selezionando il tema dal menu a tendina.

Il cambio di tema aggiorna all'istante l'intera applicazione: finestre, pannelli, sfondo
della vista e icone tematiche SVG, renderizzate dinamicamente nel colore del testo della
tavolozza per garantire leggibilità ottimale in ogni condizione di luce.
La scelta del tema viene salvata nelle impostazioni utente. In modalità di controllo automatica
(`--script`) viene sempre utilizzato il tema chiaro.

## Personalizzazione delle scorciatoie da tastiera

La finestra **Customize Shortcuts** permette di esaminare, riassegnare ed esportare le
scorciatoie da tastiera associate alle azioni del programma. Si apre da *Tools → Customize Shortcuts...*.

Caratteristiche:
- **Elenco completo delle azioni**: tabella con nome dell'azione, descrizione estesa, combinazione
  di tasti attualmente attiva e combinazione predefinita di fabbrica.
- **Filtro di ricerca**: campo testo per filtrare rapidamente le azioni in base a parole chiave nel nome
  o nella descrizione.
- **Modifica intuitiva**: selezionata un'azione, premendo la combinazione desiderata nel campo
  di modifica questa viene registrata (es. `Ctrl+Alt+L`).
- **Rilevamento conflitti**: se la sequenza di tasti digitata è già impiegata da un'altra azione
  nel medesimo contesto, il programma segnala il conflitto indicando il nome dell'azione concorrente
  e impedisce l'assegnazione non valida.
- **Pulsanti di gestione**:
  - *Remove*: rimuove la scorciatoia dall'azione selezionata;
  - *Reset*: ripristina la combinazione predefinita per l'azione selezionata;
  - *Reset All*: ripristina tutte le scorciatoie dell'applicazione ai valori originali;
  - *Export...*: salva le differenze personalizzate in un file di testo (formato `<identificativo>\t<scorciatoia>`);
  - *Import...*: carica un file di scorciatoie personalizzate precedentemente esportato, verificando la correttezza di ogni riga e segnalando eventuali conflitti.

Le modifiche confermate con **OK** entrano subito in vigore sia nella barra multifunzione sia
nei menu classici e vengono salvate nelle impostazioni utente (`QSettings`).
*Nota:* i comandi e gli alias della riga di comando (es. `L`, `PL`, `E`, `LA`) non sono
scorciatoie da tastiera di sistema e non compaiono in questa finestra.

## Nuovi snap a oggetto

Dalla tappa 0.7 il sistema di snap a oggetto include quattro nuovi tipi di aggancio
geometrico (ADR 0033), ciascuno associato a un'azione dedicata, a un'icona e a un marcatore
visivo a schermo:

| Modo | Azione / Menu *Tools → Object Snap Modes* | Punto di aggancio | Marcatore a schermo |
|---|---|---|---|
| *Intersection* | `aids.snap.intersection` | Punto d'incontro tra due curve o tra segmenti della stessa polilinea (interruttore presente, aggancio non ancora attivo) | Croce a forma di X |
| *Perpendicular* | `aids.snap.perpendicular` | Piede della perpendicolare tracciata dal punto base del comando verso la curva | Simbolo perpendicolare (⊥) |
| *Tangent* | `aids.snap.tangent` | Punto di tangenza condotto dal punto base verso cerchi, archi o segmenti curvi di polilinea (interruttore presente, aggancio non ancora attivo) | Cerchio con linea tangente in alto |
| *Quadrant* | `aids.snap.quadrant` | Punti cardinali (0°, 90°, 180°, 270°) di cerchi, archi e archi di polilinea | Rombo |

### Regole geometriche

- **Quadrante:** individua i quattro punti a 0°, 90°, 180° e 270° rispetto agli assi del
  disegno. Per un arco o per un segmento curvo di polilinea, lo snap interviene solo se
  l'angolo cade all'interno dello sviluppo effettivo dell'arco.
- **Perpendicolare:** richiede che il comando in esecuzione disponga di un punto base
  (come il primo punto di `LINE`, il punto base di `MOVE` o il grip caldo); senza punto
  base non produce alcun aggancio. Il punto calcolato viene accettato solo se cade
  sull'entità geometrica reale e non sul suo prolungamento ideale.
- **Intersezione e Tangente:** i comandi e le voci di menu sono già presenti per
  attivare o disattivare questi modi, ma il nucleo geometrico non calcola ancora i loro
  punti di aggancio (arriveranno con i pacchetti successivi della tappa 0.7: per ora non
  agganciano punti).

### Priorità e attivazione

Tutti i punti di snap candidati entro l'apertura di 10 pixel competono tra loro e viene
scelto quello più vicino al puntatore. A parità di distanza si applica l'ordine di
priorità: *Endpoint*, *Intersection*, *Midpoint*, *Quadrant*, *Perpendicular*, *Tangent*
(per ora *Intersection* e *Tangent* non generano punti di aggancio). Se nessuno di questi
rientra nell'apertura, operano *Center* e *Insertion* quando si passa sopra l'entità.

All'avvio del programma, *Intersection* è attivo per impostazione predefinita come interruttore
(insieme ai modi storici *Endpoint*, *Midpoint*, *Center*, *Insertion*); *Perpendicular*,
*Tangent* e *Quadrant* sono disattivati per difetto e si possono attivare dal menu
*Tools → Object Snap Modes* o dal gruppo *Aids* della barra multifunzione. Come precisato
sopra, *Intersection* e *Tangent* non agganciano ancora alcun punto.

## Tracciamento polare (`POLAR`)

Il tracciamento polare proietta linee guida allineate ad angoli regolari a partire dal
punto base del comando o dal grip caldo.

### Attivazione e scorciatoia `F10`

- Si attiva o disattiva premendo il tasto funzione `F10`, scegliendo la voce di menu
  *Tools → Polar Tracking*, oppure cliccando sul pulsante **POLAR** nella barra di stato.
- Nello storico della riga di comando compaiono i messaggi `<Polar on>` e `<Polar off>`.

### Esclusione reciproca con la modalità Orto (`F8`)

Come in AutoCAD, la modalità ortogonale (`ORTHO`, `F8`) e il tracciamento polare
(`POLAR`, `F10`) si escludono reciprocamente:

- Se attivi il tracciamento polare con `F10` mentre l'orto è acceso, l'orto si spegne
  automaticamente (con i messaggi `<Ortho off>` e `<Polar on>`).
- Se attivi l'orto con `F8` mentre il polare è acceso, il polare si spegne
  automaticamente (con i messaggi `<Polar off>` e `<Ortho on>`).

### Feedback grafico e suggerimento a schermo

Quando il puntatore si trova entro una tolleranza di 10 pixel da una delle semirette
polari uscenti dal punto base:

- la vista disegna una **semiretta tratteggiata** che si estende dal punto base lungo la
  direzione polare;
- accanto al puntatore appare un suggerimento di testo (*tooltip*) con la distanza e
  l'angolo polare: `Polar: <distanza> < <angolo>°` (ad esempio `Polar: 75.0000 < 45°`).

Uno snap a oggetto riconosciuto ha sempre la precedenza sul tracciamento polare.

### Incremento angolare e `POLARANG`

L'angolo di incremento predefinito è di 90° (direzioni a 0°, 90°, 180° e 270°). Con il
comando da tastiera `POLARANG` puoi impostare qualsiasi incremento desiderato tra 0° e 180°
(ad esempio 45°, 30° o 15°):

```
Command: polarang
Enter new value for POLARANG <90>: 45
```

Quando una semiretta polare è agganciata, digitando un valore numerico singolo sulla
riga di comando (es. `50`) il punto viene calcolato a quella distanza esatta dal punto
base lungo la direzione polare attiva.

## Visualizzazione dello spessore delle linee (`LWT`)

Il pulsante **LWT** nella barra di stato (accanto agli aiuti al disegno `SNAP`, `ORTHO` e `POLAR`)
attiva o disattiva la visualizzazione dello spessore effettivo delle linee nella vista:

- Con `LWT` attivo (`ON`), linee, polilinee, archi e cerchi vengono renderizzati con il loro
  spessore reale espresso in millimetri (compresi gli spessori ereditati da `ByLayer`).
- Con `LWT` disattivato (`OFF`), tutte le entità vengono disegnate con uno spessore nominale
  minimo uniforme a schermo (1 pixel logico).

Il pulsante corrisponde all'azione `aids.lwdisplay` e al comando da tastiera `LWDISPLAY`.
Come per lo snap e la modalità orto, commutare `LWT` non modifica il documento e non crea
passi nello storico di annullamento.

---

Vedi anche:
- [Manuale dei comandi](comandi.md) — comandi da tastiera, sintassi ed esempi.
- [Architettura](../ARCHITETTURA.md) — organizzazione dei moduli e componenti grafici.
