# Manuale dell'interfaccia

Questo manuale descrive l'interfaccia grafica introdotta nella tappa 0.6:
la barra multifunzione (*ribbon*) e l'interfaccia classica, i pannelli agganciabili
per i layer e le proprietà, la finestra di selezione rapida `QSELECT`, i temi chiaro e
scuro, la personalizzazione delle scorciatoie da tastiera e la visualizzazione dello
spessore delle linee (`LWT`).

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

## Visualizzazione dello spessore delle linee (`LWT`)

Il pulsante **LWT** nella barra di stato (accanto agli aiuti al disegno `SNAP` e `ORTHO`)
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
