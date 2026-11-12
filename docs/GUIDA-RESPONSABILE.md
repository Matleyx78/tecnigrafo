# Guida del responsabile, passo per passo

Questa guida è per il responsabile del progetto. Dice **cosa fare, in che
ordine, e cosa scrivere agli agenti**, senza dover ricordare le regole. Le regole
complete sono in [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md),
[ROADMAP.md](ROADMAP.md), nell'[ADR 0011](adr/0011-modalita-di-controllo.md),
nel nucleo di [AGENTS.md](../AGENTS.md) e in [agenti/](agenti/).

È l'**unica guida del responsabile per i due progetti**: Tecnigrafo e il suo
kernel, [solid-space](https://github.com/Matleyx78/solid-space). La guida di
solid-space rimanda qui. Ci sono le cose di ogni giorno e il ciclo delle tappe;
il resto sta in altri due documenti:

| Che cosa | Dove |
|---|---|
| Lavoro di ogni giorno e ciclo di una tappa, nei due repository | questa guida |
| Preparare un PC: Debian 13, WSL, Windows, account, agenti | [AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md) |
| Impostazioni dei repository, etichette, Project, un agente nuovo, versioni di correzione | [MANUTENZIONE.md](MANUTENZIONE.md) |

Come si usa:

- la **parte A** è quello che fai tutti i giorni, in tutti e due i repository;
- la **parte B** si ripete a ogni tappa, dall'inizio alla fine: una tappa di
  Tecnigrafo e, se chiede qualcosa al kernel, la tappa di solid-space che le
  corrisponde;
- la **parte C** serve quando qualcosa non va;
- le frasi da copiare sono nei riquadri grigi: si copiano così come sono,
  cambiando solo ciò che sta tra `< >`.

---

## A. Ogni giorno

### A1. Una chat, un repository

Cartelle di lavoro sul PC: `~/progetti/tecnigrafo` e `~/progetti/solid-space`.

I due progetti hanno lo stesso responsabile, ma ognuno ha le sue regole. La regola
per gli agenti è la stessa nei due `AGENTS.md` ("Dove puoi scrivere"): **una
chat, un repository**.

- **Nel repository della chat** l'agente scrive codice e documenti, entro il suo
  pacchetto.
- **Nell'altro** legge soltanto. Scrive **un commento** su una issue o pull
  request solo se glielo chiedi tu, indicando quale. Niente file, rami, commit,
  pull request, etichette, chiusure.
- Una sola eccezione in più: un agente di Tecnigrafo può **aprire una
  richiesta** in solid-space, se glielo chiedi tu (B2).

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

### A2. Aggiornare la cartella e avviare un agente

Prima di avviare un agente nel terminale, porta la cartella all'ultimo `main`.

In Tecnigrafo:

```sh
cd ~/progetti/tecnigrafo && git checkout main && git pull \
  && git submodule update --init third_party/solid-space
```

Il comando dopo `git pull` porta solid-space, il kernel, al commit che `main`
fissa. Si scrive **senza** `--recursive`, e si ripete dopo ogni `git pull`.

In solid-space:

```sh
cd ~/progetti/solid-space && git checkout main && git pull
```

Poi, nella stessa cartella, avvia l'agente:

| Agente | Comando | Modello |
|---|---|---|
| Claude Code | `claude` | quello che il piano indica per il pacchetto |
| Codex | `codex` | quello che il piano indica per il pacchetto |

Modello e impegno di ogni pacchetto sono nella tabella del piano; le fasce sono
in [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md), "Modello e impegno", e
quando cambiano i modelli disponibili si segue quella tabella. Dal 2026-11-11
gli agenti sono solo Claude e Codex ([DECISIONI.md](DECISIONI.md)).

Ogni pull request dichiara strumento, modello e impegno: servono alla misura
dei pacchetti ([DECISIONI.md](DECISIONI.md), 2026-11-10). Se l'agente scrive
«impegno non noto», correggi la riga prima di unire, con l'impegno che hai
scelto all'avvio.

### A3. Unire una pull request

Vale per **ogni** pull request, nei due repository.

1. Apri la pull request su GitHub. In basso tutti i controlli devono essere
   verdi.
2. Scheda **Files changed**: i file devono essere quelli del pacchetto.
   In Tecnigrafo, se la pull request tocca `app/` o `resources/`, apri la sua
   esecuzione **CI** e scarica **galleria** da **Artifacts**. Il riepilogo del
   lavoro Linux elenca i dodici PNG: finestra con ribbon, finestra classica,
   ribbon, Properties, Layers e barra di stato, ciascuno nei temi chiaro e
   scuro. Confrontali con le [immagini dell’ADR 0039](adr/0039/README.md) e con
   la galleria della pull request precedente prima di unire. Le finestre sono
   a 1600×1000 pixel, scala 1, con una polilinea selezionata e impostazioni
   temporanee vuote; i nomi restano uguali fra le pull request. La CI controlla
   la generazione delle immagini; la valutazione dell’aspetto spetta a te.
3. **Revisione prima del merge.** In solid-space, se la descrizione comincia con
   **Revisione del contratto: da fare**, la pull request cambia un'intestazione
   del contratto già accettata: uniscila solo dopo il commento **Verbale della
   revisione del contratto**, di un agente diverso da chi l'ha scritta (di
   norma Codex; Claude se l'ha scritta Codex). Se manca, avvia la revisione:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
   lettera. Rivedi la pull request #<numero>, che porta "Revisione del
   contratto: da fare", come dice docs/CONTRATTI.md in "Le correzioni del
   contratto": verbale nella pull request e una issue contratto per ogni buco
   nuovo. Prendila in carico e scrivi i commenti, ma non modificarla: niente
   commit né cambi a file, ramo, titolo o descrizione. Non decidere il
   contratto e non scrivere in Tecnigrafo. Poi fermati.
   ```

   In Tecnigrafo vale lo stesso per **Revisione della correzione: da fare**
   (una pull request che applica le tue decisioni sui buchi delle schede), con
   il **Verbale della revisione della correzione** (B10a, passo 4).

   Il verbale blocca il merge solo per due cose:
   - un **buco visibile**, secondo la regola dei buchi visibili
     ([ADR 0013 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0013-esatto-dove-serve.md),
     punto 3; in Tecnigrafo [agenti/pacchetto.md](agenti/pacchetto.md),
     "Buchi visibili e no"): lo decidi tu, e la decisione entra nella stessa
     pull request;
   - una **regola persa o cambiata** senza una tua decisione.

   Una fascia al confine di una soglia o un buco non visibile li sistema
   l'autore nella stessa pull request, senza una tua decisione
   ([agenti/revisione.md](agenti/revisione.md)). Il secondo giro guarda solo il
   commit di correzione; se la correzione rimette un testo di `main` o fa
   esattamente ciò che il revisore ha chiesto, basta il tuo controllo, senza un
   altro giro ([DECISIONI.md](DECISIONI.md), 2026-11-03).
4. Premi **Squash and merge**.
5. Nel messaggio che compare, controlla che l'ultima riga sia:

   ```text
   Signed-off-by: Matleyx78 <18114626+Matleyx78@users.noreply.github.com>
   ```

   Se manca, aggiungila tu.
6. Premi **Confirm squash and merge**.
7. Se ci sono altre pull request aperte nello stesso repository, aggiornale con
   `main`: su ognuna premi **Update branch** e scegli sempre **Update with merge
   commit**, mai il rebase ([DECISIONI.md](DECISIONI.md), 2026-11-03). Il
   pulsante si vede con l'impostazione del repository **Always suggest updating
   pull request branches**. In alternativa, nel terminale:
   `gh pr update-branch <numero>`.

Quando unisci il pacchetto di una tappa, guarda anche B11.

### A4. Controllare le dipendenze

`scripts/dipendenze.py` confronta il piano della tappa aperta con quello che sta
su GitHub. Ti dice quali issue possono partire, chi blocca le altre, e dove
testo delle issue, relazioni «bloccata da» e Project non seguono più il piano.
Legge anche solid-space, ma non ci scrive mai. Il piano fa fede: lo script
allinea le issue al piano, mai il contrario.

**La prima volta**, sul PC: serve Python 3.11 o più recente (Debian 13 ha la
3.13) e `gh` con lo scope del Project, come per `apri-tappa.sh`:

```sh
gh auth refresh -s project
```

**Come si lancia**, dalla cartella di Tecnigrafo aggiornata (A2):

```sh
scripts/dipendenze.py <tappa>             # controlla e basta
scripts/dipendenze.py <tappa> --applica   # mostra le correzioni, chiede, corregge
```

Con `--applica` lo script elenca le modifiche e chiede `Go on? [s/N]`. Con `s`
riscrive "Dipende da" nel testo delle issue, aggiunge e toglie le relazioni
«bloccata da» e sposta le schede nel Project tra *Bloccato* e *Da fare*. Non
scrive mai il piano, non chiude issue e non tocca solid-space.
`--senza-project` salta il Project, per esempio se `gh` non ha ancora lo scope.

**Come si legge**, sezione per sezione:

| Sezione | Che cosa vuol dire | Che cosa fai |
|---|---|---|
| *Ready* | issue aperte con tutte le dipendenze chiuse, con l'agente e «(in charge)» se qualcuno ci lavora | avvia l'agente della sua etichetta (B7 o B8) |
| *Blocked* | issue aperte e chi le blocca, nei due repository | niente: aspetti quei merge |
| *Without an issue* | righe del piano senza issue, che aspettano una scheda | rilancia `apri-tappa.sh` quando la scheda è unita (B6) |
| *To fix* | testo, relazioni o Project diversi dal piano; righe che citano ancora una richiesta, con la cella "Dipende da" nuova già pronta | per le issue e il Project, `--applica`; per le celle nuove, la pull request del piano (B6, passo 5) |
| *To check* | richieste di solid-space con tutte le issue di «Si chiude con» chiuse | controlla il «quando è soddisfatta» e chiudi la richiesta in solid-space (B11, punto 3) |
| *Information* | etichetta `agente:…` diversa dal piano, avvisi | niente, se l'hai riassegnata tu: decide l'etichetta |

In fondo scrive «Nothing to fix or to check.» se *To fix* e *To check* sono
vuote. Codici di uscita: 0 tutto a posto, 1 qualcosa da correggere o da
controllare, 2 una lettura non è riuscita (il messaggio dice quale), 3 hai
risposto di no.

**Quando si usa:**

| Quando | Perché | Comando |
|---|---|---|
| Subito dopo aver aperto la tappa di solid-space (B6, passo 5) | le righe che citano le richieste devono citare i pacchetti: lo script ti dà le celle nuove da copiare nel piano | `<tappa>`, poi `--applica` dopo la pull request del piano |
| Ogni volta che unisci un pacchetto di solid-space (B11, punto 2) | qualche issue di Tecnigrafo diventa pronta | `--applica` |
| Ogni volta che unisci un pacchetto di Tecnigrafo che sta in «Si chiude con» (B11, punto 3) | una richiesta può chiudersi | `<tappa>`, sezione *To check* |
| Prima di avviare un lotto o un agente in Tecnigrafo (B7, B8) | sai già chi ha lavoro pronto, e non avvii un agente per sentirti dire che non c'è niente | `<tappa>`, sezione *Ready* |
| Quando un agente risponde che nessun pacchetto è pronto e non te lo aspetti | vedi chi lo blocca, oppure un "Dipende da" rimasto indietro | `<tappa>` |
| Dopo aver cambiato "Dipende da" nel piano a tappa aperta (una richiesta nuova, "Modifiche al piano") | issue e relazioni devono seguire il piano | `--applica` |
| Prima di chiudere le tappe (B12) | niente in *To fix* né in *To check*, e nessuna richiesta della tappa ancora aperta | `<tappa>` |

Lo script legge il piano dalla cartella, quindi lancialo **su `main`
aggiornato**: su un ramo vecchio confronterebbe GitHub con un piano superato.

**Il commit del sottomodulo** (ADR 0030, punto 9) non lo scrivi più a mano. Il
workflow *Submodule commit* (`.github/workflows/sottomodulo.yml`) gira ogni
giorno e lancia `scripts/dipendenze.py --sottomodulo --applica`: in ogni issue
aperta con l'etichetta `pacchetto` le cui dipendenze in solid-space sono tutte
chiuse, scrive sotto "Dipende da" la riga

```text
**Sottomodulo di solid-space:** <commit> (chiude <pacchetto> con la pull request Matleyx78/solid-space#N)
```

con il commit di unione su `main` della pull request più recente che chiude
quei pacchetti (le richieste non contano). Se la riga c'è già con un altro
commit **non la cambia**: lo scrive in un commento della issue, una volta sola,
con i due commit e il perché; la correggi tu, se serve. Non sposta mai il
puntatore e non scrive in solid-space. Il workflow fallisce, e GitHub te lo
segnala, se un pacchetto di solid-space è chiuso senza una pull request unita
che lo chiuda, o come «non pianificato»: il log dice quale controllare.

Dopo il merge di un pacchetto di solid-space non serve aspettare il giorno
dopo: dalla scheda *Actions* del repository scegli *Submodule commit* e *Run
workflow*. Dal PC, senza scrivere niente, `scripts/dipendenze.py --sottomodulo`
mostra che cosa scriverebbe (codice 1 se c'è qualcosa da scrivere, da
segnalare o da controllare).

---

## B. Il ciclo di una tappa

Una tappa di Tecnigrafo che chiede qualcosa al kernel ha una tappa
corrispondente di solid-space («solid-space 0.2 per Tecnigrafo 0.7»), e le due
avanzano insieme ([ADR 0030](adr/0030-solid-space-kernel-di-tecnigrafo.md),
punto 13; [ADR 0007 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0007-tappe-su-richiesta.md)).
Le tappe di solid-space non si pianificano in anticipo: nascono dalle richieste
di Tecnigrafo. Il ciclo è uno solo:

| Passo | Repository della chat | Chi |
|---|---|---|
| B1. Preparare il piano di Tecnigrafo | Tecnigrafo | tu e Claude |
| B2. Aprire le richieste | solid-space | tu, con uno script o con un agente |
| B3. Valutare le richieste | solid-space | tu e Claude |
| B3a. Rivedere il contratto | solid-space | Codex, poi tu |
| B4. Le schede dopo la revisione del contratto | Tecnigrafo | tu e Claude |
| B5. Preparare il piano di solid-space | solid-space | tu e Claude |
| B6. Aprire le tappe | tutti e due | tu, con lo script o con Claude |
| B7. Riferimenti | tutti e due | Codex o Claude, l'agente che non fa il lavoro; anche a lotti |
| B8. Lavoro | tutti e due | Claude e Codex |
| B9. Seconda opinione | tutti e due | chi ha scritto i riferimenti; anche a lotti |
| B10. Un riferimento contestato | tutti e due | chi ha scritto i riferimenti |
| B10a. Un agente si ferma su un buco | Tecnigrafo | tu, poi Claude e Codex |
| B11. Unire i pacchetti | tutti e due | tu |
| B12. Chiudere le tappe | tutti e due | Codex, poi tu |
| B13. La pausa tra le tappe | tutti e due | tu e Claude |

Una tappa che non chiede nulla a solid-space salta B2–B5. Da B7 in poi i passi
sono gli stessi nei due repository; le frasi indicano il repository con
`<repository>` (`tecnigrafo` o `solid-space`) e la tappa con `<tappa>`, per
esempio `0.7` per Tecnigrafo e `0.2` per solid-space. Quello che cambia tra i
due:

| | Tecnigrafo | solid-space |
|---|---|---|
| Cartella | `~/progetti/tecnigrafo` | `~/progetti/solid-space` |
| Dopo `git pull` | `git submodule update --init third_party/solid-space` (A2) | niente |
| Casi di verifica | `tests/verifica/` | `tests/corpus/` |
| Controllo che separa riferimenti e lavoro | `Reference separation (ADR 0011)` | `Reference separation (ADR 0004)` |
| Chi crea le issue all'apertura | tu, con `scripts/apri-tappa.sh` | Claude, su tua richiesta |
| Valore di *Tappa* nel Project | `tg-<tappa>` | `ss-<tappa>` |
| Prova manuale alla chiusura | Windows e Linux | quella delle note di rilascio |

Gli script del responsabile stanno in `scripts/` di Tecnigrafo e sono due:
`richiesta.sh` apre una richiesta in solid-space (B2), `apri-tappa.sh` apre una
tappa di Tecnigrafo (B6). Tutti e due mostrano prima che cosa faranno e chiedono
conferma; non uniscono, non chiudono e non accettano nulla. Le opzioni sono
scritte in testa a ciascuno. Scrivono nel Project «Tecnigrafo» se `gh` ha il
permesso `project` ([AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md), sezione 5);
senza, lavorano lo stesso e lo dicono.

### B1. Preparare il piano di Tecnigrafo (tu e Claude)

1. Apri una chat con Claude nel progetto, con Tecnigrafo in scrittura.
2. Scrivi:

   ```text
   Prepariamo il piano dettagliato della tappa <tappa> partendo dalla scheda in
   docs/tappe e dai modelli tappe/0000-modello.md e tappe/0000-pacchetto.md,
   con le regole di docs/LAVORO-IN-PARALLELO.md ("Preparare il piano"). Per ogni
   pacchetto indica difficoltà, agente del lavoro, fascia di modello e impegno,
   e chi scrive i riferimenti: l'altro agente, che poi rivede la pull request
   del lavoro (Codex per i pacchetti di Claude, Claude per quelli di Codex), in
   fascia forte per i casi delicati e leggera per gli ordinari, oppure "non
   servono" con il motivo. Per ogni divisione e per ogni impegnativo scrivi il
   motivo in una riga, e cerca prima le divisioni che tolgono file comuni dalle
   catene. Proponi anche le bozze degli ADR che servono, e i bisogni da
   chiedere a solid-space (docs/RICHIESTE-A-SOLID-SPACE.md) con il testo delle
   richieste, ciascuna nella forma del file di scripts/richiesta.sh. Scrivi
   subito le schede dei pacchetti che non dipendono da solid-space; per gli
   altri metti solo la riga nella tabella, con "scheda dopo il contratto".
   Se una scheda introduce tipi o funzioni nuove in core, aggiungi il
   pacchetto di revisione delle interfacce (tappe/0000-modello.md, sezione 6).
   Le idee sono già decise nella pausa (B13): metti nel piano quelle che ho
   approvato per questa tappa. Se ne trovi altre aperte nate dopo la pausa,
   dimmi quali potrebbero entrare e perché: le approvo io, una per una.
   ```

3. Discutete finché il piano ti convince. Gli ADR nuovi nascono *Proposti*: li
   accetti quando ti convincono, anche subito ([GOVERNANCE.md](../GOVERNANCE.md)).
4. Claude apre una pull request con piano e ADR. Leggila. Se il piano non ha
   richieste a solid-space, uniscila (A3) e passa a B6. Se le ha, non unirla
   ancora: prima B2.

### B2. Aprire le richieste a solid-space

Una richiesta nasce in Tecnigrafo: di solito mentre prepari il piano (B1),
oppure perché un agente di Tecnigrafo si ferma in un pacchetto e ne scrive il
testo nella issue (C). Il testo segue il modello
[richieste/0000-modello.md](https://github.com/Matleyx78/solid-space/blob/main/docs/richieste/0000-modello.md)
di solid-space ([RICHIESTE-A-SOLID-SPACE.md](RICHIESTE-A-SOLID-SPACE.md)): sette
campi, dice di che cosa c'è bisogno e non come soddisfarlo, e ogni riferimento è
assoluto (`Matleyx78/tecnigrafo#N`). In "Da dove nasce" ognuna cita la pull
request del piano, come `Matleyx78/tecnigrafo#<numero>`.

Le richieste si aprono subito, senza aspettare l'apertura della tappa, e **solo
in solid-space**: in Tecnigrafo non se ne tiene una copia. Hai tre modi:

- **chiedi a Claude**, nella chat del piano, collegando solid-space in scrittura
  (in una sessione cloud lo script apre la issue ma non arriva al Project: lo
  dice, e la aggiungi tu come in [MANUTENZIONE.md](MANUTENZIONE.md), sezione 3):

  ```text
  Apri in solid-space le richieste che abbiamo approvato, con
  scripts/richiesta.sh e --tappa ss-<tappa di solid-space>, come dice
  AGENTS.md in "Dove puoi scrivere". Poi scrivi i riferimenti nella tabella
  "Richieste a solid-space" del piano, nella pull request del piano.
  ```

- **dal tuo PC**, un file per richiesta, nella cartella di Tecnigrafo:

  ```sh
  scripts/richiesta.sh richiesta.md --tappa ss-<tappa di solid-space>
  ```

  Lo script mostra la issue e chiede conferma; alla fine stampa il
  riferimento (`Matleyx78/solid-space#N`): chiedi a Claude di scriverlo nella
  tabella del piano, sezione 5;
- **da GitHub**: in solid-space **Issues** → **New issue** → **Richiesta di
  Tecnigrafo**, compila i campi e premi **Create**. L'etichetta `richiesta` la
  mette il modulo. Poi aggiungi la issue al Project
  ([MANUTENZIONE.md](MANUTENZIONE.md), sezione 3) con *Tipo* = richiesta e
  *Tappa* = `ss-<tappa>`.

Entro un paio di minuti il controllo automatico di solid-space legge la issue. Se
compare un suo commento con un elenco, completa la issue (**⋯** → **Edit**): il
commento si aggiorna da solo e, quando è tutto a posto, lo dice. Il controllo
guarda solo campi e riferimenti, non decide nulla. Senza questa issue nessuno
valuta nulla.

Quando la tabella del piano ha i riferimenti, unisci la pull request del piano
(A3). Da lì le richieste le segui nella vista «Richieste» del Project.

### B3. Valutare le richieste (tu e Claude, in solid-space)

1. Apri una chat con Claude nel progetto, con solid-space in scrittura, e
   scrivi:

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
2. Leggi la valutazione e il contratto, e discutetene. Gli ADR nuovi nascono
   *Proposti*: quando ti convincono, scrivi nella chat «accetto gli ADR», aspetta
   che li porti in stato *Accettato* e unisci (A3), nel repository di solid-space
   ([GOVERNANCE.md](https://github.com/Matleyx78/solid-space/blob/main/GOVERNANCE.md)).
   Da quel momento le intestazioni sono il contratto: appena gli ADR sono
   accettati avvia la revisione in B3a, anche con la tappa in preparazione.
   Le schede di Tecnigrafo che le usano aspettano la chiusura di `[X.Y-C]`
   (B4), senza aspettare il codice di solid-space.
3. Se l'esito è 4, chiudi la issue con la motivazione. Con gli esiti 1, 2 e 3 la
   richiesta entra nella tappa di solid-space della sua tappa di Tecnigrafo:
   - se quella tappa è **in preparazione**, quando le valutazioni di tutte le
     sue richieste conosciute sono accettate, fai B3a e B4, poi B5;
   - se è **già aperta** (la richiesta è nata mentre Tecnigrafo percorreva la
     tappa), rivedi anche il contratto nuovo come in B3a prima di B4; poi passa
     a B5 con la frase per aggiungere una richiesta;
   - se è aperta un'altra tappa di solid-space, quella di una tappa precedente
     di Tecnigrafo, la richiesta aspetta che si chiuda. Prima passa quella che
     blocca la tappa aperta di Tecnigrafo.

### B3a. Rivedere il contratto (Codex, poi tu, in solid-space)

Appena gli ADR di valutazione sono accettati, senza aspettare l'apertura della
tappa, crea in solid-space la issue `[X.Y-C]` di revisione del contratto,
secondo [docs/CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md):
etichette `tappa-X.Y`, `pacchetto`, `contratto`, `impegnativa` e
`agente:codex`. Indica gli ADR e le intestazioni da controllare. La revisione
vale dalla prossima tappa di solid-space; i blocchi sui buchi valgono subito
anche nei pacchetti rimasti della tappa 0.2 (Tecnigrafo 0.7).

1. Avvia un agente di fascia forte con impegno alto, **diverso da chi ha scritto
   l'ADR e le intestazioni**, di norma Codex. Con solid-space in scrittura e
   Tecnigrafo in lettura, scrivi:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
   lettera. Prendi la issue [<tappa>-C] #<numero>, assegnata a te, e segui
   docs/CONTRATTI.md: controlla ogni funzione con la lista fissa, apri una issue
   contratto per ogni buco visibile, annota nel verbale quelli non visibili e
   scrivi il verbale nella issue della revisione. Non decidere il contratto e
   non scrivere in Tecnigrafo. Poi fermati.
   ```

2. Leggi il verbale e ogni issue `contratto`: che cosa manca, l'esempio con i
   dati, le scelte possibili e il consiglio motivato. **Decidi tu** nella issue
   di ciascun buco. La issue `contratto` e la tua decisione servono solo per i
   buchi visibili; quelli non visibili restano annotati nel verbale
   ([docs/CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md),
   "Buchi visibili e no"). Se non ci sono buchi visibili, chiudi `[X.Y-C]` dopo
   il verbale: non serve una pull request.
3. Se ci sono buchi visibili, dopo averli decisi tutti avvia lo stesso agente
   in una nuova sessione:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo alla
   lettera. Riprendi la issue [<tappa>-C] #<numero> e applica le mie decisioni
   nelle issue contratto collegate, secondo docs/CONTRATTI.md. Apri una sola
   pull request con intestazioni, riga Corretto: nell'ADR, schede interessate,
   docs/DECISIONI.md e Modifiche al piano. La descrizione usa Closes #N per
   chiudere la revisione e le issue dei buchi. Porta la CI al verde e fermati.
   Non scrivere in Tecnigrafo.
   ```

4. Anche questa pull request cambia il contratto: porta in testa **Revisione
   del contratto: da fare**, e prima del merge la rivede con la lista un agente
   diverso da chi l'ha scritta, di norma Claude se l'ha scritta Codex (A3,
   punto 3). Leggi il verbale e uniscila con la CI verde. Controlla che
   `[X.Y-C]` e le issue dei buchi siano chiuse. **Solo allora passa a B4**.
   Nel piano di solid-space (B5) tutti i riferimenti e i pacchetti di lavoro
   che usano quelle intestazioni dipendono da `[X.Y-C]`.

### B4. Le schede dopo la revisione del contratto (tu e Claude, in Tecnigrafo)

Quando la issue `[X.Y-C]` di solid-space è **chiusa** (B3a), apri una chat con
Claude (Tecnigrafo in scrittura, solid-space in lettura):

```text
Scrivi le schede dei pacchetti della tappa <tappa> che aspettavano il
contratto di solid-space, ora che la issue [<tappa-ss>-C] #<numero> è chiusa,
sulle intestazioni accettate e riviste in solid-space, con le regole di
docs/tappe/0000-modello.md ("Richieste a solid-space"). Ogni scheda dice quali intestazioni usa, il commit del contratto da cui parte e,
se sposta il sottomodulo, a quale pacchetto di solid-space. Riferimenti
limitati alla facciata e ai comandi. Per le schede che introducono tipi o
funzioni nuove in core aggiungi il pacchetto di revisione delle interfacce
(docs/tappe/0000-modello.md, sezione 6), prima di loro nella tabella.
Annota la modifica in "Modifiche al piano" e apri una pull request.
```

La unisci (A3). Se la tappa è già aperta, rilancia poi lo script di B6: crea
solo le issue nuove.

**Revisione delle interfacce.** Le schede che introducono tipi o funzioni nuove
in `core` si controllano con la lista di
[docs/CONTRATTI.md di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/CONTRATTI.md)
(punti 1–11, con il 10 e l'11) **prima che i loro pacchetti partano**: lo fa il
pacchetto di revisione della tabella, che quei pacchetti citano in "Dipende
da" (vale per ogni piano, anche per le schede scritte in B1 senza contratto).

1. Quando la sua issue esiste, avvia un agente di fascia forte con impegno
   alto, diverso da chi ha scritto le schede (di norma Codex), con Tecnigrafo in
   scrittura e solid-space in lettura:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Prendi la issue [<tappa>-P<N>] #<numero>, revisione delle
   interfacce, assegnata a te, e segui la sua scheda: verbale nella issue, con
   un commento per ogni buco visibile e quelli non visibili annotati nel
   verbale. Non correggere le schede e non scrivere in solid-space. Poi
   fermati.
   ```

2. Leggi il verbale. Se non ci sono buchi visibili, chiudi la issue: i
   pacchetti partono.
3. Se ci sono buchi visibili, **decidi tu** ciascuno; quelli non visibili
   restano annotati nel verbale e li sceglie chi implementa
   ([agenti/pacchetto.md](agenti/pacchetto.md), "Buchi visibili e no"). Poi,
   in una chat con Claude, fai
   scrivere le decisioni in `docs/DECISIONI.md`, le correzioni delle schede e
   una riga in "Modifiche al piano", in una pull request che chiude la issue
   della revisione con `Closes #N`. Prima del merge falla rileggere con la lista
   all'agente della revisione, sulle sole interfacce corrette (verbale nella
   pull request), poi uniscila (A3). La correzione segue B10a, passi 3–5:
   esempi nelle schede, riga **Revisione della correzione: da fare**, due
   domande in più nella rilettura.
4. Un buco che sta nel contratto di solid-space e non nella scheda segue la
   strada solita (C, buco nel contratto).

### B5. Preparare il piano di solid-space (tu e Claude)

Una tappa di solid-space raccoglie tutte le richieste di una tappa di
Tecnigrafo.

1. Apri una chat con Claude nel progetto, con solid-space in scrittura.
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/solid-space. Leggi AGENTS.md e seguilo.
   Prepariamo la tappa <tappa> di solid-space per la tappa <tappa-tg> di
   Tecnigrafo, dalle richieste #<a>, #<b>, … e dai loro ADR di valutazione
   <NNNN>, <NNNN>, …: scrivi la scheda e il piano dettagliato partendo dai
   modelli tappe/0000-modello.md e tappe/0000-pacchetto.md,
   con le regole di docs/LAVORO-IN-PARALLELO.md ("Preparare il piano"). La
   scheda rimanda agli ADR senza ripeterli. Riporta il pacchetto [<tappa>-C]
   di revisione del contratto e la sua issue già chiusa (B3a) come prima riga:
   tutti i riferimenti e i pacchetti di lavoro che usano quelle intestazioni
   dipendono da lui. I pacchetti realizzano le intestazioni del contratto
   rivisto senza cambiarle; metti prima i pacchetti che
   sbloccano la tappa di Tecnigrafo, e scrivi nella scheda di ciascuno quali
   issue di Tecnigrafo sblocca, se esistono già. I casi concreti delle richieste
   e i DXF di Tecnigrafo entrano nei pacchetti di riferimenti, come dice
   tests/corpus/README.md. Per ogni
   pacchetto indica difficoltà, agente del lavoro, fascia di modello e impegno,
   e chi scrive i riferimenti: l'altro agente, che poi rivede la pull request
   del lavoro (Codex per i pacchetti di Claude, Claude per quelli di Codex), in
   fascia forte per i casi delicati e leggera per gli ordinari, oppure "non
   servono" con il motivo. Per ogni divisione e per ogni impegnativo scrivi il
   motivo in una riga. Proponi anche le bozze degli ADR che servono.
   Le idee sono già decise nella pausa (B13): metti nel piano quelle che ho
   approvato per questa tappa. Se ne trovi altre aperte nate dopo la pausa,
   dimmi quali potrebbero entrare e perché: le approvo io, una per una.
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
   accetti quando ti convincono, anche subito.
4. Claude apre una pull request con piano e ADR. La leggi e la unisci (A3), nel
   repository di solid-space.

### B6. Aprire le tappe

Prima di tutto, nel Project «Tecnigrafo» aggiungi al campo *Tappa* i valori
della tappa nuova: **⋯** → **Settings** → *Tappa* → aggiungi `tg-<tappa>` (e
`ss-<tappa>` per quella di solid-space). Gli script non possono crearli.

**Tecnigrafo, con lo script.** Le issue le crea `scripts/apri-tappa.sh`, dalla
tabella dei pacchetti del piano unito. Si lancia dal tuo PC, perché serve
l'accesso al Project.

1. Aggiorna la cartella come in A2, poi:

   ```sh
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
4. Unisci la pull request dello script (A3).

Quando più avanti aggiungi pacchetti alla tappa (per esempio le schede di B4),
rilancia i due comandi: lo script crea solo le issue nuove.

**solid-space, con Claude.** La tappa di solid-space si apre quando la sua tappa
precedente è chiusa. Vale anche per i pacchetti aggiunti a tappa aperta: per
quelli si creano solo le issue nuove.

1. In una chat con solid-space in scrittura, scrivi a Claude:

   ```text
    Apri la tappa <tappa> secondo docs/ROADMAP.md, "Aprire una tappa": crea il
    milestone v<tappa>, l'etichetta tappa-<tappa>, le issue [<tappa>-PN] e
    [<tappa>-RN] con le etichette del piano (tappa-<tappa>, pacchetto o
    riferimenti, la difficoltà e l'agente della riga). Con tutte le issue create,
    scrivi i loro numeri nel documento della tappa e nella riga "Dipende da" di
    ogni issue, e segna le dipendenze «bloccata da» come dice
    docs/LAVORO-IN-PARALLELO.md, sezione 2. Aggiorna la tabella della ROADMAP. Apri
    una pull request con le modifiche ai documenti. Usa solo il piano che sta in
    main: se docs/tappe/<tappa>-*.md non c'è in main, fermati e dimmelo, senza usare
    rami o pull request aperte.
   ```

2. Su GitHub apri **Issues** e filtra per il milestone: controlla che ogni issue
   abbia l'etichetta `agente:…` giusta.
3. Metti le issue nuove nel Project: nella vista a tabella, **+ Add item** →
   **Add item from repository** → solid-space, seleziona le issue del
   milestone. A ciascuna metti *Tappa* = `ss-<tappa>` e *Tipo* = pacchetto o
   riferimenti. Controlla che compaiano nella vista «Tappe aperte».
4. Unisci la pull request (A3), nel repository di solid-space.
5. **Subito dopo, in Tecnigrafo:** le issue che in "Dipende da" citano le
   richieste devono citare i pacchetti di solid-space. Finché citano la
   richiesta, che resta aperta fino alla fine, nessun agente le prende, anche
   quando quello che serve è già unito (è successo con R14 e R22 della 0.7, il
   2026-11-01). Nel terminale, nella cartella di Tecnigrafo aggiornata (A2):

   ```sh
   scripts/dipendenze.py <tappa>
   ```

   Per ogni riga che cita una richiesta lo script stampa la cella "Dipende da"
   nuova, presa dalla tabella «Che cosa sblocca in Tecnigrafo» del piano di
   solid-space (riga «cites the request …; with the packages of solid-space it
   is: …»). In una chat con Tecnigrafo in scrittura e solid-space in lettura,
   incolla quelle righe e scrivi a Claude:

   ```text
   Copia nella colonna "Dipende da" del piano della tappa <tappa> le celle
   proposte da scripts/dipendenze.py qui sopra, aggiungi una riga in "Modifiche
   al piano", scrivi nella issue dei pacchetti di Tecnigrafo che spostano il
   sottomodulo il commit di solid-space, se è già unito, e apri una pull request
   con il piano. Non scrivere niente in solid-space.
   ```

   Unisci la pull request (A3), aggiorna la cartella e lancia:

   ```sh
   scripts/dipendenze.py <tappa> --applica
   ```

   Lo script mostra le modifiche e chiede `Go on? [s/N]`: con `s` riscrive
   "Dipende da" nel testo delle issue, sistema le relazioni «bloccata da» e
   sposta nel Project da *Bloccato* a *Da fare* le issue pronte. Non tocca il
   piano né solid-space. Rilancialo senza opzioni: deve dire «Nothing to fix or
   to check.».

### B7. Riferimenti (l'altro agente, anche a lotti)

I riferimenti di un pacchetto li scrive l'agente che non lo realizza: Codex per
i pacchetti di Claude, Claude per quelli di Codex, in fascia forte per i casi
delicati e leggera per gli ordinari, come dice il piano. Puoi avviarlo su un
solo pacchetto, con la frase di B8, o su tutti quelli pronti, a lotti, con la
frase qui sotto.

1. Aggiorna la cartella del repository e avvia l'agente con il modello e
   l'impegno del piano (A2). In Tecnigrafo, prima, lancia
   `scripts/dipendenze.py <tappa>` (A4): se sotto *Ready* non c'è nessun
   pacchetto di riferimenti con la sua etichetta, non avviarlo.
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/<repository>. Leggi AGENTS.md e seguilo alla
   lettera. Prendi i pacchetti di riferimenti pronti della tappa <tappa>
   assegnati a te, uno dopo l'altro, come dice AGENTS.md in "Lavoro a lotti".
   Per ognuno apri la pull request e porta la CI al verde; alla fine scrivimi
   un solo riepilogo.
   ```

   L'agente lavora da solo finché i pacchetti pronti non finiscono. Puoi
   allontanarti: alla fine trovi il riepilogo con tutte le pull request.

3. Rivedi tu **ogni** pull request del lotto, una alla volta. È il controllo più
   importante di tutta la tappa. Guarda queste cose:
   - [ ] ogni condizione "Fatto quando" del pacchetto ha almeno un caso;
   - [ ] i valori attesi hanno senso e non vengono dal programma che verificano
         (per esempio: in Tecnigrafo un cerchio di raggio 10 ha circonferenza
         62.83…; in solid-space un cubo di lato 1 ha volume 1, area 6 e 6
         facce);
   - [ ] ci sono casi limite (in Tecnigrafo: Esc a metà comando, annulla e
         ripeti, punti coincidenti; in solid-space: valori nulli, punti
         coincidenti o quasi allineati, facce tangenti, spessori minimi);
   - [ ] per ogni file preso dal web c'è il link alla licenza, e la licenza
         permette di usarlo (MIT, BSD, CC0, CC BY, GPL 3 vanno bene; "solo uso
         personale" o nessuna licenza no);
   - [ ] solo in Tecnigrafo: i valori presi dall'archivio di solid-space citano
         il caso, con percorso e commit;
   - [ ] la pull request tocca solo i casi di verifica (`tests/verifica/` in
         Tecnigrafo, `tests/corpus/` in solid-space).
4. Se qualcosa non va, scrivilo come commento nella pull request e di'
   all'agente che l'ha scritta:

   ```text
   Leggi i commenti della pull request #<numero> e correggi.
   ```

5. Se va bene, unisci (A3 e B11).

### B8. Lavoro (Claude e Codex)

Ogni agente in una sessione sua. Possono lavorare nello stesso momento.

1. Aggiorna la cartella del repository e avvia l'agente (Claude Code o Codex)
   con il modello e l'impegno che il piano indica per il pacchetto (A2).
2. Scrivi:

   ```text
   Lavora sul repository Matleyx78/<repository>. Leggi AGENTS.md e seguilo alla
   lettera. Prendi il prossimo pacchetto della tappa <tappa> assegnato a te,
   secondo le regole di AGENTS.md. Realizzalo, apri la pull request, porta la CI
   al verde e fermati, come dice AGENTS.md. Se è una revisione delle
   interfacce, segui invece la sua scheda: verbale nella issue, senza
   correggere le schede né aprire pull request, poi fermati.
   ```

   La stessa frase vale per i pacchetti di revisione delle interfacce, che il
   piano mette nella tabella con gli altri (B4, "Revisione delle interfacce").

3. Se l'agente risponde che nessun pacchetto è pronto, è normale: vuol dire che
   aspetta un merge (una dipendenza o i suoi riferimenti). Riprovi dopo. In
   Tecnigrafo, per sapere che cosa aspetta, e per non avviarlo a vuoto, guarda
   prima *Ready* e *Blocked* di `scripts/dipendenze.py <tappa>` (A4).

Gli agenti fanno le prove locali con `scripts/controlla.sh` (AGENTS.md,
"Comandi"), aspettano la CI con `scripts/ci-attendi.sh` e, se è rossa, ne leggono
la causa con `scripts/ci-errori.sh` (AGENTS.md, "Quando fermarsi"). Una pull
request di soli documenti, commenti o casi non si compila in locale (AGENTS.md,
"Prima di dichiarare il lavoro finito"). Per te non cambia niente: conta solo la
CI verde della pull request.

### B9. Seconda opinione (chi ha scritto i riferimenti, anche a lotti)

Una pull request di lavoro con la CI verde la rivede l'agente che ha scritto i
riferimenti del pacchetto: Codex le pull request di Claude, Claude quelle di
Codex. Le pull request in fascia media che toccano parti delicate hanno una
revisione mirata in fascia forte. Quando ci sono alcune pull request di lavoro
aperte, avvia l'agente (A2) e scrivi:

```text
Lavora sul repository Matleyx78/<repository>. Leggi AGENTS.md e seguilo alla
lettera. Rivedi le pull request aperte, come dice AGENTS.md in "Lavoro a
lotti", e alla fine scrivimi un solo riepilogo.
```

L'agente lascia un commento di revisione in ognuna, senza modificare file, e
rivede solo quelle scritte da un modello diverso dal suo. Leggi i commenti prima
di unire: se uno chiede correzioni che condividi, passalo all'agente che ha
scritto la pull request.

### B10. Un riferimento contestato

Un agente di lavoro che trova un caso di verifica sbagliato si ferma e lo scrive
nella issue del pacchetto (AGENTS.md, "Modalità di controllo e riferimenti").
Leggi il suo commento: se ha torto, rispondi nella issue e fallo continuare. Se
ha ragione, o se non sai decidere, avvia l'agente che ha scritto i riferimenti
(A2) e scrivi:

```text
Lavora sul repository Matleyx78/<repository>. Leggi AGENTS.md e seguilo alla
lettera. Leggi i commenti della issue #<numero>: l'agente che lavora al
pacchetto segnala un errore nei riferimenti della pull request #<numero>. Se ha
ragione, correggi i riferimenti; altrimenti spiega il perché in poche righe in
un commento della issue.
```

Quando i riferimenti corretti sono uniti, il pacchetto di lavoro si riprende con
la frase di B8.

### B10a. Un agente si ferma su un buco (tu, poi Claude e Codex)

Un agente si ferma **solo per un buco visibile**, anche nelle schede
([agenti/pacchetto.md](agenti/pacchetto.md), "Buchi visibili e no"; ADR 0013 di
solid-space, punto 3). Si ferma prima di scrivere codice o casi e scrive nella
issue del suo pacchetto i buchi, ciascuno con un esempio e le scelte possibili.
Un buco non visibile invece non lo ferma: lo sceglie chi implementa, lo scrive
nella scheda o nel commento dell'interfaccia, lo fissa con un test e lo segnala
nella pull request. Tu lo leggi al merge (A3).

Succederà ancora: la revisione delle interfacce riduce i buchi, non li elimina,
e una decisione scritta come regola breve può aprirne di nuovi (B10 e B11 della
#464 hanno aperto due dei quattro buchi di P22 nella #373). Per i buchi
visibili, ogni volta:

1. **Smista.** Per ogni buco: sta nella **scheda** (il contratto di solid-space
   determina la risposta, Tecnigrafo no), nel **contratto** (manca anche in
   solid-space) o **non si sa**. Un buco che non si sa lo smisti tu, in chat
   con Claude, prima di decidere. I buchi nel contratto prendono la strada di
   C (issue `contratto` in solid-space); gli altri restano qui.
2. **Decidi** nella issue del pacchetto, un buco per commento, scegliendo una
   delle uscite scritte dall'agente (A, B, …) o scrivendone una con lo stesso
   esempio. Se una scelta dipende da AutoCAD, la prova a mano si fa **prima**
   di decidere ([dxf-di-prova.md](tappe/0.7/dxf-di-prova.md)).
3. **Una sola pull request di correzione**, in una chat con Claude e
   Tecnigrafo in scrittura:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Applica le mie decisioni sui buchi della issue #<numero> come dice
   la guida in B10a, passo 3: docs/DECISIONI.md, le schede che usano le
   interfacce corrette, la riga Corretto: degli ADR se il loro testo cambia e
   Modifiche al piano. Ogni decisione va nella scheda con il suo esempio e la
   risposta scelta, in "Test richiesti". La descrizione comincia con
   "Revisione della correzione: da fare" e le regole che cambiano. Porta la CI
   al verde e fermati.
   ```

   La pull request non chiude la issue del pacchetto: il pacchetto riparte dopo
   il merge. Se un buco va in solid-space, la pull request lo scrive nella
   scheda ("Dipende da") e il pacchetto aspetta anche la issue `contratto`.
4. **Rilettura con la lista.** Prima del merge la rilegge un agente diverso da
   chi l'ha scritta (di norma Codex), sulle sole regole nuove o corrette:

   ```text
   Lavora sul repository Matleyx78/tecnigrafo. Leggi AGENTS.md e seguilo alla
   lettera. Rivedi la pull request #<numero>, che porta "Revisione della
   correzione: da fare", come dice AGENTS.md in "Le correzioni delle schede si
   rivedono": verbale nella pull request, con un buco nuovo per commento.
   Prendila in carico e scrivi i commenti, ma non modificarla: niente commit
   né cambi a file, ramo, titolo o descrizione. Non scrivere in solid-space.
   Poi fermati.
   ```

   Se il verbale apre buchi visibili nuovi, torna al passo 2 sulla stessa pull
   request. Il secondo giro segue A3, punto 3.
5. **Unisci** (A3). Se una decisione cambia una risposta che si vede con una
   domanda del catalogo ([verifica/domande.md](verifica/domande.md)), apri una
   issue di riferimenti con la lettera dopo il numero (`[X.Y-R<N>b]`, di chi ha
   scritto i riferimenti del pacchetto) e
   aggiungila in "Dipende da" del pacchetto; altrimenti bastano i test della
   scheda.
6. **Riprendi il pacchetto** con la frase di B8.

### B11. Unire i pacchetti

Ogni pull request si unisce come in A3. In più, durante una tappa:

1. **Riavvia** l'agente di lavoro che ha finito con la stessa frase di B8:
   prenderà il pacchetto successivo. I lotti invece non si rilanciano dopo ogni
   merge: rilancia quello di B7 quando un merge ha sbloccato nuovi pacchetti
   di riferimenti (la loro riga "Dipende da" è tutta chiusa), e quello di B9
   quando si sono accumulate nuove pull request da rivedere.
2. **Quando unisci un pacchetto di solid-space**, la sua pull request elenca
   nella riga «Sblocca:» le issue di Tecnigrafo che aspettavano proprio quel
   pacchetto: possono partire subito, senza aspettare la fine della tappa di
   solid-space. La riga «Sblocca:» è un promemoria scritto a mano: fa fede
   "Dipende da" della issue di Tecnigrafo. Nella cartella di Tecnigrafo lancia
   `scripts/dipendenze.py <tappa> --applica`: elenca sotto «Ready» le issue
   che possono partire, e con `s` le sposta da *Bloccato* a *Da fare* nel
   Project. Non chiudere la richiesta anche se tutti i suoi pacchetti sono
   uniti: si chiude al punto 3.
   La riga «Sottomodulo di solid-space» della issue la scrive il workflow
   *Submodule commit* (A4): lancialo a mano dopo il merge, se l'agente deve
   partire subito.
3. **Quando unisci il pacchetto di Tecnigrafo che usa il pezzo nuovo**, il
   sottomodulo deve stare al commit della riga «Sottomodulo di solid-space»
   della sua issue, o a uno successivo di `main` di solid-space (ADR 0030, punto
   9). Lo controlla il lavoro *Submodule pointer (ADR 0030)* della CI: fallisce,
   dicendo perché, se il puntatore non è su `main` di solid-space, se è
   precedente al commit della riga, o se la issue non ha la riga (allora lancia
   *Submodule commit* e rilancia il lavoro). Ogni issue chiusa deve avere la
   sua riga. Una tua pull request senza issue da chiudere controlla solo che il
   commit sia su `main`. Il controllo legge i `Closes #N` della descrizione,
   quindi la CI riparte, tutta, anche quando la descrizione cambia. Se il pacchetto è
   l'ultimo della colonna «Si chiude con» di una richiesta (piano, sezione 5,
   «Richieste a solid-space»), controlla il «quando è soddisfatta»
   della richiesta e chiudi la sua issue in solid-space. `scripts/dipendenze.py
   <tappa>` lo segnala sotto «To check».

Ripeti B7–B11 finché nel milestone non restano issue aperte.

### B12. Chiudere le tappe

Si chiude prima la tappa di solid-space, se c'è, poi quella di Tecnigrafo.
Prima di cominciare, `scripts/dipendenze.py <tappa>` di Tecnigrafo (A4) non deve
avere niente in *To fix* né in *To check*: le richieste della tappa sono già
chiuse.

Le regole di metodo non cambiano durante una tappa, salvo un vero blocco: un
lavoro che non può andare avanti con le regole attuali, e lo stabilisci tu.
Durante la tappa le proposte si raccolgono in una issue per tappa, «[X.Y]
Proposte di metodo», con l'etichetta `proposta`, e si decidono nella pausa dopo
la chiusura (B13; [DECISIONI.md](DECISIONI.md), 2026-11-03 e 2026-11-07).

1. Avvia Codex, in fascia leggera (A2), e scrivi:

   ```text
   Lavora sul repository Matleyx78/<repository>. Leggi AGENTS.md. Tutte le issue
   del milestone v<tappa> sono chiuse: prepara la pull request di chiusura della
   tappa come descritto in docs/LAVORO-IN-PARALLELO.md, "Chiudere la tappa", sul
   ramo docs/chiusura-<tappa>. Poi fermati.
   ```

2. Leggi la pull request: per ogni criterio di chiusura deve esserci la prova (un
   caso di verifica o un test), oppure "da provare a mano". In Tecnigrafo
   controlla anche che `project(... VERSION <tappa>.0)` nel `CMakeLists.txt`
   principale sia aggiornato.
3. Fai la **prova manuale** descritta nelle note di rilascio (per Tecnigrafo su
   Windows e su Linux) e scrivi l'esito nella tabella. Puoi chiedere all'agente
   di compilarla per te, dandogli gli esiti.
4. Unisci (A3).
5. Nel terminale, nella cartella del repository:

   ```sh
   git checkout main && git pull
   git tag -a v<tappa>.0 -m "<Tecnigrafo o solid-space> <tappa>.0"
   git push origin v<tappa>.0
   ```

6. Su GitHub: **Releases** → **Draft a new release** → scegli il tag
   `v<tappa>.0` → copia nel testo le note di `docs/rilasci/<tappa>.md` →
   **Publish release**.
7. Solo per solid-space: le richieste della tappa dovrebbero essere già chiuse
   (B11, punto 3). Per quelle ancora aperte controlla dal lato di Tecnigrafo il
   «quando è soddisfatta» e chiudile, oppure scrivi nella issue che cosa manca.
8. Passa alla pausa (B13), e solo dopo a B1 per la tappa successiva di
   Tecnigrafo. Se ha già richieste valutate, prepara anche la sua tappa di
   solid-space (B5); altrimenti solid-space aspetta le prossime richieste (B2).

### B13. La pausa tra le tappe

Chiuse le tappe dei due progetti, con **tutto fermo** (nessun pacchetto aperto,
nessun agente al lavoro), si ragiona prima di ripartire. Solo quando tutto è
deciso e sistemato si apre la tappa successiva (B1).

1. Apri una chat con Claude nel progetto e scrivi:

   ```text
   Le tappe sono chiuse. Analizza tutte le issue aperte dei due repository con
   l'etichetta idea e le issue «Proposte di metodo» e «Documenti da
   sistemare»: per ognuna dimmi se è già decisa e come, oppure che cosa resta
   da decidere.
   ```

2. Decidi, issue per issue: un'idea entra nella prossima tappa, resta per dopo
   o si scarta; ogni proposta di metodo si applica o si scarta, con il motivo;
   ogni correzione dei documenti si fa.
3. Claude apre, in ciascun repository, una pull request di soli documenti che
   registra le decisioni in `docs/DECISIONI.md`, applica le regole di metodo
   (lo stesso testo nei due repository per le voci comuni), corregge i
   documenti, prepara la scheda della tappa successiva e chiude con
   `Closes #N` le issue esaurite; per le altre scrive un commento con la
   decisione.
4. Leggila, falla correggere se serve e uniscila (A3).
5. Passa a B1.

---

## C. Se qualcosa va storto

| Cosa succede | Cosa fai |
|---|---|
| La CI è rossa | Non unire. Scrivi all'agente: `La CI della pull request #<numero> è rossa: leggi l'errore e correggi.` |
| Fallisce *Reference separation* | La pull request mescola riferimenti e codice. Scrivi all'agente: `Dividi la pull request #<numero> in due, come chiede ADR <0011 in Tecnigrafo, 0004 in solid-space>.` |
| Un agente dice che un riferimento è sbagliato | B10. |
| Conflitto con `main` | Scrivi all'agente di quella pull request: `Aggiorna la pull request #<numero> con main e risolvi i conflitti.` |
| Una issue ha `AA-Presa in carico` ma nessuno ci lavora più | Togli l'etichetta a mano e riavvia l'agente. |
| Un agente non risponde da 14 giorni | Il pacchetto torna libero: cambi l'etichetta `agente:…` e lo assegni a un altro. |
| Arriva una pull request fuori pacchetto, fuori tappa o con la CI rossa da chi contribuisce | Chiudila con un rimando a `CONTRIBUTING.md`, senza discutere il codice ([ROADMAP.md](ROADMAP.md), "Cosa si può fare fuori dalla tappa aperta"). |
| Un agente di Tecnigrafo si ferma e scrive il testo di una richiesta a solid-space nella issue | Leggi il testo. Se serve davvero, aprila in solid-space come in B2 (lo script legge quel testo così com'è, salvato in un file), e annota la modifica nel piano in "Modifiche al piano"; il pacchetto aspetta valutazione e revisione del contratto (B3, B3a, B4). Se non serve, rispondi nella issue e fallo continuare. |
| Un riferimento di Tecnigrafo e l'archivio dei casi di solid-space non concordano | Apri una issue in solid-space con i due valori: decide l'archivio (ADR 0030, punto 14; in solid-space, `tests/corpus/README.md`, "Disaccordi con Tecnigrafo"). Se è sbagliato l'archivio, lo corregge un pacchetto di riferimenti di solid-space; se è sbagliato il riferimento, un pacchetto di riferimenti di Tecnigrafo. |
| Il controllo automatico ha commentato una richiesta di solid-space | Completa la issue come dice il commento (B2). Il commento si aggiorna da solo a ogni modifica; non decide nulla, quindi puoi anche ignorarlo se il riferimento che segnala è voluto. |
| Un agente di Tecnigrafo trova un buco visibile nel contratto di solid-space | Si ferma prima di scrivere codice o casi e scrive nella issue del suo pacchetto il testo della issue `contratto`: che cosa manca, un esempio con i dati, le scelte possibili, titolo con il nome dell'intestazione. La apri tu in solid-space e decidi lì; finché è aperta non parte nessun pacchetto che usa quell'intestazione. Fai applicare la decisione in solid-space e aggiorna le schede di Tecnigrafo interessate (B10a, passi 3–5); poi fai riprendere il pacchetto (AGENTS.md, "Se serve qualcosa a solid-space"). Per un buco non visibile l'agente non si ferma (B10a). |
| Un agente di Tecnigrafo trova un buco visibile nella sua scheda: un'interfaccia di Tecnigrafo che intestazione, ADR e scheda non determinano, mentre il contratto di solid-space sì (come in #359) | B10a: smista i buchi, decidi nella issue, fai scrivere a Claude una sola pull request di correzione con **Revisione della correzione: da fare**, falla rileggere con la lista a Codex, uniscila e fai riprendere il pacchetto. Nessuna issue in solid-space, salvo per i buchi che risultano nel contratto. Un buco non visibile non lo ferma: lo sceglie chi implementa, lo scrive, lo fissa con un test e lo segnala nella pull request, e tu lo leggi al merge (B10a). |
| Serve cambiare un'intestazione di solid-space già accettata come contratto | Apri in solid-space una issue con l'etichetta `contratto` che dice che cosa cambia e perché, e guarda quali schede di Tecnigrafo la usano. Decidi tu per i due progetti; se approvi, la scheda del pacchetto che la cambia lo dice esplicitamente, e in Tecnigrafo si aggiornano le schede che la usano (ADR 0007 di solid-space, punto 15). |
| Un agente ha scritto o proposto qualcosa nell'altro repository senza che tu l'abbia chiesto | Non unire e non rispondere. Chiudi la pull request o cancella il commento, e di' all'agente: `La regola "Dove puoi scrivere" di AGENTS.md vieta di scrivere nell'altro repository: spiega cos'è successo.` (A1) |
| `apri-tappa.sh` o `richiesta.sh` dice che il piano o il file non è pronto | Elenca che cosa correggere, riga per riga. Correggi (o fai correggere a Claude) e rilancia: finché non è tutto a posto non scrive nulla. |
| Non sai più a che punto sei | Scrivi `SITUAZIONE GENERALE` in una chat. |
| Non sai cosa fare | Apri una chat con Claude nel progetto e incolla questa riga: `Sono al passo <passo> della guida e succede questo: …` |
