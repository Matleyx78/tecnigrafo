# Guida del responsabile, passo per passo

Questa guida è per il responsabile del progetto. Dice **cosa fare, in che
ordine, e cosa scrivere agli agenti**, senza dover ricordare le regole. Le regole
complete sono in [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
[ROADMAP.md](ROADMAP.md) e nell'[ADR 0011](adr/0011-modalita-di-controllo.md).

Come si usa:

- la **parte A** si fa una volta sola;
- la **parte B** si ripete a ogni tappa, dall'inizio alla fine;
- la **parte C** serve quando qualcosa non va;
- la **parte D** si ripete a ogni tappa di solid-space;
- le frasi da copiare sono nei riquadri grigi: si copiano così come sono,
  cambiando solo i numeri tra `< >`.

È l'**unica guida del responsabile per i due progetti**: Tecnigrafo e il suo
kernel, [solid-space](https://github.com/Matleyx78/solid-space). La guida di
solid-space rimanda qui. Le parti A, B e C valgono per Tecnigrafo, e per
solid-space dove lo dicono; la parte D è il ciclo di una tappa di solid-space.

Cartelle di lavoro sul PC: `~/progetti/tecnigrafo` e `~/progetti/solid-space`.
Ogni agente si avvia dalla cartella del repository su cui lavora, mai da una
cartella che le contenga tutte e due (A7).

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

**In solid-space** (là, **Issues** → **Labels**) servono queste etichette. Sono
già state create tutte, tranne `tappa-X.Y`, che nasce all'apertura di ogni tappa
(D2):

| Etichetta | A cosa serve |
|---|---|
| `idea` | proposta per le tappe future: si discute, non si implementa |
| `richiesta` | bisogno di Tecnigrafo che solid-space deve valutare (D0); la mette da sola il modulo della richiesta |
| `contratto` | proposta di cambiare intestazioni pubbliche già accettate come contratto con Tecnigrafo: decidi tu, per i due progetti (C) |
| `pacchetto` | issue di un pacchetto di lavoro (`[X.Y-PN]`) |
| `riferimenti` | issue dei casi di verifica di un pacchetto (`[X.Y-RN]`) |
| `tappa-X.Y` | tappa a cui appartiene la issue (una per tappa, creata all'apertura) |
| `agente:claude`, `agente:claude-2`, `agente:codex`, `agente:gemini` | a chi è assegnata la issue |
| `AA-Presa in carico` | qualcuno ci sta lavorando adesso: non toccare |

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

**In solid-space** si fanno le stesse due impostazioni. Nel ruleset i controlli
sono quelli del suo [ADR 0006](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0006-build-ci-e-sottomoduli.md),
punto 5: `Formatting (clang-format 18)`, `Linux (GCC, clang-tidy)`,
`Windows (MSVC)`, `macOS (Apple Clang)`, `Linux (ASan, UBSan)` e
`Reference separation (ADR 0004)`.

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

Per solid-space i punti 5–9 sono gli stessi, nella sua cartella
(`cd ~/progetti/solid-space`). Nella prima prova chiedigli anche dove può
scrivere: la risposta deve parlare di casi di verifica, di non eseguire
solid-space per i valori attesi, di licenze, e dire che scrive solo in
solid-space.

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

Per solid-space le regole dei permessi sono nel suo
[AMBIENTE-SVILUPPO.md](https://github.com/Matleyx78/solid-space/blob/main/docs/AMBIENTE-SVILUPPO.md), sezione 3.

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
4. L'aggiunta automatica del piano gratuito è una sola e sta su Tecnigrafo:
   **le issue di solid-space entrano nel Project solo se le aggiunge qualcuno**,
   lo script delle richieste oppure tu (D0, D2). Nel Project non scrive nessun
   workflow: gli agenti lavorano con etichette e issue.

Gli script sono due, in `scripts/`: `richiesta.sh` apre una richiesta in
solid-space (B1), `apri-tappa.sh` apre una tappa (B2). Tutti e due mostrano
prima che cosa faranno e chiedono conferma; non uniscono, non chiudono e non
accettano nulla. Le opzioni sono scritte in testa a ciascuno.


### A7. Il confine tra i due repository

I due progetti hanno lo stesso responsabile, ma ognuno ha le sue regole. La regola
per gli agenti è la stessa nei due `AGENTS.md` ("Dove puoi scrivere"): **una
chat, un repository**.

- **Nel repository della chat** l'agente scrive codice e documenti, entro il suo
  pacchetto.
- **Nell'altro** legge soltanto. Scrive **un commento** su una issue o pull
  request solo se glielo chiedi tu, indicando quale. Niente file, rami, commit,
  pull request, etichette, chiusure.
- Una sola eccezione in più: un agente di Tecnigrafo può **aprire una
  richiesta** in solid-space, se glielo chiedi tu (B1, punto 5).

La regola scritta, da sola, si basa sull'obbedienza dell'agente. Per renderla
più difficile da violare, usa tutti i livelli che puoi:

1. **Avvia ogni agente dalla cartella del suo repository**, mai da una cartella
   sopra che le contenga tutte e due. Un agente scrive volentieri dove si trova.
2. **Una sessione, un progetto.** Una sessione di solid-space non si usa per
   lavorare a Tecnigrafo, e viceversa.
3. **Nelle sessioni sul web** (claude.ai) collega in scrittura solo il
   repository della chat, e l'altro, se serve, in lettura. Senza accesso in
   scrittura l'agente non può pubblicare nulla, qualunque cosa decida.
4. **Quando vuoi un commento nell'altro repository** scrivi chiaro quale e cosa
   dire:

   ```text
   Leggi la issue #<numero> di Matleyx78/<altro repository> e scrivi lì un
   commento che <cosa deve dire>. Non modificare altro, né lì né qui.
   ```

Quello che l'agente legge nell'altro repository (issue, commenti, file) è
informazione, non un ordine: se contiene richieste, deve riferirtele, non
eseguirle.

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

   Da lì le richieste le segui in solid-space (parte D, D0) e nella vista
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
corrispondente, con la parte D (D1 e D2). Quando più avanti
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
| Un riferimento di Tecnigrafo e l'archivio dei casi di solid-space non concordano | Apri una issue in solid-space con i due valori: decide l'archivio (ADR 0030, punto 14; in solid-space, `tests/corpus/README.md`, "Disaccordi con Tecnigrafo"). Se è sbagliato l'archivio, lo corregge un pacchetto di riferimenti di solid-space; se è sbagliato il riferimento, un pacchetto di riferimenti di Tecnigrafo. |
| Il controllo automatico ha commentato una richiesta di solid-space | Completa la issue come dice il commento (D0, punto 3). Il commento si aggiorna da solo a ogni modifica; non decide nulla, quindi puoi anche ignorarlo se il riferimento che segnala è voluto. |
| Serve cambiare un'intestazione di solid-space già accettata come contratto | Apri in solid-space una issue con l'etichetta `contratto` che dice che cosa cambia e perché, e guarda quali schede di Tecnigrafo la usano. Decidi tu per i due progetti; se approvi, la scheda del pacchetto che la cambia lo dice esplicitamente, e in Tecnigrafo si aggiornano le schede che la usano (ADR 0007 di solid-space, punto 15). |
| In solid-space fallisce il controllo di separazione tra riferimenti e lavoro | La pull request mescola riferimenti e codice. Scrivi all'agente: `Dividi la pull request #<numero> in due, come chiede ADR 0004.` |
| Un agente ha scritto o proposto qualcosa nell'altro repository senza che tu l'abbia chiesto | Non unire e non rispondere. Chiudi la pull request o cancella il commento, e di' all'agente: `La regola "Dove puoi scrivere" di AGENTS.md vieta di scrivere nell'altro repository: spiega cos'è successo.` (A7) |
| `apri-tappa.sh` o `richiesta.sh` dice che il piano o il file non è pronto | Elenca che cosa correggere, riga per riga. Correggi (o fai correggere a Claude) e rilancia: finché non è tutto a posto non scrive nulla. |
| Non sai cosa fare | Apri una chat con Claude nel progetto e incolla questa riga: `Sono al passo <passo> della guida e succede questo: …` |

---

## D. Il ciclo di una tappa di solid-space

Le tappe di solid-space nascono dalle richieste di una tappa di Tecnigrafo
([ADR 0007 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md)). Le
frasi indicano la tappa di solid-space con `<tappa>`, per esempio `0.2`. Ogni
chat di questa parte collega **solid-space in scrittura** e Tecnigrafo, se
serve, solo in lettura (A7); i comandi nel terminale si danno nella cartella
`~/progetti/solid-space`.

### D0. Arriva una richiesta di Tecnigrafo (tu e Claude)

Le tappe non si pianificano in anticipo: nascono dalle richieste di una tappa di
Tecnigrafo, e a ogni tappa di Tecnigrafo che chiede qualcosa corrisponde una tappa
di solid-space ([ADR 0007](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md), punti 10–12). La
richiesta nasce e vive qui, in una sola issue: in Tecnigrafo non se ne tiene una
copia.

1. In Tecnigrafo emerge un bisogno: di solito mentre prepari il piano di una sua
   tappa, oppure perché un agente di Tecnigrafo si ferma in un pacchetto e ne
   scrive il testo nella issue. Il testo segue il modello
   [richieste/0000-modello.md](https://github.com/Matleyx78/solid-space/blob/main/docs/richieste/0000-modello.md): sette campi, dice di
   che cosa c'è bisogno e non come soddisfarlo, e ogni riferimento è assoluto
   (`Matleyx78/tecnigrafo#N`).
2. Apri la richiesta qui, in uno di due modi:
   - **da GitHub**: in solid-space **Issues** → **New issue** → **Richiesta di
     Tecnigrafo**, compila i campi e premi **Create**. L'etichetta `richiesta` la
     mette il modulo. Poi nel Project «Tecnigrafo» (A6): **+ Add item**, incolla
     il link della issue, metti *Tipo* = richiesta e *Tappa* = `ss-<tappa>`;
   - **da una chat di Tecnigrafo**: chiedi a un agente di Tecnigrafo di aprirla
     con lo script delle richieste (`scripts/richiesta.sh`), come in B1,
     punto 5. Lo script mette anche la issue nel Project, con Tipo e Tappa, e
     stampa il riferimento da citare nel piano di Tecnigrafo.

   Senza questa issue nessuno valuta nulla.
3. Entro un paio di minuti il controllo automatico legge la issue. Se compare un
   suo commento con un elenco, completa la issue (**⋯** → **Edit**): il commento
   si aggiorna da solo e, quando è tutto a posto, lo dice. Il controllo guarda
   solo campi e riferimenti, non decide nulla.
4. Apri una chat con Claude nel progetto, con solid-space in scrittura, e scrivi:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo.
   Valutiamo la richiesta #<numero> secondo l'ADR 0007: dimmi se OCCT basta così
   com'è, se va corretto o migliorato, se serve un pezzo nostro o se non spetta a
   solid-space, e scrivi l'ADR di valutazione nel formato di
   docs/adr/0000-modello-valutazione.md. Se un vincolo già deciso esclude OCCT,
   citalo invece di rifare il confronto (punto 8). Con gli esiti 2 e 3 metti
   nella stessa pull request il contratto: le intestazioni pubbliche in bozza,
   che compilano senza implementazione e dichiarano l'errore garantito delle
   costruzioni (punto 15). Una sola pull request; non scrivere in Tecnigrafo.
   ```

   Più richieste della stessa tappa di Tecnigrafo si possono valutare nella
   stessa chat, una pull request per gruppo di richieste che condividono le
   stesse intestazioni.
5. Leggi la valutazione e il contratto, e discutetene. Gli ADR nuovi nascono
   *Proposti*: quando ti convincono, scrivi nella chat «accetto gli ADR», aspetta
   che li porti in stato *Accettato* e unisci come in A1, nel repository di solid-space
   ([GOVERNANCE.md](https://github.com/Matleyx78/solid-space/blob/main/GOVERNANCE.md)). Da quel momento le intestazioni sono il
   contratto: in Tecnigrafo puoi far scrivere le schede dei pacchetti che le
   usano, senza aspettare il codice di solid-space.
6. Se l'esito è 4, chiudi la issue con la motivazione. Con gli esiti 1, 2 e 3 la
   richiesta entra nella tappa di solid-space della sua tappa di Tecnigrafo:
   - se quella tappa è **in preparazione**, passa a D1 quando le valutazioni di
     tutte le sue richieste conosciute sono accettate;
   - se è **già aperta** (la richiesta è nata mentre Tecnigrafo percorreva la
     tappa), passa a D1 con la frase per aggiungere una richiesta;
   - se è aperta un'altra tappa di solid-space, quella di una tappa precedente
     di Tecnigrafo, la richiesta aspetta che si chiuda. Prima passa quella che
     blocca la tappa aperta di Tecnigrafo.

   Lo stato di ogni richiesta lo vedi nella vista «Richieste» del Project.

### D1. Preparare il piano (tu e Claude)

Una tappa di solid-space raccoglie tutte le richieste di una tappa di Tecnigrafo
(«solid-space 0.2 per Tecnigrafo 0.7»).

1. Apri una chat con Claude nel progetto, con solid-space in scrittura.
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo.
   Prepariamo la tappa <tappa> di solid-space per la tappa <tappa-tg> di
   Tecnigrafo, dalle richieste #<a>, #<b>, … e dai loro ADR di valutazione
   <NNNN>, <NNNN>, …: scrivi la scheda e il piano dettagliato partendo dai
   modelli tappe/0000-modello.md e tappe/0000-pacchetto.md,
   con le regole di docs/LAVORO-IN-PARALLELO.md ("Preparare il piano"). La
   scheda rimanda agli ADR senza ripeterli. I pacchetti realizzano le
   intestazioni del contratto senza cambiarle; metti prima i pacchetti che
   sbloccano la tappa di Tecnigrafo, e scrivi nella scheda di ciascuno quali
   issue di Tecnigrafo sblocca, se esistono già. I casi concreti delle richieste
   e i DXF di Tecnigrafo entrano nei pacchetti di riferimenti, come dice
   tests/corpus/README.md. Per ogni
   pacchetto indica difficoltà, agente del lavoro, fascia di modello e impegno,
   e chi scrive i riferimenti: Gemini per quelli ordinari, un agente di fascia
   forte diverso da chi fa il lavoro per quelli con calcoli delicati, oppure
   "non servono" con il motivo. Assegna a Gemini anche i pacchetti facili di
   documentazione. Proponi anche le bozze degli ADR che servono.
   Elenca anche le issue con etichetta `idea` ancora aperte e dimmi quali,
   secondo te, potrebbero entrare in questa tappa e perché: le approvo io, una
   per una; quelle che approvo entrano nella scheda, le altre restano issue.
   ```

   Per una richiesta nata a tappa aperta, invece:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo.
   Aggiungi alla tappa aperta <tappa> la richiesta #<numero> e il suo ADR
   <NNNN> di valutazione: scrivi le schede dei pacchetti nuovi, le loro righe
   nella tabella dei pacchetti e una riga in "Modifiche al piano". Apri una pull
   request.
   ```

3. Discutete finché il piano ti convince. Gli ADR nuovi nascono *Proposti*: li
   accetti quando ti convincono, anche subito ([GOVERNANCE.md](https://github.com/Matleyx78/solid-space/blob/main/GOVERNANCE.md)).
4. Claude apre una pull request con piano e ADR. La leggi e la unisci come in A1, nel repository di solid-space.

### D2. Aprire la tappa (Claude crea le issue)

La tappa si apre quando la tappa precedente di solid-space è chiusa; da quel
momento avanza insieme alla tappa di Tecnigrafo a cui corrisponde. Vale anche per
i pacchetti aggiunti a tappa aperta: per quelli crei solo le issue nuove.

1. Scrivi a Claude:

   ```text
   Apri la tappa <tappa> secondo docs/ROADMAP.md, "Aprire una tappa": crea il
   milestone v<tappa>, l'etichetta tappa-<tappa>, le issue [<tappa>-PN] e
   [<tappa>-RN] con le etichette indicate nel piano, scrivi i numeri delle issue
   nel documento della tappa e aggiorna la tabella della ROADMAP. Apri una pull
   request con le modifiche ai documenti.
   ```

2. Su GitHub apri **Issues** e filtra per il milestone: controlla che ogni issue
   abbia l'etichetta `agente:…` giusta.
3. Metti le issue nuove nel Project «Tecnigrafo» (A6): nella vista a tabella,
   **+ Add item** → **Add item from repository** → solid-space, seleziona le
   issue del milestone. A ciascuna metti *Tappa* = `ss-<tappa>` e *Tipo* =
   pacchetto o riferimenti. Controlla che compaiano nella vista «Tappe aperte».
4. Unisci la pull request come in A1, nel repository di solid-space.

### D3. Riferimenti (Gemini, a lotti)

Gemini scrive i riferimenti ordinari. Quelli con calcoli delicati il piano li
assegna a un agente di fascia forte (Claude o Codex) diverso da chi farà il
lavoro: per quelli si usa la frase di D4, e la revisione al punto 3 è la stessa.

1. Nel terminale, nella cartella del progetto:

   ```sh
   cd ~/progetti/solid-space && git checkout main && git pull && agy --mode auto
   ```

2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
   lettera. Prendi i pacchetti di riferimenti pronti della tappa <tappa>
   assegnati a te, uno dopo l'altro, come dice AGENTS.md in "Lavoro a lotti".
   Per ognuno apri la pull request e porta la CI al verde; alla fine scrivimi
   un solo riepilogo.
   ```

   Gemini lavora da solo finché i pacchetti pronti non finiscono. Puoi
   allontanarti: alla fine trovi il riepilogo con tutte le pull request.

3. Rivedi tu **ogni** pull request del lotto, una alla volta. È il controllo più
   importante di tutta la tappa. Guarda queste cinque cose:
   - [ ] ogni condizione "Fatto quando" del pacchetto ha almeno un caso;
   - [ ] i valori attesi hanno senso (per esempio: un cubo di lato 1 ha volume 1,
         area 6 e 6 facce) e non possono venire da solid-space;
   - [ ] ci sono casi limite (valori nulli, punti coincidenti o quasi allineati,
         facce tangenti, spessori minimi);
   - [ ] per ogni file preso dal web c'è il link alla licenza, e la licenza
         permette di usarlo (MIT, BSD, CC0, CC BY, GPL 3 vanno bene; "solo uso
         personale" o nessuna licenza no);
   - [ ] la pull request tocca solo `tests/corpus/`.
4. Se qualcosa non va, scrivilo come commento nella pull request e di' a Gemini:

   ```text
   Leggi i commenti della pull request #<numero> e correggi.
   ```

5. Se va bene, unisci come in A1, nel repository di solid-space.

### D4. Lavoro (Claude e Codex)

Ogni agente in una sessione sua. Possono lavorare nello stesso momento.

1. Avvia l'agente (Claude Code o Codex) nella cartella del progetto, dopo
   `git checkout main && git pull`. Scegli il modello e l'impegno che il piano
   indica per il pacchetto (tabella in
   [LAVORO-IN-PARALLELO.md](https://github.com/Matleyx78/solid-space/blob/main/docs/LAVORO-IN-PARALLELO.md), "Modello e impegno").
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
   lettera. Prendi il prossimo pacchetto della tappa <tappa> assegnato a te,
   secondo le regole di AGENTS.md. Realizzalo, apri la pull request, porta la CI
   al verde e fermati, come dice AGENTS.md.
   ```

3. Se l'agente risponde che nessun pacchetto è pronto, è normale: vuol dire che
   aspetta un merge (una dipendenza o i suoi riferimenti). Riprovi dopo.

Per te non cambia niente nei dettagli della CI: conta solo la CI verde della
pull request.

### D5. Seconda opinione (Gemini, a lotti)

Quando ci sono alcune pull request di lavoro aperte, in una sessione di
Antigravity (`agy --mode auto`):

```text
Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
lettera. Rivedi le pull request aperte, come dice AGENTS.md in "Lavoro a
lotti", e alla fine scrivimi un solo riepilogo.
```

Gemini lascia un commento di revisione in ognuna, senza modificare file. Leggi i
commenti prima di unire (D6): se uno chiede correzioni che condividi, passalo
all'agente che ha scritto la pull request.

### D5.1. Segnalazione errori

Quando un agente trova un errore nei riferimenti:

```text
Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
lettera. Controlla la issue #<numero> e leggi i commenti. L'agente che se ne
occupa segnala un errore. Se ha ragione, correggi; altrimenti commenta
spiegando il perché.
```

### D6. Unire

1. Apri la pull request. In basso tutti i controlli devono essere verdi.
2. Scheda **Files changed**: i file devono essere quelli del pacchetto.
3. Squash and merge come in A1, nel repository di solid-space, con la riga `Signed-off-by`.
4. Se ci sono altre pull request aperte, su ognuna premi **Update branch**.
5. Se la pull request ha una riga «Sblocca:» con issue di Tecnigrafo, nel
   Project spostale da *Bloccato* a *Da fare*: il pacchetto unito è già
   utilizzabile, e il pacchetto di Tecnigrafo che lo usa aggiorna lui la versione
   di solid-space fissata in Tecnigrafo, al commit di questo merge. Se con questo
   merge tutti i pacchetti di una richiesta sono uniti, controlla dal lato di
   Tecnigrafo il suo «quando è soddisfatta» e chiudi la issue della richiesta.
6. Riavvia l'agente che ha finito con la stessa frase di D4: prenderà il
   pacchetto successivo. Gemini invece non va riavviato dopo ogni merge: rilancia
   il lotto di D3 quando un merge ha sbloccato nuovi pacchetti di riferimenti
   (la loro riga "Dipende da" è tutta chiusa), e quello di D5 quando si sono
   accumulate nuove pull request da rivedere.

Ripeti D3–D6 finché nel milestone non restano issue aperte.

### D7. Chiudere la tappa

1. A Gemini:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md. Tutte le issue
   del milestone v<tappa> sono chiuse: prepara la pull request di chiusura della
   tappa come descritto in docs/LAVORO-IN-PARALLELO.md, "Chiudere la tappa", sul
   ramo docs/chiusura-<tappa>. Poi fermati.
   ```

2. Leggi la pull request: per ogni criterio di chiusura deve esserci la prova (un
   caso di verifica o un test), oppure "da provare a mano".
3. Fai la **prova manuale** descritta nelle note di rilascio e scrivi l'esito
   nella tabella (puoi chiedere all'agente di compilarla per te, dandogli gli
   esiti).
4. Unisci come in A1, nel repository di solid-space.
5. Nel terminale:

   ```sh
   git checkout main && git pull
   git tag -a v<tappa>.0 -m "solid-space <tappa>.0"
   git push origin v<tappa>.0
   ```

6. Su GitHub: **Releases** → **Draft a new release** → scegli il tag
   `v<tappa>.0` → copia nel testo le note di `docs/rilasci/<tappa>.md` →
   **Publish release**.
7. Per le richieste della tappa ancora aperte, controlla dal lato di Tecnigrafo
   il «quando è soddisfatta» e chiudi le loro issue.
8. Se la tappa successiva di Tecnigrafo ha già richieste valutate, torna a D1
   per la sua tappa di solid-space; altrimenti aspetta le prossime richieste
   (D0).
