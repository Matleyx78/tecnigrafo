# Manuale dei comandi

Questo manuale spiega come si disegna con Tecnigrafo dalla versione 0.2: la riga
di comando, il modo di indicare i punti, il comando `PLINE` e l'annullamento.

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
| `Esc` | annulla il comando in corso e svuota il campo |
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
| clic sinistro nella vista | indica un punto al comando in corso |
| clic destro nella vista | come `Invio`: conferma il comando in corso, oppure ripete l'ultimo comando |
| movimento del puntatore | aggiorna l'anteprima del comando in corso |

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

**Correggere mentre disegni:** se l'ultimo punto è sbagliato, scrivi `u` e
indicane un altro; la polilinea in corso non viene interrotta.

## Annullare e ripetere

Ogni polilinea conclusa è un **passo** che si può annullare e poi ripetere.

| Azione | Menu | Tastiera | Riga di comando |
|---|---|---|---|
| Annulla | *Edit → Undo* | `Ctrl+Z` (`Cmd+Z` su macOS) | `U` o `UNDO` |
| Ripeti | *Edit → Redo* | `Ctrl+Y` (`Cmd+Y` su macOS), oppure la combinazione standard del sistema, per esempio `Ctrl+Shift+Z` su Linux e `Cmd+Shift+Z` su macOS | `REDO` |

- Le voci del menu dicono che cosa annullano o ripetono, per esempio
  *Undo Polyline*, e sono disattivate quando non c'è nulla da fare.
- Lo storico conferma ogni passo: `Undone: Polyline.` e `Redone: Polyline.`;
  se non c'è nulla, `Nothing to undo.` o `Nothing to redo.`
- La polilinea ripristinata è identica a quella annullata.
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
- Concludere una nuova polilinea cancella i passi che si potevano ripetere, come
  in qualunque programma.

## Messaggi

| Messaggio | Significato |
|---|---|
| `Unknown command: <nome>.` | non esiste un comando con quel nome o alias |
| `Invalid point or option.` | la scrittura non è una coordinata valida né un'opzione della domanda, oppure manca un dato (nessun ultimo punto, nessuna direzione del puntatore): la domanda resta la stessa |
| `Point coincides with the previous point.` | il punto indicato coincide con il vertice precedente ed è stato ignorato |
| `*Cancel*` | il comando è stato annullato con `Esc` |
| `Undone: <passo>.` / `Redone: <passo>.` | passo annullato o ripetuto |
| `Nothing to undo.` / `Nothing to redo.` | non ci sono passi da annullare o da ripetere |
