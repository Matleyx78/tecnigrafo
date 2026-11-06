# Manuale dei comandi

Questo manuale descrive gli strumenti disponibili nella tappa 0.7: i file, la riga di
comando, i punti, `LINE`, `CIRCLE`, `ARC`, `PLINE`, `TEXT`, la selezione,
`ERASE`, i comandi di modifica (`MOVE`, `COPY`, `ROTATE`, `SCALE`, `MIRROR` con
`MIRRTEXT`, `EXPLODE`, `TRIM` ed `EXTEND` con `TRIMEXTENDMODE`, `OFFSET`,
`FILLET`, `CHAMFER`, `JOIN`), gli snap a oggetto, la modalità orto e il tracciamento
polare (`POLARANG`), i layer (`-LAYER`, `CLAYER`), le proprietà correnti e delle
entità (`COLOR`, `-LINETYPE`, `LTSCALE`, `CELTSCALE`, `LWEIGHT`, `LWDISPLAY`,
`CHPROP`), le sequenze negli script e l’annullamento. L'interfaccia grafica
(ribbon, pannelli `LAYER` e `PROPERTIES`, `QSELECT`, grip, temi, scorciatoie e
`LWT`) è descritta nel [manuale dell'interfaccia](interfaccia.md). Lo stato della
tappa è nella [roadmap](../ROADMAP.md).

Domande e messaggi del programma sono per ora in inglese (la traduzione arriverà
con la tappa 0.9): qui sono riportati come appaiono sullo schermo.

Per zoom e pan della vista vedi il [README](../../README.md#comandi-della-vista).

## La riga di comando

Sotto il disegno c'è la riga di comando, come in AutoCAD. È fatta di due parti:

- lo **storico**, in alto: tre righe visibili, scorrevoli, con i messaggi del
  programma e ciò che hai scritto;
- la **riga di scrittura**, in basso: la domanda del momento (il *prompt*)
  seguita dal campo in cui scrivi.

Quando nessun comando è in corso, il prompt è `Command:`.

### Scrivere

Non serve cliccare nel campo: se la vista è in primo piano, lettere, cifre e i
caratteri `@ < , . -` finiscono direttamente nella riga di comando. Scrivi `pl` e
premi `Invio`: parte il comando `PLINE`.

| Tasto | Effetto |
|---|---|
| `Invio` | invia ciò che hai scritto |
| `Spazio` | come `Invio`, tranne durante `Enter text:`, dove scrive uno spazio |
| `Esc` | svuota il campo; chiude prima un eventuale rettangolo di selezione, altrimenti interrompe il comando conservando il lavoro già completato (vedi sotto) o, a riposo, svuota la selezione |
| `Invio` a riga vuota | durante un comando: conferma il valore predefinito o conclude; senza comando: ripete l’ultimo comando |

Nomi dei comandi e opzioni si scrivono in maiuscolo o minuscolo, indifferentemente.

Ogni scambio resta nello storico come in AutoCAD, per esempio:

```
Command: pl
Specify start point: 0,0
Specify next point or [Arc/Undo]: @50,0
```

### Il mouse

| Azione | Effetto |
|---|---|
| clic sinistro nella vista | seleziona entità a riposo o durante `Select objects:`; indica un punto quando il comando lo chiede |
| `Shift`+clic durante la selezione | toglie entità dalla selezione |
| clic destro nella vista | come `Invio`: conferma il comando in corso, oppure ripete l'ultimo comando |
| movimento del puntatore | aggiorna l'anteprima, il rettangolo di selezione aperto e il marcatore dello snap |

## Indicare i punti

Quando un comando chiede un punto puoi cliccare nella vista oppure scrivere le
coordinate. La sintassi è quella di AutoCAD:

| Scrittura | Significato | Esempio |
|---|---|---|
| `x,y` | punto assoluto | `100,50` |
| `@dx,dy` | spostamento dall'ultimo punto | `@20,0` → 20 unità a destra |
| `d<a` | polare assoluto: distanza `d` dall'origine, angolo `a` | `100<30` |
| `@d<a` | polare relativo: distanza `d` dall'ultimo punto, angolo `a` | `@50<90` → 50 unità in su |
| `d` | distanza diretta: `d` unità dall'ultimo punto, verso il puntatore | `75` |

### Regole dei numeri

- Il separatore decimale è **sempre il punto**: `12.5`. La virgola separa X da Y,
  quindi `1,5` è il punto di coordinate X = 1 e Y = 5, non "uno virgola cinque".
  Vale qualunque sia la lingua del sistema.
- Sono ammessi il segno (`-20`, `+20`) e la notazione esponenziale (`1.5e3` vale
  1500).
- Gli spazi prima e dopo la scrittura non contano; dentro la scrittura non sono
  ammessi.

### Angoli

Gli angoli sono in gradi. Lo zero è la direzione dell'asse X positivo (verso
destra) e gli angoli crescono in senso antiorario: `90` è verso l'alto, `180`
verso sinistra, `270` (o `-90`) verso il basso.

### L'ultimo punto

Le forme con `@` e la distanza diretta partono dall'**ultimo punto** indicato. Il
programma lo ricorda anche tra un comando e l'altro: all'inizio di una nuova
polilinea, `@10,0` indica un punto a 10 unità a destra dell'ultimo vertice della
polilinea precedente. Se non hai ancora indicato nessun punto, queste forme non
si possono usare e il programma risponde `Invalid point or option.`

### La distanza diretta

Scrivi solo un numero: il punto si trova a quella distanza dall'ultimo punto,
nella direzione del puntatore. È il modo più rapido per disegnare un segmento di
lunghezza nota: punta il mouse nella direzione voluta, scrivi `75` e `Invio`.

La distanza diretta non funziona se il puntatore non si è ancora mosso nella vista
o se si trova esattamente sull'ultimo punto, perché manca una direzione.

Con l'orto attivo la direzione segue l'asse scelto dall'anteprima; se c'è uno
snap, segue invece la direzione verso il punto di aggancio. Le altre quattro
forme di coordinate della tabella restano esatte anche con snap e orto attivi:
vedi [Snap, orto e coordinate scritte](#snap-orto-e-coordinate-scritte).

## Rispondere a distanza, angolo e testo

Il significato di un numero dipende dalla domanda:

| Richiesta | Numero da solo | Punto scritto o clic nella vista |
|---|---|---|
| punto | distanza diretta verso il puntatore | punto esatto o punto del clic |
| distanza (raggio, diametro, altezza) | valore in unità di disegno | distanza dal punto base del comando |
| angolo (rotazione del testo) | valore in gradi antiorari da +X | direzione dal punto base verso il punto indicato |
| testo (`Enter text:`) | testo letterale | una coordinata scritta è testo letterale |

Per il raggio e il diametro il punto base è il centro; per altezza e rotazione
è il punto iniziale del testo. Nel modo diametro di `CIRCLE`, un punto dà la
lunghezza del diametro misurata dal centro: il raggio è metà di quella distanza.
I punti scritti seguono la sintassi delle coordinate; ai clic si applicano snap
e orto. Un valore numerico scritto non dipende dalla posizione del puntatore.

Un valore tra `< >` è quello **predefinito**: `Invio` a campo vuoto lo accetta.
All’avvio l’altezza del testo è `<2.5000>` e la rotazione `<0>`; il raggio non ha
un valore predefinito finché non completi un cerchio. I comandi successivi
propongono gli ultimi valori usati per creare le entità. Questi valori durano
solo fino alla chiusura del programma. Una conferma senza valore predefinito
termina il comando; per `LINE` e `PLINE` conserva i tratti già indicati.

Durante `Enter text:` gli spazi, anche iniziali e finali, e le lettere accentate
restano come scritti. `10,20`, `U` e `Close` sono testo, non punti o opzioni.
`Spazio` scrive uno spazio; solo una riga davvero vuota confermata con `Invio`
termina l’inserimento. Un clic nella vista non scrive una riga di testo.

## File

Dalla tappa 0.5 puoi aprire e salvare i disegni, sia nel formato nativo di
Tecnigrafo (`.cadx`) sia nel formato DXF di AutoCAD.

Tecnigrafo lavora su un solo disegno alla volta: l'apertura di un file o la
creazione di un nuovo disegno sostituiscono il disegno corrente. All'apertura o
creazione di un disegno la cronologia di annullamento si azzera.

### I comandi

| Comando | Scorciatoia | Menu File | Effetto |
|---|---|---|---|
| `NEW` | `Ctrl+N` | New | Crea un nuovo disegno vuoto |
| `OPEN` | `Ctrl+O` | Open… | Apre un file `.cadx` o `.dxf` esistente |
| `QSAVE` | `Ctrl+S` | Save | Salva rapidamente nel file corrente; se il disegno non ha ancora un nome, chiede come salvarlo |
| `SAVE` | — | — | Salva una copia del disegno corrente con un nome diverso |
| `SAVEAS` | `Ctrl+Shift+S` | Save As… | Salva il disegno con un nome diverso, che diventa il file corrente |
| `INSUNITS` | — | — | Visualizza o cambia l'unità di misura del disegno |

Le voci del menu File e le scorciatoie corrispondenti si disattivano mentre un
altro comando è in corso.

### Finestre di dialogo e riga di comando

Nell'interfaccia grafica, quando lanci `NEW`, `OPEN`, `QSAVE` (senza nome di file
ancora assegnato) o `SAVEAS` dal menu, con le scorciatoie da tastiera oppure
scrivendo il comando senza argomenti, si apre la consueta finestra di dialogo del
sistema operativo per scegliere dove salvare o quale file aprire. Se chiudi o
annulli la finestra di dialogo, il comando si interrompe, esattamente come se
avessi premuto `Esc`.

Negli script della modalità di controllo (`tecnigrafo-script` e l'opzione
`--script`), le finestre di dialogo non compaiono mai: tutte le domande avvengono
sulla riga di comando, chiedendo di digitare il percorso del file.

### Modifiche non salvate

Quando provi ad aprire un altro disegno con `OPEN`, a crearne uno nuovo con `NEW`
o a chiudere la finestra di Tecnigrafo e ci sono modifiche non salvate, il
programma ti chiede prima se vuoi salvare il lavoro:

```
Save changes to Drawing1.cadx? [Yes/No/Cancel] <Yes>:
```

- `Yes` (oppure `Invio` a riga vuota, confermando il valore predefinito): salva
  il disegno corrente prima di procedere. Se il disegno non ha ancora un nome,
  ti chiederà con quale nome salvarlo. Se il salvataggio fallisce o viene annullato,
  l'operazione si interrompe e il disegno modificato resta aperto.
- `No`: scarta tutte le modifiche non salvate e procede con l'apertura, la
  creazione del nuovo disegno o la chiusura.
- `Cancel` (oppure `Esc`): annulla il comando e ti riporta al disegno corrente,
  senza perdere nulla.

Se chiudi la finestra dell'applicazione con modifiche non salvate, compare una
finestra di avviso che ti permette di scegliere tra salvare (`Save`), scartare le
modifiche (`Discard`) o annullare la chiusura (`Cancel`).

### Salvare un disegno: `QSAVE`, `SAVE` e `SAVEAS`

Per salvare il disegno hai a disposizione tre comandi:

- **`QSAVE`** (*Quick Save*): è il comando richiamato da `Ctrl+S` e dalla voce
  *Save* del menu File. Se il disegno ha già un file associato, salva subito
  senza fare domande. Se invece il disegno è nuovo (si chiama ancora
  `Drawing1.cadx`), chiede il nome:
  ```
  Save drawing as <Drawing1.cadx>:
  ```
  Premendo `Invio` accetti il nome proposto `Drawing1.cadx`, oppure puoi
  scrivere un percorso o un nome a tua scelta.
- **`SAVE`**: chiede con quale nome salvare il disegno. Come in AutoCAD, `SAVE`
  scrive una **copia** del disegno con il nome indicato, ma il disegno aperto
  nella sessione di lavoro continua a fare riferimento al file precedente e
  conserva il suo stato di modifica non salvata (a meno che non indichi lo stesso
  nome del file corrente).
- **`SAVEAS`**: chiede con quale nome salvare il disegno. A differenza di `SAVE`,
  il file salvato diventa il **disegno corrente** di Tecnigrafo, marcato come
  salvato.

Se indichi il nome di un file che esiste già, Tecnigrafo chiede se vuoi
sovrascriverlo:

```
A drawing with this name already exists. Do you want to replace it? [Yes/No] <No>:
```

Rispondendo `No` (o con `Invio` a vuoto) torni alla richiesta del nome del file.
Rispondendo `Yes` il file viene sostituito in modo atomico: Tecnigrafo scrive
prima un file temporaneo e poi sostituisce quello esistente solo a scrittura
completata con successo, così un eventuale errore non danneggia il vecchio file.

### Formati: `.cadx` e DXF

Tecnigrafo supporta due formati di file:

- **`.cadx`**: è il formato nativo di Tecnigrafo, un archivio compresso aperto
  (basato sullo standard ZIP) contenente la definizione geometrica del disegno
  e i metadati. I dettagli della struttura interna sono descritti in
  [`docs/formati/cadx.md`](../formati/cadx.md).
- **`.dxf`**: formato standard Drawing Exchange Format di Autodesk, in formato
  testo (ASCII).

Il formato viene riconosciuto automaticamente dall'estensione:
- Se scrivi un nome senza estensione (per esempio `progetto`), Tecnigrafo aggiunge
  automaticamente `.cadx`.
- Se indichi un'estensione diversa da `.cadx` o `.dxf`, il comando risponde:
  `Cannot save plan.dwg: only .cadx and .dxf files are supported.`

#### Quale versione viene scritta

- Con **`SAVEAS`**, per i disegni nuovi e all'interno dei file `.cadx`, Tecnigrafo
  scrive sempre in formato **AutoCAD 2018** (`AC1032`).
- Quando salvi con **`QSAVE`** (o con `SAVE` sul file corrente) un file DXF
  esistente aperto in precedenza, Tecnigrafo lo riscrive nella **stessa versione**
  in cui era stato letto (dalla versione AutoCAD 2000 `AC1015` ad AutoCAD 2018
  `AC1032`). In questo modo ciò che Tecnigrafo non gestisce resta intatto e
  utilizzabile dal programma che ha generato il file originario ([ADR 0017](../adr/0017-modulo-io-e-dxf.md)).

### Aprire un disegno: `OPEN`

Con `OPEN` (o `Ctrl+O`) apri un disegno esistente:

```
Command: open
Enter name of drawing to open: pianta.dxf
```

In lettura Tecnigrafo riconosce i file `.cadx` e i DXF ASCII da AutoCAD 2000 fino
ad AutoCAD 2018. Se il file è danneggiato o non supportato, ricevi un messaggio di
errore e il disegno corrente resta invariato:

```
Cannot open plan.dxf: line 812: expected a group code, found "abc".
```

#### Entità non gestite

Nella tappa 0.5 Tecnigrafo gestisce le cinque entità base: linee, cerchi, archi,
polilinee 2D e testo. Un file DXF creato con altri programmi può contenere molte
altre entità (per esempio tratteggi `HATCH`, quote o blocchi `INSERT`), oppure
entità collocate nello spazio carta.

Tecnigrafo non perde questi dati: legge e conserva tutte le entità non gestite
esattamente come sono nel file originale ([ADR 0018](../adr/0018-conservazione-dati-non-gestiti.md)). Quando apri il file, un
messaggio sullo storico ti riepiloga le entità non gestite trovate:

```
3 entities not supported: HATCH 2, INSERT 1
```

(o `1 entity not supported: HATCH 1` se è una sola). Anche se queste entità non
vengono visualizzate né selezionate a schermo, quando risalvi il file Tecnigrafo
le riscrive identiche carattere per carattere, senza perdita di informazioni.

### Unità del disegno: `INSUNITS`

La variabile e comando `INSUNITS` definisce l'unità di disegno (corrisponde alla
variabile `$INSUNITS` del DXF):

