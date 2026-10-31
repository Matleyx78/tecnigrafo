# Casi di verifica

Come si scrive, si esegue e si legge un caso di verifica della modalità di
controllo ([ADR 0011](../adr/0011-modalita-di-controllo.md), punti 6 e 7). Le
regole su chi scrive cosa sono in [AGENTS.md](../../AGENTS.md), sezione
"Modalità di controllo e riferimenti".

## La cartella di un caso

Ogni caso è una cartella `tests/verifica/casi/<area>/<nome>/`, con `<area>` e
`<nome>` fatti di lettere, cifre, `_`, `-` e `.` (per esempio
`casi/pline/coordinate-polari/`). Contiene **una** di queste due cose:

- uno **script**: `script.tgs` ([linguaggio](script.md)) e `atteso.txt`, il
  [rapporto](rapporto.md) atteso; al livello 2 anche `atteso-<nome>.png` per
  ogni `!screenshot <nome>` da confrontare;
- delle **domande**: il solo `domande.tgq` ([catalogo](domande.md)), che
  contiene già le risposte attese.

Una cartella con entrambi, o con nessuno dei due, non è un caso valido.

Nel repository stanno solo i file creati da noi. I file presi da raccolte
pubbliche non si copiano: si elencano in `tests/verifica/sorgenti.json` (vedi
sotto).

### File del caso

Dalla tappa 0.5 un caso può contenere, accanto a `script.tgs` o a
`domande.tgq`, dei **file creati da noi** che lo script apre o le domande
leggono: per esempio un DXF scritto a mano, anche danneggiato apposta, o un
`.cadx` costruito con lo strumento `zip`. Anche in sottocartelle (`dati/`).
Lo script o le domande dicono, con un commento, come è fatto ogni file
creato a mano e perché.

