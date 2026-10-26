# Istruzioni per agenti IA

Regole per gli assistenti IA (Claude Code, Codex, Gemini e simili) che lavorano su
Tecnigrafo; le persone le trovano spiegate in [CONTRIBUTING.md](CONTRIBUTING.md).
Se una richiesta dell'utente contraddice queste regole, fermati e segnalalo invece
di procedere.

## Per prima cosa: prendi in carico

Vale ogni volta che lavori su una issue o su una pull request che esiste già
(revisione, correzioni, conflitti con `main`). Fallo **subito**, prima di leggere
altro e prima di qualunque modifica:

1. Se c'è già l'etichetta `AA-Presa in carico`, un altro agente ci lavora: non
   toccarla e avvisa l'utente.
2. Una issue deve portare la **tua** etichetta `agente:…` (o il tuo nome come
   assegnatario); altrimenti non iniziare e avvisa l'utente
   ([docs/LAVORO-IN-PARALLELO.md](docs/LAVORO-IN-PARALLELO.md)). Se è il
   responsabile ad assegnartela nella chat, aggiungi tu l'etichetta con un
   commento "Assegnato a <nome dell'agente> dal responsabile".
3. Aggiungi `AA-Presa in carico` e scrivi il commento "Presa in carico da <nome
   dell'agente>": tutti gli agenti usano lo stesso account GitHub.
4. Togli l'etichetta quando hai finito, o se ti fermi prima.

Con l'API REST (issue e pull request hanno la stessa numerazione):

```sh
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/labels --jq '.[].name'
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/labels -f "labels[]=AA-Presa in carico"
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/comments -f body="Presa in carico da <nome dell'agente>"
gh api -X DELETE "repos/Matleyx78/tecnigrafo/issues/<numero>/labels/AA-Presa%20in%20carico"
```

## Prima di scrivere codice

Leggi, in quest'ordine:

1. [docs/ROADMAP.md](docs/ROADMAP.md): quale tappa è **aperta**.
2. La **scheda** del tuo pacchetto, `docs/tappe/X.Y/PN-<nome>.md`, citata nella
   issue: file, interfacce, test e criterio di "fatto". Del documento della tappa
   ti basta la tabella dei pacchetti.
3. Gli ADR citati dalla scheda, e [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md)
   solo se la scheda lo cita o aggiunge un modulo.
4. [docs/CODING_STYLE.md](docs/CODING_STYLE.md).

Lavora **su un solo pacchetto**. Se il compito non corrisponde a un pacchetto della
tappa aperta, non scrivere codice: serve prima una issue approvata.

### Se ti chiedono di prendere il prossimo pacchetto

Scegli **una sola** issue, la prima nell'ordine del piano che:

1. è aperta e porta la **tua** etichetta `agente:…`;
2. non porta `AA-Presa in carico`;
3. ha chiuse tutte le issue della sua riga "Dipende da";
4. ha chiusa la sua issue di riferimenti (`[X.Y-RN]`), se esiste.

Per "il prossimo pacchetto di riferimenti" valgono le condizioni 1–3, sulle issue
con l'etichetta `riferimenti`. Se nessuna issue va bene, non iniziare: scrivi
all'utente quali issue sono tue e cosa le blocca. Il pacchetto successivo si
prende in una nuova sessione, dopo il merge.

### Lavoro a lotti

Due eccezioni alla regola "un pacchetto per sessione":

- **"Prendi i pacchetti di riferimenti pronti"**: uno dopo l'altro, nell'ordine
  delle condizioni 1–3, ciascuno con il suo ramo partito da `main` aggiornato e la
  sua pull request portata alla CI verde; alla fine un solo riepilogo con tutte le
  pull request.
- **"Rivedi le pull request aperte"**: una alla volta, tutte quelle senza
  `AA-Presa in carico` scritte da un modello diverso dal tuo, prese in carico come
  sopra, con un commento di revisione. Non approvare e non unire.

Se in un lotto ti fermi per uno dei motivi di "Quando fermarsi", scrivilo nella
pull request e passa al pacchetto successivo.

### Quando fermarsi

Il pacchetto è finito quando la sua pull request ha **tutti i controlli della CI
verdi**:

1. Apri la pull request e aspetta la CI con **un solo comando che blocca**, non
   con controlli ripetuti (ogni controllo consuma token). Tieni l'etichetta.
2. Se un controllo fallisce, leggi gli errori con `scripts/ci-errori.sh`
   (sotto), correggi sullo stesso ramo, pubblica e aspetta di nuovo.