```
Command: insunits
Enter new value for INSUNITS <4>: 6
```

Il valore tra `< >` è l'unità attualmente impostata nel disegno (`4` per i nuovi
disegni). Il valore deve essere un numero intero compreso tra 0 e 24; se inserisci
un valore diverso, il programma risponde:

```
Requires an integer between 0 and 24.
```

I valori più usati sono:

| Valore | Unità di misura |
|---|---|
| 0 | Senza unità (*Unspecified*) |
| 1 | Pollici (*Inches*) |
| 4 | Millimetri (*Millimeters*) — valore predefinito nei nuovi disegni |
| 5 | Centimetri (*Centimeters*) |
| 6 | Metri (*Meters*) |

Come per le altre modifiche geometriche, anche il cambio di unità con `INSUNITS`
può essere annullato con il comando `U` (*Undo*).

### File recenti

Nell'interfaccia grafica, il menu **File ▸ Recent Files** mostra l'elenco degli
ultimi nove file aperti o salvati, ordinati a partire dal più recente. Selezionando
un file dall'elenco, Tecnigrafo avvia l'apertura con `OPEN`. Se un file recente è
stato spostato o cancellato dal disco, il programma mostra un messaggio di errore
e rimuove automaticamente la voce dall'elenco.

## Selezione

Puoi scegliere le entità prima di avviare `ERASE`, oppure dopo, quando compare
`Select objects:`. I gesti sono gli stessi nei due casi. Le entità scelte sono
evidenziate con una linea tratteggiata.

### Clic, finestra e intersezione

| Gesto | Risultato |
|---|---|
| clic sul bordo di un'entità o entro 3 pixel | aggiunge l'entità alla selezione corrente |
| `Shift`+clic sul bordo | toglie l'entità dalla selezione |
| clic nel vuoto, poi secondo clic a destra del primo | **finestra**: prende solo le entità interamente racchiuse, bordi compresi |
| clic nel vuoto, poi secondo clic a sinistra del primo | **intersezione**: prende anche le entità toccate dal rettangolo |
| `Shift` durante il secondo clic del rettangolo | toglie le entità individuate invece di aggiungerle |

Il rettangolo si costruisce con **due clic**, non trascinando il mouse. Dopo il
primo clic compare `Specify opposite corner:`; il secondo angolo segue il
puntatore. La finestra è blu trasparente con bordo continuo, l'intersezione è
verde trasparente con bordo tratteggiato. La tolleranza del clic resta di 3 pixel
anche cambiando lo zoom. Un clic su un'entità già scelta non la aggiunge due volte.

Linee, archi e cerchi si selezionano sulla curva. Un clic dentro un cerchio
non lo sceglie. Il testo invece si seleziona anche cliccando **dentro il suo
rettangolo d’ingombro**, ruotato come il testo: la finestra deve racchiuderlo
tutto, mentre l’intersezione basta che lo tocchi.

Una polilinea chiusa si seleziona sul **bordo**: cliccare al suo interno non la
sceglie. Anche un rettangolo a intersezione completamente interno a una
polilinea, senza toccarne il bordo, non la sceglie.

**Esempio:** traccia una finestra da sinistra a destra che racchiuda un foro del
disegno di esempio e tagli il contorno esterno: viene scelto solo il foro.
Tracciando un rettangolo da destra a sinistra che tocchi anche il contorno,
vengono scelte entrambe le entità. Con `Shift` sul secondo clic le togli.

### Confermare e abbandonare

- A riposo i clic costruiscono la selezione corrente senza messaggi. `Esc`
  chiude il rettangolo aperto conservando quanto era già selezionato; un altro
  `Esc`, senza rettangolo, svuota la selezione.
- Durante `Select objects:` ogni scelta scrive, per esempio, `1 found, 2 total`;
  una rimozione scrive `1 removed, 1 total`. I numeri indicano le entità trovate
  o rimosse dal gesto e il totale selezionato.
- Durante `Select objects:`, `Invio`, `Spazio` a campo vuoto o clic destro
  confermano le entità scelte. Confermare senza selezione conclude il comando
  senza effetti. `Esc` chiude prima un rettangolo aperto; senza rettangolo
  annulla il comando e svuota la selezione, scrivendo `*Cancel*`.

La selezione si svuota alla fine dei comandi e con `U`, `UNDO` e `REDO`. Avviare
un comando di disegno svuota la selezione precedente; `ERASE` invece la usa subito.

### Selezione scritta

Durante `Select objects:`, e nelle domande di selezione dei comandi (i bordi di
`TRIM` ed `EXTEND` nel modo standard, gli oggetti di `JOIN`), puoi scegliere
anche scrivendo:

| Scrivi | Risultato |
|---|---|
| un punto, per esempio `10,0` | come un clic in quel punto: sceglie l'entità che passa entro 3 pixel; nel vuoto apre un rettangolo, e compare `Specify opposite corner:` |
| `W` (*Window*), poi due punti | una **finestra**, in qualunque direzione indichi i due angoli |
| `C` (*Crossing*), poi due punti | un'**intersezione**, in qualunque direzione |
| `ALL` | tutte le entità sui layer accesi e non congelati; quelle sui layer bloccati sono scartate e contate |

Dopo `W` o `C` compaiono `Specify first corner:` e `Specify opposite corner:`:
rispondi con le coordinate o con due clic. Il primo angolo non sceglie l'entità
che c'è sotto. Mentre il programma chiede un angolo, un testo che non è un punto
produce `Invalid point or option.`. `Esc` chiude il rettangolo se il primo
angolo c'è già; prima, annulla il comando.

I messaggi sono quelli dei gesti con il mouse (`2 found, 2 total`, layer
bloccati). Le parole chiave della domanda hanno la precedenza: nella domanda dei
bordi di `TRIM`, `O` sceglie *mOde*. Le altre opzioni di AutoCAD (*Last*,
*Previous*, *Fence*, *WPolygon* e simili) non ci sono: un testo diverso produce
`Invalid selection.`.

**Esempio:** scrivi `e`, poi `c` e i due angoli `5,5` e `25,25`: vengono scelte
le entità toccate dal rettangolo, qui due linee, e `Invio` le cancella.

```
Command: ERASE
Select objects: c
Specify first corner: 5,5
Specify opposite corner: 25,25
2 found, 2 total
Select objects:
```

### Indicare un oggetto

Alcune domande vogliono **un solo oggetto**, nel punto in cui lo indichi: per
esempio `Select object to trim or shift-select to extend or [mOde]:` di `TRIM`,
o `Select first object or [...]` di `FILLET`. A queste domande:

- clicca l'entità, o scrivi un punto che le passi entro 3 pixel: il punto conta,
  perché dice quale parte dell'entità hai indicato;
- un clic o un punto nel vuoto non fa nulla e la domanda resta: non si apre un
  rettangolo;
- `Shift`+clic indica l'oggetto con `Shift` premuto, che alcuni comandi usano
  (in `TRIM` prolunga invece di tagliare);