- Le **domande** li leggono con `file=<nome>`, direttamente dalla cartella
  del caso, in sola lettura ([domande.md](domande.md#file)).
- Gli **script** li aprono con un nome relativo
  ([script.md](script.md#file)). Prima di eseguire lo script, `check`
  svuota la cartella `file/` dentro quella di `--output` e ci copia tutto il
  contenuto del caso tranne `script.tgs` e i file `atteso*`: lo script legge
  e scrive lì, sulle copie. Così un caso non cambia mai il repository, e un
  file salvato da un'esecuzione precedente non cambia la successiva. La CI
  allega la cartella di uscita, con i file scritti dagli script, agli
  artefatti.
- I file pubblici di `sorgenti.json` si leggono con `@sources/<nome>` negli
  script e con `source=<nome>` nelle domande, dalla cartella di `--sources`.

### Casi controllati dall'oracolo

Un caso di domande i cui valori attesi vengono da ezdxf
([ADR 0020](../adr/0020-fuzzing-e-oracolo-ezdxf.md), punto 2) lo dichiara
nella **prima riga**:

```
; oracle: ezdxf
D = DXF source=librecad-sample
? count D -> ARC 2; LINE 12
```

La riga è un commento per `tecnigrafo-script`. Il passo `Oracolo ezdxf`
della CI (pacchetto P9) controlla che le risposte scritte nel caso coincidano
con quelle di ezdxf, con le stesse regole di confronto; le domande che ezdxf
non sa dare (per esempio `roundtrip`) si saltano.

## Eseguire un caso

```sh
cmake --preset ci && cmake --build --preset ci
ctest --preset ci -L verifica                     # tutti i casi
ctest --preset ci -R verifica/pline/              # i casi di un'area
./build/ci/script/tecnigrafo-script check tests/verifica/casi/<area>/<nome> --output /tmp/caso \
    --sources build/verifica-sorgenti
./build/ci/script/tecnigrafo-script run <script.tgs> [--report <file>] [--sources <cartella>]
```

- `run` esegue uno script al livello 1 e scrive il rapporto (senza `--report`,
  sullo schermo).
- `check` esegue il caso, lo confronta con il risultato atteso e stampa le
  differenze, fino a 20, con il numero della riga del file atteso. Scrive nella
  cartella di `--output` (per difetto quella corrente) il risultato ottenuto:
  `ottenuto.txt` per uno script, `ottenuto.tgq` per le domande, con le risposte
  del programma al posto di quelle attese.
- Prima di eseguire il caso, `check` fa i **controlli anticipati** (vedi
  sotto): un caso che non li supera non è valido, anche con `--pending`.
- Con `--pending` il risultato si capovolge, come per i casi in attesa (vedi
  sotto). `--no-early-checks`, solo insieme a `--pending`, salta i controlli
  anticipati: lo usa `ctest` per i casi `esente`.
- Con `--pending`, uno script di livello 1 che non passa si esegue di nuovo
  fino al suo **confine** (vedi sotto), e `check` scrive accanto a
  `ottenuto.txt` il rapporto ottenuto fin lì, `ottenuto-confine.txt`.
- `--sources <cartella>` è la cartella dei file di `sorgenti.json` già
  scaricati e controllati (in CMake `TG_VERIFICA_SOURCES_DIR`, per difetto
  `build<suffisso>/verifica-sorgenti`). `ctest` la passa sempre. Senza,
  `@sources/<nome>` e `source=<nome>` sono un errore che lo dice, e un caso
  di domande che li usa non è valido.
- `run` legge e scrive i file dello script nella cartella corrente.

Codici di uscita:

| Codice | Significato |
|---|---|
| 0 | superato; con `--pending`, non superato (con un avviso), e uguale all'atteso fino al confine |
| 1 | non superato; con `--pending`, superato: va tolto da `in-attesa/` |
| 2 | caso non valido: file mancante o scritto male, argomenti sbagliati. Vale anche con `--pending` |
| 2 | caso che non supera i controlli anticipati: `INVALID: <caso>: <file>, line <n>: <motivo>`. Vale anche con `--pending`, non con `--no-early-checks` |
| 1 | con `--pending`, script che differisce dall'atteso **prima del confine**: `FAILED: <caso>: differs before the point the program cannot do yet (script.tgs, line <n>: <motivo>)`, poi le differenze. Non con `--no-early-checks` |
| 1 | con `--pending`, script che non passa ma non ha un confine: `FAILED: <caso>: fails, but uses nothing the program does not have yet: fix the reference, or write "; cambia con #<issue>" before the line where the behaviour changes (docs/adr/0037)`. Non con `--no-early-checks` |
| 77 | il caso richiede il livello 2 (`!level 2`): `ctest` lo conta come saltato |

Ciò che il catalogo prevede ma il programma non ha ancora **non** è un caso non
valido: è un caso non superato, codice 1 (con `--pending`, codice 0 e un
avviso). Vale per una domanda non realizzata, per un tipo di entità che arriva
con un pacchetto successivo e per ciò che le specifiche descrivono in anticipo:
una direttiva (`!osnap`, `!polar`, `!scr`), un modo di snap (`intersection`,
`perpendicular`, `tangent`, `quadrant`) o un argomento di domanda (`from=`),
fino al pacchetto che li realizza. Ciò che le specifiche **non** descrivono, un
nome sbagliato o un argomento che non esiste, resta un caso non valido.

## Controlli anticipati

I primi due li fa `check` prima di eseguire il caso, leggendo solo i file
([ADR 0037](../adr/0037-controllo-anticipato-dei-riferimenti.md), punti 2 e
3): trovano nella pull request di riferimenti gli errori di forma e le righe
vuote di troppo, che altrimenti emergerebbero solo con il lavoro. Il primo che
non passa rende il caso non valido, con il file, la riga e il motivo. Il
terzo, per i soli casi in attesa, confronta la parte dello script che il
programma sa già fare ([il confine](#il-confine-di-un-caso-in-attesa), punti
4–6).

**La forma dei file**, per ogni caso, in attesa o no:

| Regola | Motivo stampato |
|---|---|
| `script.tgs`, `atteso.txt` e `domande.tgq` non hanno `CR` (righe terminate da LF) | `carriage return: lines end with LF only` |
| … né tabulazioni | `tab character` |
| `atteso.txt` e `domande.tgq` non finiscono con righe vuote; quelle in fondo a `script.tgs` sono `Invio` e restano ammesse | `empty line at the end of the file` |
| in `atteso.txt` una riga vuota sta solo prima di una riga che apre una sezione (`[...]`) | `empty line not followed by a section` |
| spazi in fondo a una riga: mai in `domande.tgq`; in `script.tgs` solo sulle righe normali, mai su commenti e direttive; in `atteso.txt` solo sull'eco di una riga normale dello script che li ha (una riga che finisce con `: ` e il testo intero di quella riga) | `trailing spaces` |

Il BOM UTF-8 iniziale è ammesso e non conta.

**L'eco delle righe dello script**, solo se lo script non ha una riga
`!level 2` (al livello 2 lo spazio scritto con la tastiera vale `Invio`).
Ogni riga normale scrive la sua eco nella cronologia
([rapporto](rapporto.md#la-cronologia-messages)). Il controllo prende le righe
di tutti gli elenchi `messages:` di `atteso.txt`, nell'ordine delle sezioni, e
cerca, nell'ordine dello script e ciascuna dopo quella della riga precedente:

- per una riga non vuota `t`, una riga che finisce con `: ` seguito da `t`;
- per una riga vuota, una riga che finisce con `:`, prima dell'eco della riga
  non vuota successiva.

Una riga senza eco dà `no echo of this line in the messages of atteso.txt (an
empty line is Enter: at Command: it repeats the last command)`, con il numero
della riga dello script. Il caso tipico è una riga vuota dopo un comando già
finito: a `Command:` è `Invio`, che ripete l'ultimo comando.

### Il confine di un caso in attesa

Un caso in attesa fallisce perché il programma non sa ancora fare qualcosa, ma
ciò che viene **prima** si può già confrontare
([ADR 0037](../adr/0037-controllo-anticipato-dei-riferimenti.md), punti 4–6).
Con `--pending`, uno script di livello 1 che non passa ha un **confine**: la
prima di queste righe.

1. Una riga che usa ciò che il catalogo descrive ma il programma non ha
   ancora: una direttiva annunciata (`!polar`). Le righe sotto si leggono
   comunque, e una riga scritta male, anche sotto il confine, rende il caso non
   valido.
2. Una riga normale scritta a `Command:`, senza un comando in corso, a cui il
   programma risponderebbe `Unknown command`: non è `U`, `UNDO`, `REDO`, un
   comando né un alias registrato. La stessa riga scritta come risposta a una
   domanda di un comando in corso non è un confine.
3. Un commento **`; cambia con #<numero>`**: `;` nella prima colonna, spazi
   facoltativi, `cambia con #` e il numero della issue del lavoro che cambia
   apposta un comportamento che esiste già (un'opzione nuova cambia la domanda
   di un comando). Il riferimento lo scrive **prima** della riga da cui il
   comportamento cambia; resta nel caso anche dopo il merge, come
   documentazione. Per un caso non in attesa non cambia nulla: si confronta
   sempre per intero.

`check` esegue le azioni prima del confine e scrive il rapporto come se lo
script finisse lì (`ottenuto-confine.txt`). Si confrontano, con le stesse
tolleranze del caso:

- la prima riga del rapporto;
- le sezioni `[report 1]` … `[report k]` scritte prima del confine, riga per
  riga, con le sezioni di `atteso.txt` con lo stesso titolo; una sezione che
  manca in `atteso.txt` è una differenza;
- le righe di `messages:` della sezione `[final]` ottenuta, che devono essere
  le **prime** righe di `messages:` della sezione di `atteso.txt` che segue
  `[report k]` (la prima sezione, se non c'è un `!report` prima del confine).

Il resto non si confronta. Gli esiti, con `--pending` e senza
`--no-early-checks`:

- il caso passa per intero: codice 1, va tolto da `in-attesa/` (come
  sempre);
- uguale all'atteso fino al confine: codice 0, e l'avviso dice fin dove
  (`...; as expected up to script.tgs, line 12: unknown command TRIM`);
- una differenza prima del confine: codice 1, `differs before the point the
  program cannot do yet`, con il confine e le differenze. Il caso tipico è una
  domanda di un comando che esiste già scritta male (`Specify start point`
  invece di `Specify first point` in `LINE`) o un handle sbagliato in un
  `[report]` prima del comando nuovo;
- nessun confine: codice 1, `fails, but uses nothing the program does not
  have yet`. O il riferimento è sbagliato, o manca `; cambia con`.

Gli script di livello 2 e i casi di domande non hanno ancora questo controllo;
per le domande arriva con il pacchetto F3.

## Come si confronta

- **Rapporti**: riga per riga. I numeri sono uguali se differiscono al più di
  `1e-9` (tolleranza assoluta, `!tolerance absolute`) o di una frazione del loro
  valore (`!tolerance relative`, per difetto 0); il resto della riga deve
  coincidere carattere per carattere. Un numero è una sequenza di cifre con
  segno, punto ed esponente facoltativi che non segue una lettera, una cifra,
  `_` o `.`: in `R10` le cifre sono testo. Il terminatore di riga (LF o CRLF),
  un terminatore finale e il BOM non contano; le righe vuote sì.
- **Domande**: come sopra, risposta per risposta, con gli elenchi in qualunque
  ordine ([domande.md](domande.md#le-risposte)).
- **Immagini** (livello 2, solo Linux): un pixel è diverso se un canale
  differisce di più di 8 su 255 (`!tolerance channel`), e il caso fallisce se i
  pixel diversi superano lo 0,1% (`!tolerance pixels`).

Le tolleranze si cambiano solo nel file del caso, cioè solo da chi scrive i
riferimenti.

## `tests/verifica/in-attesa/`

L'elenco dei casi il cui lavoro non è ancora unito. È una cartella con **un file
per blocco di riferimenti**, che prende il nome dal tag della issue di
riferimenti: `<X.Y>-R<N>.txt`, per esempio `in-attesa/0.7-R16.txt` per
`[0.7-R16]`. In ogni file, una riga per caso:

```
# commento
pline/esc-conserva #90
cerchio/domande-base #82
grip/cerchio-e-modifica-non-valida #352 esente
```

- Una riga per caso: `<area>/<nome>`, uno o più spazi, `#` e il numero della
  **issue del lavoro** che lo farà passare. Le righe che iniziano con `#` sono
  commenti; le righe vuote non contano.
- **`esente`**, dopo il numero e solo quella parola: il caso si esegue con
  `--pending --no-early-checks`, cioè senza i controlli anticipati
  ([ADR 0037](../adr/0037-controllo-anticipato-dei-riferimenti.md), punto 9).
  È la transizione per i casi in attesa scritti prima di un controllo: la
  aggiunge solo il pacchetto che porta un controllo anticipato, se la sua
  scheda lo dice; la toglie la pull request di riferimenti che corregge il
  caso, o il lavoro quando cancella il file. Un caso non in attesa non può
  essere `esente`.
- Un caso elencato si esegue con `--pending`: il suo fallimento non blocca la
  CI, ma se **passa** la CI fallisce, con il messaggio di toglierlo dall'elenco.
  Così un lavoro finito non resta mai non verificato.
- La pull request di **riferimenti** aggiunge i suoi casi e il **suo file**; la
  pull request del **lavoro** cancella quel file, e da quel momento i casi sono
  obbligatori. Due pacchetti non toccano mai lo stesso file, quindi l'elenco non
  causa conflitti tra pull request in parallelo. Un file elenca i casi di un
  solo pacchetto di lavoro, quello della sua issue.
- Un file senza casi (solo commenti) è un residuo da cancellare: è un errore.
  I file che cominciano con `.` si ignorano; il `.gitkeep` tiene viva la
  cartella quando l'ultimo file è stato cancellato.
- Sono errori di configurazione di CMake: un nome che non sia
  `<X.Y>-R<N>.txt` (anche una sottocartella), una riga scritta male, un caso che
  non esiste, un caso elencato due volte o in due file, e il vecchio file unico
  `tests/verifica/in-attesa.txt`, che non esiste più. La lettura è in
  `tests/verifica/pending_list.cmake`, provata dal test
  `verifica-in-attesa/lettura`.
- Aggiungere, cancellare o cambiare un file riconfigura da solo alla prossima
  compilazione, anche con una build già fatta.

## `tests/verifica/sorgenti.json`

I file presi da raccolte pubbliche (DXF dalla tappa 0.5, in futuro STEP) non
stanno nel repository: questo file li elenca, e prima dei test CMake li scarica
e ne controlla l'hash (pacchetto P4). La CI li conserva in una cache, così i
test girano senza rete.

```json
{
  "format": 1,
  "sources": [
    {
      "name": "esempio-planimetria",
      "url": "https://example.org/archivio/planimetria-1.2.dxf",
      "version": "1.2",
      "sha256": "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
      "license": "CC-BY-4.0",
      "license-url": "https://creativecommons.org/licenses/by/4.0/"
    }
  ]
}
```

| Chiave | Contenuto |
|---|---|
| `format` | versione del formato di questo file: `1` |
| `sources` | l'elenco dei file, uno per oggetto, con le chiavi che seguono |
| `name` | nome con cui i casi indicano il file: lettere minuscole, cifre e `-`, unico |
| `url` | indirizzo da cui scaricare il file, fisso (non "l'ultima versione") |
| `version` | versione o data del file nella raccolta |
| `sha256` | hash SHA-256 del file, 64 cifre esadecimali minuscole |
| `license` | identificativo SPDX della licenza |
| `license-url` | pagina della licenza, da riportare anche nella pull request |

Si accettano solo file con una licenza compatibile con la GPLv3 che ne permetta
la ridistribuzione ([ADR 0003](../adr/0003-licenza.md)). Nel dubbio, il file
non si usa e lo si scrive nella pull request: la CI controlla l'hash, non la
licenza.

## Scrivere un caso di riferimenti

1. Per ogni condizione "Fatto quando" del pacchetto, almeno un caso; più i casi
   limite (valori nulli, punti coincidenti o quasi allineati, `Esc` a metà
   comando, annulla e ripeti).
2. Valori attesi da calcoli, file pubblici o programmi di riferimento, **mai**
   eseguendo Tecnigrafo: `ottenuto.txt` e `ottenuto.tgq` servono a chi fa il
   lavoro, non a chi scrive i riferimenti.
3. Nel rapporto atteso gli identificativi delle entità non si riusano mai
   ([ADR 0004](../adr/0004-modello-geometrico-2d.md)): un caso che annulla un
   segmento si aspetta un buco nella numerazione.
4. Per i comandi, conviene un `!report` nei punti che il pacchetto descrive
   (dopo ogni domanda importante, prima e dopo `Esc`): la sezione finale da sola
   nasconde gli stati intermedi.
5. Elencare i nuovi casi nel file del proprio blocco,
   `in-attesa/<X.Y>-R<N>.txt`, con il numero della issue del lavoro.
6. Controllare in locale che i casi siano validi, controlli anticipati
   compresi: `ctest --preset ci -L verifica` deve passare, e `tecnigrafo-script check <caso> --pending` deve
   scrivere "pending" (o "skipped" per il livello 2), mai "INVALID". Se dà
   "INVALID" per una direttiva, un modo di snap o un argomento che le
   specifiche descrivono, il programma non lo annuncia ancora (gli elenchi sono
   in `script/src/script.cpp` e `script/src/question_table.cpp`): non
   cambiare il caso e scrivilo nella issue.
