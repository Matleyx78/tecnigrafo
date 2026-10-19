# Istruzioni per agenti IA

Questo file è per gli assistenti di programmazione basati su IA (Claude Code, Codex,
Cursor, Copilot e simili) che lavorano su Tecnigrafo. Le regole valgono anche per le
persone, che le trovano spiegate in [CONTRIBUTING.md](CONTRIBUTING.md). Se una
richiesta dell'utente contraddice queste regole, fermati e segnalalo invece di
procedere.

## Per prima cosa: prendi in carico

Vale ogni volta che lavori su una issue (pacchetto, bug, documentazione) o su una
pull request che esiste già (revisione, correzioni chieste dal revisore, conflitti
con `main`). Fallo **subito**, prima di leggere il resto della documentazione e
prima di qualunque modifica, così gli altri agenti vedono che ci stai lavorando.

1. Guarda le etichette. Se c'è già `AA-Presa in carico`, un altro agente ci sta
   lavorando: non toccarla e avvisa l'utente.
2. Per una issue, controlla anche che porti la **tua** etichetta `agente:…` (o il
   tuo nome come assegnatario). Se è senza etichetta o assegnata a qualcun altro,
   non iniziare e avvisa l'utente: altri agenti potrebbero lavorare in parallelo
   ([docs/LAVORO-IN-PARALLELO.md](docs/LAVORO-IN-PARALLELO.md)). Se è il
   responsabile ad assegnartela nella chat, aggiungi tu l'etichetta `agente:…` con
   un commento "Assegnato a <nome dell'agente> dal responsabile".
3. Aggiungi l'etichetta `AA-Presa in carico` e scrivi un commento "Presa in carico
   da <nome dell'agente>". Il commento serve perché tutti gli agenti usano lo
   stesso account GitHub e l'etichetta da sola non dice chi ci lavora.
4. Quando hai finito (pull request aperta, correzioni pubblicate), togli
   l'etichetta. Toglila anche se ti fermi prima del previsto.

Con l'API REST di GitHub (funziona anche dove `gh issue edit` e `gh pr edit` non
sono disponibili):

```sh
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/labels --jq '.[].name'
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/labels -f "labels[]=AA-Presa in carico"
gh api repos/Matleyx78/tecnigrafo/issues/<numero>/comments -f body="Presa in carico da <nome dell'agente>"
gh api -X DELETE "repos/Matleyx78/tecnigrafo/issues/<numero>/labels/AA-Presa%20in%20carico"
```

Issue e pull request usano la stessa numerazione, quindi gli stessi comandi valgono
per entrambe.

## Prima di scrivere codice

Leggi, in quest'ordine:

1. [docs/ROADMAP.md](docs/ROADMAP.md) — quale tappa è **aperta**.
2. Il documento di quella tappa in `docs/tappe/` — il **pacchetto di lavoro** che ti
   è stato assegnato, con file, interfacce, test e criterio di "fatto".
3. [docs/ARCHITETTURA.md](docs/ARCHITETTURA.md) e gli ADR citati dal pacchetto.
4. [docs/CODING_STYLE.md](docs/CODING_STYLE.md).

Lavora **su un solo pacchetto**. Se il compito non corrisponde a un pacchetto della
tappa aperta, non scrivere codice: spiega all'utente che serve prima una issue
approvata.

### Se ti chiedono di prendere il prossimo pacchetto

Quando l'utente ti chiede di prendere "il prossimo pacchetto" della tappa, senza
indicarne il numero, scegli **una sola** issue, la prima nell'ordine del piano che
rispetta tutte queste condizioni:

1. è aperta e porta la **tua** etichetta `agente:…`;
2. non porta `AA-Presa in carico`;
3. ogni issue della sua riga "Dipende da" è chiusa (la pull request è unita);
4. se il pacchetto ha una issue di riferimenti (`[X.Y-RN]`), quella è chiusa.