- un'entità su un layer bloccato non si sceglie e compare `1 was on a locked
  layer.`;
- le parole chiave della domanda si scrivono come sempre.

### Layer bloccati nella selezione

Le entità situate su layer bloccati vengono escluse automaticamente da qualsiasi
gesto di selezione (clic sul bordo, finestra o intersezione), sia a riposo sia durante
`Select objects:`. Quando un comando chiede `Select objects:`, se il gesto ha incontrato
oggetti su layer bloccati, il programma li scarta e scrive subito dopo il conteggio:
`1 was on a locked layer.` oppure `<n> were on a locked layer.`

## ERASE — cancellare entità

- **Nome:** `ERASE`
- **Alias:** `E`
- **Menu:** *Edit → Erase*
- **Tasto:** `Canc`

### Con una selezione già pronta

1. Scegli le entità con clic, finestra o intersezione.
2. Scrivi `erase` o `e` e premi `Invio`, oppure usa *Edit → Erase* o `Canc`.
3. Le entità vengono cancellate subito, tutte in un unico passo di annullamento.

### Scegliendo dopo il comando

1. Senza una selezione, scrivi `e` e premi `Invio`: compare `Select objects:`.
2. Scegli le entità. Puoi aggiungerle e toglierle con gli stessi gesti usati a
   riposo; lo storico mostra il totale dopo ogni scelta.
3. Premi `Invio`, `Spazio` a campo vuoto o clic destro: le entità vengono cancellate.

`ERASE` non aggiunge un messaggio di completamento. `Invio` a riga vuota subito
dopo la cancellazione ripete `ERASE`, che chiede una nuova selezione. `Ctrl+Z`
ripristina tutto il gruppo cancellato; `Ctrl+Y` lo cancella di nuovo.

**Esempio:** scrivi `e`, clicca il bordo di due fori del disegno di esempio e
premi `Invio`. I due fori spariscono insieme. `Ctrl+Z` li ripristina entrambi.

### Canc e il campo di scrittura

*Edit → Erase* e `Canc` cancellano solo una selezione già pronta, quando nessun
comando è attivo. Senza selezione non fanno nulla: usa `E` per avviare la domanda
`Select objects:`.

Quando il campo della riga di comando ha il fuoco:

- **campo vuoto:** `Canc` cancella la selezione del disegno, se il programma è a
  riposo;
- **campo con testo:** `Canc` modifica il testo come di consueto, senza cancellare
  entità. Vale anche se quel tasto elimina l'ultimo carattere: soltanto un'altra
  pressione, a campo ormai vuoto, può cancellare la selezione del disegno.

Durante `Select objects:` usa `Invio` per confermare: `Canc` non conclude la scelta.

## Snap a oggetto

Lo snap aggancia i punti indicati con il mouse alla geometria esistente mentre
un comando chiede un punto. Il marcatore mostra il punto che verrà accettato
cliccando; l'anteprima usa lo stesso punto. Durante la selezione delle entità non
si applica lo snap.

| Modo nel menu *Tools → Object Snap Modes* | Punto di aggancio | Marcatore |
|---|---|---|
| *Endpoint* | estremo di una linea o di un arco, o vertice della polilinea | quadrato |
| *Midpoint* | metà del segmento; per un arco è sulla curva, non sulla corda | triangolo |
| *Center* | centro di un cerchio, di un arco o di un segmento ad arco | cerchio |
| *Insertion* | punto d’inserimento del testo, sulla linea di base | due quadrati sovrapposti e sfalsati |
| *Intersection* | incontro tra due curve, o tra due segmenti della stessa polilinea, che passano entrambi entro l'apertura | croce a X |
| *Perpendicular* | piede della perpendicolare dal punto base del comando | simbolo ⊥ |
| *Tangent* | punto di tangenza dal punto base del comando a un cerchio, a un arco o a un arco di polilinea | cerchio con linea tangente in alto |
| *Quadrant* | punti cardinali a 0°, 90°, 180°, 270° di cerchi e archi | rombo |

### Attivare e scegliere i modi

- `F3`, *Tools → Object Snap* e il pulsante **OSNAP** nella barra di stato
  attivano o spengono insieme lo snap. Lo storico mostra `<Osnap on>` oppure
  `<Osnap off>`.
- In *Tools → Object Snap Modes* puoi attivare o disattivare separatamente
  i singoli modi, anche quando OSNAP è spento. Spegnere `F3` conserva i modi scelti.
  Se tutti i modi sono disattivati, non c'è alcun aggancio anche con OSNAP acceso.
- All'avvio lo snap è acceso con i modi *Endpoint*, *Midpoint*, *Center*, *Insertion* e
  *Intersection*; *Perpendicular*, *Tangent* e *Quadrant* sono disattivati per difetto.
  Le impostazioni non si conservano alla chiusura del programma.
- Le regole di intersezione, perpendicolare, tangente e quadrante, e la loro
  priorità, sono nel [manuale dell'interfaccia](interfaccia.md#snap-a-intersezione-tangente-perpendicolare-e-quadrante).

Per estremi e punti medi l'apertura è di **10 pixel**: avvicina il puntatore al
punto voluto finché compare il marcatore. La dimensione del marcatore e l'apertura
restano in pixel a qualunque zoom. Fra estremi e medi nell'apertura vince il più
vicino; a pari distanza l'estremo ha precedenza.

Per il **centro** passa sul cerchio o sull’arco, entro l'apertura: il cerchio compare
sul suo centro anche quando è lontano dal puntatore. Il centro viene proposto
quando nell'apertura non c'è un estremo o un punto medio attivo da preferire.
Avvicinarsi soltanto al centro, lontano dalla curva, non basta.

Per l’**inserimento** passa sopra il testo o entro 10 pixel dal suo rettangolo
d’ingombro: il marcatore appare sul punto iniziale della linea di base, anche
se è lontano dal puntatore. Come il centro, perde la precedenza se c’è un
estremo o un medio attivo nell’apertura. Per provarlo, crea un testo con `DT`,
avvia `PL` e passa sulle lettere; togli *Insertion* da *Tools → Object Snap
Modes* per disattivare solo questo aggancio.

**Esempio:** avvia `PL`, avvicinati a uno spigolo del contorno di esempio e clicca
quando compare il quadrato: il primo vertice coincide con quello spigolo. Su un
lato, il triangolo sceglie il punto medio. Per agganciare il centro di un foro,
lascia attivo *Center* e passa sul suo arco; puoi spegnere *Endpoint* e *Midpoint*
per scegliere solo il centro. `F3` spegne tutti gli agganci senza cambiare i modi.

## Orto

`F8`, *Tools → Ortho* e il pulsante **ORTHO** nella barra di stato attivano o
spengono il vincolo ortogonale. Lo storico mostra `<Ortho on>` oppure
`<Ortho off>`. All'avvio l'orto è spento; l'impostazione non si conserva alla
chiusura del programma.

Dal secondo punto di un comando, il puntatore e il clic vengono allineati
all'orizzontale o alla verticale che passa per il punto precedente dello stesso
comando, secondo la direzione più vicina al puntatore. A 45° prevale
l'orizzontale. L'anteprima segue lo stesso vincolo. L'orto non modifica i gesti
di selezione.

### Snap, orto, polare e coordinate scritte

| Come indichi il punto | Effetto |
|---|---|
| clic con uno snap disponibile | usa il punto di snap, anche fuori dall'asse dell'orto o della linea polare: **lo snap prevale su orto e polare** |
| clic senza snap, con Ortho, dal secondo punto del comando | allinea il punto in orizzontale o verticale |
| clic senza snap, con Polar, entro 10 pixel da un raggio | aggancia il punto alla semiretta multipla di `POLARANG` |
| coordinate `x,y`, `@dx,dy`, `d<a`, `@d<a` | usa le coordinate esatte, senza correggerle con snap, orto o polare |
| sola distanza `d` | usa quella lunghezza dall'ultimo punto, nella direzione del puntatore già corretto da snap, orto o polare |

**Esempio con orto e distanza diretta:** spegni OSNAP se è acceso e accendi ORTHO.
Avvia `PL` e scrivi `0,0`. Porta il puntatore verso destra, più vicino
all'orizzontale che alla verticale, scrivi `50` e premi `Invio`: il nuovo vertice
è `(50,0)`. Porta il puntatore verso l'alto rispetto a questo vertice e scrivi
`30`: ottieni `(50,30)`. Un altro `Invio` conclude la polilinea.

Durante un `PL` con ORTHO acceso, un punto scritto come `@10,7` aggiunge
esattamente 10 unità in X e 7 in Y: la diagonale scritta non viene raddrizzata.
Se riaccendi OSNAP e clicchi un estremo riconosciuto fuori asse, viene scelto
quell'estremo.

**Primo punto di un nuovo comando:** l'orto non lo allinea, come in AutoCAD: il
primo clic di un nuovo `PL` cade dove clicchi (o sullo snap). Anche dopo aver
annullato con `Undo` tutti i vertici di un `PL`, il clic successivo non viene
allineato. Le coordinate scritte invece partono ancora dall'ultimo punto, anche
se è del comando precedente: `@20,0` o `@50<90` come primo punto si misurano da
lì, e una distanza scritta da sola prende la direzione del puntatore allineato
dall'orto rispetto a quel punto. Nella 0.3.0 anche il primo clic veniva
allineato ([bug #64](https://github.com/Matleyx78/tecnigrafo/issues/64)).

Su alcuni portatili e su macOS `F3` e `F8` possono richiedere `Fn`. Puoi sempre
usare le voci di *Tools* o i pulsanti OSNAP e ORTHO.

## LINE — linee concatenate

- **Nome:** `LINE`
- **Alias:** `L`

`Specify first point:` chiede il primo punto; poi `Specify next point or
[Undo]:` chiede quelli successivi. Dopo tre punti il prompt diventa
`Specify next point or [Close/Undo]:`.

Ogni coppia di punti crea una **linea distinta**, selezionabile da sola.
`U` (*Undo*) toglie l’ultimo segmento e torna al punto precedente; togliendo
anche il primo punto si riparte. `C` (*Close*), disponibile da tre punti,
aggiunge la linea verso il primo punto e conclude. `Invio` a campo vuoto o `Esc`
concludono conservando i segmenti già disegnati; il tratto verso il puntatore
non viene aggiunto. Con un solo punto non resta niente. Tutte le linee dello
stesso comando si annullano insieme con un solo `U` a riposo.

**Esempio: un triangolo di tre linee.**

```
Command: l
Specify first point: 0,0
Specify next point or [Undo]: 50,0
Specify next point or [Undo]: 25,30
Specify next point or [Close/Undo]: c
```

`Invio` alla prima domanda (`Specify first point:`) fa ripartire la linea
dall’ultimo punto indicato nel disegno, ad esempio l'estremo di
una linea o di un arco precedente. Se non è ancora presente alcun punto
precedente, la domanda resta attiva; per uscire dal comando premi `Esc`.
Un punto coincidente con il precedente viene ignorato con
`Point coincides with the previous point.`.

## CIRCLE — cerchio

- **Nome:** `CIRCLE`
- **Alias:** `C`

Alla domanda `Specify center point for circle or [3P/2P/Ttr (tan tan radius)]:`
scegli il metodo:

| Metodo | Risposte |
|---|---|
| centro e raggio | indica il centro, poi scrivi il raggio o indica un punto sul cerchio |
| centro e diametro | indica il centro, scrivi `D` (*Diameter*), poi il diametro come numero o distanza dal centro a un punto |
| due punti | scrivi `2P`, poi i due estremi del diametro |
| tre punti | scrivi `3P`, poi tre punti sulla circonferenza |
| tangente, tangente, raggio | scrivi `T` o `Ttr`, indica due oggetti, poi il raggio |

Il raggio o diametro deve essere positivo: altrimenti compare
`Value must be positive and nonzero.` e puoi riprovare. In `3P`, punti
allineati, quasi allineati o coincidenti producono `The three points are
collinear.`: indica un altro terzo punto.

**Esempi:** `C`, `0,0`, `25` crea un cerchio di centro `(0,0)` e raggio 25.
Un nuovo `C`, `80,0`, `Invio` riusa quel raggio, mostrato come `<25.0000>`.
`C`, `80,0`, `D`, `50` dà lo stesso raggio. `C`, `2P`, `0,0`, `50,0` crea
invece un cerchio di centro `(25,0)` e raggio 25. `C`, `3P`, `10,0`, `0,10`,
`-10,0` crea quello di centro `(0,0)` e raggio 10.

Il cerchio si completa con l’ultima risposta; prima, `Esc` non lascia entità.
Un `Invio` senza valore predefinito termina senza effetti. Il clic per la
selezione deve essere sulla circonferenza, non nel suo interno.

### Tangente, tangente, raggio (*Ttr*)

Disegna un cerchio del raggio dato, tangente a due oggetti:

```
Specify point on object for first tangent of circle:
Specify point on object for second tangent of circle:
Specify radius of circle <25.0000>:
```

- Alle prime due domande clicca un oggetto, o scrivi un punto che vi passi
  vicino: valgono linee, archi, cerchi e polilinee; di una polilinea conta il
  segmento più vicino al punto. Un clic nel vuoto non fa nulla e la domanda
  resta.
- Al raggio scrivi un numero, indica due punti (la loro distanza: dopo il primo
  compare `Specify second point:`) o premi `Invio` per l'ultimo raggio usato,
  mostrato tra `< >` solo se c'è già. Un valore nullo o negativo produce
  `Value must be positive and nonzero.` e la domanda torna.
- Fra i cerchi tangenti possibili vince quello più vicino ai due punti
  indicati: dove clicchi decide in quale angolo cade il cerchio.
- Se il nucleo geometrico non dà un cerchio da disegnare compare
  `Circle does not exist.`: nessun cerchio di quel raggio tocca i due oggetti,
  oppure ce ne sono infiniti (per esempio due segmenti sulla stessa retta),
  oppure uno dei due oggetti è un testo o il raggio è oltre i limiti che il
  programma rappresenta. Se il cerchio esiste ma cade fuori da quei limiti
  compare invece `Result is beyond the drawing limits.`. In tutti e due i casi il comando finisce senza disegnare nulla.

**Esempio:** con le linee da `0,0` a `40,0` e da `0,0` a `0,40`:

```
Command: c
Specify center point for circle or [3P/2P/Ttr (tan tan radius)]: t
Specify point on object for first tangent of circle: 10,0
Specify point on object for second tangent of circle: 0,10
Specify radius of circle: 8
```

nasce il cerchio di centro `(8,8)` e raggio 8, tangente alle due linee.

## ARC — arco

- **Nome:** `ARC`
- **Alias:** `A`

Sono disponibili due metodi:

- **Tre punti:** rispondi a `Specify start point of arc:`, `Specify second point
  of arc or [Center]:` e `Specify end point of arc:`. L’arco passa per i tre
  punti nell’ordine dato, anche in senso orario.
- **Inizio, centro, fine:** indica l’inizio, poi `C` (*Center*), il centro e il
  punto finale. L’arco va in senso **antiorario** dall’inizio verso la direzione
  centro-fine. Il raggio è la distanza inizio-centro: se l’ultimo punto è a
  un’altra distanza, determina solo la direzione della fine.

**Esempi:** `A`, `10,0`, `0,10`, `-10,0` disegna il semicerchio superiore di
raggio 10. `A`, `10,0`, `C`, `0,0`, `0,20` disegna un quarto di cerchio di
raggio 10 che termina in `(0,10)`.

Tre punti allineati, quasi allineati o coincidenti danno `The three points are
collinear.`. Il centro uguale all’inizio dà `Center coincides with the start
point.`; una fine al centro o sulla stessa direzione dell’inizio dà
`End point gives a zero-length arc.`. In questi casi la domanda resta.
`Esc` o `Invio` prima di completare l’arco non lasciano nulla.

## PLINE — polilinea

Disegna un’unica polilinea di segmenti retti e archi tangenti, aperta o chiusa.

- **Nome:** `PLINE`
- **Alias:** `PL`

### Le domande e le opzioni

`Specify start point:` chiede il primo vertice. Nel modo linea compare
`Specify next point or [Arc/Undo]:`; dopo tre vertici compare anche *Close*:
`Specify next point or [Arc/Close/Undo]:`.

Con `A` passi al modo arco: `Specify end point of arc or [Line/Undo]:`, oppure
`Specify end point of arc or [CLose/Line/Undo]:` da tre vertici.

| Opzione | Scrittura | Effetto |
|---|---|---|
| *Arc* | `A` o `arc` | dal modo linea passa agli archi tangenti |
| *Line* | `L` o `line` | dal modo arco torna ai segmenti retti |
| *Close* nel modo linea | `C` o `close` | chiude con un segmento retto verso il primo punto e conclude (almeno 3 vertici) |
| *CLose* nel modo arco | `CL` o `close` | chiude con un arco tangente verso il primo punto e conclude (almeno 3 vertici) |
| *Undo* | `U` o `undo` | toglie l’ultimo vertice senza cambiare modo; senza vertici torna alla prima domanda |

Un arco parte tangente al segmento precedente, anche se era un arco. Come primo
segmento parte tangente all’asse X positivo. Un punto dritto davanti alla
tangente crea un segmento retto; dritto dietro dà `Invalid end point of arc.`
e puoi riprovare. L’anteprima segue il puntatore nei due modi.

### Concludere

- `Invio`, `Spazio` o clic destro a campo vuoto concludono la polilinea aperta.
- *Close* conclude la polilinea chiusa, con il tratto del modo attivo.
- `Esc` conserva una polilinea **aperta** con i vertici già dati, senza il tratto
  dell’anteprima. Con meno di due vertici non resta niente.

Un punto coincidente con il precedente viene ignorato con
`Point coincides with the previous point.`. La polilinea è una sola entità e un
solo passo di annullamento, anche se conclusa con `Esc`.

### Esempi

**Un quadrato di lato 50:**

```
Command: pl
Specify start point: 0,0
Specify next point or [Arc/Undo]: @50,0
Specify next point or [Arc/Undo]: @50<90
Specify next point or [Arc/Close/Undo]: 0,50
Specify next point or [Arc/Close/Undo]: c
```

**Linea, due archi, linea:** `PL`, `0,0`, `50,0`, `A`, `60,10`, `50,20`,
`L`, `30,20`, `Invio`. Il primo arco parte tangente alla linea; il secondo è
tangente al primo. Con `U` durante il comando togli l’ultimo vertice e puoi
indicarne un altro. Sostituendo l’ultimo `Invio` con `Esc` resta lo stesso
profilo aperto.

Per un profilo a L con lunghezze note usa la [distanza diretta con
orto](#snap-orto-e-coordinate-scritte).

## TEXT — testo su una riga

- **Nome:** `TEXT`
- **Alias:** `DT`, `DTEXT`

1. `Specify start point of text:` — indica l’inizio della linea di base.
2. `Specify height <2.5000>:` — altezza delle maiuscole: numero positivo oppure
   distanza dal punto iniziale a un punto scritto o cliccato; `Invio` accetta
   il valore proposto.
3. `Specify rotation angle of text <0>:` — gradi antiorari da +X, oppure
   direzione dal punto iniziale a un punto; `Invio` accetta il valore proposto.
4. `Enter text:` — scrivi una riga e premi `Invio`; ripeti per altre righe.
   `Invio` su una riga vuota conclude.

Ogni riga è un’entità distinta, allineata a sinistra sulla linea di base.
La successiva parte sotto la precedente di **5/3 dell’altezza**, nel sistema
ruotato del testo. Tutte le righe dello stesso comando si annullano con un solo
`U` a riposo. `Esc` conserva le righe già confermate e scarta quella ancora nel
campo; prima della prima riga non lascia nulla. Il rettangolo d’anteprima,
grande come una “M”, indica la posizione della riga successiva.

Altezza e rotazione di un testo creato diventano i valori proposti dal comando
successivo. Altezza zero o negativa dà `Value must be positive and nonzero.`
senza avanzare alla domanda successiva.

### Codici di controllo e font

| Scritto nel testo | Disegnato |
|---|---|
| `%%c` | simbolo di diametro, mostrato come `Ø` nel font incluso |
| `%%d` | `°` |
| `%%p` | `±` |
| `%%%` | `%` |

Il valore conserva i codici originali. Codici non gestiti, come `%%u`, e codici
incompleti restano letterali. Si usa **Liberation Sans Regular 2.1.5**, incluso
nel programma sui tre sistemi. `⌀` e `∅` sono mostrati come `Ø`; gli altri
caratteri mancanti usano il glifo mancante del font. Sotto 2 pixel di altezza
delle maiuscole il testo appare come il solo rettangolo d’ingombro: avvicina lo
zoom per leggerlo. Gli stili e il testo multiriga come entità unica non sono
ancora disponibili.

**Esempio con i valori iniziali:**

```
Command: dt
Specify start point of text: 0,0
Specify height <2.5000>:
Specify rotation angle of text <0>:
Enter text: Hello world
Enter text: Foro %%c10 %%p0.1
Enter text:
```

Ottieni due testi di altezza 2.5, rotazione 0, con la seconda linea di base a
Y = −4.1667 circa. Gli spazi restano nel testo e la seconda riga mostra
`Foro Ø10 ±0.1`.

**Testo inclinato:** `DT`, `100,50`, `5`, `30`, `Prova d’uso è 100%%c`,
`Seconda riga`, `Invio`. Le due righe sono inclinate di 30° e distano 25/3
unità nella direzione perpendicolare verso il basso del testo.

## MOVE — spostare entità

Sposta le entità selezionate applicando un vettore di spostamento.

- **Nome:** `MOVE`
- **Alias:** `M`

### Le domande e le opzioni

Se avvii il comando senza una selezione attiva, compare prima la richiesta di selezione
degli oggetti: `Select objects:`. Scegli le entità e premi `Invio` a riga vuota. Con una
selezione già attiva prima del comando, `MOVE` la utilizza subito e passa alla richiesta
del punto base:

```
Specify base point or [Displacement] <Displacement>:
```

- **Punto base e secondo punto:** indica un punto base con un clic nella vista o scrivendo
  le coordinate. Il comando chiede il punto di destinazione:
  ```
  Specify second point or <use first point as displacement>:
  ```
  - Indicando un secondo punto, le entità vengono spostate della differenza tra il secondo
    e il primo punto (`secondo punto − punto base`).
  - Premendo `Invio` a riga vuota, le coordinate del primo punto vengono usate direttamente
    come vettore di spostamento rispetto all'origine `(0,0)`.
- **Opzione *Displacement*:** scrivi `D` o premi `Invio` alla prima domanda per inserire
  direttamente lo spostamento:
  ```
  Specify displacement <0.0000, 0.0000>:
  ```
  - Digita le coordinate dello spostamento (es. `10,20`).
  - Premendo `Invio` viene confermato lo spostamento predefinito `<0.0000, 0.0000>`, lasciando
    le entità nella posizione corrente.

Durante lo spostamento del puntatore, l'anteprima mostra gli oggetti nella posizione
risultante. Le entità spostate conservano il loro identificativo (`handle`), la posizione
nell'ordine del disegno e tutti i dati non gestiti. L'operazione genera un unico passo di
annullamento (`Move`).

**Esempio:** sposta un cerchio di 50 unità a destra:

```
Command: m
Select objects: 1 found, 1 total
Select objects: 
Specify base point or [Displacement] <Displacement>: 0,0
Specify second point or <use first point as displacement>: 50,0
```

Oppure con *Displacement*:

```
Command: move
Select objects: 1 found, 1 total
Select objects: 
Specify base point or [Displacement] <Displacement>: d
Specify displacement <0.0000, 0.0000>: 50,0
```

## COPY — copiare entità

Crea una o più copie delle entità selezionate applicando un vettore di spostamento.

- **Nome:** `COPY`
- **Alias:** `CO`, `CP`

### Le domande e le opzioni

Dopo aver selezionato gli oggetti (`Select objects:`), compare il messaggio informativo
sulla modalità multipla:

```
Current settings:  Copy mode = Multiple
Specify base point or [Displacement] <Displacement>:
```

- **Punto base e copie multiple:** indica il punto base; compare la richiesta:
  ```
  Specify second point or <use first point as displacement>:
  ```
  - Indicando un secondo punto viene creata la prima copia, e il comando passa alla
    creazione di copie successive:
    ```
    Specify second point or [Exit/Undo] <Exit>:
    ```
    - Ogni ulteriore punto indicato aggiunge una nuova copia con quello spostamento dal punto base.
    - `U` (*Undo*) annulla l'ultima copia creata nel comando.
    - `E` (*Exit*) o `Invio` a riga vuota conclude il comando.
  - Premendo `Invio` alla prima richiesta del secondo punto, il punto base viene usato
    come vettore di spostamento: viene creata una copia e il comando passa alla richiesta
    delle copie successive (`Specify second point or [Exit/Undo] <Exit>:`).
- **Opzione *Displacement*:** digitando `D` o premendo `Invio` alla prima domanda:
  ```
  Specify displacement <0.0000, 0.0000>:
  ```
  Scrivi il vettore di spostamento per creare una singola copia e concludere.

Interrompere `COPY` con `Esc` conclude il comando conservando tutte le copie già
create (ADR 0015). Le nuove entità ricevono nuovi handle, vengono inserite in coda
all'ordine del disegno e conservano proprietà e dati non gestiti dell'originale
(tranne i gruppi 102). L'intero comando costituisce un unico passo di annullamento (`Copy`).

Negli script `.scr` (o con `!scr on`), `COPY` adotta la sequenza degli script di AutoCAD
concludendosi automaticamente dopo la prima copia.

**Esempio:** crea due copie di una linea a distanza 20 e 40 lungo l'asse X:

```
Command: co
Select objects: 1 found, 1 total
Select objects: 
Current settings:  Copy mode = Multiple
Specify base point or [Displacement] <Displacement>: 0,0
Specify second point or <use first point as displacement>: 20,0
Specify second point or [Exit/Undo] <Exit>: 40,0
Specify second point or [Exit/Undo] <Exit>: 
```

## ROTATE — ruotare entità

Ruota le entità selezionate attorno a un punto base.

- **Nome:** `ROTATE`
- **Alias:** `RO`

### Le domande e le opzioni

All'avvio il comando ricorda la convenzione degli angoli e chiede il punto base:

```
Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0
Specify base point:
```

Indica il punto perno attorno a cui ruotare gli oggetti (premendo `Invio` a riga
vuota il comando si conclude senza effetti). Segue la domanda dell'angolo:

```
Specify rotation angle or [Copy/Reference] <0>:
```

- **Angolo di rotazione:**
  - Scrivi un valore numerico in gradi (in senso antiorario da +X): gli oggetti vengono
    ruotati di quell'angolo.
  - Clicca un punto nella vista: l'angolo applicato corrisponde alla direzione della
    linea dal punto base verso il punto cliccato.
  - Premi `Invio` a riga vuota per confermare l'ultimo angolo usato (mostrato tra
    parentesi angolari `< >`, all'avvio `<0>`).
- **Opzione *Copy*:** scrivi `C` per ruotare una copia mantenendo gli oggetti originali.
  Compare il messaggio `Rotating a copy of the selected objects.` e viene riproposta
  la domanda dell'angolo. Le copie ricevono nuovi handle.
- **Opzione *Reference*:** scrivi `R` per ruotare specificando un angolo di partenza
  e l'orientamento finale desiderato:
  ```
  Specify the reference angle <0>:
  ```
  Indica l'angolo di riferimento numericamente oppure con due punti (`Specify second point:`).
  Quindi inserisci il nuovo angolo:
  ```
  Specify the new angle or [Points] <0>:
  ```
  Digita l'angolo, clicca dal punto base, oppure scegli `P` (*Points*) per indicare due
  punti arbitrari (`Specify first point:`, `Specify second point:`). La rotazione
  effettiva applicata corrisponde a `nuovo angolo − angolo di riferimento`.

L'anteprima segue dinamicamente il cursore mostrando l'inclinazione risultante. La
rotazione sul posto conserva gli handle originali delle entità. Il comando crea un
passo di annullamento (`Rotate`).

**Esempio:** ruota un oggetto di 45 gradi attorno all'origine:

```
Command: ro
Select objects: 1 found, 1 total
Select objects: 
Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0
Specify base point: 0,0
Specify rotation angle or [Copy/Reference] <0>: 45
```

Con opzione *Reference*: allinea a 90° un oggetto orientato a 30°:

```
Command: ro
Select objects: 1 found, 1 total
Select objects: 
Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0
Specify base point: 0,0
Specify rotation angle or [Copy/Reference] <0>: r
Specify the reference angle <0>: 30
Specify the new angle or [Points] <0>: 90
```

## SCALE — scalare entità

Ridimensiona in modo uniforme le entità selezionate attorno a un punto base.

- **Nome:** `SCALE`
- **Alias:** `SC`

### Le domande e le opzioni

Indica il punto perno rispetto a cui calcolare il ridimensionamento (premendo
`Invio` a riga vuota il comando si conclude senza effetti):

```
Specify base point:
```

Seguito dal fattore di scala o da un'opzione:

```
Specify scale factor or [Copy/Reference]:
```

- **Fattore di scala:**
  - Inserisci un numero positivo strettamente maggiore di zero (es. `2` per raddoppiare,
    `0.5` per dimezzare). Valori negativi o nulli mostrano il messaggio d'errore
    `Value must be positive and nonzero.` e ripropongono la domanda.
  - Clicca un punto nella vista: il fattore di scala sarà la distanza tra il punto
    indicato e il punto base.
  - Premendo `Invio` a riga vuota il comando si conclude senza effetti.
- **Opzione *Copy*:** scrivi `C` per applicare la scala a una copia degli oggetti,
  lasciando invariati gli originali. Il programma scrive:
  `Scaling a copy of the selected objects.` e ripropone la richiesta del fattore.
- **Opzione *Reference*:** scrivi `R` per scalare in base a una lunghezza di riferimento
  e alla nuova lunghezza voluta:
  ```
  Specify reference length <1.0000>:
  ```
  Digita la lunghezza o indica due punti (`Specify second point:`). Segue:
  ```
  Specify new length or [Points] <1.0000>:
  ```
  Digita la nuova lunghezza, indica un punto (distanza dal punto base), oppure scrivi
  `P` (*Points*) per definire la nuova lunghezza con due punti (`Specify first point:`,
  `Specify second point:`). Il fattore applicato è `nuova lunghezza / lunghezza di riferimento`.

L'anteprima mostra in tempo reale le dimensioni scalate. La scala sul posto conserva
gli handle originali. Crea un passo di annullamento (`Scale`).

**Esempio:** scala un oggetto di un fattore 2.5 rispetto all'origine:

```
Command: sc
Select objects: 1 found, 1 total
Select objects: 
Specify base point: 0,0
Specify scale factor or [Copy/Reference]: 2.5
```

Con opzione *Reference*: porta un segmento lungo 40 unità alla misura di 100 unità:

```
Command: sc
Select objects: 1 found, 1 total
Select objects: 
Specify base point: 0,0
Specify scale factor or [Copy/Reference]: r
Specify reference length <1.0000>: 40
Specify new length or [Points] <1.0000>: 100
```

## MIRROR e MIRRTEXT — specchiare entità

`MIRROR` riflette le entità selezionate rispetto a un asse di specchiatura definito da due
punti. La variabile `MIRRTEXT` controlla se il testo viene specchiato graficamente
oppure mantenuto orientato in modo leggibile.

- **Nome:** `MIRROR`
- **Alias:** `MI`

### Le domande e le opzioni di `MIRROR`

1. `Specify first point of mirror line:` indica il primo punto dell'asse di riflessione.
2. `Specify second point of mirror line:` indica il secondo punto dell'asse. Se il
   secondo punto coincide con il primo, la domanda resta attiva finché non viene fornito
   un asse valido.
3. `Erase source objects? [Yes/No] <No>:`
   - `No` (o `Invio` a vuoto): crea copie specchiate mantenendo gli oggetti originari.
     Le nuove entità prendono nuovi identificativi (`handle`).
   - `Yes` (o `Y`): riflette le entità sul posto ed elimina gli oggetti originari. Le
     entità mantengono i loro handle originari.

L'anteprima mostra dinamicamente gli oggetti riflessi non appena si sposta il puntatore
per scegliere il secondo punto dell'asse. Il comando crea un passo di annullamento (`Mirror`).

### La variabile `MIRRTEXT`

Comando per regolare il comportamento del testo durante la specchiatura con `MIRROR`:

```
Command: mirrtext
Enter new value for MIRRTEXT <0>:
```

- `0` (valore predefinito): il testo specchiato viene mantenuto leggibile e non si legge
  allo specchio (non raddrizza però un testo che era già all'indietro o capovolto: ne
  conserva i relativi segnali).
- `1`: il testo viene specchiato integralmente come geometria (effetto immagine riflessa
  in uno specchio).

Valori diversi da `0` e `1` producono il messaggio `Requires an integer between 0 and 1.`.

**Esempio:** specchia un profilo rispetto alla retta verticale X = 50 conservando gli originali:

```
Command: mi
Select objects: 1 found, 1 total
Select objects: 
Specify first point of mirror line: 50,0
Specify second point of mirror line: 50,100
Erase source objects? [Yes/No] <No>: n
```

E per `MIRRTEXT`:

```
Command: mirrtext
Enter new value for MIRRTEXT <0>: 1
```

## EXPLODE — scomporre polilinee

Scompone le polilinee 2D selezionate nei singoli segmenti retti (`LINE`) e archi (`ARC`)
di cui sono composte.

- **Nome:** `EXPLODE`
- **Alias:** `X`

### Funzionamento

Avviato il comando, compare la richiesta `Select objects:`. Seleziona le polilinee da
scomporre e conferma con `Invio`.

- Ogni segmento rettilineo della polilinea diventa un'entità `LINE`; ogni segmento curvo
  diventa un'entità `ARC`.
- Le nuove entità generate ricevono nuovi handle, si aggiungono in fondo all'ordine del
  disegno ed ereditano layer, colore, tipo di linea, spessore e dati non gestiti della
  polilinea di origine.
- Se la polilinea contiene larghezze dei vertici o larghezza costante, queste vengono
  rimosse durante la scomposizione. Il programma avvisa con due messaggi nello storico:
  ```
  Exploding this polyline has lost width information.
  The UNDO command will restore it.
  ```
- Entità diverse dalle polilinee 2D (come linee, cerchi, archi, testi), polilinee senza
  segmenti, o polilinee aventi quota z (*elevation*) o spessore 3D (*thickness*)
  diversi da zero (gruppi DXF 38 e 39) non possono essere scomposte. Vengono lasciate
  intatte e segnalate con:
  `1 was not able to be exploded.` oppure `<n> were not able to be exploded.`

Tutte le scomposizioni del comando avvengono in un singolo passo di annullamento (`Explode`):
un solo `U` o `Ctrl+Z` ripristina la polilinea originale integra con le sue larghezze.

**Esempio:** scompone una polilinea in linee e archi:

```
Command: x
Select objects: 1 found, 1 total
Select objects: 
```

## TRIM ed EXTEND — tagliare e prolungare

`TRIM` toglie il pezzo dell'oggetto compreso tra i due bordi più vicini al punto
che indichi. `EXTEND` prolunga l'estremo più vicino al punto fino al primo bordo
che incontra. I due comandi si scambiano con `Shift`: in `TRIM` un `Shift`+clic
prolunga, in `EXTEND` taglia.

- **Nomi e alias:** `TRIM` (`TR`) ed `EXTEND` (`EX`)

### I due modi e `TRIMEXTENDMODE`

| Modo | `TRIMEXTENDMODE` | Bordi | Messaggio all'avvio |
|---|---|---|---|
| **rapido** | 1, all'avvio del programma | tutte le linee, gli archi, i cerchi e le polilinee sui layer visibili, anche bloccati | `Current settings: Trim mode = Quick, Projection=UCS, Edge=None` |
| **standard** | 0 | quelli che scegli all'inizio del comando | `Current settings: Projection=UCS, Edge=None, Mode=Standard` |

Il testo non fa mai da bordo. Nel modo standard, dopo il messaggio, compaiono
`Select cutting edges ...` (in `EXTEND`, `Select boundary edges ...`) e la
domanda dei bordi:

```
Select objects or [mOde] <select all>:
```

Scegli i bordi con i gesti della selezione o con la selezione scritta, poi
premi `Invio`. `Invio` senza aver scelto nulla prende tutti i bordi, come nel
modo rapido.

- **`TRIMEXTENDMODE`** imposta il modo con cui partono i due comandi:
  `Enter new value for TRIMEXTENDMODE <1>:`. Accetta `0` o `1`; altro testo
  produce `Requires an integer between 0 and 1.` e `Invio` conserva il valore.
  Vale per la sessione: non si salva nel disegno e non è un passo di
  annullamento.
- **Opzione *mOde*** (`O`), sia alla domanda dei bordi sia a quella dell'oggetto:
  `Enter a trim mode option [Quick/Standard] <Quick>:`, con il modo in uso tra
  `< >`. Cambia anche `TRIMEXTENDMODE`. Passando al modo standard il comando chiede i bordi; `Invio`
  lascia il modo com'è.
- **Negli script** di AutoCAD vale sempre il modo standard, anche dopo *mOde*
  (sezione "Negli script").

### Tagliare

La domanda è `Select object to trim or shift-select to extend or [mOde]:`; dopo
la prima operazione diventa `[mOde/Undo]`. Indica l'oggetto nel punto del pezzo
da togliere (sezione "Indicare un oggetto"). Si tagliano linee, archi, cerchi e
polilinee:

- si toglie il pezzo tra l'ultimo taglio prima del punto e il primo dopo; se il
  punto cade proprio su un taglio, si toglie il pezzo che segue;
- una linea, un arco o una polilinea aperta tagliati in mezzo diventano due
  entità: la prima conserva l'entità originale, la seconda è una copia con le
  stesse proprietà;
- un cerchio tagliato diventa un arco, la stessa entità; una polilinea chiusa
  tagliata diventa aperta;
- **una polilinea che è tra i bordi fa da bordo anche a se stessa**: i suoi
  vertici e i punti in cui si incrocia delimitano i pezzi, come in AutoCAD. Un
  clic su un lato di un rettangolo toglie quel lato da vertice a vertice, anche
  se un'altra linea lo attraversa più lontano;
- un bordo tangente taglia nel punto di tangenza; un bordo che ha un tratto in
  comune con l'oggetto (sulla stessa retta o sullo stesso cerchio) non lo taglia;
- un pezzo che resta e che è troppo piccolo per le unità del disegno non si
  tiene: se non resta nulla, l'oggetto si cancella;
- **senza tagli**, nel modo rapido l'oggetto si cancella; nel modo standard
  compare `Object does not intersect an edge.` e l'oggetto resta;
- il testo non si taglia: il clic non ha effetto.

### Prolungare

La domanda di `EXTEND` è `Select object to extend or shift-select to trim or
[mOde]:`, poi `[mOde/Undo]`. Si prolunga l'estremo più vicino al punto
indicato; a pari distanza, la fine:

- una linea lungo la sua retta, un arco lungo il suo cerchio;
- una polilinea aperta con il suo primo o il suo ultimo segmento, dritto o
  curvo;
- fino al **primo** bordo che l'estremo incontra.

Cerchi, polilinee chiuse e testi non si prolungano. Se non c'è un bordo da
raggiungere, l'oggetto resta com'è, senza messaggi.

### Annullare e concludere

- **Opzione *Undo*** (`U`): annulla l'ultima operazione del comando, che resta
  aperto.
- `Invio`, `Spazio` a campo vuoto o clic destro concludono. Tutto il comando è
  un solo passo di annullamento, `Trim` o `Extend`; `Esc` conserva le
  operazioni già fatte.
- Un clic nel vuoto non apre un rettangolo. Le opzioni *cuTting edges*,
  *Crossing*, *Fence*, *Project*, *Edge* ed *eRase* di AutoCAD non ci sono.
- Se un'operazione non riesce (dati oltre i limiti che il programma
  rappresenta, o un pezzo che non si può costruire) compare `Cannot trim that
  object.` o `Cannot extend that object.`, e l'oggetto resta com'è.

**Esempio nel modo rapido:** con la linea da `0,0` a `30,0` e due linee verticali
che la attraversano in `x = 10` e `x = 20`:

```
Command: tr
Current settings: Trim mode = Quick, Projection=UCS, Edge=None
Select object to trim or shift-select to extend or [mOde]: 15,0
Select object to trim or shift-select to extend or [mOde/Undo]:
```

restano le linee da `0,0` a `10,0` e da `20,0` a `30,0`. Un `Shift`+clic in
`7,20` su una linea da `0,20` a `8,20` la prolunga invece fino alla verticale in
`x = 10`.

**Esempio nel modo standard:** dopo `TRIMEXTENDMODE` `0`, la stessa linea da
`0,20` a `8,20` si prolunga fino alla verticale scelta come bordo:

```
Command: ex
Current settings: Projection=UCS, Edge=None, Mode=Standard
Select boundary edges ...
Select objects or [mOde] <select all>: 10,25
1 found, 1 total
Select objects or [mOde] <select all>:
Select object to extend or shift-select to trim or [mOde]: 7,20
Select object to extend or shift-select to trim or [mOde/Undo]:
```

## OFFSET — copia parallela

Crea una copia parallela di una linea, di un arco, di una polilinea, o
concentrica di un cerchio, a una distanza data o alla distanza di un punto.

- **Nome:** `OFFSET`
- **Alias:** `O`

### Le domande e le opzioni

All'avvio il comando mostra le impostazioni e chiede la distanza:

```
Current settings: Erase source=No  Layer=Source  OFFSETGAPTYPE=0
Specify offset distance or [Through/Erase/Layer] <Through>:
```

- **Distanza:** scrivi un numero, oppure indica due punti (dopo il primo compare
  `Specify second point:`) e vale la loro distanza. Un valore nullo o negativo
  produce `Value must be positive and nonzero.`. `Invio` usa il valore tra
  `< >`: all'avvio *Through*, poi l'ultima distanza.
- ***Through*** (`T`): la distanza la dà un punto che indicherai.
- ***Erase*** (`E`): `Erase source object after offsetting? [Yes/No] <No>:`;
  con *Yes* l'oggetto di partenza si cancella dopo la copia.
- ***Layer*** (`L`): `Enter layer option for offset objects [Current/Source]
  <Source>:`; con *Current* la copia va sul layer corrente, con *Source* sul
  layer dell'oggetto.

Dopo *Erase* e *Layer* torna la domanda della distanza. Distanza, *Erase* e
*Layer* restano per i prossimi `OFFSET` della sessione. Poi il comando ripete:

```
Select object to offset or [Exit/Undo] <Exit>:
Specify point on side to offset or [Exit/Multiple/Undo] <Exit>:
```

Con *Through* la seconda domanda è `Specify through point or
[Exit/Multiple/Undo] <Exit>:`.

- Il punto indica il **lato**. Per una polilinea conta il segmento più vicino al
  punto (a pari distanza, il primo). Un punto esattamente sulla retta o sul
  cerchio di quel segmento non indica un lato e non dà copie.
- Con *Through* la distanza è quella del punto dall'oggetto, misurata dal
  punto più vicino dell'oggetto, estremi compresi, e il lato è quello del
  punto. Per una linea o un arco la copia passa per il punto quando il punto
  sta di fronte all'oggetto: la sua perpendicolare alla linea, o la retta dal
  centro dell'arco, cade sull'oggetto, anche proprio in un estremo. Se invece
  il punto sta oltre un estremo, la distanza si misura da quell'estremo e la
  copia non passa per il punto. Per la linea da `0,0` a `10,0`: con il punto
  `10,5` la copia va da `0,5` a `10,5` e finisce nel punto; con il punto `13,4`
  la distanza è 5, misurata da `10,0`, e la copia è la stessa, lontana dal
  punto.
- ***Multiple*** (`M`): una copia dopo l'altra, ciascuna dall'ultima creata;
  `Invio` torna alla domanda dell'oggetto.
- ***Undo*** (`U`): toglie l'ultima copia.
- ***Exit*** (`E`), `Invio` a campo vuoto o clic destro concludono.

La copia è un'entità nuova con le proprietà dell'oggetto (salvo il layer con
*Current*). Nelle polilinee ogni segmento della copia tiene le larghezze del
segmento da cui viene; gli archi di giunzione tra un segmento e l'altro hanno
larghezza 0, salvo la larghezza costante della polilinea. Tutto il comando è un
solo passo di annullamento (`Offset`); `Esc` conserva le copie già fatte.

### Che cosa è garantito

La copia la costruisce il nucleo geometrico, e Tecnigrafo garantisce ciò che
garantisce il nucleo, niente di più. Quando la copia non esiste o non si può
costruire (un cerchio verso l'interno con distanza pari o maggiore del raggio,
una polilinea troppo stretta per la distanza, un testo, un punto senza lato) il
comando scrive `Cannot offset that object.` e **non crea nulla: mai una copia
sbagliata**. In casi rari la copia di una polilinea può differire da quella di
AutoCAD; le giunzioni seguono AutoCAD con `OFFSETGAPTYPE` 0.

**Esempio:** copia a distanza 5, verso l'alto, la linea da `0,0` a `50,0`:

```
Command: o
Current settings: Erase source=No  Layer=Source  OFFSETGAPTYPE=0
Specify offset distance or [Through/Erase/Layer] <Through>: 5
Select object to offset or [Exit/Undo] <Exit>: 10,0
Specify point on side to offset or [Exit/Multiple/Undo] <Exit>: 10,5
Select object to offset or [Exit/Undo] <Exit>:
```

Nasce la linea da `0,5` a `50,5`.

## FILLET — raccordare

Unisce due oggetti con un arco tangente a entrambi, o con uno spigolo vivo.

- **Nome:** `FILLET`
- **Alias:** `F`

### Le domande e le opzioni

```
Current settings: Mode = TRIM, Radius = 0.0000
Select first object or [Undo/Polyline/Radius/Trim/Multiple]:
Select second object or shift-select to apply corner or [Radius]:
```

- **Gli oggetti:** due entità diverse tra linee, archi e cerchi, oppure due
  segmenti dritti e consecutivi della stessa polilinea (indica la polilinea due
  volte, vicino ai due segmenti). Il punto di ogni clic dice quale parte
  dell'oggetto tenere. Una polilinea con un'altra entità, gli archi di una
  polilinea e il testo non si raccordano.
- **`Shift`+clic sul secondo oggetto:** fa lo spigolo, come con raggio 0.
- ***Radius*** (`R`): `Specify fillet radius <0.0000>:`. Scrivi il raggio o
  indica due punti (`Specify second point:`). Un raggio negativo produce
  `Value must be positive.`; 0 fa lo spigolo. Dopo, il comando chiede di nuovo
  il primo oggetto.
- ***Trim*** (`T`): `Enter Trim mode option [Trim/No trim] <Trim>:`. L'impostazione
  è comune a `FILLET` e `CHAMFER`.
- ***Polyline*** (`P`): `Select 2D polyline or [Radius]:` raccorda tutti i
  vertici di una polilinea, uno dopo l'altro, e scrive `<n> lines were
  filleted`, poi `<m> were too short` se qualche vertice resta com'è (segmenti
  troppo corti per il raggio, segmenti curvi).
- ***Multiple*** (`M`): il comando continua dopo ogni raccordo, finché non premi
  `Invio`; ***Undo*** (`U`) annulla l'ultimo raccordo fatto nel comando.

Raggio e *Trim* restano per i prossimi `FILLET` della sessione.

### Il risultato

- **Con *Trim*** linee e archi si accorciano o si allungano fino ai punti di
  tangenza; i cerchi non si tagliano mai. Su due segmenti della stessa polilinea
  il raccordo diventa un segmento curvo della polilinea. Un'entità di cui non
  resta nulla, o resta un pezzo troppo piccolo per le unità del disegno, si
  cancella.
- **Con *No trim*** le entità non cambiano e si aggiunge solo l'arco, anche
  sulle polilinee e con *Polyline*: ogni arco è un'entità a sé.
- **L'arco** prende le proprietà del primo oggetto se i due hanno le stesse
  proprietà, altrimenti quelle correnti. Un arco troppo piccolo per le unità del
  disegno non si aggiunge: il comando fa lo spigolo.

Tutto il comando è un solo passo di annullamento (`Fillet`); `Esc` conserva i
raccordi già fatti.

| Messaggio | Quando |
|---|---|
| `Cannot fillet those objects.` | il raccordo non esiste, o gli oggetti non si raccordano |
| `Radius is too large.` | il raggio non sta nella parte da tenere |
| `Pick a point that is not on the other object.` | un punto indicato cade sulla retta o sul cerchio dell'altro oggetto |
| `Result is beyond the drawing limits.` | il raccordo esiste ma cade oltre i limiti che il programma rappresenta |

**Esempio:** con le linee da `0,0` a `30,0` e da `0,0` a `0,30`:

```
Command: f
Current settings: Mode = TRIM, Radius = 0.0000
Select first object or [Undo/Polyline/Radius/Trim/Multiple]: r
Specify fillet radius <0.0000>: 5
Select first object or [Undo/Polyline/Radius/Trim/Multiple]: 20,0
Select second object or shift-select to apply corner or [Radius]: 0,20
```

Le linee diventano da `5,0` a `30,0` e da `0,5` a `0,30`, e nasce l'arco di
centro `(5,5)` e raggio 5 che le unisce.

## CHAMFER — smussare

Unisce due linee con un tratto dritto, lo smusso, o con uno spigolo vivo.

- **Nome:** `CHAMFER`
- **Alias:** `CHA`

### Le domande e le opzioni

```
(TRIM mode) Current chamfer Dist1 = 0.0000, Dist2 = 0.0000
Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]:
Select second line or shift-select to apply corner or [Distance/Angle/Method]:
```

Con *No trim* il messaggio comincia con `(NOTRIM mode)`; con il metodo
dell'angolo mostra `Length = …, Angle = …`.

- **Gli oggetti:** due linee, oppure due segmenti dritti e consecutivi della
  stessa polilinea. Ogni punto indica la parte della linea da tenere.
- **`Shift`+clic sulla seconda linea:** fa lo spigolo, con le due distanze a 0.
- ***Distance*** (`D`): `Specify first chamfer distance <0.0000>:` e
  `Specify second chamfer distance <…>:`, che propone la prima. Le distanze si
  misurano dallo spigolo, la prima sulla prima linea. Sceglie il metodo delle
  distanze.
- ***Angle*** (`A`): `Specify chamfer length on the first line <0.0000>:` e
  `Specify chamfer angle from the first line <0>:`, in gradi. Sceglie il metodo
  dell'angolo.
- ***mEthod*** (`E`, alla seconda domanda `M`): `Enter trim method
  [Distance/Angle] <Distance>:` sceglie quale delle due coppie di valori usare.
- ***Trim*** (`T`), ***Polyline*** (`P`), ***Multiple*** (`M`) e ***Undo***
  (`U`): come in `FILLET`. *Polyline* chiede `Select 2D polyline or
  [Distance/Angle/Method]:` e scrive `<n> lines were chamfered`, poi `<m> were
  too short`.

Alle domande dei numeri vale solo un numero: altro testo produce
`Requires numeric distance.`, un valore negativo `Value must be positive.`, e
`Invio` conserva il valore tra `< >`. Dopo un'opzione torna la domanda da cui
l'hai scelta: alla seconda linea, la prima resta scelta. Distanze, angolo,
metodo e *Trim* restano per i prossimi `CHAMFER` della sessione.

### Il risultato

Lo spigolo è il punto in cui si incontrano le due rette prolungate. Gli estremi
dello smusso stanno alle distanze date dallo spigolo, ciascuno sulla parte della
sua linea indicata dal clic. Con *Trim* le linee si accorciano o si allungano
fino allo smusso; con *No trim* restano com'erano e si aggiunge solo lo smusso,
anche sulle polilinee. Le proprietà dello smusso, *Undo*, `Esc` e il passo di
annullamento (`Chamfer`) seguono le regole di `FILLET`.

Le distanze a zero, con il metodo delle distanze, non funzionano come il raggio
0 di `FILLET`:

- **tutte e due 0:** lo spigolo, senza linea nuova (come `Shift`+clic);
- **una sola 0:** nessuno smusso, compare `Lines are parallel.` e il disegno
  resta com'è, anche se l'altra distanza è troppo lunga, in qualunque ordine
  scegli le linee;
- una distanza uguale alla parte da tenere, con *Trim*, cancella quella parte;
  con *No trim* le linee restano com'erano e si aggiunge solo lo smusso. Una
  distanza maggiore dà `Chamfer distance too large.`.

| Messaggio | Quando |
|---|---|
| `Lines are parallel.` | le due rette non hanno un punto comune, oppure una sola delle due distanze è 0 |
| `Chamfer distance too large.` | una distanza, o la lunghezza, supera la parte da tenere della sua linea |
| `Cannot chamfer those objects.` | gli oggetti non sono linee o segmenti dritti consecutivi |
| `Pick a point that is not on the other object.` | un punto indicato cade sulla retta dell'altra linea |
| `Result is beyond the drawing limits.` | lo spigolo cade oltre i limiti che il programma rappresenta |

**Esempio:** con le linee da `0,0` a `30,0` e da `0,0` a `0,30`:

```
Command: cha
(TRIM mode) Current chamfer Dist1 = 0.0000, Dist2 = 0.0000
Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]: d
Specify first chamfer distance <0.0000>: 5
Specify second chamfer distance <5.0000>: 8
Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]: 20,0
Select second line or shift-select to apply corner or [Distance/Angle/Method]: 0,20
```

Le linee diventano da `5,0` a `30,0` e da `0,8` a `0,30`, e lo smusso va da
`5,0` a `0,8`.

## JOIN — unire

Unisce più entità in una sola.

- **Nome:** `JOIN`

### Le domande

```
Select source object or multiple objects to join at once:
```

Scegli le entità con i gesti della selezione o con la selezione scritta, poi
premi `Invio`. La **sorgente** è la prima scelta, e l'ordine in cui scegli le
altre conta. `Invio` senza aver scelto nulla conclude senza effetti, come `Esc`
in qualunque momento.

### Che cosa si unisce

- **Linee allineate** diventano una linea, anche con spazi tra loro.
- **Archi dello stesso cerchio** diventano un arco, anche con spazi: l'arco va
  dall'inizio della sorgente, in senso antiorario, fino al più lontano dei loro
  estremi. Se gli archi coprono il giro intero, il comando chiede
  `Arc segments combined form a circle. Convert to circle? [Yes/No] <Yes>:`:
  *Yes* o `Invio` danno un cerchio, *No* lascia il disegno com'era.
- Linea e arco nascono così solo se **tutte** le entità scelte sono linee
  allineate alla sorgente, o archi del suo cerchio. Altrimenti linee, archi e
  polilinee **che si toccano agli estremi** diventano una **polilinea**.
- La polilinea è una catena: parte dalla sorgente, cresce prima dalla sua fine,
  poi dal suo inizio, e a ogni passo aggiunge la prima entità, nell'ordine di
  scelta, che ha un estremo sull'estremo libero della catena. Si ferma quando
  nessuna si aggiunge o quando la catena si chiude. Le entità che non entrano
  nella catena, e le polilinee chiuse diverse dalla sorgente, si scartano.

Quando due estremi **coincidono**, quando due linee sono **allineate** e quando
due archi stanno sullo **stesso cerchio** lo decidono le unità del disegno
(`INSUNITS`): la tolleranza è quella del nucleo geometrico per quell'unità, e
cambiare `INSUNITS` cambia le decisioni dei `JOIN` successivi, non le entità già
disegnate. Due estremi che coincidono diventano un vertice, nel punto
dell'entità che segue.

Se il risultato è dello stesso tipo della sorgente (una linea, un arco, la
polilinea sorgente allungata), la sorgente resta la stessa entità; altrimenti
nasce un'entità nuova con le sue proprietà. Il comando è un passo di
annullamento (`Join`).

| Messaggio | Risultato |
|---|---|
| `<n> lines joined into 1 line` | linee allineate unite |
| `<n> arcs joined into 1 arc` | archi dello stesso cerchio uniti |
| `<n> arcs joined and converted to circle` | archi uniti in un cerchio |
| `<n> objects converted to 1 polyline` | una polilinea nuova |
| `<n> segments added to polyline` | la polilinea sorgente è cresciuta (`1 segment added to polyline` per un segmento) |
| `0 objects joined, <n> objects discarded from the operation.` | niente si unisce |

Con entità scartate il messaggio continua con `, <n> objects discarded from the
operation.` (`1 object …` per una).

**Esempio:** due linee da `0,0` a `10,0` e da `20,0` a `30,0`:

```
Command: join
Select source object or multiple objects to join at once: 5,0
1 found, 1 total
Select source object or multiple objects to join at once: 25,0
1 found, 2 total
Select source object or multiple objects to join at once:
2 lines joined into 1 line
```

danno la linea da `0,0` a `30,0`. Con una linea da `0,0` a `10,0` e una da
`10,0` a `10,10` il messaggio è invece `2 objects converted to 1 polyline`.

## POLARANG — incremento del tracciamento polare

Imposta l'incremento angolare per il tracciamento polare (`Polar Tracking`), in gradi.

- **Nome:** `POLARANG`

### Funzionamento

```
Command: polarang
Enter new value for POLARANG <90>:
```

- Digita un valore in gradi strettamente maggiore di 0 e minore o uguale a 180 (ad esempio
  `45`, `30`, `15` o `22.5`).
- Valori $\le 0$ o $> 180$ danno l'errore `Value must be between 0 and 180.` e la domanda resta.
- Premendo `Invio` a vuoto si conferma l'angolo corrente indicato tra parentesi angolari.

Quando il tracciamento polare è attivo (`F10` o pulsante **POLAR** nella barra di stato),
il cursore si aggancia a tutte le semirette che formano angoli multipli di questo valore
a partire dal punto base del comando (0°, 45°, 90°, 135°, 180°, ecc.).

**Esempio:** imposta l'incremento polare a 45 gradi:

```
Command: polarang
Enter new value for POLARANG <90>: 45
```

## Layer: -LAYER e CLAYER

I layer permettono di organizzare il disegno in livelli logici, assegnando a ciascun
livello colore, tipo di linea, spessore e visibilità. Il layer `0` è presente in ogni
disegno: non può essere rinominato né cancellato.

La gestione da riga di comando si effettua con `-LAYER` (alias `-LA`), mentre per
impostare direttamente il layer corrente si può usare `CLAYER`. Per il pannello
interattivo vedi il [manuale dell'interfaccia](interfaccia.md#il-pannello-dei-layer-layer).

### `-LAYER` — gestire i layer da tastiera

All'avvio il comando mostra il layer corrente e propone il menu delle opzioni:

```
Command: -la
Current layer:  "0"
Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/Plot/Freeze/Thaw/LOck/Unlock]:
```

Premi `Invio` a riga vuota per concludere il comando.

| Opzione | Tasto | Domande | Effetto |
|---|---|---|---|
| `?` | `?` | `Enter layer name(s) to list <*>:` | elenca i layer con stato, colore, tipo di linea, spessore e preferenza di stampa |
| `Make` | `M` | `Enter name for new layer (becomes the current layer) <0>:` | crea il layer se non esiste e lo rende corrente |
| `Set` | `S` | `Enter layer name to make current:` | rende corrente un layer esistente e non congelato |
| `New` | `N` | `Enter name list for new layer(s):` | crea uno o più layer separati da virgola |
| `Rename` | `R` | `Enter old layer name:` poi `Enter new layer name:` | rinomina un layer (eccetto il layer `0`) |
| `ON` / `OFF` | `ON` / `OFF` | `Enter name list of layer(s) to turn on/off:` | accende o spegne i layer specificati |
| `Freeze` / `Thaw` | `F` / `T` | `Enter name list of layer(s) to freeze/thaw:` | congela o scongela i layer specificati |
| `LOck` / `Unlock` | `LO` / `U` | `Enter name list of layer(s) to lock/unlock:` | blocca o sblocca i layer specificati |
| `Color` | `C` | `New color [Truecolor]:` poi `Enter name list of layer(s) for color <colore> <layer>:` | cambia il colore dei layer |
| `Ltype` | `LT` | `Enter a loaded linetype name <Continuous>:` poi `Enter name list of layer(s) for linetype "<tipo>" <layer>:` | cambia il tipo di linea dei layer |
| `LWeight` | `LW` | `Enter lineweight (0.0mm - 2.11mm):` poi `Enter name list of layer(s) for lineweight <spessore> <layer>:` | cambia lo spessore dei layer |
| `Plot` | `P` | `Enter a plotting preference [Plot/No plot] <Plot>:` poi `Enter name list of layer(s) for this plot preference <layer>:` | imposta la preferenza di stampa |

#### Elenchi di nomi e caratteri jolly

Nelle domande che accettano elenchi (`name list`), puoi indicare più layer separati
da virgola (es. `Muri,Porte,Finestre`). Sono supportati i caratteri jolly:
- `*`: corrisponde a qualsiasi sequenza di caratteri (es. `Mur*` trova `Muri`, `Muratura`);
- `?`: corrisponde a un singolo carattere (es. `Piano?` trova `Piano1`, `Piano2`).

I nomi non distinguono maiuscole e minuscole. Se nessun layer corrisponde ai criteri, il
programma risponde: `No matching layer names found.`

#### Regole e vincoli dei layer

- **Nomi validi:** non possono essere vuoti, superare 255 caratteri, né contenere caratteri
  non consentiti (`< > / \ " : ; ? * | , = `). In caso contrario: `Invalid layer name.`
