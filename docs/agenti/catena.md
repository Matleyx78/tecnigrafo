# Catena degli agenti

Da leggere ogni volta che lavori a una issue o a una pull request, comunque
tu sia stato avviato: da un commento su GitHub o dalla chat. Il passaggio
finale ("Fine del lavoro") vale nei due casi, perché anche chi parte dalla
chat lascia il lavoro alla catena ([DECISIONI.md](../DECISIONI.md),
2026-11-28). Da leggere anche quando il messaggio cita questo file, un giro di
revisione o una «Decisione del responsabile» ([AGENTS.md](../../AGENTS.md),
"Se ... leggi anche ...").
Vale insieme al nucleo e ai file del lavoro che fai: qui c'è solo **come il
lavoro passa da un agente all'altro** senza il responsabile
([ADR 0053](../adr/0053-catena-degli-agenti.md)).

Il responsabile avvia un lavoro con **un solo commento** nella issue; gli
agenti si passano il lavoro con commenti scritti dagli script. Lui torna solo
quando trova un'etichetta:

| Etichetta | Che cosa vuol dire | Chi la mette |
|---|---|---|
| `pronta` | lettura finale, DCO e merge | l'ultimo agente della catena |
| `decisione` | serve una sua decisione, che scrive **nella issue** | chi si ferma ("Fermarsi per una decisione") |
| `ferma` | la catena si è interrotta senza dirlo | la sentinella ("Sentinella") |

Non togliere mai `ferma` né `pronta`: le toglie il responsabile.

## Ruoli

Su ogni issue lavorano due agenti:

- l'**esecutore** è quello dell'etichetta `agente:…` della issue: realizza il
  lavoro e poi corregge;
- il **revisore** è **l'altro** (Claude se l'esecutore è Codex, e viceversa):
  rivede e non fa mai commit sulla pull request. **Non aspetta la CI** (niente
  `scripts/ci-attendi.sh`): la catena gli passa la pull request solo quando la
  CI è finita, verde per una revisione, rossa dopo tre tentativi per
  un'analisi ("Fine del lavoro").

