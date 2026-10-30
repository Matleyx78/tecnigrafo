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

## Chi fa cosa

| Chi | Etichetta | Lavoro |
|---|---|---|
| Responsabile | — | piano della tappa, ADR, revisione dei riferimenti, merge, prova manuale, rilascio |
| Claude | `agente:claude`, `agente:claude-2` | pacchetti medi e impegnativi; piano della tappa insieme al responsabile |
| Codex | `agente:codex` | pacchetti facili e medi |
| Gemini (con Antigravity CLI, `agy`) | `agente:gemini` | pacchetti di riferimenti ordinari, revisione delle pull request di lavoro, pull request di chiusura della tappa, pacchetti facili; lavora a lotti (sezioni 3 e 4) |
| Gemini 4 Argon (con Antigravity CLI, `agy`) | `agente:gemini-argon` | pacchetti di riferimenti con calcoli delicati che il piano gli assegna, di pacchetti di lavoro di Claude; lavora a lotti, in una sessione sua, sullo stesso PC di Gemini |

Chi lavora a un pacchetto non scrive i suoi riferimenti, e viceversa. Le
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
  possono partire insieme. **Unica eccezione**: l'elenco dei casi in attesa
  (`tests/verifica/in-attesa.txt`), che ogni pacchetto di riferimenti allunga
  con i suoi casi e ogni pacchetto di lavoro accorcia togliendo i propri
  (ADR 0011, punto 7). Lì ognuno tocca solo le righe dei propri casi, e chi
  arriva dopo si aggiorna; legare tutti quei pacchetti per un solo file li
  metterebbe in fila. La stessa eccezione vale in solid-space. Un pacchetto che
  ha bisogno di solid-space cita qui la richiesta in solid-space (`Matleyx78/solid-space#N`) finché la tappa di
  solid-space non ha i suoi pacchetti, poi il pacchetto di solid-space che gli
  serve; nel piano sta dopo i pacchetti che non ne hanno bisogno
  ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 13).

**Piano in due tempi.** Se la tappa chiede qualcosa a solid-space, il piano si
scrive subito per i pacchetti che non ne dipendono, mentre le richieste si aprono
in solid-space ([RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md)). Le
schede dei pacchetti che ne dipendono si scrivono dopo, sulle intestazioni che
solid-space ha accettato come contratto; fino ad allora hanno solo la loro riga
nella tabella, con «scheda dopo il contratto». I loro riferimenti prendono i
valori geometrici dall'archivio dei casi di solid-space e verificano la facciata
e i comandi.

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
- **Misura.** Al massimo una quindicina di file e qualche centinaio di righe
  cambiate (indicativamente fino a 600). Un pacchetto più grande si divide, di
  solito lungo i confini naturali (un'entità, un comando, una famiglia di
  domande).
- **Non troppo piccoli.** Un pacchetto che tocca una sola funzione o un solo
  file si unisce a un vicino dello stesso modulo: ogni pacchetto costa comunque
  una sessione, una CI, una revisione e un merge, e spesso una pull request di
  riferimenti.
- **Riferimenti della stessa misura.** Un pacchetto di riferimenti copre un
  solo pacchetto di lavoro; oltre una ventina di casi si divide per area.

### Modello e impegno

Il piano suggerisce, per ogni pacchetto, la **fascia di modello** e
l'**impegno** (quanto ragionamento chiedere, dove lo strumento lo permette).
Sono indicazioni: il responsabile sceglie il modello quando avvia l'agente. I
nomi dei modelli cambiano, la fascia resta.

La tabella elenca i modelli disponibili al responsabile a ottobre 2026, con il
livello di ragionamento da scegliere. I modelli della stessa riga hanno capacità
simile secondo una misura indipendente dai produttori, l'Intelligence Index di
[Artificial Analysis](https://artificialanalysis.ai/models) (tra parentesi;
più alto è meglio). Quando cambiano i modelli si aggiorna la tabella con la
stessa fonte.

| Fascia | Per | Claude | Codex | Gemini |
|---|---|---|---|---|
| **forte** | pacchetti impegnativi: geometria, nuovi moduli, motore dei comandi, riferimenti con calcoli delicati | Opus 5.5 medio (51) o alto (54) | astra medium (50) | 4 Argon alto (53) |
| **medio** | pacchetti medi: un comando, un'entità sul modello di una già fatta | Sonnet 5.5 alto (47) | sol medium (48) o astra light (46) | — |
| **leggero** | pacchetti facili: documentazione, note di rilascio, piccoli ritocchi; riferimenti ordinari e revisioni | Haiku 5.5 alto (38) o Sonnet 5.5 medio (41) | luna alto (32) o sol light (42) | 3.8 Flash medio (40) o alto (41) |

