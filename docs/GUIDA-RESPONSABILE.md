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

### A2. Creare due etichette

1. Su GitHub: **Issues** → **Labels** → **New label**.
2. Nome `riferimenti`, descrizione `Casi di verifica di un pacchetto (ADR 0011)`,
   colore a piacere. **Create label**.
3. Di nuovo **New label**: nome `agente:gemini`, descrizione
   `Assegnato a Gemini`. **Create label**.

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
     `macOS (Apple Clang)`, `Reference separation (ADR 0011)`.
   - Nella sezione **Bypass list** aggiungi *Repository admin*, così in caso di
     emergenza puoi ancora unire tu.
   - **Create**.

   Da quel momento GitHub non ti lascia unire una pull request con la CI rossa
   per distrazione. Quando arriverà il passo `Verifica` (tappa 0.4, pacchetto 4)
   lo aggiungerai qui allo stesso modo.

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
   cambiarlo scrivi `/model`, scegli il modello Gemini dell'elenco (sul piano
   gratuito, Gemini Flash; i modelli Gemini più potenti, come Gemini 4 Argon,
   richiedono un abbonamento Google AI Pro o Ultra) e con le frecce `←` `→`
   porta *Effort* su **high**. Poi `Invio`.

9. Prima prova. Scrivi:

   ```text
   Leggi GEMINI.md e AGENTS.md. Spiegami in cinque righe cosa devi fare e cosa
   non devi mai fare in un pacchetto di riferimenti.
   ```

   Se la risposta parla di casi di verifica, di non eseguire Tecnigrafo per i
   valori attesi e di licenze, è pronto.

10. Antigravity chiede il permesso prima di eseguire comandi o modificare file.
    Leggi e approva. Non usare mai l'opzione `--dangerously-skip-permissions`,
    che approva tutto da sola.

Antigravity si aggiorna da solo. Per uscire premi `Ctrl+C` (se non basta, una
seconda volta).

---

## B. Il ciclo di ogni tappa

Le frasi indicano la tappa con `<tappa>`, per esempio `0.4`.

### B1. Preparare il piano (tu e Claude)

1. Apri una chat con Claude nel progetto Tecnigrafo.
2. Scrivi:

   ```text
   Prepariamo il piano dettagliato della tappa <tappa> partendo dalla scheda in
   docs/tappe e dal modello tappe/0000-modello.md. Per ogni pacchetto indica
   difficoltà, agente del lavoro (Codex facili e medie, Claude medie e
   impegnative) e riferimenti (agente gemini, oppure "non servono" con il
   motivo). Proponi anche gli ADR che servono.
   ```

3. Discutete finché il piano ti convince. Gli ADR nuovi nascono *Proposti*: li
   accetti quando ti convincono, anche subito (GOVERNANCE.md).
4. Claude apre una pull request con piano e ADR. La leggi e la unisci come in A1.

### B2. Aprire la tappa (Claude crea le issue)

1. Scrivi a Claude:

   ```text
   Apri la tappa <tappa> secondo docs/ROADMAP.md, "Aprire una tappa": crea il
   milestone v<tappa>, le issue [<tappa>-PN] e [<tappa>-RN] con le etichette
   indicate nel piano, scrivi i numeri delle issue nel documento della tappa e
   aggiorna la tabella della ROADMAP. Apri una pull request con le modifiche ai
   documenti.
   ```

2. Su GitHub apri **Issues** e filtra per il milestone: controlla che ogni issue
   abbia l'etichetta `agente:…` giusta.
3. Unisci la pull request come in A1.

> **Solo nella 0.4:** i primi quattro pacchetti costruiscono il sistema di
> verifica e non hanno riferimenti. Gemini comincia dal pacchetto di riferimenti
> R2 (i casi per ciò che esiste già), che dipende da P1.

### B3. Riferimenti (Gemini)

1. Nel terminale, nella cartella del progetto:

   ```sh
   cd ~/progetti/tecnigrafo && git checkout main && git pull && agy
   ```

2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Prendi il prossimo pacchetto di riferimenti della tappa <tappa>
   assegnato a te, secondo le regole di AGENTS.md. Scrivi i casi di verifica,
   apri la pull request e fermati.
   ```

3. Quando Gemini ha aperto la pull request, la **rivedi tu**. È il controllo più
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
   `git checkout main && git pull`.
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Prendi il prossimo pacchetto della tappa <tappa> assegnato a te,
   secondo le regole di AGENTS.md. Realizzalo, apri la pull request e fermati.
   ```

3. Se l'agente risponde che nessun pacchetto è pronto, è normale: vuol dire che
   aspetta un merge (una dipendenza o i suoi riferimenti). Riprovi dopo.

### B5. Seconda opinione (Gemini, facoltativa)

Per una pull request di lavoro importante, in una sessione di Antigravity (`agy`,
con un modello Gemini):

```text
Rivedi la pull request #<numero> del repository Matleyx78/tecnigrafo seguendo
AGENTS.md. Non modificare file: scrivi le tue osservazioni come commento nella
pull request.
```

### B6. Unire

1. Apri la pull request. In basso tutti i controlli devono essere verdi.
2. Scheda **Files changed**: i file devono essere quelli del pacchetto.
3. Squash and merge come in A1, con la riga `Signed-off-by`.
4. Se ci sono altre pull request aperte, su ognuna premi **Update branch**.
5. Riavvia l'agente che ha finito con la stessa frase di B4 (o di B3 per
   Gemini): prenderà il pacchetto successivo.

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
| Un agente dice che un riferimento è sbagliato | Leggi il suo commento nella issue. Se ha ragione, fai correggere il riferimento a Gemini (B3, indicando la issue); se ha torto, rispondi nella issue e fallo continuare. |
| Conflitto con `main` | Scrivi all'agente di quella pull request: `Aggiorna la pull request #<numero> con main e risolvi i conflitti.` |
| Una issue ha `AA-Presa in carico` ma nessuno ci lavora più | Togli l'etichetta a mano e riavvia l'agente. |
| Un agente non risponde da 14 giorni | Il pacchetto torna libero: cambi l'etichetta `agente:…` e lo assegni a un altro. |
| Gemini ha finito il limite di uso (`/usage`) | Riprendi quando il limite si azzera, oppure fai scrivere quei riferimenti a un agente che **non** lavora a quel pacchetto. |
| Non sai cosa fare | Apri una chat con Claude nel progetto e incolla questa riga: `Sono al passo <passo> della guida e succede questo: …` |