Vale per pacchetti, riferimenti e revisioni delle interfacce. Il revisore
lavora sulla pull request indicata anche se la richiesta arriva nella issue
che la pull request chiude, e lì prende in carico, commenta e mette le
etichette: per lui non vale il punto 2 del nucleo ("Per prima cosa: prendi in
carico"), che riguarda le issue.

## Passaggi: solo con gli script

**Ogni chiocciola davanti al nome di un agente avvia una sessione**, anche in
un esempio o fra apici, perché tutti scrivono con l'account del responsabile.
Quindi non scriverla mai: i passaggi si fanno solo con lo script, che scrive il
testo giusto a nome del responsabile.

```sh
scripts/passa.sh <claude|codex> <numero della pull request> revisione <N>
scripts/passa.sh <claude|codex> <numero della pull request> correzioni <N>
scripts/passa.sh <claude|codex> <numero della pull request> analisi
```

Le richieste di revisione e di analisi per Codex lo script le scrive nella
issue collegata (quella di `Closes #N` o, se manca, di `Avvia dopo il merge:
#N`): su una pull request attiverebbero la sua revisione automatica.

Nella pull request e nei verbali scrivi modello e impegno con cui sei stato
avviato, come chiede il nucleo; se non li sai, «modello e impegno non noti».

## Fine del lavoro (esecutore)

Il lavoro **esiste solo su GitHub**: un commit rimasto nel tuo ambiente non lo
vede nessuno, e la catena si ferma senza dirlo. Pubblica sempre il ramo con
`git push` e apri tu la pull request con `gh pr create`, anche se il tuo
strumento propone di «preparare» la pull request in un altro modo. Se `git
push` o `gh pr create` falliscono, scrivi l'errore esatto in un commento nella
issue e fermati ("Fermarsi per una decisione").

Prima di terminare, come **ultima cosa**, dopo i passi del nucleo ("Quando
fermarsi"):

| Lavoro | Con la CI verde | Con la CI rossa dopo tre tentativi |
|---|---|---|
| pacchetto di lavoro (`[X.Y-PTN]`, `[FN]`) | `scripts/passa.sh <revisore> <numero> revisione 1` | `scripts/passa.sh <revisore> <numero> analisi` |
| correzione delle schede ("Ripresa dopo una decisione") | `scripts/passa.sh <revisore> <numero> revisione 1` | come sopra |
| riferimenti (`[X.Y-RTN]`) | etichetta `pronta` sulla pull request: li rivede il responsabile ([guida](../GUIDA-RESPONSABILE.md), B7) | come sopra |
| revisione delle interfacce | — (verbale nella issue, senza pull request): almeno una [decisione], "Fermarsi per una decisione"; altrimenti `pronta` sulla issue | — |
| chiusura della tappa | `pronta` sulla pull request | come sopra |

Una pull request di **riferimenti** porta nella descrizione la riga
`Avvia dopo il merge: #<issue del lavoro>`: al merge il pacchetto parte da
solo ("Dopo il merge").

## Fermarsi per una decisione

Chiunque tu sia, ti fermi per una decisione in tutti i casi in cui i file del
tuo lavoro dicono di fermarsi e scrivere nella issue o nella pull request: un
caso di riferimento che sembra sbagliato ([pacchetto.md](pacchetto.md)), un
buco visibile nella scheda (lo stesso file), un buco nel contratto o un bisogno
verso solid-space ([contratto.md](contratto.md)), un riferimento che il
programma contraddice ([riferimenti.md](riferimenti.md)), una pull request da
rivedere scritta con il tuo stesso modello ([revisione.md](revisione.md)). In
revisione ti fermi anche per una segnalazione **[decisione]** o al giro 3 con
delle [bloccante] ancora aperte.

**Prima lo smistamento.** Se ti fermi per almeno una segnalazione
**[decisione]** (un revisore, o un esecutore su un buco della scheda), non
mettere ancora `decisione`: scrivi il tuo commento con le [decisione] come
sotto, togli `AA-Presa in carico` e passa all'altro agente con
`scripts/passa.sh <altro> <numero> smistamento` ("Smistamento"). Vai diretto
ai punti qui sotto solo se lo smistamento c'è già stato sullo stesso punto, o
se ti fermi per un altro motivo (un caso di riferimento sbagliato, il
contratto, il giro 3, il tuo stesso modello). Allora, oltre a ciò che chiede il
file del tuo lavoro:

1. aggiungi l'etichetta `decisione` alla issue e, se c'è, alla pull request;
2. scrivi **un solo** commento (nella pull request se c'è, altrimenti nella
   issue) che comincia con «Serve una decisione del responsabile», con il
   problema, le scelte possibili e, come ultima riga, quella da copiare,
   **senza la chiocciola**, e **dove** scriverla. Se proponi una correzione
   concreta (un dato, un valore, un caso riscritto), la riga la contiene già
   scritta per intero, così chi corregge applica il dato esatto invece di
   interpretarlo. Se chiede di cambiare dei file, la riga finisce sempre con
   «Leggi AGENTS.md e docs/agenti/catena.md e seguili alla lettera. Il lavoro
   esiste solo su GitHub: pubblica il ramo con git push e apri tu la pull
   request con gh pr create» (per una pull request già aperta: «pubblica i
   commit con git push sullo stesso ramo»): una riga che porta solo il dato
   lascia l'agente senza le regole della catena, e il lavoro resta nel suo
   ambiente (solid-space, #247):
   «Per riprendere scrivi **nella issue #<numero>**, con @ davanti:
   `<nome> Decisione del responsabile: …`». Il posto è sempre una **issue**:
   su una pull request la menzione di Codex avvia la sua revisione
   automatica. Nome e issue sono di norma i tuoi; per un caso di riferimento
   sbagliato sono quelli di chi ha scritto i riferimenti e della sua issue
   `[X.Y-RTN]`, perché chi realizza un pacchetto non ne tocca i casi. Quella
   issue è già chiusa, perché i riferimenti sono uniti: la riga comincia con
   «Riapri la issue #<numero> e scrivi lì, con @ davanti:» e chiede una pull
   request di soli riferimenti con `Closes #<numero>` e
   `Avvia dopo il merge: #<issue del pacchetto>`. Senza `Closes` la issue dei
   riferimenti resta aperta e il pacchetto, che la aspetta nella sua riga
   «Riferimenti», non riparte ("Dopo il merge");
3. se il commento è nella pull request, scrivi nella issue «Serve una
   decisione del responsabile: vedi la pull request #<numero>»;
4. togli `AA-Presa in carico` e termina, senza aspettare la CI.

Per un buco nel contratto o un bisogno verso solid-space la issue in
solid-space la apre il responsabile (nucleo, "Dove puoi scrivere"): tu scrivi
solo il testo, come dice [contratto.md](contratto.md).

### Fermate senza decisione

Due fermate non chiedono niente al responsabile, quindi **non** mettono
`decisione` ([DECISIONI.md](../DECISIONI.md), 2026-11-19):

- **CI da approvare:** `scripts/ci-attendi.sh` esce con 6, dopo aver scritto
  una volta «CI da approvare» nella pull request. Basta che il responsabile
  approvi l'esecuzione: scrivi nella pull request che ti fermi per questo,
  togli `AA-Presa in carico` e termina. Riprende chi viene avviato di nuovo;
- **main rosso:** la tua CI fallisce solo nei casi elencati nella issue
  «main rosso» aperta (la apre e la chiude da sola
  `.github/workflows/main-rosso.yml`). È il motivo estraneo del nucleo:
  scrivilo nella pull request con il rimando alla issue, non proporre una
  correzione di `main`, togli `AA-Presa in carico` e termina. Riprende chi
  aggiorna la pull request con `main` dopo la correzione.

## Ripresa dopo una decisione

Il responsabile riprende con un commento nella issue che comincia con la
menzione del tuo nome e `Decisione del responsabile:`. Se ti eri fermato in
una pull request lavori lì: la issue è solo il posto da cui riparti. Prendi in
carico, togli `decisione` dalla issue e dalla pull request, e poi:

- **se eri l'esecutore**: continua applicando la decisione, fino alla fine del
  lavoro (o a una nuova decisione). Se la decisione riguarda un **buco della
  scheda**, prima la applichi in **una** pull request di correzione come dice
  la [guida](../GUIDA-RESPONSABILE.md), B10a, passo 3 (scheda, `docs/DECISIONI.md`,
  ADR se serve), con in testa **Revisione della correzione: da fare**, senza
  `Closes` e con `Avvia dopo il merge: #<issue del pacchetto>`, e la passi in
  revisione: il pacchetto riparte da solo dopo il merge. Lo stesso per le
  decisioni di una **revisione delle interfacce**, con `Closes #<issue della
  revisione>`;
- **se hai scritto i riferimenti** e la decisione dice che un tuo caso è
  sbagliato: correggilo in una pull request di soli riferimenti con
  `Closes #<issue dei riferimenti>` (riaperta dal responsabile) e
  `Avvia dopo il merge: #<issue del pacchetto>`, come in "Fine del lavoro".
  Prima di aprirla ricontrolla il caso corretto con la scheda, compreso ogni
  passo che segue dall'estremità o dallo stato nuovi: la correzione deve
  togliere il motivo indicato da chi si è fermato, non solo cambiare i dati;
- **se eri il revisore**: scrivi **un solo** commento che comincia con
  «Ripresa dopo la decisione (giro N)», dove N è il giro che si era fermato
  (senza scrivere «Giro di revisione:»: non è un giro nuovo), con ogni modifica
  che la decisione chiede come **[bloccante]** e le [bloccante] di quel giro
  ancora aperte. Se ce n'è almeno una:
  `scripts/passa.sh <esecutore> <numero> correzioni <N>`; altrimenti
  l'etichetta `pronta`. Togli `AA-Presa in carico` e fermati.

## Revisione (revisore)

Prendi in carico la pull request e lascia **un solo commento** che comincia
con «Revisione di <nome dell'agente>», con la riga «Giro di revisione: N». N è
il numero dei commenti della pull request che contengono «Giro di revisione:»,
più uno. Dal giro 2 rivedi **solo i commit di correzione** e le segnalazioni
del giro prima. Che cosa blocca e che cosa no lo dice
[revisione.md](revisione.md); ogni segnalazione comincia con la sua classe:

- **[bloccante]**: un errore vero o una regola persa: la issue o la scheda non
  è soddisfatta, un test manca o è sbagliato, una regola dei documenti degli
  agenti è violata. La risposta è già nella scheda o nelle regole;
- **[decisione]**: un buco visibile che nessuna regola esistente decide
  ([pacchetto.md](pacchetto.md), "Buchi visibili e no");
- **[facoltativa]**: tutto il resto, compresi i dubbi di forma, le fasce di una
  soglia e gli ingressi fuori dall'uso reale, che revisione.md chiama «non
  bloccanti».

Alla fine, in quest'ordine:

- almeno una [decisione]: lo smistamento di "Fermarsi per una decisione",
  con `scripts/passa.sh <esecutore> <numero> smistamento`; [bloccante] al
  giro 3 (scrivi «Fermato dopo tre giri»): "Fermarsi per una decisione";
- [bloccante] ai giri 1 o 2: `scripts/passa.sh <esecutore> <numero>
  correzioni <N>`;
- altrimenti (solo [facoltativa] o nessuna segnalazione): etichetta `pronta`.

Poi togli `AA-Presa in carico` e fermati. Non fare commit.

Per una pull request con **Revisione della correzione: da fare** il verbale è
quello di [revisione.md](revisione.md), "Le correzioni delle schede si
rivedono", con le stesse classi: un buco nuovo visibile è una [decisione], e
non lo scegli tu.

## Smistamento

Chi riceve uno smistamento (`scripts/passa.sh … smistamento`) è l'altro
agente: prende in carico, legge le segnalazioni [decisione] dell'ultimo
verbale o commento di arresto e controlla ognuna contro le regole esistenti
(la scheda, gli ADR, [DECISIONI.md](../DECISIONI.md),
[REGOLE-NUMERICHE.md](../REGOLE-NUMERICHE.md),
[revisione.md](revisione.md)). Non cambia file e non fa commit. Lascia **un
solo** commento che comincia con «Smistamento di <nome dell'agente>»:

- una [decisione] che una regola esistente decide diventa **[bloccante]**, con
  la regola citata e l'esempio risolto;
- solo quelle che nessuna regola decide restano **[decisione]**, con la riga
  da copiare come in "Fermarsi per una decisione", punto 2.

Poi, in quest'ordine:

- almeno una [decisione] rimasta: "Fermarsi per una decisione", punti 1–4,
  con il tuo commento come quello del punto 2;
- altrimenti, su una pull request rivista: `scripts/passa.sh <esecutore>
  <numero> correzioni <N>`, dove N è il giro che si era fermato;
- altrimenti, per un esecutore fermo su un buco (nella issue, senza pull
  request): `scripts/avvia.sh <issue> "Smistamento: applica le regole del
  commento «Smistamento di <nome>»."`, che lo fa ripartire.

Togli `AA-Presa in carico` e fermati. Il giro successivo del revisore
controlla come la regola è stata applicata, quindi la revisione resta
dell'altro agente; se il revisore ripropone lo stesso punto, va diretto al
responsabile, senza un secondo smistamento ([DECISIONI.md](../DECISIONI.md),
2026-11-28).

## Analisi di una CI rossa (revisore)

Prendi in carico, leggi gli errori con `scripts/ci-errori.sh` e il codice,
lascia **un solo commento** che comincia con «Analisi di <nome dell'agente>»
con la causa probabile e la correzione che suggerisci. Non fare commit. Poi:

- **Caso diretto.** Se causa e correzione stanno **solo nei file del
  pacchetto** (codice, test, autoprove), la correzione **non** cambia una
  scheda, un ADR, un contratto, un caso di riferimento o una regola, e la strada
  è **una sola**, scrivi nell'analisi una riga con il motivo per cui è un caso
  diretto, togli l'etichetta e, come ultima cosa,
  `scripts/passa.sh <esecutore> <numero> correzioni <N>`, **senza
  `decisione`** ([DECISIONI.md](../DECISIONI.md), 2026-11-19).
- **Altrimenti fermati** come in "Fermarsi per una decisione", punti 1, 3 e 4:
  se l'analisi tocca uno di quegli elementi, se le strade sono più di una, e
  **sempre alla seconda analisi** sulla stessa pull request, segno che il
  pacchetto non converge. L'ultima riga del commento è quella da copiare per
  l'**esecutore**, nella issue collegata alla pull request (`Closes #N` o
  `Avvia dopo il merge: #N`) («`<esecutore> Decisione del responsabile: applica
  l'analisi della pull request #<numero>`», con la frase finale di "Fermarsi
  per una decisione", punto 2, nella forma per una pull request già aperta),
  perché decidere se seguirla spetta al responsabile.

## Correzioni (esecutore)

Prendi in carico, lavora sul ramo della pull request
(`gh pr checkout <numero>`), correggi solo le **[bloccante]** del giro
indicato e, se c'è, del commento «Ripresa dopo la decisione» di quel giro (le
[facoltativa] e le [decisione] non si toccano). Pubblica i commit con `git
push` sullo stesso ramo ("Fine del lavoro"), aspetta la CI come dice il
nucleo, rispondi con un commento che dice che cosa hai corretto, togli
l'etichetta e, come ultima cosa,
`scripts/passa.sh <revisore> <numero> revisione <N+1>`.

## Dopo il merge

Al merge di una pull request, il workflow *Dopo il merge*
(`.github/workflows/dopo-il-merge.yml`) legge le righe
`Avvia dopo il merge: #N` della descrizione e avvia la issue N con il suo
agente (`scripts/avvia.sh`), se non aspetta ancora un'altra issue della sua
riga "Dipende da" o "Riferimenti". Conta solo una **riga a sé**, che comincia
con quelle parole (al più dopo un trattino di elenco) e porta una sola issue:
le stesse parole dentro una frase sono una citazione e non avviano niente, e
le righe dentro un blocco di codice (fra tre backtick o tre tilde) sono esempi
e non contano, nemmeno per `Closes #N` (`scripts/catena.py`). L'avvio non toglie `decisione`: la toglie l'agente
quando prende in carico.

Poi avvia anche le **issue che il merge libera** (`scripts/liberati.py`):
quelle aperte con un'etichetta `agente:claude` o `agente:codex` che hanno fra
le issue delle righe "Dipende da" o "Riferimenti" una che la pull request
chiude (`Closes #N`), e ora le hanno tutte chiuse, se non sono prese in
carico, non hanno un commento di avvio e nessuna pull request aperta le
nomina. Le catene aperte (pull request aperte non ancora `pronta` e issue
prese in carico senza pull request) sono **al più 8**: quelle liberate oltre
il limite il workflow le scrive in un commento della pull request unita, e si
avviano a mano con `scripts/avvia.sh` ([DECISIONI.md](../DECISIONI.md),
2026-11-28). Ogni agente legge l'AGENTS.md del suo
ramo: una pull request nata prima di un cambio di queste regole va aggiornata
su `main` prima di entrare nella catena.

## Sentinella

Ogni 30 minuti il workflow *Sentinella* (`.github/workflows/sentinella.yml`)
mette `ferma`, con un commento, sulle issue e pull request della catena
senza movimento da almeno 60 minuti che non portano `pronta` né `decisione`, e
sulle decisioni scritte dal responsabile e mai riprese (per esempio per la
quota di Codex). Una pull request e le issue collegate (`Closes #N`, la riga
`Avvia dopo il merge: #N`) sono una catena sola: i passaggi a Codex scritti
nella issue contano per la pull request. Lascia stare le pull request con la
CI ancora in corso e segnala `AA-Presa in carico` rimasta.

Prima di mettere `ferma` su una pull request, guarda se il passo successivo è
**uno solo e certo**: CI dell'ultimo commit verde, `AA-Presa in carico`
tolta, nessuna `decisione`, nessun verbale, analisi o passaggio in revisione
già scritti. Allora il passo lo fa lei, e lo scrive in un commento
«Passaggio fatto dalla sentinella»: per un pacchetto (`[X.Y-PTN]`, `[FN]`) o
una correzione delle schede, `scripts/passa.sh <revisore> <numero> revisione
1`, dove il revisore è l'altro agente della issue collegata; per i
riferimenti (`[X.Y-RTN]`), l'etichetta `pronta`. Negli altri casi, o se il
passaggio non riesce, resta `ferma` ([DECISIONI.md](../DECISIONI.md),
2026-11-28). È una rete di sicurezza: il passaggio finale resta compito tuo
("Fine del lavoro"). Come avvio o
decisione conta solo un commento dell'account del responsabile, che è anche
quello degli agenti; i commenti di altri account non contano.
