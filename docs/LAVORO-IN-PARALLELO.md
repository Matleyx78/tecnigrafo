# Lavoro in parallelo

Come far lavorare più persone o più agenti IA sulla stessa tappa senza che si
intralcino. Il principio è uno: **GitHub è l'unico registro di chi fa cosa**, e
solo il responsabile assegna i pacchetti e unisce le pull request.

Dalla tappa 0.4 vale anche la modalità di controllo
([ADR 0011](adr/0011-modalita-di-controllo.md)): chi scrive i riferimenti non è
chi scrive il codice, e un pacchetto è finito quando la CI conferma che il
programma li rispetta. La guida pratica, passo per passo, per il responsabile è
in [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md).

Quando una tappa chiede qualcosa a solid-space, le tappe aperte sono due, una per
progetto, e avanzano insieme ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md),
punto 13). Il responsabile le vede sulla stessa tavola, il Project «Tecnigrafo»
del suo account, che raccoglie issue e pull request dei due repository. Il
Project è una vista: gli agenti continuano a lavorare con etichette e issue, e la
fonte resta GitHub.

## Come leggere i riferimenti

GitHub usa **una sola numerazione** per issue e pull request: ogni elemento nuovo,
qualunque sia il tipo, prende il numero successivo. Per questo un numero può
indicare una issue o una pull request.

| Scrittura | Significato | Dove si trova |
|---|---|---|
| `[0.4-P5]` | tappa 0.4, pacchetto P5 | titolo della issue del pacchetto |
| `[0.4-R5]` | tappa 0.4, riferimenti del pacchetto P5 | titolo della issue dei riferimenti |
| `#5` | issue o pull request numero 5 | scheda **Issues** o **Pull requests** |
| `Closes #5` | la pull request, quando viene unita, chiude la issue 5 | descrizione della pull request |
| `p0.4-05-…` | ramo su cui si lavora al pacchetto P5 della tappa 0.4 | scheda **Branches** |
| `r0.4-05-…` | ramo dei riferimenti del pacchetto P5 della tappa 0.4 | scheda **Branches** |

Il documento della tappa (`docs/tappe/…`) riporta per ogni pacchetto il numero
della sua issue; ogni pull request riporta in alto la issue che chiude.

**Prefissi dei due progetti, dalla tappa 0.8.** Perché nelle chat non ci siano
due «P20» (uno di Tecnigrafo e uno di solid-space), i pacchetti di Tecnigrafo si
scrivono `PT`, `RT`, `CT` (`[0.8-PT5]`, `[0.8-RT5]`) e quelli di solid-space
`PS`, `RS`, `CS`; i rami seguono (`pt0.8-05-…`). Le tappe già chiuse e la 0.7.5
restano con `P`, `R`, `C`, e nulla si rinomina. Valgono dopo il pacchetto della
tappa 0.7.5 che adegua script e controlli della CI
([tappe/0.7.5-correzione-di-rotta.md](tappe/0.7.5-correzione-di-rotta.md)).

## Chi fa cosa

| Chi | Etichetta | Lavoro |
|---|---|---|
| Responsabile | — | piano della tappa, ADR, revisione dei riferimenti, merge, prova manuale, rilascio |
| Claude | `agente:claude` | pacchetti impegnativi e la maggior parte dei medi, anche in più sessioni nello stesso momento; riferimenti e revisione delle pull request dei pacchetti di Codex; piano della tappa insieme al responsabile |
| Codex | `agente:codex` | revisione del contratto di ogni tappa di solid-space e delle sue correzioni; revisione delle interfacce delle schede di Tecnigrafo (sezione 1); riferimenti e revisione delle pull request dei pacchetti di Claude; pacchetti facili e medi che il piano non dà a Claude; pull request di chiusura della tappa |

Gli agenti sono due ([DECISIONI.md](DECISIONI.md), 2026-11-11). Ogni pacchetto
fa un **giro a tre**: un agente scrive i riferimenti, l'altro realizza il
pacchetto, e chi ha scritto i riferimenti rivede la pull request del lavoro.
Quindi Codex, Claude, Codex per i pacchetti di Claude, e Claude, Codex, Claude
per quelli di Codex. I passaggi da un agente all'altro li fa la **catena degli
agenti**, avviata con un commento ([agenti/catena.md](agenti/catena.md);
[ADR 0053](adr/0053-catena-degli-agenti.md)). Le etichette `agente:gemini`, `agente:gemini-argon` e `agente:copilot`
restano su GitHub, ma non si assegnano.