- **Layer già esistente:** in `New` o `Rename`, se il nome esiste già compare `Layer "<nome>" already exists.`
- **Layer `0`:** non può essere rinominato (`Cannot rename layer "0".`) né eliminato.
- **Layer corrente:**
  - non può essere congelato (`Cannot freeze layer "<nome>".  It is the CURRENT layer.`);
  - se si tenta di spegnerlo, il comando chiede conferma:
    `Really want layer <nome> (the CURRENT layer) off? [Yes/No] <No>:`
  - non è possibile rendere corrente un layer congelato (`Cannot set current to a frozen layer.`).
- **Transazione unica:** l'intero comando `-LAYER` costituisce un unico passo di annullamento.
  Se interrotto con `Esc`, le operazioni già completate e confermate vengono conservate.

**Esempio di creazione e impostazione di layer:**

```
Command: -la
Current layer:  "0"
Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/Plot/Freeze/Thaw/LOck/Unlock]: m
Enter name for new layer (becomes the current layer) <0>: Muri
Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/Plot/Freeze/Thaw/LOck/Unlock]: c
New color [Truecolor]: 1
Enter name list of layer(s) for color 1 (red) <Muri>: Muri
Enter an option [?/Make/Set/New/Rename/ON/OFF/Color/Ltype/LWeight/Plot/Freeze/Thaw/LOck/Unlock]:
```

