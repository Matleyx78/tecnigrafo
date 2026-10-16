# ADR 0008: Selezione, snap a oggetto e orto

- **Stato:** Proposto
- **Data:** 2026-10-16

## Contesto

Dalla tappa 0.3 l'utente sceglie entità esistenti e disegna agganciandosi alla
loro geometria. Selezione, snap e orto resteranno al centro del programma: grip,
comandi di modifica (tappa 0.7) e filtri (tappa 0.6) ci si appoggeranno. Chi viene
da AutoCAD si aspetta i suoi gesti e le sue regole.

Come per i comandi (ADR 0005), la logica non deve dipendere da Qt: deve essere
provata dai test e un giorno pilotata da script. Le tolleranze però nascono in
pixel, cioè nella vista.

## Decisione

### 1. Dove vive la selezione

- La **selezione corrente** è un `cmd::SelectionSet` posseduto da
  `cmd::CommandRunner`: un elenco di `EntityId`, ciascuno una volta, nell'ordine
  in cui l'utente li ha scelti.
- Non fa parte del documento: non si annulla, non si salva, non cambia il disegno.
- Si svuota con `Esc` a riposo, all'avvio di ogni comando (dopo averla consegnata
  al comando che la usa), alla fine di ogni comando e con `U`, `UNDO` e `REDO`.
  Così non può mai contenere a lungo entità che non esistono più.

### 2. Gesti

Valgono a riposo (per costruire la selezione corrente) e nella domanda "Select
objects:":

| Gesto | Effetto |
|---|---|
| clic entro 3 pixel da un'entità | la aggiunge; con `Shift` la toglie |
| clic nel vuoto | primo angolo di un rettangolo, che segue il puntatore |
| clic successivo, a destra del primo | **finestra**: le entità interamente dentro |
| clic successivo, a sinistra del primo | **intersezione**: le entità dentro o toccate |
| `Shift` sul clic che chiude il rettangolo | toglie invece di aggiungere |
| `Esc` con un rettangolo aperto | chiude il rettangolo |

Con `Shift` premuto i clic tolgono, senza `Shift` aggiungono, come con il
valore predefinito di AutoCAD (`PICKADD` = 1). Il rettangolo si traccia con due
clic; il trascinamento non fa parte di questa decisione. A parità di distanza dal
clic, vince l'entità con l'id più alto.

Una polilinea chiusa è una curva, non una regione: un clic al suo interno non la
seleziona, e un rettangolo a intersezione che sta tutto dentro di essa senza
toccarne il bordo non la prende.

### 3. Comandi che lavorano su entità esistenti

- Un comando lo dichiara con `Command::needsSelection()`.
- È il **runner** a chiedere "Select objects:", a interpretare i gesti, a scrivere
  i messaggi (`3 found, 5 total`) e a chiudere la domanda con `Invio`, spazio o
  clic destro. Il comando non vede i singoli gesti.
- Se all'avvio c'è una selezione corrente, la domanda si salta.
- Il comando parte (`start()`) solo quando la selezione è pronta e la riceve in un
  solo input, `SelectionInput`, mai vuoto. Se l'utente conferma senza aver scelto
  nulla, il comando finisce senza effetti.

Come la lettura delle coordinate (ADR 0005), la selezione è identica in tutti i
comandi perché sta in un solo posto. Le opzioni scritte di AutoCAD (`W`, `C`,
`ALL`, `L`, `P`, recinto) si aggiungeranno lì, senza toccare i comandi.

### 4. Snap a oggetto

