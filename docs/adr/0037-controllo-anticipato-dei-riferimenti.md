# ADR 0037: Controllo anticipato dei riferimenti

- **Stato:** Proposto
- **Data:** 2026-10-31

## Contesto

Un caso di verifica scritto prima del lavoro sta in `tests/verifica/in-attesa.txt`
e si esegue con `--pending`: fallire è previsto, quindi la CI della pull request
di riferimenti non dice niente sul suo contenuto
([ADR 0011](0011-modalita-di-controllo.md), punto 7). Un errore del riferimento
emerge solo quando l'agente del lavoro toglie il caso dall'elenco; allora si
ferma (AGENTS.md, "fermati del tutto"), e servono una pull request di
correzione, una CI e un merge prima che il lavoro riparta.

Commit di correzione dei riferimenti: 8 nella 0.4, 0 nella 0.5, 7 nella 0.6,
già 3 nella 0.7. Quasi nessuno riguarda la geometria; sono dettagli
dell'interfaccia che nessuno controlla prima del merge
([#426](https://github.com/Matleyx78/tecnigrafo/issues/426)):

- richieste di comandi che esistono già, scritte male (`Specify start point`
  invece di `Specify first point` in `LINE`, #301);
- handle sbagliati (257 invece di 258, #296);
- righe vuote in più negli script: una riga vuota è `Invio`, che a `Command:`
  ripete l'ultimo comando (#417, #421);
- spazi, formato di una risposta che esiste già (#295), una sintassi non ancora
  prevista (#414).

Una prova sui 128 casi a script di livello 1 di oggi (2026-10-31) conferma il
tipo delle righe vuote: in cinque casi in attesa una riga vuota segue un
comando già finito e non ha eco in `atteso.txt`.

L'idea #426 propone tre controlli. Questo ADR ne fissa le regole; il terzo,
le richieste annunciate, si rimanda (punto 8).

## Decisione

### 1. Dove

I controlli stanno in `tecnigrafo-script check`, quindi valgono per ogni caso
che `ctest` esegue: nel job `Verifica` della CI, in `Linux (ASan, UBSan)`, su
Windows e su macOS, e con `scripts/controlla.sh --solo verifica`. Non cambiano
`.github/`, `cmake/` né il passo `Verifica` della CI.

Si chiamano **controlli anticipati**. Non eseguono mai i comandi oltre il punto
che il programma sa fare, e non cambiano i riferimenti.

### 2. La forma dei casi (tutti i casi)

Valgono per ogni caso, in attesa o no. Un caso che non le rispetta **non è
valido** (codice 2), anche con `--pending`:

1. `script.tgs`, `atteso.txt` e `domande.tgq` non contengono `CR` (fine riga
   CRLF) né tabulazioni.
2. `atteso.txt` e `domande.tgq` non finiscono con righe vuote. In `atteso.txt`
   una riga vuota sta solo prima di una riga che apre una sezione (`[...]`),
   come vuole il [rapporto](../verifica/rapporto.md).
3. Spazi in fondo a una riga: mai in `domande.tgq`; in `script.tgs` solo sulle
   righe normali, che sono testo scritto dall'utente, mai sui commenti e sulle
   direttive; in `atteso.txt` solo su una riga che finisce con `: ` e il testo
   intero di una riga normale dello script che ha quegli spazi in fondo, cioè
   sulla sua eco. Vale ai due livelli: oggi lo usano un caso di livello 1 e uno
   di livello 2.
4. Le righe vuote in fondo a `script.tgs` restano ammesse: sono `Invio`, e le
   controlla il punto 3.

### 3. L'eco delle righe dello script (script al livello 1)

Ogni riga normale dello script (non un commento, non una direttiva; anche
vuota) scrive un'eco nella cronologia: la domanda, uno spazio e il testo; per
una riga vuota la sola domanda ([rapporto](../verifica/rapporto.md), "La
cronologia"). Il controllo prende le righe di tutti gli elenchi `messages:` di
`atteso.txt`, nell'ordine delle sezioni, e cerca l'eco di ogni riga normale,
nell'ordine dello script e ciascuna dopo quella della riga precedente:

- per una riga non vuota `t`, una riga che finisce con `: ` seguito da `t`,
  esattamente;
- per una riga vuota, una riga che finisce con `:`, che venga **prima**
  dell'eco della riga non vuota successiva.

Una riga senza eco rende il caso non valido, con il numero della riga dello
script. Il controllo non esegue il programma: confronta due file scritti dallo
stesso agente. Gli script di livello 2 non hanno questo controllo, perché lì
lo spazio scritto con la tastiera vale `Invio`.

### 4. Il confine di un caso in attesa

Il **confine** è il primo punto del caso che il programma non sa ancora fare,
o che il riferimento dichiara diverso apposta. Negli script è la prima di
queste righe:

1. una riga che usa ciò che il catalogo descrive ma il programma non ha ancora
   (la stessa regola che oggi rende il caso "non superato", non "non valido":
   una direttiva annunciata, un modo di snap, un argomento);
2. una riga normale scritta a `Command:`, senza un comando in corso, a cui il
   programma risponderebbe `Unknown command`: non è `U`, `REDO`, un comando né
   un alias registrato;
3. un commento `; cambia con #<numero>` (punto 5).

Nelle domande:

1. una domanda che il programma non sa ancora dare (domanda non realizzata,
   argomento o tipo di entità non realizzati, entità non gestita di un file)
   si salta, e le altre si confrontano;
2. una definizione o una direttiva non ancora realizzata, o un commento
   `; cambia con #<numero>`, è il confine del resto del file: le righe sotto
   non si valutano.

### 5. L'eccezione dichiarata: `; cambia con #<numero>`

Un pacchetto di lavoro può cambiare apposta un comportamento che esiste già
(un nuovo snap acceso per difetto cambia la risposta di `snap`; un'opzione
nuova cambia la domanda di un comando). Il riferimento lo scrive con un
commento, nello script o nelle domande, **prima** della riga da cui il
comportamento cambia:

```
; cambia con #346
```

`<numero>` è la issue del lavoro che cambia il comportamento. Il commento non
cambia il significato del file per chi lo legge senza `--pending`: un caso non
in attesa si confronta sempre per intero. Il commento resta dopo il merge del
lavoro, come documentazione del caso; il responsabile lo vede in revisione.

### 6. Il confronto prima del confine

Con `--pending`, oltre al confronto di oggi, `check` esegue il caso fino al
confine:

- **script**: esegue le azioni prima del confine e scrive il rapporto come se
  lo script finisse lì. Le sezioni `[report N]` scritte prima del confine
  devono coincidere con le stesse sezioni di `atteso.txt`; le righe
  `messages:` della sezione finale ottenuta devono essere le prime righe
  `messages:` della sezione successiva di `atteso.txt`. Il resto non si
  confronta;
- **domande**: ogni domanda prima del confine e non saltata deve dare la
  risposta attesa.

Una differenza prima del confine è un errore **anche con `--pending`**: `check`
esce con 1 e lo dice ("differs before the point the program cannot do yet").
Un caso in attesa senza confine, che però non passa, è lo stesso errore: o il
riferimento è sbagliato, o manca `; cambia con`.

Se il caso passa per intero vale ancora la regola di oggi: va tolto da
`in-attesa.txt`.

### 7. Chi fa cosa

- **Riferimenti.** La pull request di riferimenti è finita con la CI verde,
  compresi i controlli anticipati. L'agente può eseguire i casi che scrive
  (`scripts/controlla.sh --solo verifica`, o la CI) per sapere se sono validi;
  il divieto dell'ADR 0011 e dell'ADR 0027 (punto 7) resta: non ricava mai un
  valore atteso da ciò che il programma stampa. Se il controllo segnala una
  differenza prima del confine, l'agente rifà il valore dalle sue fonti
  (calcoli, file pubblici, AutoCAD). Se le fonti confermano il suo valore, la
  differenza può essere un errore del programma: non adatta il riferimento, si
  ferma e lo scrive nella issue; decide il responsabile (una issue di
  correzione, e nel caso `; cambia con #<correzione>`).
- **Lavoro.** Nulla cambia: toglie da `in-attesa.txt` le righe dei suoi casi,
  e da quel momento il confronto è intero.
- **Responsabile.** In revisione guarda ogni `; cambia con` nuovo: è
  l'eccezione che spegne il controllo per il resto del caso.

### 8. Le richieste annunciate: rimandate

L'idea #426 propone anche un elenco di **richieste annunciate**: il testo delle
domande dei comandi nuovi, scritto nella scheda del pacchetto e letto dalla
CI. Chiede un formato nuovo, una riga nel [modello della
scheda](../tappe/0000-pacchetto.md) e un passo nell'apertura della tappa
(`scripts/apri-tappa.sh`). Si decide con il piano della tappa 0.8; fino ad
allora una domanda che cambia si dichiara con `; cambia con` (punto 5).

### 9. Transizione: `esente`

I casi già in attesa sono scritti senza queste regole. Il pacchetto che porta
un controllo anticipato lo prova su tutti i casi esistenti; quelli in attesa
che non lo superano restano in `in-attesa.txt` con la parola `esente` in fondo
alla riga:

```
grip/cerchio-e-modifica-non-valida #352 esente
```

- Un caso `esente` si esegue con `--pending`, come oggi, ma senza i controlli
  anticipati.
- Aggiunge `esente` solo il pacchetto che porta un controllo anticipato, sulle
  righe che esistono al momento della sua pull request, e le elenca nella pull
  request; nessun'altra pull request la aggiunge.
- La toglie una pull request di riferimenti che corregge il caso, o il lavoro
  quando toglie la riga.
- I casi non in attesa non possono essere `esente`: oggi superano tutti i
  controlli della forma.

### 10. Attuazione

Tre pacchetti fuori tappa, come miglioramento dei test
([ROADMAP.md](../ROADMAP.md), "Cosa si può fare fuori dalla tappa aperta"),
uno dopo l'altro:

1. [F1](../tappe/fuori-tappa/F1-forma-dei-casi.md): la forma e l'eco (punti 2
   e 3), `esente` (punto 9) e le regole in AGENTS.md;
2. [F2](../tappe/fuori-tappa/F2-parte-gia-verificabile.md): il confine, il
   commento `; cambia con` e il confronto prima del confine negli script
   (punti 4–6);
3. [F3](../tappe/fuori-tappa/F3-domande-gia-verificabili.md): lo stesso per le
   domande.

## Alternative considerate

- **Le regole della forma come nell'idea, senza eccezioni.** Vietare gli spazi
  in fondo e le righe vuote finali anche negli script rifiuterebbe casi
  giusti: due casi scrivono apposta testo con spazi in fondo, e 59 script
  finiscono con una riga vuota che chiude il comando. L'eco (punto 3) prende
  invece proprio le righe vuote sbagliate di #417 e #421.
- **Controlli in un passo nuovo della CI** (uno script Python nel job
  `Verifica`). Il confine richiede il registro dei comandi e il parser degli
  script, che stanno già in `tecnigrafo-script`; un passo a parte rifarebbe il
  parser e non varrebbe in `controlla.sh` né sugli altri sistemi.
- **Il commento `cambia con` in `atteso.txt`.** Il rapporto non ha commenti, ed
  è un contratto che si cambia solo aggiungendo campi; lo script e le domande
  li hanno già.
- **Solo una segnalazione non bloccante.** Costa meno, ma nessuno è obbligato a
  leggerla; l'eccezione `; cambia con` basta a coprire i cambiamenti voluti.
- **Correggere prima i casi in attesa, senza `esente`.** I casi in attesa
  cambiano con ogni pull request della tappa aperta: l'elenco da correggere si
  sposterebbe mentre il pacchetto lavora, e lo fermerebbe.

## Conseguenze

- Gli errori di dettaglio dell'interfaccia (richieste di comandi che esistono,
  handle prima del confine, righe vuote, spazi) si trovano nella pull request
  di riferimenti, prima del merge.
- Un caso in attesa ha due esiti nuovi: non valido per la forma, e fallito per
  una differenza prima del confine.
- Restano alla revisione gli errori dopo il confine, come l'impostazione
  geometrica di #414, e i casi `esente`.
- Le pull request di riferimenti costano un po' di più: chi le scrive deve
  portarle al verde, e talvolta aggiungere `; cambia con`.
- Una differenza prima del confine può essere un errore del programma: il
  controllo la fa emergere prima, e la decide il responsabile.