### `CLAYER` — impostare il layer corrente

Il comando `CLAYER` imposta direttamente il layer di lavoro corrente:

```
Command: clayer
Enter new value for CLAYER <"0">: Muri
```

Il layer specificato deve esistere e non essere congelato. Se il layer non esiste compare
`Layer "<nome>" not found.`; se è congelato compare `Cannot set current to a frozen layer.`.
In entrambi i casi la richiesta viene ripetuta.

## Layer bloccati, spenti e congelati

Ogni layer può trovarsi in diversi stati di visibilità e protezione, con effetti precisi
sul disegno, sulla selezione e sulle modifiche:

| Stato | Disegnato a video | Selezionabile con clic o finestra | Snap a oggetto | Modificabile |
|---|---|---|---|---|
| **Acceso** | sì | sì | sì | sì |
| **Spento** | no | no | no | no |
| **Congelato** | no | no | no | no |
| **Bloccato** | sì | no (escluso dai comandi) | sì | no |

### Differenze principali

- **Spento e Congelato:** le entità appartenenti a questi layer non vengono disegnate
  nella vista, non rispondono agli snap e non possono essere selezionate né modificate.
- **Bloccato:** le entità sono regolarmente visibili e i loro punti notevoli (estremi,
  punti medi, centri) possono essere agganciati con gli snap a oggetto. Tuttavia, le
  entità sono protette da modifiche accidentali: i gesti di selezione le escludono
  automaticamente e comandi come `ERASE` o `CHPROP` non possono alterarle.