Più sessioni di Claude nello stesso momento usano la stessa etichetta:
`AA-Presa in carico` basta perché due sessioni non prendano la stessa issue.
Le issue aperte prima con `agente:claude-2` le prende qualunque sessione di
Claude; per le nuove l'etichetta non si usa più.

Chi lavora a un pacchetto non scrive i suoi riferimenti, e viceversa. Vale
anche per i **modelli**: chi scrive i riferimenti usa un modello diverso da chi
realizza il pacchetto, e chi rivede una pull request un modello diverso da chi
l'ha scritta; pull request e verbali dicono il modello e l'impegno usati. Le
etichette servono perché gli agenti lavorano tutti attraverso l'account GitHub
del responsabile, e non si possono distinguere in altro modo. La lista delle
issue filtrata per etichetta mostra in ogni momento chi sta facendo cosa.

## 1. Preparare il piano

Il piano dettagliato della tappa (modello in [tappe/0000-modello.md](tappe/0000-modello.md))
dice, per ogni pacchetto:

- **Difficoltà**, **agente**, **modello** e **impegno** del lavoro (vedi sotto);
- **Riferimenti**: l'agente che li scrive, oppure "non servono" con il motivo
  (documentazione, note di rilascio, infrastruttura che non cambia il
  comportamento);
- **Dipende da**: le issue che devono essere chiuse prima. Due pacchetti che
  toccano gli stessi file sono **sempre** legati da "Dipende da", così non
  possono partire insieme. Nessuna eccezione: nemmeno l'elenco dei casi in
  attesa è un file condiviso, perché è la cartella `tests/verifica/in-attesa/`
  con un file per blocco di riferimenti, che il pacchetto di riferimenti
  aggiunge e quello di lavoro cancella (ADR 0011, punto 7); lo stesso in
  solid-space. Una dipendenza da una pull request si scrive come la issue che
  quella pull request chiude: GitHub non accetta una issue «bloccata da» una
  pull request. Un pacchetto che
  ha bisogno di solid-space cita qui la richiesta in solid-space (`Matleyx78/solid-space#N`) finché la tappa di
  solid-space non ha i suoi pacchetti, poi il pacchetto di solid-space che gli
  serve; nel piano sta dopo i pacchetti che non ne hanno bisogno
  ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 13).

**Piano in due tempi.** Se la tappa chiede qualcosa a solid-space, il piano si
scrive subito per i pacchetti che non ne dipendono, mentre le richieste si aprono
in solid-space ([RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md)). Le
schede dei pacchetti che ne dipendono si scrivono dopo che la issue `[X.Y-C]`
di solid-space è chiusa, sulle intestazioni accettate e riviste
([CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md));
fino ad allora hanno solo la loro riga
nella tabella, con «scheda dopo il contratto». I loro riferimenti prendono i
valori geometrici dall'archivio dei casi di solid-space e verificano la facciata
e i comandi.

