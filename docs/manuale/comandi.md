# Manuale dei comandi

Questo manuale descrive gli strumenti della tappa 0.3: la riga di comando, i
punti, `PLINE`, la selezione, `ERASE`, gli snap a oggetto, la modalità orto e
l'annullamento. Lo stato del rilascio è nelle [note della 0.3](../rilasci/0.3.md).

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
| `Spazio` | come `Invio` |
| `Esc` | svuota il campo; chiude prima un eventuale rettangolo di selezione, altrimenti annulla il comando in corso o, a riposo, svuota la selezione |
| `Invio` o `Spazio` a riga vuota | durante un comando: conferma (per esempio conclude la polilinea); senza comando: ripete l'ultimo comando |

Nomi dei comandi e opzioni si scrivono in maiuscolo o minuscolo, indifferentemente.

Ogni scambio resta nello storico come in AutoCAD, per esempio:

```
Command: pl
Specify start point: 0,0
Specify next point or [Undo]: @50,0
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
`PLINE` svuota la selezione precedente; `ERASE` invece la usa subito.

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
| *Endpoint* | estremo di un segmento, cioè un vertice della polilinea | quadrato |
| *Midpoint* | metà del segmento; per un arco è sulla curva, non sulla corda | triangolo |
| *Center* | centro del cerchio di un segmento ad arco | cerchio |

### Attivare e scegliere i modi

- `F3`, *Tools → Object Snap* e il pulsante **OSNAP** nella barra di stato
  attivano o spengono insieme lo snap. Lo storico mostra `<Osnap on>` oppure
  `<Osnap off>`.
- In *Tools → Object Snap Modes* puoi attivare o disattivare separatamente
  *Endpoint*, *Midpoint* e *Center*, anche quando OSNAP è spento. Spegnere `F3`
  conserva i modi scelti. Se tutti i modi sono disattivati, non c'è alcun aggancio
  anche con OSNAP acceso.
- All'avvio lo snap è acceso con tutti e tre i modi. Le impostazioni non si
  conservano alla chiusura del programma.

Per estremi e punti medi l'apertura è di **10 pixel**: avvicina il puntatore al
punto voluto finché compare il marcatore. La dimensione del marcatore e l'apertura
restano in pixel a qualunque zoom. Fra estremi e medi nell'apertura vince il più
vicino; a pari distanza l'estremo ha precedenza.

Per il **centro** passa sul segmento ad arco, entro l'apertura: il cerchio compare
sul suo centro anche quando è lontano dal puntatore. Il centro viene proposto
quando nell'apertura non c'è un estremo o un punto medio attivo da preferire.
Avvicinarsi soltanto al centro, lontano dall'arco, non basta.

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

Con un ultimo punto disponibile, il puntatore e il clic vengono allineati
all'orizzontale o alla verticale che passa per quel punto, secondo la direzione
più vicina al puntatore. A 45° prevale l'orizzontale. L'anteprima segue lo stesso
vincolo. L'orto non modifica i gesti di selezione.

### Snap, orto e coordinate scritte

| Come indichi il punto | Effetto |
|---|---|
| clic con uno snap disponibile | usa il punto di snap, anche fuori dall'asse dell'orto: **lo snap prevale sull'orto** |
| clic senza snap, con Ortho e un ultimo punto | allinea il punto in orizzontale o verticale |
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

**Primo punto di un nuovo comando:** nella 0.3 l'orto usa l'ultimo punto ricordato
anche dal comando precedente. Può quindi allineare anche il primo clic di un
nuovo `PL`. Per iniziare in una posizione esatta, scrivi le coordinate del primo
punto oppure spegni ORTHO per quel clic. È un difetto noto, da correggere in una
versione 0.3.x ([bug #64](https://github.com/Matleyx78/tecnigrafo/issues/64));
è descritto anche nelle [note di rilascio](../rilasci/0.3.md#limiti-noti).

Su alcuni portatili e su macOS `F3` e `F8` possono richiedere `Fn`. Puoi sempre
usare le voci di *Tools* o i pulsanti OSNAP e ORTHO.

## PLINE — polilinea

Disegna una polilinea fatta di segmenti retti, aperta o chiusa.

- **Nome:** `PLINE`
- **Alias:** `PL`

### Le domande

1. `Specify start point:` — indica il primo punto.
2. `Specify next point or [Undo]:` — indica il secondo punto; da qui in poi vedi
   il segmento elastico che segue il puntatore.
3. `Specify next point or [Close/Undo]:` — dal terzo punto in poi compare anche
   l'opzione *Close*.

Ogni punto aggiunge un vertice. Le opzioni si scelgono scrivendo il nome intero o
la lettera maiuscola tra parentesi quadre:

| Opzione | Scrittura | Effetto |
|---|---|---|
| *Close* | `C` o `close` | chiude la polilinea con un segmento verso il primo punto e conclude il comando (servono almeno 3 punti) |
| *Undo* | `U` o `undo` | toglie l'ultimo punto; se li togli tutti si torna a `Specify start point:` |

### Concludere

- `Invio`, `Spazio` o clic destro concludono la polilinea **aperta**. Servono
  almeno 2 punti: con meno, il comando termina senza disegnare nulla.
- *Close* conclude la polilinea **chiusa**.
- `Esc` annulla il comando: nel disegno non resta nulla di quanto indicato e lo
  storico mostra `*Cancel*`.

Se indichi due volte di seguito lo stesso punto, il secondo viene ignorato con il
messaggio `Point coincides with the previous point.`

### Esempi

**Un quadrato di lato 50**, scrivendo solo coordinate:

```
Command: pl
Specify start point: 0,0
Specify next point or [Undo]: @50,0
Specify next point or [Undo]: @50<90
Specify next point or [Close/Undo]: 0,50
Specify next point or [Close/Undo]: c
```

Il secondo vertice è 50 unità a destra del primo, il terzo 50 unità sopra il
secondo, il quarto è scritto in forma assoluta; `c` chiude il quadrato.

**Un profilo a L aperto**, con la distanza diretta: avvia `PL`, clicca il primo
punto, porta il puntatore verso destra e scrivi `120`, poi portalo verso l'alto e
scrivi `40`, quindi `Invio`. Si ottengono due segmenti lunghi esattamente 120 e 40.
Per garantirne l'angolo retto, usa ORTHO con OSNAP spento, come nell'esempio
[con orto e distanza diretta](#snap-orto-e-coordinate-scritte).

**Correggere mentre disegni:** se l'ultimo punto è sbagliato, scrivi `u` e
indicane un altro; la polilinea in corso non viene interrotta.

## Annullare e ripetere

Ogni polilinea conclusa e ogni gruppo cancellato con `ERASE` sono un **passo**
che si può annullare e poi ripetere.

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

### Che cosa non si annulla

- Il disegno di esempio che compare all'avvio non fa parte della cronologia e non
  si può annullare.
- Una polilinea conclusa con meno di 2 punti, o annullata con `Esc`, non lascia
  alcun passo: non c'è nulla da annullare.
- Una nuova modifica confermata, come una polilinea o una cancellazione, elimina
  i passi che si potevano ripetere. Un comando senza effetti conserva il redo.
- La selezione, lo zoom, il pan e le impostazioni di snap e orto non sono passi
  di annullamento. `U`, `UNDO` e `REDO` svuotano la selezione corrente.

## Messaggi

| Messaggio | Significato |
|---|---|
| `Unknown command: <nome>.` | non esiste un comando con quel nome o alias |
| `Invalid point or option.` | la scrittura non è una coordinata valida né un'opzione della domanda, oppure manca un dato (nessun ultimo punto, nessuna direzione del puntatore): la domanda resta la stessa |
| `Select objects:` | scegli le entità per il comando, poi conferma a campo vuoto |
| `Specify opposite corner:` | un rettangolo di selezione è aperto: indica il secondo angolo |
| `<n> found, <t> total` / `<n> removed, <t> total` | entità trovate o rimosse dal gesto e totale della selezione durante `Select objects:` |
| `Invalid selection.` | durante la selezione è stato scritto testo non supportato; scegli le entità con il mouse |
| `Point coincides with the previous point.` | il punto indicato coincide con il vertice precedente ed è stato ignorato |
| `*Cancel*` | il comando è stato annullato con `Esc` |
| `Undone: <passo>.` / `Redone: <passo>.` | passo annullato o ripetuto |
| `Nothing to undo.` / `Nothing to redo.` | non ci sono passi da annullare o da ripetere |
| `<Osnap on>` / `<Osnap off>` | snap a oggetto attivato o spento |
| `<Ortho on>` / `<Ortho off>` | modalità orto attivata o spenta |