- Modi della tappa 0.3: **estremo** (ogni vertice), **punto medio** (metà di ogni
  segmento, sull'arco se è un arco) e **centro** (di ogni segmento ad arco).
  Ciascuno si attiva da sé; `F3` attiva o spegne lo snap nel suo insieme.
- **Apertura** di 10 pixel, come il valore predefinito di AutoCAD.
- **Scelta**: si considerano i segmenti entro l'apertura dal puntatore; tra i loro
  estremi e punti medi entro l'apertura vince il più vicino (a parità l'estremo,
  poi l'id più alto). Se non ce n'è, vince il centro dell'arco più vicino al
  puntatore: il centro si aggancia passando sull'arco, come in AutoCAD, non
  avvicinandosi al centro.
- **Marcatori**, centrati sul punto e di dimensione fissa in pixel: quadrato per
  l'estremo, triangolo per il punto medio, cerchio per il centro.

### 5. Orto

Con l'orto attivo (`F8`) e un ultimo punto, il punto del mouse si sposta
sull'orizzontale o sulla verticale passante per l'ultimo punto, quella più vicina
alla direzione del puntatore (a 45° esatti, l'orizzontale).

### 6. A quali punti si applicano

- Snap e orto correggono **solo i punti dati con il mouse mentre un comando chiede
  un punto**: il clic e il puntatore che guida l'anteprima. Mai durante la
  selezione.
- **Lo snap prevale sull'orto**: se c'è un punto di snap, si usa quello.
- Le **coordinate scritte** non si correggono. La distanza diretta (`d`) prende la
  direzione dal puntatore già corretto, quindi con l'orto segue l'orizzontale o la
  verticale, come in AutoCAD.
- I punti inviati con `CommandRunner::submit(PointInput)` (script e test) non si
  correggono.

### 7. Tolleranze e vista

Tolleranza di scelta (3 pixel) e apertura dello snap (10 pixel) sono costanti di
`cmd`. La vista comunica al runner soltanto quante unità del disegno vale un
pixel (`setPixelSize`), a ogni cambio di zoom; tutto il resto, compreso il
rettangolo in costruzione e lo snap attivo, sta nel runner e la vista lo chiede per
disegnarlo.

### 8. Aspetto

- Entità selezionate ridisegnate tratteggiate in un colore di evidenziazione.
- Rettangolo a finestra: riempimento blu trasparente, bordo blu continuo.
- Rettangolo a intersezione: riempimento verde trasparente, bordo verde
  tratteggiato.
- Il `Painter` astratto riceve per questo lo stile della linea (continua o
  tratteggiata, in pixel) e il riempimento di poligoni. Il tratteggio dei tipi di
  linea del disegno (tappa 0.6) è un'altra cosa: sarà in unità del disegno.

### 9. Impostazioni

Snap e orto sono un `cmd::DrawingAids` nel runner. All'avvio: snap attivo con i
tre modi, orto spento. Non si salvano tra una sessione e l'altra fino alla tappa
0.9.

## Alternative considerate

- **Selezione nel documento**, come stato annullabile: AutoCAD non annulla la
  selezione, e mescolerebbe uno stato dell'interfaccia con il disegno.
- **Gesti gestiti dalla vista Qt**: più rapido da scrivere, ma non provabile senza
  finestre e da rifare per ogni futura interfaccia.
- **Ogni comando chiede da sé "Select objects:"**: porterebbe a selezioni diverse
  da un comando all'altro, lo stesso problema evitato per le coordinate.
- **Selezione inviata come gesti al comando**: lascerebbe a ogni comando la
  gestione di finestre, `Shift` e messaggi.
- **Snap calcolato nella vista**: legherebbe la geometria all'interfaccia e
  renderebbe la regola "lo snap prevale sull'orto" impossibile da provare.

## Conseguenze

- I comandi di modifica della tappa 0.7 dichiarano `needsSelection()` e ricevono
  un elenco di id: nessuno di loro reimplementa la selezione.
- Le opzioni scritte di selezione, i filtri e i nuovi snap si aggiungono in `cmd` e
  `core`, in un solo punto.
- La vista resta sottile: inoltra clic e movimenti, comunica lo zoom, disegna ciò
  che il runner le dice.
- `Input` riceve una nuova alternativa (`SelectionInput`): i comandi esistenti
  la ignorano, perché non dichiarano di usare la selezione.
- La firma di `Painter::setPen` cambia e ogni backend va aggiornato.
