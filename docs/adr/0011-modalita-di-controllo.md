# ADR 0011: Modalità di controllo e verifica automatica

- **Stato:** Accettato (2026-10-18)
- **Sostituito in parte da:** ADR 0023 (le entità di una sessione nuova partono da 257)
- **Corretto:** 2026-10-31 — l'elenco dei casi in attesa non è più un file ma la
  cartella `tests/verifica/in-attesa/`, con un file per blocco di riferimenti
  `<X.Y>-R<N>.txt` (punti 7 e 9): con un file solo, ogni pacchetto di riferimenti
  lo allungava e ogni pacchetto di lavoro lo accorciava, e le pull request in
  parallelo andavano in conflitto una dopo l'altra. Il pacchetto di riferimenti
  aggiunge il suo file e quello di lavoro lo cancella, quindi nessun file è
  condiviso tra pacchetti in parallelo. La regola della CI («un caso in lista che
  passa fa fallire la CI») resta. Lo decide il responsabile.
- **Data:** 2026-10-18

## Contesto

I pacchetti di Tecnigrafo li scrivono più agenti IA in parallelo (Claude, Codex) e
il responsabile li prova a mano prima di unirli. Con la crescita del programma le
prove manuali diventano il collo di bottiglia, e un agente oggi non ha un modo
oggettivo per dire "quello che ho fatto va bene": scrive il codice e anche i test
che lo giudicano, quindi i test passano anche quando il codice è sbagliato.

Vogliamo una **modalità di controllo**: un modo unico e documentato di pilotare il
programma senza persone davanti, e di confrontare il risultato con riferimenti
fissati prima e da qualcun altro. Un lavoro è finito quando la CI, eseguendo questa
modalità, dice che il risultato coincide con i riferimenti.

Cosa c'è già:

- `cmd::CommandRunner` riceve testo, clic, movimento del puntatore, dimensione del
  pixel e impostazioni di snap e orto, senza Qt (ADR 0005 e 0008). I punti dati
  con `submit()` non vengono corretti: sono i punti esatti di script e test.
- `core::Document` numera le entità da 1 in avanti, sempre nello stesso modo.
- Il modulo `app` ha l'opzione `--screenshot` e test con eventi di tastiera e mouse
  simulati e piattaforma Qt `offscreen`.
- La CI compila e prova tutto su Windows, Linux e macOS, e fa fede: una pull
  request con la CI rossa non si rivede (GOVERNANCE.md).

Vincoli:

- `core`, `render`, `edit` e `cmd` restano senza Qt e senza librerie esterne
  (ADR 0001).
- I test non dipendono dalla rete né dall'ora (CONTRIBUTING.md).
- Il progetto è GPLv3 (ADR 0003): un file di riferimento preso da fuori deve avere
  una licenza compatibile e permetterne la ridistribuzione.
- Il programma distribuito non deve aprire nuovi punti di attacco.
- Il 3D (fase 2) userà Open CASCADE: la modalità deve poterlo accogliere senza
  essere riprogettata.

## Decisione

### 1. Principio

- Un lavoro è **fatto** quando la CI è verde, comprese le verifiche di questo ADR.
  Ciò che un agente dichiara di aver provato non conta: fa fede solo la CI.
- **Chi fissa i riferimenti non è chi fa il lavoro.** Criteri e file di
  riferimento li scrive il responsabile o un agente diverso da quello che realizza
  il pacchetto, e sono uniti in `main` **prima** che il lavoro cominci (punto 7).
- **I valori attesi non vengono da Tecnigrafo.** Si ricavano da calcoli, da file
  pubblici o da programmi di riferimento, mai eseguendo il programma che si sta
  verificando: altrimenti si registrerebbe il suo comportamento, errori compresi.
- Si confrontano soprattutto **dati in testo** (entità, coordinate, messaggi). Le
  immagini servono solo a verificare il disegno a schermo.

### 2. Due livelli

**Livello 1, senza interfaccia.** Un nuovo modulo `script` (`tg::script`, dipende
da `core`, `edit` e `cmd`, senza Qt) legge uno script, lo esegue su un
`CommandRunner` e produce un **rapporto** in testo (punto 4). Lo usa un piccolo
eseguibile da riga di comando, `tecnigrafo-script`, e lo usano i test. È veloce e
gira su Windows, Linux e macOS.