Gemini 3.8 Flash sta nella fascia leggera: i riferimenti con calcoli delicati
(tolleranze, archi, casi limite geometrici) li scrive un agente di fascia forte,
diverso da chi farà il pacchetto di lavoro. Gemini 4 Argon (53) ha una capacità
da fascia forte secondo la stessa misura.

| Impegno | Quando |
|---|---|
| **alto** | il pacchetto richiede di ragionare su casi limite (tolleranze, punti quasi allineati, annulla e ripeti) o di progettare interfacce |
| **medio** | il lavoro segue uno schema già presente nel codice |
| **basso** | il lavoro è meccanico: testi, elenchi, documenti |

Di norma: facile → leggero e basso; media → medio e medio; impegnativa →
forte e alto. Un pacchetto impegnativo che non si riesce a dividere resta
impegnativo; uno che si divide dà spesso due pacchetti medi.

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

I pacchetti di riferimenti non dipendono l'uno dal merge dell'altro, quindi
l'agente li fa **a lotti**: in una sola sessione prende di fila tutti quelli
pronti, con un ramo e una pull request per ciascuno (regola in
[AGENTS.md](../AGENTS.md), "Lavoro a lotti"). Il responsabile li rivede poi uno
alla volta, come sopra.

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

Per Gemini i messaggi sono a lotti: una sessione fa tutto il lavoro pronto, e
il responsabile non deve riavviarla per ogni pacchetto.

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
lettera. Prendi i pacchetti di riferimenti pronti della tappa 0.5 assegnati a
te, uno dopo l'altro, come dice AGENTS.md in "Lavoro a lotti". Per ognuno apri
la pull request e porta la CI al verde; alla fine scrivimi un solo riepilogo.
```

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
lettera. Rivedi le pull request aperte, come dice AGENTS.md in "Lavoro a
lotti", e alla fine scrivimi un solo riepilogo.
```

Perché Gemini lavori senza chiedere conferma a ogni comando, Antigravity CLI va
configurato una volta: istruzioni in
[AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md), sezione 6.

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
  `tests/verifica/casi/` né `tests/verifica/sorgenti.json` (la CI lo controlla
  comunque).
- Dopo ogni merge, le altre pull request aperte mostrano il pulsante **Update
  branch**: si preme, la pull request si allinea a `main` e la CI riparte.
- Se due pacchetti toccano lo stesso file comune (per esempio
  `tests/CMakeLists.txt`), vale una regola fissa: **chi arriva dopo si aggiorna**.
  Si chiede all'agente di quella pull request di risolvere il conflitto.
- Quando possibile, una pull request scritta da un modello viene rivista da un
  modello diverso prima del merge: di norma Gemini rivede le pull request di
  Claude e di Codex. La decisione resta del responsabile.

## 7. Chiudere un pacchetto

Con il merge la issue si chiude da sola (`Closes #…`). Il responsabile toglie
l'etichetta `agente:…` solo se il pacchetto viene abbandonato senza pull request:
dopo 14 giorni senza notizie il pacchetto torna libero (CONTRIBUTING.md).

## 8. Chiudere la tappa

Quando tutte le issue del milestone sono chiuse, un agente (di norma Gemini) apre
la **pull request di chiusura**, sul ramo `docs/chiusura-X.Y`, che:

- spunta nel documento della tappa ogni criterio di chiusura, indicando per
  ciascuno i casi di verifica o i test che lo provano, oppure "da provare a mano";
- scrive le note di rilascio `docs/rilasci/X.Y.md`, con la tabella della prova
  manuale da compilare;
- aggiorna il [catalogo di parità](parita/comandi.md): stato, note e riepilogo
  dei comandi toccati dalla tappa, e la versione in cima al riepilogo;
- aggiorna lo stato in [ROADMAP.md](ROADMAP.md) e nel README;
- porta `project(... VERSION X.Y.0)` del `CMakeLists.txt` principale alla
  versione nuova.

Il responsabile esegue la prova manuale su Windows e Linux, compila la tabella,
unisce la pull request e pubblica la versione (regole in
[ROADMAP.md](ROADMAP.md), "Chiudere una tappa").