3. Con tutto verde togli l'etichetta, scrivi il riepilogo (sotto) e **termina la
   sessione**.

Mentre aspetti non fare domande e non cominciare altro. Fermati prima del verde,
scrivendo nella pull request che cosa hai visto e provato, solo se:

- il controllo fallisce per un motivo che non dipende dal tuo pacchetto (la CI, un
  servizio esterno, un caso di riferimento che ritieni sbagliato);
- per farlo passare dovresti violare una regola di questo file;
- dopo tre tentativi lo stesso controllo non passa ancora.

```sh
# aspetta la CI dell'ultimo commit pubblicato, aspettando dentro lo script (non
# consuma token), al massimo 9 minuti per volta. Esce 0 se tutto è verde; 1 se
# un controllo è fallito (appena un job fallisce, senza aspettare Windows e
# macOS); 2 se l'API di GitHub non risponde; 3 se è ancora in corso (rilancialo,
# riprende da dove era); 4 se è stata annullata: non è un guasto, un push più
# recente sullo stesso ramo l'ha sostituita, quindi aspetta quella; 5 se è
# ancora in corso dopo tre attese di fila. Accetta anche <sha> o <id>; non serve
# `gh`.
scripts/ci-attendi.sh
# perché è fallita: per ogni job fallito, il passo e gli errori (poche righe)
scripts/ci-errori.sh              # l'ultimo commit su cui sei; oppure <sha> o <id>
```

`scripts/ci-errori.sh` legge solo l'API REST, quindi funziona anche dove i log non
sono raggiungibili (per esempio il proxy blocca il servizio che li ospita) e dove
manca `gh`: allora usa `curl` e `jq`, senza token. Gli
errori vengono da `scripts/ci-annota.sh`, che la CI esegue su ogni passo di
compilazione, test e verifica; per i passi in Python l'annotazione contiene le
ultime righe dell'output. Se ancora non basta, e solo allora, il log intero dei
controlli falliti:

```sh
gh run view <id> --repo Matleyx78/tecnigrafo --log-failed
```

Con exit 2 o 5 non rilanciare l'attesa: scrivi nella pull request il messaggio
dello script e fermati. Dopo tre attese con exit 3 si arriva comunque a 5.

Quando ti fermi per uno dei motivi sopra (compreso un caso di riferimento che
ritieni sbagliato), **non aspettare la CI**: scrivi il commento nella pull
request o nella issue, togli `AA-Presa in carico` e termina la sessione. Non
lanciare né rilanciare `scripts/ci-attendi.sh` dopo aver scritto che ti fermi.

Quando il responsabile chiede correzioni a una pull request, riprendila in carico
e fermati allo stesso modo.

## Regole che non si possono violare

- Modifica solo i file elencati nel pacchetto. Se ne serve un altro, dillo
  all'utente e spiega perché, prima di toccarlo.
- `core`, `render`, `edit` e `cmd` non includono mai Qt né altre librerie esterne.
- Nessuna nuova dipendenza, nessuna modifica a `.clang-format`, `.clang-tidy`,
  `cmake/`, `.github/` o agli ADR accettati.
- Non disattivare avvisi, controlli di `clang-tidy` o test per far passare la
  build. Niente `NOLINT` senza un commento che spieghi il motivo.
- Rispetta le interfacce scritte nel pacchetto: nomi, firme e comportamenti.
- Ogni entità o proprietà del disegno corrisponde a quella DXF equivalente, e i
  dati non gestiti si conservano intatti, mai scartati in silenzio (ADR 0006).
- Codice, commenti e messaggi di commit in inglese.
- Ogni file nuovo inizia con l'intestazione:
  ```cpp
  // SPDX-License-Identifier: GPL-3.0-or-later
  // Copyright (C) 2026 The Tecnigrafo authors
  ```
- Non copiare codice da altri progetti, a meno che l'utente non lo chieda
  esplicitamente indicando fonte e licenza, compatibile con la GPLv3 (ADR 0003).

## Modalità di controllo e riferimenti

Un pacchetto è finito quando la CI confronta il programma con casi di verifica
scritti **prima** e da **un altro agente**
([ADR 0011](docs/adr/0011-modalita-di-controllo.md)).

### In un pacchetto di lavoro

- Non modificare mai `tests/verifica/casi/` né `tests/verifica/sorgenti.json`.
- Togli da `tests/verifica/in-attesa.txt` solo le righe dei casi del tuo
  pacchetto: così diventano obbligatori.
