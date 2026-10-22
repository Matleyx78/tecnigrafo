# Manuale dei comandi

Questo manuale descrive gli strumenti disponibili nella tappa 0.4: la riga di
comando, i punti, `LINE`, `CIRCLE`, `ARC`, `PLINE`, `TEXT`, la selezione,
`ERASE`, gli snap a oggetto, la modalità orto e l’annullamento. Lo stato della
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
- Le opzioni scritte `W`, `C`, `ALL`, `L`, `P` e il recinto non sono disponibili:
  altro testo durante la domanda produce `Invalid selection.`. Scegli con il
  mouse, poi conferma a campo vuoto.

La selezione si svuota alla fine dei comandi e con `U`, `UNDO` e `REDO`. Avviare
un comando di disegno svuota la selezione precedente; `ERASE` invece la usa subito.

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

### Attivare e scegliere i modi

- `F3`, *Tools → Object Snap* e il pulsante **OSNAP** nella barra di stato
  attivano o spengono insieme lo snap. Lo storico mostra `<Osnap on>` oppure
  `<Osnap off>`.
- In *Tools → Object Snap Modes* puoi attivare o disattivare separatamente
  *Endpoint*, *Midpoint*, *Center* e *Insertion*, anche quando OSNAP è spento. Spegnere `F3`
  conserva i modi scelti. Se tutti i modi sono disattivati, non c'è alcun aggancio
  anche con OSNAP acceso.
- All'avvio lo snap è acceso con tutti e quattro i modi. Le impostazioni non si
  conservano alla chiusura del programma.

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

### Snap, orto e coordinate scritte

| Come indichi il punto | Effetto |
|---|---|
| clic con uno snap disponibile | usa il punto di snap, anche fuori dall'asse dell'orto: **lo snap prevale sull'orto** |
| clic senza snap, con Ortho, dal secondo punto del comando | allinea il punto in orizzontale o verticale |
| coordinate `x,y`, `@dx,dy`, `d<a`, `@d<a` | usa le coordinate esatte, senza correggerle con snap o orto |
| sola distanza `d` | usa quella lunghezza dall'ultimo punto, nella direzione del puntatore già corretto da snap e orto |

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

`Invio` al primo punto termina: continuare dall’ultimo segmento con `Invio`
non è ancora disponibile. Un punto coincidente viene ignorato con
`Point coincides with the previous point.`.

## CIRCLE — cerchio

- **Nome:** `CIRCLE`
- **Alias:** `C`

Alla domanda `Specify center point for circle or [3P/2P]:` scegli il metodo:

| Metodo | Risposte |
|---|---|
| centro e raggio | indica il centro, poi scrivi il raggio o indica un punto sul cerchio |
| centro e diametro | indica il centro, scrivi `D` (*Diameter*), poi il diametro come numero o distanza dal centro a un punto |
| due punti | scrivi `2P`, poi i due estremi del diametro |
| tre punti | scrivi `3P`, poi tre punti sulla circonferenza |

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

## Annullare e ripetere

Ogni comando di disegno concluso e ogni gruppo cancellato con `ERASE` sono un
**passo** che si può annullare e poi ripetere. Le linee di un `LINE` e le righe
di un `TEXT` sono un solo passo per comando, anche se conservate con `Esc`.

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
| `CIRCLE`, `ARC` | niente: prima dell’ultima risposta l’entità non è completa |
| `ERASE` | nessuna cancellazione; la selezione viene svuotata |

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
- La selezione, lo zoom, il pan e le impostazioni di snap e orto non sono passi
  di annullamento. `U`, `UNDO` e `REDO` svuotano la selezione corrente.

## Messaggi

| Messaggio | Significato |
|---|---|
| `Unknown command: <nome>.` | non esiste un comando con quel nome o alias |
| `Invalid point or option.` | la scrittura non è una coordinata valida né un'opzione della domanda, oppure manca un dato (nessun ultimo punto, nessuna direzione del puntatore): la domanda resta la stessa |
| `Requires a distance, a point or an option.` | la richiesta vuole una distanza, un punto o un’opzione valida |
| `Requires an angle, a point or an option.` | la richiesta vuole un angolo, un punto o un’opzione valida |
| `Value must be positive and nonzero.` | raggio, diametro o altezza non valido: indica un valore positivo |
| `The three points are collinear.` | impossibile costruire il cerchio o l’arco: cambia il terzo punto |
| `Center coincides with the start point.` | per l’arco il centro deve essere diverso dall’inizio |
| `End point gives a zero-length arc.` | la fine non definisce un arco valido |
| `Invalid end point of arc.` | il punto è dietro la tangente nel modo arco di `PLINE` |
| `Select objects:` | scegli le entità per il comando, poi conferma a campo vuoto |
| `Specify opposite corner:` | un rettangolo di selezione è aperto: indica il secondo angolo |
| `<n> found, <t> total` / `<n> removed, <t> total` | entità trovate o rimosse dal gesto e totale della selezione durante `Select objects:` |
| `Invalid selection.` | durante la selezione è stato scritto testo non supportato; scegli le entità con il mouse |
| `Point coincides with the previous point.` | il punto indicato coincide con il vertice precedente ed è stato ignorato |
| `*Cancel*` | il comando è stato interrotto con `Esc`; il lavoro completato resta secondo la tabella sopra |
| `Undone: <passo>.` / `Redone: <passo>.` | passo annullato o ripetuto |
| `Nothing to undo.` / `Nothing to redo.` | non ci sono passi da annullare o da ripetere |
| `<Osnap on>` / `<Osnap off>` | snap a oggetto attivato o spento |
| `<Ortho on>` / `<Ortho off>` | modalità orto attivata o spenta |