**Livello 2, con l'interfaccia.** Il programma accetta l'opzione
`--script <file>`: apre la finestra principale (in CI con
`QT_QPA_PLATFORM=offscreen`), esegue lo stesso script mandando eventi veri di
tastiera e mouse ai widget, come i test di `app`, scrive il rapporto e le
immagini richieste, poi esce. Prova tutto ciò che passa dall'interfaccia: riga di
comando, clic, movimento del cursore, scorciatoie, anteprima, marcatori di snap.

I due livelli usano lo stesso linguaggio e lo stesso formato del rapporto: uno
script del livello 1 si esegue anche al livello 2 e deve dare lo stesso rapporto.

**Domande al modello, al livello 1.** Non tutto si vede con uno script: un
pacchetto che aggiunge un'entità o un algoritmo al modello geometrico (`core`,
in futuro il modellatore 3D) non è ancora raggiungibile da nessun comando. Per
questo `tecnigrafo-script` legge anche un secondo tipo di file, le **domande**
(punto 3, "Domande al modello"): definisce delle entità e chiede al modello
riquadri, distanze, punti di snap, volumi, con le risposte attese scritte
accanto. Così anche la geometria ha i suoi riferimenti, scritti prima del lavoro
e da un altro agente, come i comandi.

### 3. Linguaggio degli script

Un file di testo UTF-8 con estensione `.tgs`, una riga per azione, nello spirito
degli script `.scr` di AutoCAD:

- una riga normale è il **testo scritto sulla riga di comando**, con la sintassi
  dell'ADR 0005 (`PLINE`, `0,0`, `@100<45`, `C`);
- una riga vuota è **Invio**;
- una riga che inizia con `;` è un commento;
- una riga che inizia con `!` è una **direttiva**.

Direttive minime (nomi in inglese, come comandi e messaggi):

| Direttiva | Effetto |
|---|---|
| `!view cx,cy scale WxH` | vista fissa: centro, unità per pixel, dimensioni in pixel |
| `!move x,y` | il cursore va al punto del disegno indicato |
| `!click x,y` / `!click x,y remove` | clic sinistro, con `Shift` per togliere |
| `!rightclick` / `!esc` | conferma e annullamento |
| `!snap on\|off` / `!ortho on\|off` | aiuti al disegno |
| `!key <tasto>` | un tasto o una scorciatoia (`F3`, `F8`, `Ctrl+Z`); solo livello 2 |
| `!report` | aggiunge al rapporto lo stato in quel momento |
| `!screenshot <nome>` | salva la vista; solo livello 2 |
| `!level 2` | il caso richiede il livello 2 |
| `!tolerance <tipo> <valore>` | cambia una tolleranza del confronto (punto 6) |

- Le coordinate delle direttive sono **del disegno**. Al livello 1 vanno al
  `CommandRunner` (`pointerMoved`, `click`); al livello 2 la vista fissata con
  `!view` le converte in pixel e il programma riceve eventi del mouse veri. Così lo
  stesso script prova lo snap, che dipende dai pixel, a entrambi i livelli.
- Uno script `.scr` di AutoCAD che usa solo comandi presenti in Tecnigrafo è già
  uno script valido.
- La specifica completa, con l'elenco definitivo delle direttive, si pubblica in
  `docs/verifica/script.md` con il primo pacchetto. Da quel momento il linguaggio
  è un contratto: una direttiva si aggiunge, non si cambia; per cambiarla serve un
  nuovo ADR.

#### Domande al modello

Un file di testo UTF-8 con estensione `.tgq`, una riga per istruzione:

- una riga vuota o che inizia con `;` non conta;
- `!tolerance <tipo> <valore>` cambia una tolleranza, come negli script;
- una **definizione** dà un nome a un'entità, scritta con lo stesso tipo e le
  stesse proprietà del rapporto (punto 4), per esempio
  `A = CIRCLE center=0,0 radius=10`. Le entità definite formano un documento,
  numerate nell'ordine in cui compaiono;
- una **domanda** chiede una cosa al modello e scrive accanto la risposta attesa:
  `? distance A 20,0 -> 10`.

Esempio:

```
; cerchio di raggio 10 nell'origine
A = CIRCLE center=0,0 radius=10
B = LINE start=20,0 end=30,0
? bbox A                  -> -10,-10 10,10
? distance A 20,0         -> 10
? snaps A                 -> Center 0,0
? pick 0,0 tolerance=3    -> none
? pick 10,1 tolerance=3   -> A
? window -11,-11 11,11    -> A
? circle-through 0,0 10,0 0,10 -> CIRCLE center=5,5 radius=7.0710678118654755
```

- Ogni domanda ha un nome, degli argomenti e una forma fissa della risposta. Il
  **catalogo** delle domande si pubblica in `docs/verifica/domande.md` e ha la
  stessa regola di stabilità del linguaggio degli script: una domanda si
  aggiunge, non si cambia.
- Il catalogo copre tre famiglie, e ogni ADR che introduce un tipo di oggetto o
  un algoritmo dice quali domande lo verificano:
  - **domande su un oggetto**: riquadro, lunghezza, area, distanza da un punto,
    punto più vicino, punti di snap, contenuto in un rettangolo, intersezione
    con un rettangolo;
  - **domande sul documento**: scelta con un clic, selezione a finestra e a
    intersezione, snap dal puntatore, con le regole di priorità;
  - **costruzioni**: il risultato di un algoritmo che crea geometria (cerchio
    per tre punti, arco tangente, e in futuro raccordi, offset, operazioni sui
    solidi), scritto come una definizione.
- Un tipo nuovo di oggetto risponde alle domande che esistono già: un pacchetto
  che aggiunge un'entità ne aggiunge la lettura nelle definizioni, e le domande
  generali la trattano senza codice nuovo.
- Le risposte si confrontano con le regole del punto 6: numeri entro la
  tolleranza, il resto uguale.
- Le domande esistono solo in `tecnigrafo-script`, al livello 1: il programma
  distribuito non le legge.

### 4. Il rapporto

Un file di testo con una prima riga di versione (`tecnigrafo-report 1`) e sezioni
fisse, sempre nello stesso ordine:

- le **entità** del disegno, in ordine di `EntityId`, ciascuna con il tipo DXF e
  tutte le sue proprietà (ADR 0006);
- i **messaggi** scritti sulla riga di comando e la domanda attuale;
- la **selezione** corrente;
- la **pila di annullamento** e ripetizione, con i nomi delle transazioni;
- lo stato di snap e orto e, al livello 2, lo snap attivo sotto il cursore;
- le **statistiche**: numero di entità per tipo.

Ogni `!report` aggiunge una sezione numerata; alla fine dello script si aggiunge
sempre lo stato finale. Il formato preciso si pubblica in
`docs/verifica/rapporto.md` insieme al linguaggio, e ha la stessa regola di
stabilità.

### 5. Determinismo

In modalità di controllo (livello 1 sempre, livello 2 con `--script`):