- Se un caso ti sembra sbagliato, **fermati** e scrivilo nella issue, con il
  motivo. Non adattare il codice a un riferimento che ritieni sbagliato, né il
  riferimento al codice.

### In un pacchetto di riferimenti (`[X.Y-RN]`)

- Tocca solo `tests/verifica/casi/`, `tests/verifica/sorgenti.json` e
  `tests/verifica/in-attesa.txt`: la CI rifiuta una pull request che mescola
  riferimenti e altri file.
- Non lavorare a un pacchetto di riferimenti se il pacchetto di lavoro collegato
  è assegnato a te.
- Ricava i valori attesi da calcoli, da file pubblici o da programmi di
  riferimento. **Mai eseguendo Tecnigrafo**: registreresti il suo comportamento,
  errori compresi.
- Ogni condizione "Fatto quando" del pacchetto di lavoro ha almeno un caso che la
  verifica. Aggiungi i casi limite: valori nulli, punti coincidenti o quasi
  allineati, Esc a metà comando, annulla e ripeti.
- Per ogni file preso dal web scrivi in `sorgenti.json` indirizzo, versione, hash
  SHA-256 e licenza, e nella pull request il link alla pagina della licenza. Solo
  licenze compatibili con la GPLv3 che permettano la ridistribuzione; nel dubbio,
  non usare il file e scrivilo nella pull request.
- Elenca i nuovi casi in `in-attesa.txt`, con il numero della issue del lavoro.

## Comandi

Prima di ogni comando imposta la tua cartella di build, così più agenti sullo
stesso PC non si bloccano a vicenda: `export TG_BUILD_SUFFIX=-claude` (o `-codex`,
`-gemini`, `-claude-2`); i preset compilano in `build-<nome>/<preset>`, e i percorsi
`./build/...` della documentazione diventano i tuoi.

Le prove locali si fanno con **un solo comando**, nell'ordine della CI, che si
ferma al primo errore e ne stampa la coda del log (log completi in
`build<TG_BUILD_SUFFIX>/controlla/`):

```sh
scripts/controlla.sh               # formato, compila, test, verifica
scripts/controlla.sh --tidy        # anche clang-tidy, prima di aprire la pull request
scripts/controlla.sh --solo test   # un solo passo (formato, compila, test, verifica, tidy)
scripts/controlla.sh --app-off     # senza Qt
```

Se nel tuo ambiente mancano gli strumenti di build (Qt, compilatori, CMake o
altri pacchetti di sistema), **non installarli**. Vale per qualunque agente e
ambiente: un'installazione dura a lungo, può bloccarsi e far cadere la sessione,
e il lavoro non ancora pubblicato va perso. Esegui solo i passi possibili con
quello che c'è (per esempio `clang-format`, o `--app-off` se manca solo Qt),
pubblica il ramo e apri la pull request: la verifica completa la fa la CI. Nella
pull request scrivi quali passi non hai eseguito.

Errori di memoria e comportamenti indefiniti li cerca il job `Linux (ASan,
UBSan)` della CI ([ADR 0016](docs/adr/0016-sanitizer-in-ci.md)); in locale:
`cmake --preset asan && cmake --build --preset asan && ctest --preset asan`. Un
errore nel nostro codice si corregge, non si sopprime.

## Prima di dichiarare il lavoro finito

- [ ] `scripts/controlla.sh --tidy` passa, o hai scritto nella pull request quali
      passi non hai potuto eseguire e perché.
- [ ] I test richiesti dal pacchetto esistono; ogni condizione "Fatto quando" è
      verificata; i riferimenti non sono toccati e le righe del tuo pacchetto
      sono tolte da `in-attesa.txt`.
- [ ] Il riepilogo per l'utente elenca i file modificati e ciò che non hai potuto
      verificare (per esempio Windows e macOS).
- [ ] La CI è verde in tutti i controlli e hai tolto `AA-Presa in carico`.

## Commit e pull request

- Formato Conventional Commits: `feat(cmd): parse polar coordinates`.
- Non firmare tu il Developer Certificate of Origin (niente `git commit -s`): la
  firma `Signed-off-by` la aggiunge la persona responsabile, che ne risponde.
- Nel commit aggiungi la riga `Assisted-by: <nome dello strumento>`.
- Nella pull request compila la sezione sull'uso dell'IA del modello.
