# Manutenzione

Le operazioni che il responsabile fa di rado, sui repository e sugli strumenti
condivisi: impostazioni di GitHub, etichette, il Project «Tecnigrafo», un agente
nuovo, le versioni di correzione. Valgono per Tecnigrafo e per
[solid-space](https://github.com/Matleyx78/solid-space), dove non è detto
altro.

Il lavoro di ogni giorno e il ciclo delle tappe sono nella
[guida del responsabile](GUIDA-RESPONSABILE.md); come preparare un PC, in
[AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md).

## 1. Impostazioni dei repository

Si fanno una volta per repository, e si ricontrollano quando cambia la CI.

### 1.1 Pull request e rami

1. **Settings** → **General** → sezione **Pull Requests**.
2. Lascia attivo solo **Allow squash merging**: si unisce sempre così (guida del
   responsabile, A3).
3. Spunta **Automatically delete head branches**: i rami si cancellano da soli
   dopo il merge, e la scheda *Branches* resta pulita.

### 1.2 Il ruleset `main protetto`

Con il ruleset GitHub non ti lascia unire una pull request con la CI rossa per
distrazione.

1. **Settings** → **Rules** → **Rulesets** → **New ruleset** → **New branch
   ruleset**.
2. Nome: `main protetto`. **Enforcement status:** *Active*.
3. **Target branches** → **Add target** → *Include default branch*.
4. Spunta **Restrict deletions** e **Block force pushes**.
5. Spunta **Require status checks to pass** e aggiungi, uno per uno, i controlli
   del repository:

   | Tecnigrafo | solid-space |
   |---|---|
   | `Formatting (clang-format 18)` | `Formatting (clang-format 18)` |
   | `Linux (GCC, clang-tidy)` | `Linux (GCC, clang-tidy)` |
   | `Windows (MSVC)` | `Windows (MSVC)` |
   | `macOS (Apple Clang)` | `macOS (Apple Clang)` |
   | `Linux (ASan, UBSan)` | `Linux (ASan, UBSan)` |
   | `Reference separation (ADR 0011)` | `Reference separation (ADR 0004)` |
   | `Verifica` | |

   I controlli di solid-space sono quelli del suo
   [ADR 0006](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0006-build-ci-e-sottomoduli.md),
   punto 5. Quando la CI aggiunge o rinomina un job obbligatorio, aggiorna qui la
   tabella e il ruleset.
6. Nella sezione **Bypass list** aggiungi *Repository admin*, così in caso di
   emergenza puoi ancora unire tu.
7. **Create**.

Non chiedere approvazioni obbligatorie: tutti gli agenti lavorano con il tuo
account, e GitHub non lascia approvare le proprie pull request. La revisione è
la tua, quando unisci.

In Tecnigrafo `Verifica` esegue su Linux tutti i casi ai due livelli; Windows e
macOS eseguono il livello 1 e un caso di base del livello 2. Nei dettagli
dell'esecuzione, **Artifacts** contiene i rapporti di ogni verifica e, in caso
di errore, le immagini ottenute e le differenze.

### 1.3 CODEOWNERS e AUTHORS

`.github/CODEOWNERS` indica chi deve rivedere i file delicati (ADR, CI,
configurazione di formato e analisi): oggi `@Matleyx78` per tutto. Se il
progetto acquista un altro revisore, lo si aggiunge lì e in `AUTHORS.md`, con
una pull request.

## 2. Etichette

Per crearne una: **Issues** → **Labels** → **New label**, nome e descrizione
come nella tabella, colore a piacere, **Create label**. Le `tappa-X.Y` le crea
l'apertura della tappa (guida del responsabile, B6), le altre si creano a mano,
una volta.

| Etichetta | A cosa serve | Tecnigrafo | solid-space |
|---|---|---|---|
| `pacchetto` | issue di un pacchetto di lavoro (`[X.Y-PN]`) | sì | sì |
| `riferimenti` | casi di verifica di un pacchetto (`[X.Y-RN]`, ADR 0011 in Tecnigrafo, ADR 0004 in solid-space) | sì | sì |
| `tappa-X.Y` | tappa a cui appartiene la issue (una per tappa) | sì | sì |
| `agente:claude`, `agente:codex`, `agente:gemini` | a chi è assegnata la issue | sì | sì |
| `agente:claude-2` | solo issue aperte prima della tappa 0.8 di Tecnigrafo: le prende qualunque sessione di Claude; si elimina quando non ne restano | sì | sì |
| `agente:gemini-argon` | issue che il responsabile passa a Gemini 4 Argon, quando ha quota disponibile | sì | sì |
| `AA-Presa in carico` | qualcuno ci sta lavorando adesso: non toccare | sì | sì |
| `facile`, `media`, `impegnativa` | difficoltà del pacchetto | sì | — |
| `idea` | proposta per le tappe future: si discute, non si implementa | sì | sì |
| `proposta` | proposta in discussione | sì | — |
| `approvato` | approvato dal responsabile (di solito un'idea) | sì | — |
| `bug` | errore da correggere; la mette il modulo della issue | sì | sì |
| `buon-primo-contributo`, `riservato` | pacchetto adatto a chi contribuisce per la prima volta; pacchetto riservato al responsabile | sì | — |
| `richiesta` | bisogno di Tecnigrafo che solid-space deve valutare; la mette il modulo della richiesta | — | sì |
| `contratto` | proposta di cambiare intestazioni pubbliche già accettate come contratto con Tecnigrafo: decidi tu, per i due progetti (guida del responsabile, C) | — | sì |

`richiesta-solid-space`, in Tecnigrafo, è del vecchio flusso, quando le
richieste si scrivevano anche lì: non si usa più, e resta solo sulle issue di
allora.

## 3. Il Project «Tecnigrafo»

Il Project «Tecnigrafo» del tuo account raccoglie issue e pull request di
Tecnigrafo e di solid-space: è la tavola su cui vedi le due tappe aperte e lo
stato delle richieste.

- **Campi:** *Tappa* (`tg-0.7`, `ss-0.2`, …) e *Tipo* (pacchetto, riferimenti,
  richiesta, idea, bug). I valori di *Tappa* si aggiungono a ogni tappa nuova
  (guida del responsabile, B6): gli script non possono crearli.
- **Viste:** «Tappe aperte», «Richieste», «Bloccati» e una per agente.
- **Come entrano le issue.** Quelle di Tecnigrafo entrano da sole (aggiunta
  automatica). L'aggiunta automatica del piano gratuito è una sola e sta su
  Tecnigrafo: **le issue di solid-space entrano nel Project solo se le aggiunge
  qualcuno**, lo script delle richieste oppure tu. Nel Project non scrive nessun
  workflow: gli agenti lavorano con etichette e issue.
- **Aggiungere una issue a mano:** in fondo alla tavola **+ Add item**, incolla
  il link della issue, poi imposta *Tipo* e *Tappa*. Per molte issue di
  solid-space insieme: **+ Add item** → **Add item from repository** →
  solid-space, e selezionale.
- **Il permesso per gli script:** `gh auth refresh -s project`, una volta, sul
  PC dove li lanci ([AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md), sezione 5).

## 4. Aggiungere un agente o un modello

Quando entra un agente nuovo, o un modello che lavora in una sessione sua come
Gemini 4 Argon:

1. Decidilo e scrivilo in [DECISIONI.md](DECISIONI.md) (e nella
   ROADMAP di solid-space, se lavora anche lì).
2. Crea l'etichetta `agente:<nome>` nei repository in cui lavora (sezione 2).
3. In [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md) aggiungi la riga in
   "Chi fa cosa" e, se è un modello nuovo, la sua fascia in "Modello e impegno";
   lo stesso nel documento omonimo di solid-space.
4. In [AGENTS.md](../AGENTS.md), "Comandi", aggiungi il suo suffisso alla lista
   di `TG_BUILD_SUFFIX` (per esempio `-gemini-argon`), così la sua cartella di
   build non si scontra con quelle degli altri.
5. Nella guida del responsabile aggiungi la riga nella tabella di A2 (comando e
   modello) e, se lavora a lotti in una sessione sua, la sua frase in B7.
6. Se è un programma nuovo e non solo un modello: la sua installazione e i suoi
   permessi in [AMBIENTE-SVILUPPO.md](AMBIENTE-SVILUPPO.md), e un suo file di
   istruzioni che rimanda ad `AGENTS.md`, come `CLAUDE.md` e `GEMINI.md`.

## 5. Versioni

- `X.Y.0`: chiusura di una tappa (guida del responsabile, B12). In Tecnigrafo la
  pull request di chiusura porta `project(... VERSION X.Y.0)` del
  `CMakeLists.txt` principale alla versione nuova.
- `X.Y.Z`: solo correzioni di bug della tappa `X.Y` ([ROADMAP.md](ROADMAP.md),
  "Cosa si può fare fuori dalla tappa aperta"), su un ramo `release/X.Y` se nel
  frattempo `main` è già andato avanti. Si pubblicano come una chiusura: la
  versione in `project(... VERSION X.Y.Z)`, il tag `vX.Y.Z` e la *Release* su
  GitHub (guida del responsabile, B12, passi 5 e 6), con note brevi sulle
  correzioni.