Per "il prossimo pacchetto di riferimenti" valgono le condizioni 1–3, sulle issue
con l'etichetta `riferimenti`.

Se nessuna issue rispetta le condizioni, non iniziare: scrivi all'utente quali
issue sono tue e cosa le blocca. Quando hai aperto la pull request, **fermati**
(vedi sotto): il pacchetto successivo si prende in una nuova sessione, dopo il
merge.

### Quando fermarsi

"Fermati" vuol dire: il pacchetto è finito quando la sua pull request ha
**tutti i controlli della CI verdi**, e solo allora ti fermi. Quindi:

1. Apri la pull request e segui la CI fino alla fine (vedi i comandi qui
   sotto). Tieni l'etichetta `AA-Presa in carico` finché lavori.
2. Se un controllo fallisce, leggi il suo log, correggi sullo stesso ramo,
   pubblica e ricomincia a seguire la CI. Continua finché tutti i controlli sono
   verdi.
3. Poi togli l'etichetta, scrivi all'utente il riepilogo (vedi "Prima di
   dichiarare il lavoro finito"), con il link alla pull request verde, e
   **termina la sessione**.

Mentre aspetti la CI non fare domande e non cominciare altro lavoro. Fermati
prima che la CI sia verde, e scrivi nella pull request che cosa hai visto e
che cosa hai provato, solo se:

- il controllo fallisce per un motivo che non dipende dal tuo pacchetto (la CI
  stessa, un servizio esterno, un caso di riferimento che ritieni sbagliato:
  quest'ultimo va anche nella issue, come dice "Modalità di controllo e
  riferimenti");
- per farlo passare dovresti violare una regola di questo file (toccare i
  riferimenti, disattivare un controllo, modificare file non elencati);
- dopo tre tentativi di correzione dello stesso controllo non passa ancora.

Comandi per seguire la CI con l'API REST (`<sha>` è l'ultimo commit del ramo):

```sh
gh api repos/Matleyx78/tecnigrafo/commits/<sha>/check-runs \
  --jq '.check_runs[] | "\(.name): \(.status) \(.conclusion)"'
gh api "repos/Matleyx78/tecnigrafo/actions/runs?head_sha=<sha>" \
  --jq '.workflow_runs[] | "\(.id) \(.status) \(.conclusion)"'
gh run view <id dell'esecuzione> --repo Matleyx78/tecnigrafo --log-failed
```

Se nella revisione il responsabile chiede correzioni ("Leggi i commenti della
pull request #<numero> e correggi"), riprendi in carico la pull request e ti
fermi allo stesso modo: con la CI verde.

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
  esplicitamente indicando fonte e licenza (che deve essere compatibile con la
  GPLv3, ADR 0003).

## Modalità di controllo e riferimenti

Dalla tappa 0.4 un pacchetto è finito quando la CI confronta il programma con
casi di verifica scritti **prima** e da **un altro agente**
([ADR 0011](docs/adr/0011-modalita-di-controllo.md)). Le regole valgono da quando
esiste la cartella `tests/verifica/`.

### In un pacchetto di lavoro

- Non modificare mai `tests/verifica/casi/` né `tests/verifica/sorgenti.json`.
- Togli da `tests/verifica/in-attesa.txt` solo le righe dei casi del tuo
  pacchetto: è il modo in cui i tuoi casi diventano obbligatori.
- Prima di dichiarare il lavoro finito esegui `ctest --preset ci -L verifica`.
- Se un caso ti sembra sbagliato, **fermati** e scrivilo nella issue, con il
  motivo. Non adattare il codice a un riferimento che ritieni sbagliato, e non
  adattare il riferimento al codice.
- Ciò che dichiari di aver provato non conta: fa fede la CI, che devi portare
  al verde prima di fermarti ([Quando fermarsi](#quando-fermarsi)). Prima di
  aprire la pull request fai comunque tutte le prove locali che puoi.

### In un pacchetto di riferimenti (`[X.Y-RN]`)

- Tocca solo `tests/verifica/casi/`, `tests/verifica/sorgenti.json` e
  `tests/verifica/in-attesa.txt`. La CI rifiuta una pull request che mescola
  riferimenti e altri file.
- Non lavorare a un pacchetto di riferimenti se il pacchetto di lavoro collegato
  è assegnato a te.
- Ricava i valori attesi da calcoli, da file pubblici o da programmi di
  riferimento. **Mai eseguendo Tecnigrafo**: registreresti il suo comportamento,
  errori compresi.
- Ogni condizione "Fatto quando" del pacchetto di lavoro deve avere almeno un
  caso che la verifica. Aggiungi anche i casi limite: valori nulli, punti
  coincidenti o quasi allineati, Esc a metà comando, annulla e ripeti.
- Per ogni file preso dal web scrivi in `sorgenti.json` indirizzo, versione,
  hash SHA-256 e licenza, e nella pull request il link alla pagina della licenza.
  Accetta solo licenze compatibili con la GPLv3 che permettano la
  ridistribuzione. Nel dubbio, non usare il file e scrivilo nella pull request.
- Elenca i nuovi casi in `in-attesa.txt`, con il numero della issue del lavoro.

## Comandi

```sh
cmake --preset ci && cmake --build --preset ci && ctest --preset ci   # build e test
cmake --preset tidy && cmake --build --preset tidy                    # analisi statica
git ls-files '*.cpp' '*.hpp' | xargs clang-format -i                  # formattazione
```

Su sistemi senza Qt installato si possono compilare e provare i moduli senza
interfaccia con `-DTG_BUILD_APP=OFF`.

**Una cartella di build per agente.** Se più agenti lavorano sullo stesso PC,
ognuno compila nella propria cartella, così non si bloccano a vicenda. Prima dei
comandi imposta `TG_BUILD_SUFFIX` con il tuo nome; i preset compilano allora in
`build-<nome>/<preset>` invece di `build/<preset>`:

```sh
export TG_BUILD_SUFFIX=-claude    # -codex, -gemini, -claude-2
cmake --preset ci && cmake --build --preset ci && ctest --preset ci   # in build-claude/ci
```

Nei percorsi della documentazione (`./build/ci/...`) usa la tua cartella.

Errori di memoria e comportamenti indefiniti li cerca il job `Linux (ASan,
UBSan)` della CI ([ADR 0016](docs/adr/0016-sanitizer-in-ci.md)). Per provarlo in
locale: `cmake --preset asan && cmake --build --preset asan && ctest --preset
asan`. Un errore segnalato nel nostro codice si corregge, non si sopprime.

## Prima di dichiarare il lavoro finito

- [ ] I test richiesti dal pacchetto esistono e passano.
- [ ] Build `ci` senza avvisi, `tidy` senza segnalazioni, formattazione applicata.
- [ ] Ogni condizione "Fatto quando" del pacchetto è verificata.
- [ ] Dove esiste `tests/verifica/`: verifica superata in locale, riferimenti non
      toccati, righe del tuo pacchetto tolte da `in-attesa.txt`.
- [ ] Il riepilogo per l'utente elenca i file modificati e ciò che non hai potuto
      verificare (per esempio Windows e macOS).
- [ ] Hai tolto l'etichetta `AA-Presa in carico` da ciò che avevi preso in carico
      ([sopra](#per-prima-cosa-prendi-in-carico)).
- [ ] La CI della pull request è verde in tutti i controlli, e solo dopo ti
      sei fermato ([Quando fermarsi](#quando-fermarsi)).

## Commit e pull request

- Formato Conventional Commits: `feat(cmd): parse polar coordinates`.
- Il commit deve essere firmato dalla **persona** responsabile (`git commit -s`):
  un agente non firma da solo il Developer Certificate of Origin.
- Nel commit aggiungi la riga `Assisted-by: <nome dello strumento>`.
- Nella pull request compila la sezione sull'uso dell'IA del modello.