- **Disegnare sul layer bloccato:** se il layer corrente è bloccato, è consentito
  tracciare nuove entità (es. linee, cerchi, polilinee); le nuove entità create
  apparterranno al layer bloccato e non potranno essere modificate successivamente
  senza aver prima sbloccato il layer.

Durante la selezione con `Select objects:`, se un rettangolo o un clic interseca oggetti
appartenenti a layer bloccati, il programma li scarta e informa l'utente con il messaggio:
`1 was on a locked layer.` oppure `<n> were on a locked layer.`

## Colore: COLOR

Il comando `COLOR` (alias `COL`, `COLOUR`, `-COLOR`) definisce il colore assegnato alle
nuove entità create successivamente:

```
Command: color
Enter default object color [Truecolor] <BYLAYER>:
```

### Valori accettati

- **Nomi standard:** `red` (1), `yellow` (2), `green` (3), `cyan` (4), `blue` (5), `magenta` (6), `white` (7).
- **Indici numerici ACI:** un intero compreso tra `1` e `255`.
- **Truecolor (RGB):** scrivi `Truecolor` (o `T`), poi alla richiesta `Red, Green, Blue:`
  scrivi tre valori da 0 a 255 separati da virgola (es. `255,128,0`).
- **Logici:**
  - `ByLayer`: l'entità eredita automaticamente il colore impostato sul suo layer di appartenenza;
  - `ByBlock`: l'entità assume il colore del blocco in cui è inserita (o il bianco se non fa parte di un blocco).