- **Ora:** sempre in UTC. Date e ore nel formato ISO 8601
  (`2026-10-18T05:51:00Z`). Dove serve un orologio (per esempio il tempo
  dall'avvio), l'orologio è fisso.
- **Formattazione dei dati:** inglese europeo, indipendente dalla lingua del
  sistema: punto come separatore decimale, nessun separatore delle migliaia,
  unità metriche. Ogni numero si scrive con la rappresentazione più breve che,
  riletta, dà lo stesso valore.
- **Identificativi:** le entità si numerano da 1 in avanti, come già fa
  `core::Document`. Nessun identificativo nasce da numeri casuali, indirizzi di
  memoria o dall'ordine di un contenitore non ordinato; ciò che si elenca nel
  rapporto si ordina sempre per identificativo.
- **Vista:** dimensioni e scala da `!view`, rapporto dei pixel 1, tema chiaro,
  font incluso nel programma (tappa 0.4).

### 6. Riferimenti e confronto

Un **caso di verifica** è una cartella `tests/verifica/casi/<area>/<nome>/` con:

- `script.tgs`;
- `atteso.txt`: il rapporto atteso;
- facoltativo, `atteso-<nome>.png` per ogni `!screenshot` da confrontare;

oppure con il solo `domande.tgq`, che contiene già le risposte attese (punto 3).

Il confronto:

- **Rapporti:** riga per riga. I numeri si confrontano con una tolleranza, per
  difetto assoluta di 1e-9 unità di disegno; il resto deve coincidere.
- **Immagini:** solo su Linux, al livello 2. Per difetto un pixel è diverso se un
  canale differisce di più di 8 su 255, e il caso fallisce se i pixel diversi
  superano lo 0,1%.
- Le tolleranze si cambiano solo con `!tolerance` nello script del caso, cioè
  solo da chi scrive i riferimenti.
- In caso di errore la CI stampa le differenze e allega rapporti e immagini
  ottenuti, così chi lavora vede cosa è cambiato.

Da dove vengono i valori attesi:

1. **Matematica.** Lunghezze, aree, intersezioni e punti di snap calcolati da chi
   scrive il caso e scritti in `atteso.txt`.
2. **File pubblici.** File DXF (e in futuro STEP) presi da raccolte pubbliche.
   **Non si copiano nel repository**: il file `tests/verifica/sorgenti.json`
   elenca per ognuno indirizzo, versione, hash SHA-256 e licenza. Prima dei test
   CMake li scarica e ne verifica l'hash (`file(DOWNLOAD … EXPECTED_HASH)`); la CI
   li conserva in una cache, e i test girano senza rete. Si accettano solo file
   con licenza compatibile con la GPLv3 che ne permetta la ridistribuzione. Nel
   repository stanno solo i file creati da noi.
3. **Programmi liberi di riferimento.** Un programma libero (per esempio un lettore
   DXF) può fare da oracolo solo in CI, con la versione fissata e senza mai essere
   collegato a Tecnigrafo. Lo si aggiunge con un pacchetto di riferimenti
   approvato dal responsabile.

Le immagini catturate durante il lavoro un agente può guardarle per orientarsi, ma
non sono una prova: prova solo il confronto in CI.

### 7. Chi fa cosa: due pull request

Ogni pacchetto che cambia il comportamento del programma ha due issue e due pull
request, assegnate ad agenti diversi (etichette `agente:…` diverse). Entrambe le
issue si creano all'apertura della tappa, insieme alle altre: la issue di
riferimenti del pacchetto `PN` si chiama `[X.Y-RN]`, porta l'etichetta
`riferimenti` e precede la issue del lavoro. Il piano della tappa indica, per
ogni pacchetto, l'agente del lavoro e quello dei riferimenti. Un pacchetto che
aggiunge o cambia la geometria del modello ha riferimenti anche quando nessun
comando la usa ancora: le domande al modello (punto 3). I pacchetti che non
cambiano il comportamento (documentazione, note di rilascio, infrastruttura) lo
dichiarano nel piano con il motivo, e non hanno riferimenti.

1. **Riferimenti.** La issue `[X.Y-RN]`. La pull request tocca
   solo `tests/verifica/casi/`, `tests/verifica/sorgenti.json` e
   `tests/verifica/in-attesa/`. Aggiunge i casi del pacchetto e li elenca nel
   file del suo blocco, `tests/verifica/in-attesa/<X.Y>-R<N>.txt`, insieme al
   numero della issue del lavoro. Il responsabile la unisce per prima.
2. **Lavoro.** La pull request del pacchetto non può toccare
   `tests/verifica/casi/` né `tests/verifica/sorgenti.json`. Deve cancellare da
   `in-attesa/` il file dei riferimenti del suo pacchetto: quando è cancellato,
   i casi diventano obbligatori.

La CI fa rispettare la divisione:

- una pull request che tocca `tests/verifica/casi/` o `tests/verifica/sorgenti.json`
  insieme ad altri file, a parte la cartella `in-attesa/`, fallisce;
- un caso elencato in `in-attesa/` si esegue, ma il suo fallimento non blocca la
  CI;
- un caso elencato in `in-attesa/` che passa fa fallire la CI, con il messaggio
  di toglierlo dall'elenco: così un lavoro finito non resta mai non verificato;
- la cartella `in-attesa/` è letta dalla configurazione di CMake: un file con un
  nome che non sia `<X.Y>-R<N>.txt`, un file senza casi, un caso elencato in due
  file e il vecchio file unico `in-attesa.txt` sono errori.

Se un agente ritiene sbagliato un riferimento, si ferma e lo scrive nella issue.
La correzione è una nuova pull request di riferimenti, di un agente diverso o del
responsabile. Il lavoro non adatta mai i riferimenti a sé.

### 8. CI

- **Linux:** livello 1 e livello 2 completi, compresi i confronti di immagini.
- **Windows e macOS:** livello 1 completo e, del livello 2, un solo caso di base
  che verifica che `--script` funzioni.
- Il passo della verifica si chiama `Verifica` ed è obbligatorio come gli altri.
- I rapporti e le immagini di ogni esecuzione si allegano come artefatti della CI.

### 9. Definizione di completato per gli agenti

AGENTS.md contiene queste regole, in vigore da quando esiste
`tests/verifica/`:

- prima di dichiarare il lavoro finito, esegui la verifica in locale
  (`ctest --preset ci -L verifica`);
- non modificare mai `tests/verifica/casi/` né `tests/verifica/sorgenti.json`
  in una pull request di lavoro;
- cancella da `in-attesa/` il file dei riferimenti del tuo pacchetto;
- se un caso ti sembra sbagliato, fermati e scrivilo nella issue;
- un bug corretto arriva con un caso di verifica che lo riproduce, scritto come
  riferimento da un altro agente o dal responsabile.

Il modello della pull request riceve la casella "La verifica in CI è verde e non
ho toccato i riferimenti".

### 10. Sicurezza

- La modalità di controllo si attiva solo con opzioni da riga di comando
  (`--script`). Nessuna porta di rete, nessun canale sempre aperto, nessun
  server.
- Uno script può fare solo ciò che potrebbe fare l'utente con mouse e tastiera.
  Le domande al modello, che creano entità senza comandi, esistono solo in
  `tecnigrafo-script`, uno strumento di sviluppo che non si installa con il
  programma.
- Un canale di controllo dal vivo (per esempio un server MCP per gli agenti) si
  potrà aggiungere in futuro solo con un nuovo ADR, sopra lo stesso linguaggio.

### 11. Il 3D

Il principio vale anche per il modellatore 3D (fase 2) e la modalità nasce già
pensandoci. Le linee guida, da precisare nell'ADR del modulo `modeling`:

- gli script pilotano anche schizzi e feature con lo stesso linguaggio;
- il rapporto aggiunge, per ogni solido, volume, area delle superfici,
  baricentro, riquadro di ingombro e numero di facce, spigoli e vertici;
- le **domande al modello** valgono anche per superfici e solidi, con lo stesso
  file e la stessa sintassi: un solido si definisce con una costruzione
  (`P = BOX origin=0,0,0 size=100,50,10`, poi le operazioni come fori e
  raccordi) o si legge da un file STEP di `sorgenti.json`
  (`S = STEP source=<nome>`); il catalogo aggiunge domande come `volume`,
  `area`, `centroid`, `bbox`, `topology` (facce, spigoli, vertici),
  `contains <punto>` e `distance` tra due oggetti. Così il nucleo geometrico 3D ha
  riferimenti prima che esistano i comandi per costruirlo;
- i riferimenti sono valori calcolati a mano (per esempio il volume di una
  piastra forata) e file STEP presi da raccolte pubbliche, con le stesse regole di
  licenza e di hash del punto 6;
- i confronti tra geometrie usano tolleranze proprie, decise con quell'ADR.

### 12. Legame con la segnalazione dei crash (ADR 0010)

La cronologia dei comandi di un rapporto di crash si può trasformare in uno
script. Ogni crash segnalato e corretto diventa così un caso di verifica
permanente. L'opzione nascosta che provoca un crash per la prova in CI dell'ADR
0010 si realizza come direttiva del livello 2.

### 13. Attuazione

Le regole di processo (AGENTS.md, `docs/LAVORO-IN-PARALLELO.md`, modelli di issue
e di pull request, controllo della divisione tra riferimenti e lavoro in CI)
entrano insieme a questo ADR. Il resto si realizza con i **primi pacchetti della
tappa 0.4**, che fa da banco di prova, in quest'ordine:

1. **Modulo `script`, livello 1:** linguaggio, rapporto, domande al modello,
   determinismo, confronto con tolleranze, elenco dei casi in attesa, eseguibile
   `tecnigrafo-script`, specifiche in `docs/verifica/`.
2. **Riferimenti iniziali** (pacchetto di riferimenti): casi per ciò che esiste
   già (`PLINE`, `ERASE`, selezione, snap, orto, annulla e ripeti), compreso il
   bug dell'orto corretto con la #66, e domande sulla geometria delle polilinee.
   Li scrive un agente diverso da quello del punto 1.
3. **Livello 2 nel modulo `app`:** `--script`, eventi simulati, immagini.
4. **CI:** passo `Verifica`, scaricamento e cache dei file pubblici. Le modifiche
   a `.github/` le approva il responsabile.

Questi quattro pacchetti costruiscono il sistema di verifica e non hanno
pacchetti di riferimenti propri: si provano con i loro test e con la revisione.
Da quando sono uniti, ogni pacchetto della tappa 0.4 che cambia il comportamento
parte dalla sua pull request di riferimenti. Alla chiusura della 0.4 il
responsabile valuta se il metodo ha alleggerito il lavoro, e annota in
`docs/ROADMAP.md` le correzioni da portare nelle tappe successive.

## Alternative considerate

- **Solo i test GoogleTest scritti con il lavoro:** è la situazione di oggi. Chi
  scrive il codice scrive anche il giudice, e una prova sbagliata passa senza che
  nessuno se ne accorga.
- **Verificare la geometria solo attraverso i comandi:** un'entità o un
  algoritmo nuovo resterebbe senza riferimenti finché non arriva il comando che
  lo usa, e un errore si scoprirebbe tardi, nel pacchetto sbagliato.
- **Una direttiva degli script che crea entità senza comandi:** mescolerebbe in
  uno stesso file ciò che fa l'utente e ciò che non può fare; le domande stanno
  in un file a parte.
- **Canale di controllo dal vivo (JSON o MCP) da subito:** permette agli agenti di
  interrogare il programma mentre lavorano, ma richiede un canale aperto, gestione
  dello stato e più codice. Rimandato (punto 10): gli script coprono le verifiche
  e restano riproducibili in CI.
- **Far giudicare le immagini a un modello con la vista:** utile per orientarsi,
  ma il giudizio non è ripetibile e può sbagliare. Non vale come prova.
- **Usare AutoCAD come oracolo:** richiede una licenza e Windows, non si può
  eseguire nella CI di un progetto libero.
- **Copiare i file di prova nel repository:** semplice, ma ingrandisce il
  repository e impone di verificare per ogni file il diritto di ridistribuirlo
  insieme al codice. Scaricarli con hash fissato dà la stessa ripetibilità.
- **Simulazione completa dell'interfaccia su tutti e tre i sistemi:** più lenta e
  più fragile (font, scala, gestori delle finestre). Linux fa da riferimento;
  Windows e macOS eseguono il livello 1 e un caso di base.
- **Confronto delle immagini al pixel esatto:** fallirebbe a ogni minima
  differenza di antialiasing. Si usa una soglia.

## Conseguenze

- Il responsabile non prova più a mano ogni pacchetto: controlla che i
  riferimenti siano giusti, una volta, nella pull request dei riferimenti. La
  prova manuale resta alla chiusura della tappa.
- Ogni pacchetto costa una pull request in più, quella dei riferimenti, e un
  secondo agente.
- Nasce un modulo nuovo, `script`, da aggiungere in `docs/ARCHITETTURA.md`, e una
  cartella `tests/verifica/`.
- Linguaggio degli script, catalogo delle domande e formato del rapporto
  diventano contratti stabili: agenti e casi di verifica ci si appoggiano.
- Il formato del rapporto serve anche a scrivere le entità nelle domande: ogni
  entità nuova si deve saper scrivere e rileggere.
- La scheda della tappa 0.5 prevedeva una raccolta di DXF di prova inclusa nel
  repository: con questo ADR i file pubblici si scaricano con hash fissato (punto
  6). La decisione è registrata in `docs/ROADMAP.md` e nella scheda della 0.5.
- Ogni tappa ha un agente in più al lavoro (di norma Gemini, per i riferimenti),
  e il piano della tappa cresce: per ogni pacchetto indica anche l'agente dei
  riferimenti, o perché non servono.
- I casi del livello 2 dipendono dal font incluso: fino alla tappa 0.4 i
  confronti di immagini riguardano solo geometria, griglia e marcatori.
- La CI su Linux si allunga per il livello 2. Se diventasse troppo lenta, i casi
  del livello 2 si possono dividere in più passi paralleli.
