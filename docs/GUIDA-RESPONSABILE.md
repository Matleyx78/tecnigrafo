# Guida del responsabile, passo per passo

Questa guida è per il responsabile del progetto. Dice **cosa fare, in che
ordine, e cosa scrivere agli agenti**, senza dover ricordare le regole. Le regole
complete sono in [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
[ROADMAP.md](ROADMAP.md) e nell'[ADR 0011](adr/0011-modalita-di-controllo.md).

Come si usa:

- la **parte A** si fa una volta sola;
- la **parte B** si ripete a ogni tappa, dall'inizio alla fine;
- la **parte C** serve quando qualcosa non va;
- le frasi da copiare sono nei riquadri grigi: si copiano così come sono,
  cambiando solo i numeri tra `< >`.

---

## A. Una volta sola

### A1. Unire la pull request di questa guida

1. Apri la pull request su GitHub e leggila.
2. Premi **Squash and merge**.
3. Nel messaggio che compare, controlla che l'ultima riga sia:

   ```text
   Signed-off-by: Matleyx78 <18114626+Matleyx78@users.noreply.github.com>
   ```

   Se manca, aggiungila tu. Vale per **ogni** squash and merge.
4. Premi **Confirm squash and merge**.

### A2. Creare le etichette

1. Su GitHub: **Issues** → **Labels** → **New label**.
2. Nome `riferimenti`, descrizione `Casi di verifica di un pacchetto (ADR 0011)`,
   colore a piacere. **Create label**.
3. Di nuovo **New label**: nome `agente:gemini`, descrizione
   `Assegnato a Gemini`. **Create label**.
4. Dalla tappa 0.7, ancora **New label**: nome `agente:gemini-argon`,
   descrizione `Assegnato a Gemini 4 Argon`. **Create label**.

### A3. Due impostazioni del repository

1. **Settings** → **General** → sezione **Pull Requests**.
2. Spunta **Automatically delete head branches**: i rami si cancellano da soli
   dopo il merge, e la scheda *Branches* resta pulita.
3. Facoltativo ma consigliato: **Settings** → **Rules** → **Rulesets** →
   **New ruleset** → **New branch ruleset**.
   - Nome: `main protetto`. **Enforcement status:** *Active*.
   - **Target branches** → **Add target** → *Include default branch*.
   - Spunta **Require status checks to pass** e aggiungi, uno per uno:
     `Formatting (clang-format 18)`, `Linux (GCC, clang-tidy)`, `Windows (MSVC)`,
     `macOS (Apple Clang)`, `Reference separation (ADR 0011)`,
     `Linux (ASan, UBSan)`, `Verifica`.
   - Nella sezione **Bypass list** aggiungi *Repository admin*, così in caso di
     emergenza puoi ancora unire tu.
   - **Create**.

   Da quel momento GitHub non ti lascia unire una pull request con la CI rossa
   per distrazione. `Verifica` esegue su Linux tutti i casi ai due livelli;
   Windows e macOS eseguono il livello 1 e un caso di base del livello 2.
   Nei dettagli dell'esecuzione, **Artifacts** contiene i rapporti di ogni
   verifica e, in caso di errore, le immagini ottenute e le differenze.

### A4. Installare Gemini (Antigravity CLI) sui tuoi due PC

Gemini lavora come Claude Code e Codex: un programma nel terminale, dentro la
cartella del progetto, che usa il tuo account GitHub. Il programma si chiama
**Antigravity CLI** e il comando è `agy`. Dal 18 giugno 2026 il vecchio Gemini
CLI non accetta più l'accesso con l'account Google personale, e Google indica
Antigravity al suo posto. Si installa sul PC Debian dell'ufficio e su Debian in
WSL a casa, nello stesso modo.

1. Se avevi installato il vecchio Gemini CLI, toglilo, così non si confonde:

   ```sh
   npm uninstall -g @google/gemini-cli
   ```

   Se risponde che non c'era, va bene lo stesso.

2. Installa Antigravity CLI. Non serve `sudo`: il programma va nella tua cartella
   personale (`~/.local/bin/agy`).

   ```sh
   curl -fsSL https://antigravity.google/cli/install.sh | bash
   ```

3. **Chiudi il terminale e aprine uno nuovo**, poi controlla:

   ```sh
   which agy
   ```

   Deve rispondere con un percorso che finisce in `.local/bin/agy`. Se non
   risponde niente:

   ```sh
   echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc && source ~/.bashrc
   ```

   e riprova `which agy`.

4. Controlla che GitHub risponda con il tuo account:

   ```sh
   gh auth status
   ```

   Se dice che non sei collegato: `gh auth login` e segui le domande (GitHub.com,
   HTTPS, accesso con il browser).

5. Entra nella cartella del progetto e avvia Antigravity:

   ```sh
   cd ~/progetti/tecnigrafo
   git checkout main && git pull
   agy
   ```

6. Alla prima apertura ti chiede colori e modo di visualizzazione (va bene quello
   proposto) e se ti fidi della cartella: rispondi **sì**, è la tua.

7. Accesso con il tuo account Google. Sul PC Debian dell'ufficio si apre il
   browser da solo. In WSL potrebbe non aprirsi: allora il terminale mostra un
   indirizzo. Copialo nel browser di Windows, accedi, copia il **codice** che ti
   mostra e incollalo nel terminale. Il piano gratuito ha un limite di uso
   settimanale: lo vedi in ogni momento con `/usage`.

8. **Controlla il modello.** Il piano gratuito offre anche modelli Claude e GPT,
   ma per i riferimenti serve un modello **Gemini**, diverso da quelli che
   scrivono il codice. Il modello in uso è scritto in basso a destra. Per
   cambiarlo scrivi `/model`, scegli **Gemini 3.8 Flash** e con le frecce
   `←` `→` porta *Effort* su **medium**. Poi `Invio`. È il modello della fascia
   leggera nella tabella di
   [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md) ("Modello e impegno"): quando
   cambiano i modelli disponibili, segui quella tabella.

9. Prima prova. Scrivi:

   ```text
   Leggi GEMINI.md e AGENTS.md. Spiegami in cinque righe cosa devi fare e cosa
   non devi mai fare in un pacchetto di riferimenti.
   ```

   Se la risposta parla di casi di verifica, di non eseguire Tecnigrafo per i
   valori attesi e di licenze, è pronto.

Antigravity si aggiorna da solo. Per uscire premi `Ctrl+C` (se non basta, una
seconda volta).

### A5. Configurare Antigravity per lavorare da solo

Con le impostazioni predefinite Antigravity chiede il permesso prima di ogni
comando e di ogni modifica, e devi restare davanti allo schermo. Si configura una
volta su ogni PC; i dettagli e il perché sono in
[AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md), sezione 6.

1. Copia le regole dei permessi della sezione 6 in
   `~/.gemini/antigravity-cli/settings.json` (se il file esiste già, aggiungi
   il blocco `permissions` a quello che c'è). Autorizzano i comandi del lavoro
   (`git`, `gh`, `cmake`, `ctest`, `python3`, `scripts/controlla.sh`,
   `scripts/ci-attendi.sh`, `scripts/ci-errori.sh`) e
   vietano quelli che spettano solo a te (`gh pr merge`, i push forzati e i push
   su `main`).
2. Avvia sempre Antigravity così:

   ```sh
   agy --mode auto
   ```

3. Dentro `agy` scrivi `/permissions` e controlla che le regole del punto 1
   compaiano. Se mancano, la sintassi è cambiata: aprimi una chat con Claude
   (sezione C, ultima riga).
4. Scrivi `/config` e imposta la revisione degli artifact su
   **agent-decides**: altrimenti Gemini si ferma ad aspettare che approvi il suo
   piano.

Non usare mai `--dangerously-skip-permissions` né *always-proceed*: approvano
tutto senza chiedere, comprese le azioni irreversibili.

### A6. Il Project «Tecnigrafo» e gli script

Il Project «Tecnigrafo» del tuo account raccoglie issue e pull request di
Tecnigrafo e di solid-space: è la tavola su cui vedi le due tappe aperte e lo
stato delle richieste. Ha i campi *Tappa* (`tg-0.7`, `ss-0.2`, …) e *Tipo*
(pacchetto, riferimenti, richiesta, idea, bug) e le viste «Tappe aperte»,
«Richieste», «Bloccati» e una per agente. Le issue di Tecnigrafo entrano da sole
(aggiunta automatica); quelle di solid-space le aggiungono gli script, oppure tu.

1. Una volta sola, sul PC dove lanci gli script:

   ```sh
   gh auth refresh -s project
   ```

   Segui le domande (si apre il browser). Senza questo permesso gli script
   lavorano lo stesso, ma non scrivono nel Project e lo dicono.
2. A ogni tappa nuova aggiungi i suoi valori al campo *Tappa*: nel Project
   **⋯** → **Settings** → *Tappa* → aggiungi `tg-<tappa>` (e `ss-<tappa>` per
   quella di solid-space). Gli script non possono crearli.
3. Per aggiungere una issue a mano: in fondo alla tavola **+ Add item**, incolla
   il link della issue, poi imposta *Tipo* e *Tappa*.

Gli script sono due, in `scripts/`: `richiesta.sh` apre una richiesta in
solid-space (B1), `apri-tappa.sh` apre una tappa (B2). Tutti e due mostrano
prima che cosa faranno e chiedono conferma; non uniscono, non chiudono e non
accettano nulla. Le opzioni sono scritte in testa a ciascuno.

---

## B. Il ciclo di ogni tappa

Le frasi indicano la tappa con `<tappa>`, per esempio `0.4`.

### B1. Preparare il piano (tu e Claude)

1. Apri una chat con Claude nel progetto Tecnigrafo.
2. Scrivi:

   ```text
   Prepariamo il piano dettagliato della tappa <tappa> partendo dalla scheda in
   docs/tappe e dai modelli tappe/0000-modello.md e tappe/0000-pacchetto.md,
   con le regole di docs/LAVORO-IN-PARALLELO.md ("Preparare il piano"). Per ogni
   pacchetto indica difficoltà, agente del lavoro, fascia di modello e impegno,
   e chi scrive i riferimenti: Gemini per quelli ordinari, un agente di fascia
   forte diverso da chi fa il lavoro per quelli con calcoli delicati, oppure
   "non servono" con il motivo. Assegna a Gemini anche i pacchetti facili di
   documentazione. Proponi anche le bozze degli ADR che servono, e i bisogni da
   chiedere a solid-space (docs/RICHIESTE-A-SOLID-SPACE.md) con il testo delle
   richieste, ciascuna nella forma del file di scripts/richiesta.sh. Scrivi
   subito le schede dei pacchetti che non dipendono da solid-space; per gli
   altri metti solo la riga nella tabella, con "scheda dopo il contratto".
   Elenca anche le issue con etichetta `idea` ancora aperte e dimmi quali,
   secondo te, potrebbero entrare in questa tappa e perché: le approvo io, una
   per una; quelle che approvo entrano nella scheda, le altre restano issue.
   ```

3. Discutete finché il piano ti convince. Gli ADR nuovi nascono *Proposti*: li
   accetti quando ti convincono, anche subito (GOVERNANCE.md).
4. Claude apre una pull request con piano e ADR. Leggila, ma se il piano ha
   richieste a solid-space non unirla ancora.
5. Se il piano ha richieste a solid-space, aprile subito, senza aspettare
   l'apertura della tappa: si aprono **solo in solid-space**, e in Tecnigrafo
   non se ne tiene una copia. In "Da dove nasce" ognuna cita la pull request del
   piano, come `Matleyx78/tecnigrafo#<numero>`. Hai due modi:
   - chiedi a Claude, nella stessa chat, collegando solid-space in scrittura
     (in una sessione cloud lo script apre la issue ma non arriva al Project:
     lo dice, e la aggiungi tu come in A6):

     ```text
     Apri in solid-space le richieste che abbiamo approvato, con
     scripts/richiesta.sh e --tappa ss-<tappa di solid-space>, come dice
     AGENTS.md in "Dove puoi scrivere". Poi scrivi i riferimenti nella tabella
     "Richieste a solid-space" del piano, nella pull request del piano.
     ```

   - oppure dal tuo PC, un file per richiesta:

     ```sh
     scripts/richiesta.sh richiesta.md --tappa ss-<tappa di solid-space>
     ```

     Lo script mostra la issue e chiede conferma; alla fine stampa il
     riferimento (`Matleyx78/solid-space#N`): chiedi a Claude di scriverlo
     nella tabella del piano, sezione 5.

   Da lì le richieste le segui in solid-space (la sua guida, B0) e nella vista
   «Richieste» del Project. Quando la tabella ha i riferimenti, unisci la pull
   request del piano come in A1.
6. **Seconda metà del piano, dopo i contratti.** Quando solid-space ha accettato
   gli ADR di valutazione con le loro intestazioni, apri una chat con Claude
   (Tecnigrafo in scrittura, solid-space in lettura):

   ```text
   Scrivi le schede dei pacchetti della tappa <tappa> che aspettavano il
   contratto di solid-space, sulle intestazioni accettate in solid-space, con
   le regole di docs/tappe/0000-modello.md ("Richieste a solid-space"). Ogni
   scheda dice quali intestazioni usa e, se sposta il sottomodulo, a quale
   pacchetto di solid-space. Riferimenti limitati alla facciata e ai comandi.
   Annota la modifica in "Modifiche al piano" e apri una pull request.
   ```

   La unisci come in A1. Se la tappa è già aperta, rilancia poi lo script di
   B2: crea solo le issue nuove.

### B2. Aprire la tappa (tu, con lo script)

Le issue le crea `scripts/apri-tappa.sh`, dalla tabella dei pacchetti del piano
unito. Si lancia dal tuo PC, perché serve l'accesso al Project (A6).

1. Nel terminale, nella cartella del progetto:

   ```sh
   cd ~/progetti/tecnigrafo && git checkout main && git pull
   scripts/apri-tappa.sh <tappa> --prova
   ```

   Con `--prova` lo script mostra soltanto che cosa farebbe: per ogni riga del
   piano *create* (issue da creare, con etichette e dipendenze), *exists* (c'è
   già) o *waits* (nessuna issue per ora, per esempio perché la scheda aspetta
   il contratto di solid-space). Se segnala errori nel piano, falli correggere
   con una pull request prima di andare avanti.
2. Se l'elenco ti convince:

   ```sh
   scripts/apri-tappa.sh <tappa>
   ```

   Ripete l'elenco e chiede `Go on? [s/N]`: rispondi `s`. Crea etichetta,
   milestone e issue, segna le dipendenze («bloccata da»), mette le issue nel
   Project con *Tappa* = `tg-<tappa>` e *Tipo*, mette in *Bloccato* quelle che
   aspettano, e apre una pull request con i numeri delle issue nel piano e lo
   stato «aperta» nella ROADMAP. Gli avvisi (`WARNING`) non fermano nulla:
   dicono che cosa fare a mano, di solito una dipendenza verso solid-space che
   GitHub non accetta (resta la riga "Dipende da") o un valore che manca nel
   Project.
3. Su GitHub apri **Issues** e filtra per il milestone: controlla che ogni issue
   abbia l'etichetta `agente:…` giusta. Nel Project, vista «Tappe aperte»,
   controlla che ci siano tutte.
4. Unisci la pull request dello script come in A1.

Se la tappa chiede qualcosa a solid-space, apri anche la sua tappa
corrispondente, seguendo la guida di solid-space (B1 e B2). Quando più avanti
aggiungi pacchetti alla tappa (per esempio le schede scritte dopo i contratti,
B1 punto 6), rilancia i due comandi: lo script crea solo le issue nuove.

> **Solo nella 0.4:** i primi quattro pacchetti costruiscono il sistema di
> verifica e non hanno riferimenti. Gemini comincia dal pacchetto di riferimenti
> R2 (i casi per ciò che esiste già), che dipende da P1.

### B3. Riferimenti (Gemini, a lotti)

Gemini scrive i riferimenti ordinari. Quelli con calcoli delicati il piano li
assegna a un agente di fascia forte (Claude o Codex) diverso da chi farà il
lavoro: per quelli si usa la frase di B4, e la revisione al punto 3 è la stessa.

1. Nel terminale, nella cartella del progetto:

   ```sh
   cd ~/progetti/tecnigrafo && git checkout main && git pull && agy --mode auto
   ```

2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Prendi i pacchetti di riferimenti pronti della tappa <tappa>
   assegnati a te, uno dopo l'altro, come dice AGENTS.md in "Lavoro a lotti".
   Per ognuno apri la pull request e porta la CI al verde; alla fine scrivimi
   un solo riepilogo.
   ```

   Gemini lavora da solo finché i pacchetti pronti non finiscono. Puoi
   allontanarti: alla fine trovi il riepilogo con tutte le pull request.

   **Lotto di Gemini 4 Argon.** I riferimenti con calcoli delicati che il piano
   assegna a `gemini-argon` li fa una sessione sua, che può lavorare insieme a
   quella di Gemini 3.8 Flash. Apri un altro terminale, avvia `agy --mode auto`
   come al punto 1, scrivi `/model`, scegli **Gemini 4 Argon** e porta *Effort*
   su **high**. Poi scrivi questo messaggio, che gli dice quale etichetta e
   quale cartella di build sono sue:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Sei Gemini 4 Argon: la tua etichetta è agente:gemini-argon e la tua
   cartella di build è build-gemini-argon (TG_BUILD_SUFFIX=-gemini-argon).
   Prendi i pacchetti di riferimenti pronti della tappa <tappa> con la tua
   etichetta, uno dopo l'altro, come dice AGENTS.md in "Lavoro a lotti". Per
   ognuno apri la pull request e porta la CI al verde; alla fine scrivimi un
   solo riepilogo.
   ```

   La revisione (punto 3) è la stessa.

3. Rivedi tu **ogni** pull request del lotto, una alla volta. È il controllo più
   importante di tutta la tappa. Guarda queste cinque cose:
   - [ ] ogni condizione "Fatto quando" del pacchetto ha almeno un caso;
   - [ ] i numeri attesi hanno senso (per esempio: un cerchio di raggio 10 ha
         circonferenza 62.83…);
   - [ ] ci sono casi limite (Esc a metà comando, annulla e ripeti, punti
         coincidenti);
   - [ ] per ogni file preso dal web c'è il link alla licenza, e la licenza
         permette di usarlo (MIT, BSD, CC0, CC BY, GPL 3 vanno bene; "solo uso
         personale" o nessuna licenza no);
   - [ ] la pull request tocca solo `tests/verifica/`.
4. Se qualcosa non va, scrivilo come commento nella pull request e di' a Gemini:

   ```text
   Leggi i commenti della pull request #<numero> e correggi.
   ```

5. Se va bene, unisci come in A1.

### B4. Lavoro (Claude e Codex)

Ogni agente in una sessione sua. Possono lavorare nello stesso momento.

1. Avvia l'agente (Claude Code o Codex) nella cartella del progetto, dopo
   `git checkout main && git pull`. Dalla tappa 0.5 scegli il modello e
   l'impegno che il piano indica per il pacchetto (tabella in
   [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), "Modello e impegno").
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Prendi il prossimo pacchetto della tappa <tappa> assegnato a te,
   secondo le regole di AGENTS.md. Realizzalo, apri la pull request, porta la CI
   al verde e fermati, come dice AGENTS.md.
   ```

3. Se l'agente risponde che nessun pacchetto è pronto, è normale: vuol dire che
   aspetta un merge (una dipendenza o i suoi riferimenti). Riprovi dopo.

Gli agenti fanno le prove locali con `scripts/controlla.sh` (AGENTS.md,
"Comandi"), aspettano la CI con `scripts/ci-attendi.sh` e, se è rossa, ne leggono
la causa con `scripts/ci-errori.sh` (AGENTS.md, "Quando fermarsi"). Per te non
cambia niente: conta solo la CI verde della pull request.

### B5. Seconda opinione (Gemini, a lotti)

Quando ci sono alcune pull request di lavoro aperte, in una sessione di
Antigravity (`agy --mode auto`):

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
lettera. Rivedi le pull request aperte, come dice AGENTS.md in "Lavoro a
lotti", e alla fine scrivimi un solo riepilogo.
```

Gemini lascia un commento di revisione in ognuna, senza modificare file. Leggi i
commenti prima di unire (B6): se uno chiede correzioni che condividi, passalo
all'agente che ha scritto la pull request.

### B5.1. Segnalazione errori

Quando un agente trova un errore nei riferimenti:

```text
Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e
seguilo alla lettera. Controlla la issue #XX e leggi i commenti. L'agente che se
ne occupa segnala un errore. Se ha ragione, correggi altrimenti commenta
spiegando il perché.
```

### B6. Unire

1. Apri la pull request. In basso tutti i controlli devono essere verdi.
2. Scheda **Files changed**: i file devono essere quelli del pacchetto.
3. Squash and merge come in A1, con la riga `Signed-off-by`.
4. Se ci sono altre pull request aperte, su ognuna premi **Update branch**.
5. Riavvia l'agente che ha finito con la stessa frase di B4: prenderà il
   pacchetto successivo. Gemini invece non va riavviato dopo ogni merge: rilancia
   il lotto di B3 quando un merge ha sbloccato nuovi pacchetti di riferimenti
   (la loro riga "Dipende da" è tutta chiusa), e quello di B5 quando si sono
   accumulate nuove pull request da rivedere.

**Quando unisci un pacchetto di solid-space**, la sua pull request elenca nella
riga «Sblocca:» le issue di Tecnigrafo che aspettavano proprio quel pacchetto.
Nel Project spostale da *Bloccato* a *Da fare*: possono partire subito, senza
aspettare la fine della tappa di solid-space. Il pacchetto di Tecnigrafo che usa
il pezzo nuovo sposta da solo il sottomodulo al commit che la sua scheda indica
(ADR 0030, punto 9): nella sua pull request controlla che `third_party/solid-space`
punti a quel commit.

Ripeti B3–B6 finché nel milestone non restano issue aperte.

### B7. Chiudere la tappa

1. A Gemini:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md. Tutte le issue
   del milestone v<tappa> sono chiuse: prepara la pull request di chiusura della
   tappa come descritto in docs/LAVORO-IN-PARALLELO.md, "Chiudere la tappa", sul
   ramo docs/chiusura-<tappa>. Poi fermati.
   ```

2. Leggi la pull request: per ogni criterio di chiusura deve esserci la prova (un
   caso di verifica o un test), oppure "da provare a mano".
3. Fai la **prova manuale** descritta nelle note di rilascio, su Windows e su
   Linux, e scrivi l'esito nella tabella (puoi chiedere all'agente di compilarla
   per te, dandogli gli esiti).
4. Unisci come in A1.
5. Nel terminale:

   ```sh
   git checkout main && git pull
   git tag -a v<tappa>.0 -m "Tecnigrafo <tappa>.0"
   git push origin v<tappa>.0
   ```

6. Su GitHub: **Releases** → **Draft a new release** → scegli il tag
   `v<tappa>.0` → copia nel testo le note di `docs/rilasci/<tappa>.md` →
   **Publish release**.
7. Torna a B1 per la tappa successiva.

---

## C. Se qualcosa va storto

| Cosa succede | Cosa fai |
|---|---|
| La CI è rossa | Non unire. Scrivi all'agente: `La CI della pull request #<numero> è rossa: leggi l'errore e correggi.` |
| Fallisce *Reference separation* | La pull request mescola riferimenti e codice. Scrivi all'agente: `Dividi la pull request #<numero> in due, come chiede ADR 0011.` |
| Un agente dice che un riferimento è sbagliato | Leggi il suo commento nella issue. Se ha ragione, fai correggere il riferimento a Gemini (B3, indicando la issue); se ha torto, rispondi nella issue e fallo continuare. Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla lettera. L'agente <x> ha trovato un errore nella tua RX, pr #X. Leggi i commenti nella issue #X. Se ha ragione, correggi i riferimenti. Se ha torto scrivi un perché di poche righe nei commenti della issue #X.|
| Conflitto con `main` | Scrivi all'agente di quella pull request: `Aggiorna la pull request #<numero> con main e risolvi i conflitti.` |
| Una issue ha `AA-Presa in carico` ma nessuno ci lavora più | Togli l'etichetta a mano e riavvia l'agente. |
| Un agente non risponde da 14 giorni | Il pacchetto torna libero: cambi l'etichetta `agente:…` e lo assegni a un altro. |
| Gemini si ferma a chiedere conferma per un comando | Se il comando serve al lavoro ed è sicuro (legge, compila, prova), approvalo e aggiungilo alle regole con `/permissions`, così non lo chiede più. Se cancella, unisce o pubblica su `main`, rifiuta. Regole in A5. |
| Gemini ha finito il limite di uso (`/usage`) | Riprendi quando il limite si azzera, oppure fai scrivere quei riferimenti a un agente che **non** lavora a quel pacchetto. |
| Un agente si ferma e scrive il testo di una richiesta a solid-space nella issue | Leggi il testo. Se serve davvero, aprila in solid-space come in B1, punto 5 (lo script legge quel testo così com'è, salvato in un file), e annota la modifica nel piano in "Modifiche al piano"; il pacchetto aspetta il contratto. Se non serve, rispondi nella issue e fallo continuare. |
| Un riferimento di Tecnigrafo e l'archivio dei casi di solid-space non concordano | Apri una issue in solid-space con i due valori: decide l'archivio (ADR 0030, punto 14). Se è sbagliato l'archivio, lo corregge un pacchetto di riferimenti di solid-space; se è sbagliato il riferimento, un pacchetto di riferimenti di Tecnigrafo. |
| `apri-tappa.sh` o `richiesta.sh` dice che il piano o il file non è pronto | Elenca che cosa correggere, riga per riga. Correggi (o fai correggere a Claude) e rilancia: finché non è tutto a posto non scrive nulla. |
| Non sai cosa fare | Apri una chat con Claude nel progetto e incolla questa riga: `Sono al passo <passo> della guida e succede questo: …` |