Se viene digitato un colore non riconosciuto, compare il messaggio `Invalid color.` e la
domanda viene ripetuta. Premere `Invio` a riga vuota conferma il valore corrente tra `< >`.

## Tipi di linea: -LINETYPE

Il comando `-LINETYPE` (alias `LINETYPE`, `LT`, `LTYPE`, `-LT`) permette di caricare
definizioni di tipi di linea standard nel disegno e di impostare il tipo di linea corrente:

```
Command: -lt
Current line type: "ByLayer"
Enter an option [?/Load/Set]:
```

Premi `Invio` a riga vuota per uscire.

### Opzioni disponibili

- **`?`**: elenca tutti i tipi di linea attualmente caricati nel documento, visualizzando
  per ciascuno il nome e la descrizione grafica testuale (es. `"Continuous" Solid line`).
- **`Load` (o `L`)**:
  ```
  Enter linetype(s) to load: ACAD_ISO02W100
  ```
  Carica uno o più tipi standard ISO (es. `ACAD_ISO02W100`, `ACAD_ISO03W100`, `ACAD_ISO10W100`).
  Se il tipo è già presente nel disegno, il programma risponde: `Linetype "<nome>" is already loaded.`
  Se il nome non corrisponde a un tipo noto, risponde: `Linetype "<nome>" not found.`
- **`Set` (o `S`)**:
  ```
  Specify linetype name or [?] <ByLayer>: ACAD_ISO02W100
  ```
  Imposta il tipo di linea per le nuove entità. Può essere qualsiasi tipo precedentemente
  caricato, oppure `ByLayer` o `ByBlock`. Se il tipo non è caricato, il programma risponde:
  `Linetype "<nome>" is not loaded.`

## Scala dei tipi di linea: LTSCALE e CELTSCALE

I tratti e gli spazi dei tipi di linea non continui vengono moltiplicati per fattori di
scala per adattarsi alla dimensione e all'unità del disegno:

- **`LTSCALE` (alias `LTS`)**: imposta il fattore di scala **globale** per l'intero disegno.
  Modifica la spaziatura dei tratti di tutte le entità che usano tipi tratteggiati:
  ```
  Command: ltscale
  Enter new linetype scale factor <1.0000>: 2.0
  ```
- **`CELTSCALE`**: imposta il fattore di scala del tipo di linea per i **nuovi oggetti** creati.
  La scala effettiva di ciascuna entità sarà data dal prodotto tra il proprio `CELTSCALE` e
  il fattore globale `LTSCALE`:
  ```
  Command: celtscale
  Enter new value for CELTSCALE <1.0000>: 0.5
  ```

Entrambi i comandi richiedono un numero reale strettamente positivo (> 0). In caso di valore
negativo o zero compare `Requires a positive number.` e la richiesta viene ripetuta.

## Spessore delle linee: LWEIGHT e LWDISPLAY

Lo spessore delle linee determina lo spessore con cui gli oggetti vengono visualizzati e
stampati.

### `LWEIGHT` — spessore corrente

Il comando `LWEIGHT` (alias `LW`, `LINEWEIGHT`, `-LWEIGHT`) imposta lo spessore per le
nuove entità:

```
Command: lw
Current lineweight: ByLayer
Enter default lineweight for new objects or [?]: 0.30
```

- **Valori ammessi:** valori metrici standard in millimetri (`0.00`, `0.05`, `0.09`, `0.13`,
  `0.15`, `0.18`, `0.20`, `0.25`, `0.30`, `0.35`, `0.40`, `0.50`, `0.53`, `0.60`, `0.70`,
  `0.80`, `0.90`, `1.00`, `1.06`, `1.20`, `1.40`, `1.58`, `2.00`, `2.11`), oppure `ByLayer`,
  `ByBlock`, `Default`.
- **Opzione `?`**: stampa l'elenco completo dei valori ammessi in millimetri.
- In caso di valore non consentito compare: `Invalid lineweight.`

### `LWDISPLAY` — visualizzazione a schermo

Controlla se gli spessori delle linee devono essere visualizzati graficamente nella vista:

```
Command: lwdisplay
Enter new value for LWDISPLAY <OFF>: ON
```

- Accetta solo `ON` oppure `OFF`. Con altri valori risponde: `Requires ON or OFF.`
- La modifica ha effetto immediato sulla vista senza richiedere rigenerazione.
- `LWDISPLAY` non altera il documento e non produce passi di annullamento nello storico.

## Modificare le proprietà: CHPROP

Il comando `CHPROP` permette di cambiare le proprietà di una o più entità già disegnate.
Richiede prima la selezione degli oggetti:

```
Command: chprop
Select objects: (seleziona gli oggetti con clic o finestra)
Select objects: (conferma con Invio)
Enter property to change [Color/LAyer/LType/ltScale/LWeight]:
```

Premi `Invio` a riga vuota alla domanda principale per terminare il comando.

### Proprietà modificabili

- **`Color` (`C`)**:
  `New color [Truecolor] <{default}>:`
  Inserisci un nuovo colore per le entità selezionate.
- **`LAyer` (`LA`)**:
  `Enter new layer name <{default}>:`
  Sposta le entità selezionate sul layer specificato. Se il layer non esiste compare
  `Layer "<nome>" not found.`. È ammesso assegnare un layer congelato (in tal caso le
  entità diventano invisibili).
- **`LType` (`LT`)**:
  `Enter new linetype name <{default}>:`
  Assegna un nuovo tipo di linea alle entità. Il tipo deve essere caricato nel disegno;
  altrimenti compare `Linetype "<nome>" is not loaded.`.
- **`ltScale` (`LTS`)**:
  `Specify new linetype scale <{default}>:`
  Modifica la scala individuale del tipo di linea delle entità (> 0).
- **`LWeight` (`LW`)**:
  `Enter new lineweight <{default}>:`
  Assegna un nuovo spessore di linea alle entità.

### Valore comune e `varies`

Se tutte le entità selezionate hanno lo stesso valore per la proprietà richiesta, questo
compare tra le parentesi angolari `< >` come valore predefinito. Se invece le entità
hanno valori differenti, compare `<varies>`. Premere `Invio` a riga vuota conserva i valori
attuali di ogni entità senza modificarli.

Tutte le modifiche apportate durante la sessione di `CHPROP` vengono raggruppate in un'unica
transazione `CHPROP`: premendo `Esc` si esce dal comando conservando i cambi già confermati.

## Comandi dell'applicazione

Nell'applicazione desktop sono disponibili comandi dedicati alla gestione dell'interfaccia
grafica e dei pannelli:

| Comando | Alias | Descrizione |
|---|---|---|
| `RIBBON` | — | Mostra l'interfaccia a barra multifunzione (*ribbon*) |
| `RIBBONCLOSE` | — | Mostra l'interfaccia classica a menu e barre degli strumenti |
| `LAYER` | `LA` | Apre e porta in primo piano il pannello dei layer |
| `PROPERTIES` | `PR`, `CH`, `MO`, `PROPS` | Apre e porta in primo piano il pannello delle proprietà |
| `QSELECT` | — | Apre la finestra di dialogo per la selezione rapida degli oggetti |