**Revisione delle interfacce.** Le schede che introducono tipi o funzioni nuove
in `core` si controllano con la lista di
[CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md)
(punti 1–11, con il 10 e l'11) prima che i loro pacchetti partano: un pacchetto
di revisione, di un agente diverso da chi ha scritto le schede (di norma Codex),
sta nella tabella prima di loro, e loro lo citano in "Dipende da"
([tappe/0000-modello.md](tappe/0000-modello.md), sezione 6). La lista nasce per
il contratto di solid-space, ma i buchi più costosi stanno spesso tra una
funzione e l'altra: un'informazione decisa esattamente da solid-space che la
scheda passa avanti solo come valore arrotondato (#359). I buchi trovati li
decide il responsabile in [DECISIONI.md](DECISIONI.md).

Prima di quel pacchetto, **chi scrive le schede passa per primo la stessa
lista** sul codice che ogni scheda chiama: firme reali, chi chiama la funzione
e che cosa vede, dove vive il dato. Nella pull request del piano scrive una
riga per scheda che il passaggio è fatto. La revisione dell'altro agente
resta, ma parte da schede già passate, e il verbale porta al responsabile
solo i buchi veri: nella 0.8.1 i verbali di PT4 (#927) e PT5 (#928) avevano 20
bloccanti e 15 decisioni, in buona parte evitabili leggendo il codice
([DECISIONI.md](DECISIONI.md), 2026-11-28).

**Comandi nell'interfaccia.** Una tappa che aggiunge comandi li porta anche
nell'interfaccia, nella stessa tappa: per ogni comando nuovo l'icona a ruoli,
la voce nel registro delle azioni (menu, ribbon e barre dell'interfaccia
classica) e la scorciatoia, se ne ha una. I pacchetti dei comandi stanno in
`cmd` e non toccano `app`, quindi il piano ha un pacchetto di `app` che lo fa
per i comandi della tappa, dopo di loro, e un criterio di chiusura che lo
verifica. Nella 0.7 mancava, e i comandi di modifica sono rimasti fuori da menu
e ribbon ([DECISIONI.md](DECISIONI.md), 2026-11-07).

**A nessun pacchetto deve mancare qualcosa il giorno in cui parte**
([DECISIONI.md](DECISIONI.md), 2026-11-19, dopo gli arresti della 0.8):

- **Prova a secco dei riferimenti.** Ogni scheda con riferimenti ha
  un'appendice con un caso d'esempio vero, senza valori attesi
  ([tappe/0000-pacchetto.md](tappe/0000-pacchetto.md)). Se non si scrive con
  le direttive e le domande di `main`, manca uno strumento: diventa un
  pacchetto dell'**onda 0**, da cui dipendono gli RT che lo usano. Se si
  scrive solo con un comando o un'entità di un pacchetto che non è fra le
  dipendenze, manca una dipendenza: si aggiunge, oppure quel ramo si rimanda a
  un pacchetto successivo, scritto nella scheda. Se il pacchetto cambia una
  risposta di una fonte di verifica (oracolo ezdxf, rapporto, catalogo delle
  domande, archivio dei casi di solid-space), la prova a secco dice quali casi
  già uniti tocca e come restano verdi fino all'ultimo pacchetto che ne
  dipende, cercandoli anche nei `dati.dxf` scritti a mano; per un campo che
  l'oracolo impara solo con il pacchetto sceglie fra le strade A e B del
  modello; e dice quali regole già scritte la risposta deve rispettare
  ([DECISIONI.md](DECISIONI.md), 2026-11-28).
- **Pacchetti trasversali in testa.** Un pacchetto che cambia righe che tutti i
  rapporti contengono (come PT22 con `dimension styles:` nella 0.8) si segna
  come trasversale e va **in testa** all'ordine, così i riferimenti nascono già
  con le sue righe. In coda solo con un motivo scritto nel piano e l'elenco dei
  casi da aggiornare; mai in mezzo.
- **Onde larghe.** L'obiettivo è fino a **8 catene in parallelo**: le onde si
  costruiscono con fino a 8 pacchetti indipendenti, senza obbligo.
- **Regole numeriche.** Le regole su pareggi e riduzioni (quarti di giro,
  angoli ridotti a un giro, pareggi fra guide e fra incroci, distanze al
  quadrato, ingressi fuori dall'uso reale) stanno in un documento unico,
  [REGOLE-NUMERICHE.md](REGOLE-NUMERICHE.md); una scheda che confronta angoli, distanze o
  candidati vi rimanda, e una regola nuova si scrive lì.
- **Materiale pronto.** I DXF di prova che le schede citano sono pubblicati e
  puliti prima dell'apertura della tappa.
- **Revisione del piano.** Prima dell'apertura l'altro agente rilegge il piano
  intero, in un solo giro ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B4a;
  i punti in [agenti/revisione.md](agenti/revisione.md), "Revisione del
  piano"). Le prove a secco restano essenziali: se una richiede più di qualche
  riga, il pacchetto va diviso. Preparare una tappa non deve costare più che
  farla.

### Pacchetti leggeri (dalla tappa 0.5)

Ogni sessione di un agente paga un costo fisso in token prima di scrivere una
riga: legge le regole, il pacchetto, gli ADR e il codice che tocca. Poi ogni
pacchetto costa una CI e un merge. Il piano tiene basso il primo costo e non
moltiplica il secondo:

- **Una scheda per pacchetto.** Ogni pacchetto ha la sua scheda,
  `docs/tappe/X.Y/PN-<nome>.md` (modello in
  [tappe/0000-pacchetto.md](tappe/0000-pacchetto.md)), autosufficiente:
  l'agente legge quella, non il piano intero. Le regole della tappa che lo
  riguardano si ripetono nella scheda, in breve.
- **Un modulo per pacchetto**, più i suoi test e le righe dei file comuni.
- **Misura.** Al massimo una quindicina di file e circa 600 righe di codice
  cambiate; test, documenti e casi di riferimento non contano, né per i file né
  per le righe. Un pacchetto più grande si divide, di solito lungo i confini
  naturali (un'entità, un comando, una famiglia di domande, un'intestazione di
  solid-space).
- **Non troppo piccoli.** Un pacchetto con meno di circa 150 righe di codice si
  unisce a un vicino dello stesso modulo, salvo che sia un'unità che si verifica
  da sola (un'entità, un comando, una famiglia di domande) o infrastruttura:
  ogni pacchetto costa comunque una sessione, una CI, una revisione e un merge,
  e spesso una pull request di riferimenti.
- **Dividere.** Si preferiscono pacchetti medi, ma un pacchetto si divide solo
  per un motivo che il piano scrive in una riga: supera la misura; oppure la
  divisione toglie una parte da una catena di file comuni o da un'attesa di
  solid-space, o separa le decisioni da prendere dal codice da scrivere (con
  «Modello e impegno», qui sotto, il codice va in fascia media). Due parti che
  toccano gli stessi file restano in catena, e dividerle allunga la tappa di un
  pacchetto. Le schede nate dallo stesso pacchetto si citano a vicenda in «Da
  leggere», passano insieme dalla revisione delle interfacce, e l'ultima ha i
  casi che vedono l'insieme. Un pacchetto iniziale di sola interfaccia si fa
  solo se toglie file comuni a più pacchetti o ne fa partire due in parallelo.
- **Riferimenti della stessa misura.** Un pacchetto di riferimenti copre un
  solo pacchetto di lavoro, anche quando questo è una parte di un pacchetto
  diviso: ogni `RN` al suo `PN` ([ADR 0011](adr/0011-modalita-di-controllo.md),
  punto 7). Dove un pacchetto chiede casi delicati e casi ordinari, i suoi
  riferimenti si dividono in blocchi con la lettera: per esempio `RT14a`, di
  fascia forte, con i casi delicati, e `RT14b`, leggero, con gli ordinari.
  Oltre una ventina di casi si divide per area.

### Modello e impegno

Il piano suggerisce, per ogni pacchetto, la **fascia di modello** e
l'**impegno** (quanto ragionamento chiedere, dove lo strumento lo permette).
Sono indicazioni: il responsabile sceglie il modello quando avvia l'agente. I
nomi dei modelli cambiano, la fascia resta.

La tabella elenca i modelli disponibili al responsabile a novembre 2026, con il
livello di ragionamento da scegliere. I modelli della stessa riga hanno capacità
simile secondo una misura indipendente dai produttori, l'Intelligence Index di
[Artificial Analysis](https://artificialanalysis.ai/models) (tra parentesi;
più alto è meglio). Quando cambiano i modelli si aggiorna la tabella con la
stessa fonte.

| Fascia | Per | Claude | Codex |
|---|---|---|---|
| **forte** | decisioni delicate: interfacce nuove, comportamenti con casi limite che nessun documento fissa, riferimenti con calcoli delicati, revisione delle interfacce | Opus 5.5 medio (51) o alto (54) | astra medium (50) |
| **medio** | pacchetti medi: un comando, un'entità sul modello di una già fatta, codice che traduce decisioni già scritte | Opus 5.5 medio (51) | sol medium (48) o astra light (46) |
| **leggero** | pacchetti facili: documentazione, note di rilascio, piccoli ritocchi; riferimenti ordinari e revisioni | Haiku 5.5 alto (38) o Sonnet 5.5 medio (41) | luna alto (32) o sol light (42) |

I riferimenti con calcoli delicati (tolleranze, archi, casi limite geometrici;
in particolare curve parallele, raccordi e intersezioni) li scrive un agente di
fascia forte, diverso da chi farà il pacchetto di lavoro: Codex per i pacchetti
di Claude, e Claude per quelli di Codex. I riferimenti ordinari (i comandi:
sequenze di domande, messaggi, annulla; i casi senza calcoli delicati) possono
andare in fascia leggera, sempre all'altro agente.

Per i pacchetti medi Claude lavora con Opus medio, non con Sonnet: la quota del
piano Max lo permette, e un modello più forte fa meno giri di correzione.

| Impegno | Quando |
|---|---|
| **alto** | il pacchetto richiede di ragionare su casi limite (tolleranze, punti quasi allineati, annulla e ripeti) o di progettare interfacce |
| **medio** | il lavoro segue uno schema già presente nel codice |
| **basso** | il lavoro è meccanico: testi, elenchi, documenti |

Di norma: facile → leggero e basso; media → medio e medio; impegnativa →
forte e alto. La fascia forte va a chi prende decisioni delicate: progettare
un'interfaccia nuova, scegliere un comportamento con casi limite che nessun
documento fissa, scrivere i riferimenti con calcoli delicati, rivedere le
interfacce. Un pacchetto che traduce in codice decisioni già scritte (nella
scheda, nel contratto di solid-space o in un ADR) e coperte da riferimenti di
fascia forte è medio anche se il codice è delicato; la sua pull request ha una
revisione mirata, in fascia forte, dall'altro agente. Il piano scrive in una
riga perché un pacchetto resta impegnativo
([DECISIONI.md](DECISIONI.md), 2026-11-10).

**Pacchetti che estendono una fonte di verifica.** Un pacchetto che estende
l'oracolo ezdxf, il rapporto o il catalogo delle domande a **più di due** tipi
o tabelle nuove ha la difficoltà della somma delle schede che descrive: **si
divide**, un pacchetto per gruppo di tipi, con l'attivazione progressiva dei
tipi attivi (come l'elenco dei tipi della 0.8 nell'oracolo). Resta la regola
dei pacchetti non troppo piccoli, qui sopra ([DECISIONI.md](DECISIONI.md),
2026-11-19, dopo PT3 della 0.8).

## 2. Aprire la tappa: issue ed etichette

All'apertura si creano, nel milestone `vX.Y`:

- una issue `[X.Y-PN]` per ogni pacchetto, con le etichette `tappa-X.Y`,
  `pacchetto`, la difficoltà e `agente:…` secondo il piano;
- una issue `[X.Y-RN]` per ogni pacchetto che ha riferimenti, con le etichette
  `tappa-X.Y`, `riferimenti` e `agente:…` di chi li scrive. La issue del lavoro
  la cita nella riga "Dipende da".

Dalla tappa 0.5 il testo di ogni issue cita la scheda del pacchetto
(`docs/tappe/X.Y/PN-<nome>.md`) e ne riporta modello e impegno, così l'agente
parte da lì.

Dalla tappa 0.7 tutto questo lo fa **`scripts/apri-tappa.sh X.Y`**, partendo
dalla tabella dei pacchetti del piano (formato in
[tappe/0000-modello.md](tappe/0000-modello.md)): crea etichetta, milestone e
issue con il loro testo, segna le dipendenze native di GitHub («bloccata da»),
mette le issue nel Project con *Tappa* e *Tipo*, e apre una pull request che
scrive i numeri delle issue nel piano e lo stato nella ROADMAP. Prima mostra che
cosa farà e chiede conferma. I pacchetti senza scheda (quelli che aspettano un
contratto di solid-space) non ricevono la issue: lo script si rilancia quando
le loro schede sono unite, e crea solo le issue nuove. Lo lancia il responsabile
(GUIDA-RESPONSABILE.md, B6).

## 3. Riferimenti

1. L'agente dei riferimenti prende una issue `[X.Y-RN]` alla volta, scrive i casi
   di verifica e apre una pull request (regole in [AGENTS.md](../AGENTS.md),
   sezione "Modalità di controllo e riferimenti").
2. Il responsabile la rivede: è il punto in cui la sua attenzione rende di più.
   Controlla che ogni condizione "Fatto quando" del pacchetto abbia un caso, che
   i valori attesi siano giusti, e che i file presi dal web abbiano una licenza
   adatta (la CI controlla l'hash, non la licenza).
3. Unita la pull request, il pacchetto di lavoro collegato può partire.

Il lavoro a lotti non c'è più ([ADR 0053](adr/0053-catena-degli-agenti.md)):
ogni pacchetto di riferimenti si avvia con il suo commento, anche molti nello
stesso momento, e il responsabile li rivede uno alla volta, come sopra.

## 4. Avviare l'agente

Il messaggio normale è sempre lo stesso: l'agente sceglie da solo il pacchetto
giusto con le regole di [AGENTS.md](../AGENTS.md) ("Se ti chiedono di prendere il
prossimo pacchetto").

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
lettera. Prendi il prossimo pacchetto della tappa 0.4 assegnato a te, secondo le
regole di AGENTS.md. Realizzalo, apri la pull request, porta la CI al
verde e fermati, come dice AGENTS.md.
```

Di norma però un lavoro si avvia con un commento nella sua issue, e la
revisione la chiede la catena: le frasi sono nella
[guida del responsabile](GUIDA-RESPONSABILE.md), A2.

Se serve far partire un pacchetto preciso, si indicano issue e ramo:

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
lettera. Realizza solo il pacchetto P5 della tappa 0.4 (issue #80), che è
assegnato a te. Lavora sul ramo p0.4-05-circle-command, partendo da main
aggiornato. Alla fine apri una pull request che chiuda la issue #80, porta la CI
al verde e fermati, come dice AGENTS.md.
```

Mai dare compiti generici ("continua il progetto", "sistema quello che serve"):
un agente senza pacchetto preciso sconfina.

## 5. Mentre lavorano

- Ogni agente lavora **solo sul suo ramo** e apre **una sola pull request**.
- Nessuno scrive su `main` o sui rami degli altri.
- Se un agente scopre che gli serve un file non previsto, si ferma e lo scrive
  nella issue: decide il responsabile.
- Se un agente scopre un bisogno che spetta a solid-space, si ferma e scrive
  nella issue il testo della richiesta (AGENTS.md, "Se serve qualcosa a
  solid-space"); il responsabile decide se aprirla e come cambia il piano.
- Se un agente trova un buco nel contratto di solid-space, si ferma prima di
  scrivere codice o casi, senza scegliere una risposta. Nella issue del suo
  pacchetto scrive il testo della issue `contratto` da aprire in solid-space:
  titolo con l'intestazione, che cosa manca, un esempio con i dati, le scelte
  possibili. La apre il responsabile, che decide; l'agente pubblica l'eventuale
  ramo senza pull request, toglie `AA-Presa in carico` e termina la sessione
  (AGENTS.md, "Se serve qualcosa a solid-space"). Nessun pacchetto che usa
  un'intestazione nominata nel titolo di una issue `contratto` aperta in
  solid-space si prende in carico. Prima della pull request, chi sposta il
  sottomodulo controlla in `docs/DECISIONI.md` di solid-space i cambiamenti alle
  intestazioni rispetto al commit di partenza della scheda: si adegua o si ferma.
- Se un agente trova un buco nella sua scheda, cioè in un'interfaccia di
  Tecnigrafo che intestazione, ADR e scheda non determinano mentre il contratto
  di solid-space sì (#359), si ferma allo stesso modo, ma scrive nella issue del
  suo pacchetto che cosa manca, un esempio e le scelte possibili, senza testo
  per solid-space. Decide il responsabile in [DECISIONI.md](DECISIONI.md), e la
  scheda si corregge prima di riprendere il pacchetto (AGENTS.md, "Se serve
  qualcosa a solid-space").
- Le decisioni sui buchi si applicano in una sola pull request di correzione,
  con in testa **Revisione della correzione: da fare**: ogni decisione entra
  nella scheda con il suo esempio e la risposta scelta, e prima del merge un
  agente diverso da chi l'ha scritta (di norma Codex) rilegge le sole regole
  nuove con la lista, più due domande (vale anche al contrario? che cosa
  succede al limite del calcolo?). Passi in
  [GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B10a; regole per chi scrive e
  chi rivede in AGENTS.md, "Le correzioni delle schede si rivedono".
- Quando un pacchetto di solid-space è unito, i pacchetti di Tecnigrafo che lo
  aspettavano possono partire: la sua pull request li elenca nella riga
  «Sblocca:», e il responsabile li sposta nel Project da *Bloccato* a *Da fare*.
- Se un agente ritiene sbagliato un riferimento, si ferma e lo scrive nella
  issue. La correzione è una nuova pull request di riferimenti, mai una modifica
  nella pull request di lavoro.
- Chi prende in carico una issue (per realizzarla) o una pull request già aperta
  (per rivederla, correggerla o risolvere un conflitto) le mette **per prima
  cosa** l'etichetta `AA-Presa in carico` con un commento che dice chi è, e la
  toglie quando ha finito. Una issue o una pull request con quell'etichetta non
  si tocca: ci sta già lavorando qualcun altro. I comandi sono in
  [AGENTS.md](../AGENTS.md).

## 6. Unire

- Il responsabile unisce **una pull request alla volta**, con *Squash and merge*,
  solo con la CI verde. Prima guarda la scheda **Files changed**: i file devono
  essere quelli del pacchetto, e una pull request di lavoro non deve toccare
  `tests/verifica/casi/`, `tests/verifica/sorgenti.json` né
  `tests/verifica/sorgenti/` (la CI lo controlla comunque).
- Dopo ogni merge, le altre pull request aperte mostrano il pulsante **Update
  branch**: si preme, la pull request si allinea a `main` e la CI riparte.
- Se due pacchetti toccano lo stesso file comune (per esempio
  `tests/CMakeLists.txt`), vale una regola fissa: **chi arriva dopo si aggiorna**.
  Si chiede all'agente di quella pull request di risolvere il conflitto.
- Una pull request scritta da un modello viene rivista da un modello diverso
  prima del merge, mai dallo stesso: la rivede l'agente che ha scritto i
  riferimenti del pacchetto, quindi Codex le pull request di Claude e Claude
  quelle di Codex ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B9). La
  decisione resta del responsabile.

## 7. Chiudere un pacchetto

Con il merge la issue si chiude da sola (`Closes #…`). Il responsabile toglie
l'etichetta `agente:…` solo se il pacchetto viene abbandonato senza pull request:
dopo 14 giorni senza notizie il pacchetto torna libero (CONTRIBUTING.md).

## 8. Chiudere la tappa

Quando tutte le issue del milestone sono chiuse, un agente (di norma Codex) apre
prima, se `tests/verifica/sorgenti/` ha dei file, la pull request di soli
riferimenti che li raccoglie in `sorgenti.json` (`scripts/sorgenti.py
raccogli`; [agenti/chiusura.md](agenti/chiusura.md)), e dopo il suo merge la
**pull request di chiusura**, sul ramo `docs/chiusura-X.Y`, che:

- spunta nel documento della tappa ogni criterio di chiusura, indicando per
  ciascuno i casi di verifica o i test che lo provano, oppure "da provare a mano";
- scrive le note di rilascio `docs/rilasci/X.Y.md`, con la tabella della prova
  manuale da compilare;
- controlla che ogni comando nuovo della tappa abbia icona e voci di menu,
  ribbon e barre (sezione 1, "Comandi nell'interfaccia");
- aggiorna il [catalogo di parità](parita/comandi.md): stato, note e riepilogo
  dei comandi toccati dalla tappa, e la versione in cima al riepilogo;
- aggiorna lo stato in [ROADMAP.md](ROADMAP.md) e nel README;
- porta `project(... VERSION X.Y.0)` del `CMakeLists.txt` principale alla
  versione nuova (`VERSION X.Y.Z` per una tappa con tre numeri);
- se la tappa gemella di solid-space ha appena pubblicato una versione, porta
  `third_party/solid-space` al suo tag, così ogni versione di Tecnigrafo punta a
  una versione pubblicata di solid-space ([DECISIONI.md](DECISIONI.md),
  2026-11-21).

Il responsabile esegue la prova manuale su Windows e Linux, compila la tabella,
unisce la pull request e pubblica la versione (regole in
[ROADMAP.md](ROADMAP.md), "Chiudere una tappa").

La pull request di chiusura non decide le regole di metodo né le idee: lo si fa
dopo, nella **pausa tra le tappe**. Con tutto fermo, il responsabile decide le idee
aperte, le proposte della issue «[X.Y] Proposte di metodo» e le correzioni dei
documenti, che si applicano in una pull request di soli documenti; solo dopo si
apre la tappa successiva ([GUIDA-RESPONSABILE.md](GUIDA-RESPONSABILE.md), B13).