Per i dettagli sul funzionamento di finestre, pannelli, temi e scorciatoie, consulta il
[manuale dell'interfaccia](interfaccia.md).

## Negli script

Dalla tappa 0.7 ([ADR 0032](../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)),
quando esegue uno script `.scr` di AutoCAD o uno script `.tgs` con la direttiva `!scr on`,
Tecnigrafo adotta per alcuni comandi le sequenze storiche di AutoCAD, diverse da quelle
della sessione interattiva:

- nei file `.scr` (riconosciuti automaticamente dall'estensione);
- nei file `.tgs`, a partire dalla direttiva `!scr on` fino a `!scr off`. Senza questa
  direttiva, un file `.tgs` riproduce fedelmente l'interazione umana e usa la sequenza
  interattiva normale.

Comandi con sequenza dedicata negli script:

| Comando | Sequenza interattiva | Negli script |
|---|---|---|
| `TEXT` | Chiede una riga dopo l'altra fino a `Invio` su riga vuota | Inserisce una riga sola, poi conclude automaticamente |
| `COPY` | Chiede copie multiple fino a `Exit` o `Invio` a vuoto | Effettua una sola copia, poi conclude automaticamente |
| `TRIM`, `EXTEND` | Parte nel modo di `TRIMEXTENDMODE`, rapido all'avvio | Sempre nel modo standard, anche dopo l'opzione *mOde*: prima i bordi (`Invio` li sceglie tutti), poi gli oggetti |
| `FILLET` | Dopo *Radius* o *Trim* torna la domanda del primo oggetto | Dopo *Radius* o *Trim* il comando conclude |
| `CHAMFER` | Dopo *Distance*, *Angle*, *Trim* o *mEthod* torna la domanda da cui l'hai scelta | Dopo quelle opzioni il comando conclude |
| `JOIN` | Più oggetti insieme, con una sola selezione | Prima l'oggetto sorgente, poi gli oggetti da unire, secondo il tipo della sorgente |

Nella sequenza degli script `JOIN` chiede `Select source object:` (un oggetto;
`Invio` conclude senza effetti), poi, secondo la sorgente:

- **una linea:** `Select lines to join to source:`, una selezione; accetta le
  linee allineate. Con una sola linea unita il messaggio è
  `1 line joined to source`;
- **un arco:** `Select arcs to join to source or [cLose]:`, un arco per volta,
  finché non premi `Invio`; accetta gli archi dello stesso cerchio. L'opzione
  *cLose* (`L`) chiude l'arco in un cerchio senza domande e scrive
  `Arc converted to circle.`, oppure `<n> arcs joined and converted to circle`
  se ha unito anche altri archi. Senza *cLose*, se gli archi coprono il giro
  intero, compare la stessa domanda della conversione in cerchio della sessione
  interattiva;
- **una polilinea:** `Select objects to join to source:`, una selezione; accetta
  linee, archi e polilinee che si toccano agli estremi.

## Annullare e ripetere

Ogni comando di disegno concluso, ogni gruppo cancellato con `ERASE`, ogni
modifica geometrica (`MOVE`, `COPY`, `ROTATE`, `SCALE`, `MIRROR`, `EXPLODE`, `TRIM`,
`EXTEND`, `OFFSET`, `FILLET`, `CHAMFER`, `JOIN` o stiramento con grip), ogni modifica ai layer con `-LAYER` o `CLAYER` e ogni modifica alle proprietà con
`COLOR`, `-LINETYPE`, `LTSCALE`, `CELTSCALE`, `LWEIGHT` o `CHPROP` sono un
**passo** che si può annullare e poi ripetere. Le linee di un `LINE` e le righe
di un `TEXT` sono un solo passo per comando, anche se conservate con `Esc`; lo
stesso vale per i tagli di un `TRIM`, le copie di un `OFFSET` e i raccordi di un
`FILLET` con *Multiple*.

| Azione | Menu | Tastiera | Riga di comando |
|---|---|---|---|
| Annulla | *Edit → Undo* | `Ctrl+Z` (`Cmd+Z` su macOS) | `U` o `UNDO` |
| Ripeti | *Edit → Redo* | `Ctrl+Y` (`Cmd+Y` su macOS), oppure la combinazione standard del sistema, per esempio `Ctrl+Shift+Z` su Linux e `Cmd+Shift+Z` su macOS | `REDO` |

- Le voci del menu dicono che cosa annullano o ripetono, per esempio
  *Undo Polyline* o *Undo Erase*, e sono disattivate quando non c'è nulla da fare.
- Lo storico conferma ogni passo, per esempio `Undone: Polyline.` o `Redone: Erase.`;
  se non c'è nulla, `Nothing to undo.` o `Nothing to redo.`
- Le entità ripristinate sono identiche a quelle annullate, anche dopo una
  cancellazione di più entità in un solo passo.
- `Ctrl+Z` e `Ctrl+Y` agiscono sul disegno anche quando stai scrivendo nella riga
  di comando: non modificano il testo nel campo.
- `Ctrl+Z` dal menu o dalla tastiera equivale a scrivere `UNDO`: un `Invio` a riga
  vuota subito dopo annulla un altro passo.

### Durante un comando

Mentre un comando è in corso, annulla e ripeti sono **disattivati**: concludi il
comando o premi `Esc` prima. Scrivere `U` dentro `PLINE` non annulla il disegno ma
sceglie l'opzione *Undo* del comando, che toglie l'ultimo punto della polilinea in
corso.

### Che cosa conserva Esc

| Comando interrotto | Nel disegno resta |
|---|---|
| `LINE` | i segmenti già disegnati; niente con un solo punto |
| `PLINE` | la polilinea aperta con i vertici già indicati, se almeno due |
| `TEXT` | le righe già confermate con `Invio` |
| `CIRCLE`, `ARC` | niente: prima dell’ultima risposta l’entità non è completa, anche con *Ttr* |
| `ERASE` | nessuna cancellazione; la selezione viene svuotata |
| `MOVE`, `ROTATE`, `SCALE`, `MIRROR` | nessuna modifica; le entità restano invariate |
| `COPY` | le copie già create prima dell'interruzione |
| `TRIM`, `EXTEND` | i tagli e i prolungamenti già fatti |
| `OFFSET` | le copie già create |
| `FILLET`, `CHAMFER` | i raccordi e gli smussi già fatti |
| `JOIN` | niente: nessuna unione |
| `EXPLODE` | nessuna scomposizione |
| `-LAYER` | le modifiche e creazioni di layer già completate prima dell'interruzione |
| `CHPROP` | i cambi di proprietà già completati prima dell'interruzione |

Lo storico scrive `*Cancel*` anche quando resta lavoro completato. Un solo `U`
a riposo lo toglie e `REDO` lo ripristina. Con un rettangolo di selezione aperto
`Esc` chiude prima quello, come descritto nella sezione sulla selezione.

### Che cosa non si annulla

- Il disegno di esempio che compare all'avvio non fa parte della cronologia e non
  si può annullare.
- Un comando senza entità completate non lascia alcun passo: per esempio una
  polilinea interrotta con meno di 2 punti o un `TEXT` senza righe confermate.
- Una nuova modifica confermata, come una polilinea o una cancellazione, elimina
  i passi che si potevano ripetere. Un comando senza effetti conserva il redo.
- `LWDISPLAY` e la visualizzazione dello spessore delle linee (`LWT`), la
  selezione, lo zoom, il pan e le impostazioni di snap, orto e polare non sono passi
  di annullamento. `U`, `UNDO` e `REDO` svuotano la selezione corrente.

## Messaggi

| Messaggio | Significato |
|---|---|
| `Unknown command: <nome>.` | non esiste un comando con quel nome o alias |
| `Invalid point or option.` | la scrittura non è una coordinata valida né un'opzione della domanda, oppure manca un dato (nessun ultimo punto, nessuna direzione del puntatore): la domanda resta la stessa |
| `Requires a distance, a point or an option.` | la richiesta vuole una distanza, un punto o un’opzione valida |
| `Requires an angle, a point or an option.` | la richiesta vuole un angolo, un punto o un’opzione valida |
| `Value must be positive and nonzero.` | raggio, diametro, altezza o fattore di scala non valido: indica un valore positivo |
| `The three points are collinear.` | impossibile costruire il cerchio o l’arco: cambia il terzo punto |
| `Center coincides with the start point.` | per l’arco il centro deve essere diverso dall’inizio |
| `End point gives a zero-length arc.` | la fine non definisce un arco valido |
| `Invalid end point of arc.` | il punto è dietro la tangente nel modo arco di `PLINE` |
| `Select objects:` | scegli le entità per il comando, poi conferma a campo vuoto |
| `Specify opposite corner:` | un rettangolo di selezione è aperto: indica il secondo angolo |
| `<n> found, <t> total` / `<n> removed, <t> total` | entità trovate o rimosse dal gesto e totale della selezione durante `Select objects:` |
| `Invalid selection.` | durante la selezione è stato scritto un testo che non è un punto, `W`, `C`, `ALL` né un'opzione della domanda |
| `Specify first corner:` | dopo `W` o `C` scritti in una selezione: indica il primo angolo |
| `Point coincides with the previous point.` | il punto indicato coincide con il vertice precedente ed è stato ignorato |
| `*Cancel*` | il comando è stato interrotto con `Esc`; il lavoro completato resta secondo la tabella sopra |
| `Undone: <passo>.` / `Redone: <passo>.` | passo annullato o ripetuto |
| `Nothing to undo.` / `Nothing to redo.` | non ci sono passi da annullare o da ripetere |
| `<Osnap on>` / `<Osnap off>` | snap a oggetto attivato o spento |
| `<Ortho on>` / `<Ortho off>` | modalità orto attivata o spenta |
| `<Polar on>` / `<Polar off>` | tracciamento polare attivato o spento |
| `Current settings:  Copy mode = Multiple` | impostazione iniziale di `COPY`: copia multipla attiva fino a conclusione |
| `Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0` | convenzione degli angoli all'avvio di `ROTATE`: antiorari da +X |
| `Rotating a copy of the selected objects.` | opzione *Copy* di `ROTATE`: ruota una copia delle entità selezionate |
| `Scaling a copy of the selected objects.` | opzione *Copy* di `SCALE`: scala una copia delle entità selezionate |
| `Exploding this polyline has lost width information.` | avviso di `EXPLODE`: le larghezze della polilinea sono state rimosse |
| `The UNDO command will restore it.` | avviso di `EXPLODE`: `UNDO` può ripristinare la polilinea integra |
| `1 was not able to be exploded.` / `<n> were not able to be exploded.` | entità non scomponibili scartate da `EXPLODE` |
| `Requires an integer between 0 and 1.` | valore non valido per `MIRRTEXT` o `TRIMEXTENDMODE`: inserisci 0 o 1 |
| `Value must be between 0 and 180.` | valore non valido per `POLARANG`: inserisci un angolo positivo fino a 180° |
| `** STRETCH **` | avvio della modifica interattiva di un'entità tramite grip caldo |
| `Invalid grip edit.` | modifica con il grip non valida (es. arco degenere o raggio nullo): entità non modificata |
| `Invalid layer name.` | il nome del layer contiene caratteri non consentiti o supera 255 caratteri |
| `Layer "<nome>" already exists.` | esiste già un layer con questo nome nel disegno |
| `Layer "<nome>" not found.` | il layer specificato non esiste nel documento |
| `Cannot rename layer "0".` | il layer `0` è protetto e non può essere rinominato |
| `Cannot freeze layer "<nome>".  It is the CURRENT layer.` | il layer corrente non può essere congelato |
| `Cannot set current to a frozen layer.` | non è possibile impostare come corrente un layer congelato |
| `No matching layer names found.` | nessun layer del disegno corrisponde all'elenco o al criterio jolly inserito |
| `1 was on a locked layer.` / `<n> were on a locked layer.` | entità scartate durante la selezione perché appartenenti a layer bloccati |
| `Invalid color.` | il valore inserito non è un colore valido (nome, indice 1-255, terna RGB o ByLayer/ByBlock) |
| `Linetype "<nome>" is already loaded.` | il tipo di linea è già presente nel disegno |
| `Linetype "<nome>" not found.` | il tipo di linea indicato non è disponibile tra i tipi standard |
| `Linetype "<nome>" is not loaded.` | il tipo di linea non è ancora stato caricato nel disegno |
| `Requires a positive number.` | il fattore di scala richiede un valore numerico strettamente positivo |
| `Invalid lineweight.` | lo spessore inserito non è tra i valori consentiti |
| `Requires ON or OFF.` | l'opzione accetta esclusivamente `ON` o `OFF` |
| `Requires a number.` | una domanda di numero, come il fattore di `SCALE`, vuole un numero o un punto da cui il comando misura |
| `Current settings: Trim mode = Quick, Projection=UCS, Edge=None` | avvio di `TRIM` o `EXTEND` nel modo rapido |
| `Current settings: Projection=UCS, Edge=None, Mode=Standard` | avvio di `TRIM` o `EXTEND` nel modo standard |
| `Select cutting edges ...` / `Select boundary edges ...` | modo standard di `TRIM` / `EXTEND`: segue la domanda dei bordi |
| `Object does not intersect an edge.` | `TRIM` nel modo standard: l'oggetto non incontra bordi e resta |
| `Cannot trim that object.` / `Cannot extend that object.` | il taglio o il prolungamento non riesce; l'oggetto resta |
| `Current settings: Erase source=No  Layer=Source  OFFSETGAPTYPE=0` | impostazioni di `OFFSET` all'avvio |
| `Cannot offset that object.` | `OFFSET` non può creare la copia: nessuna copia |
| `Current settings: Mode = TRIM, Radius = 0.0000` | impostazioni di `FILLET` all'avvio |
| `(TRIM mode) Current chamfer Dist1 = 0.0000, Dist2 = 0.0000` | impostazioni di `CHAMFER` all'avvio |
| `Cannot fillet those objects.` / `Cannot chamfer those objects.` | il raccordo o lo smusso non si può fare su quegli oggetti |
| `Radius is too large.` | `FILLET`: il raggio non sta nella parte da tenere |
| `Lines are parallel.` | `CHAMFER`: le due rette non hanno un punto comune, oppure una sola distanza è 0 |
| `Chamfer distance too large.` | `CHAMFER`: una distanza supera la parte da tenere |
| `Pick a point that is not on the other object.` | `FILLET`, `CHAMFER`: il punto cade sulla retta o sul cerchio dell'altro oggetto |
| `Result is beyond the drawing limits.` | `FILLET`, `CHAMFER`, `CIRCLE` *Ttr*: il risultato cade oltre i limiti che il programma rappresenta |
| `Circle does not exist.` | `CIRCLE` *Ttr*: il nucleo non dà un cerchio da disegnare (nessuno, infiniti, un testo, un raggio troppo grande) |
| `Value must be positive.` | raggio di `FILLET` o distanza di `CHAMFER` negativi |
| `Requires numeric distance.` | alle domande dei numeri di `CHAMFER` va scritto un numero |
| `<n> lines were filleted` / `<n> lines were chamfered` / `<m> were too short` | esito dell'opzione *Polyline* di `FILLET` e `CHAMFER` |
| `<n> lines joined into 1 line`, `<n> arcs joined into 1 arc`, `<n> objects converted to 1 polyline`, `<n> segments added to polyline` | esito di `JOIN` |
| `<n> arcs joined and converted to circle` / `Arc converted to circle.` | `JOIN`: gli archi sono diventati un cerchio |
| `1 line joined to source` | `JOIN` negli script: una linea unita alla sorgente |
| `0 objects joined, <n> objects discarded from the operation.` | `JOIN`: niente si unisce |
