# Guida del responsabile

Le operazioni che spettano al responsabile del progetto e ai revisori, passo per
passo.

## Configurazione iniziale di GitHub (una volta sola)

1. Crea il repository pubblico e carica il progetto.
2. In `.github/CODEOWNERS` sostituisci `@OWNER` con il tuo nome utente GitHub; in
   `AUTHORS.md` inserisci il tuo nome.
3. **Settings → Branches → Add branch ruleset** sul ramo `main`:
   - *Require a pull request before merging*, con almeno 1 approvazione e
     *Require review from Code Owners*;
   - *Require status checks to pass*: seleziona i quattro job della CI
     (Formatting, Linux, Windows, macOS);
   - *Block force pushes*;
   - non aggiungere eccezioni, nemmeno per te.
4. **Settings → General → Pull Requests:** lascia attivo solo *Allow squash
   merging*.
5. **Issues → Labels:** crea le etichette
   `tappa-0.2` (e così via per ogni tappa), `pacchetto`, `facile`, `media`,
   `impegnativa`, `buon-primo-contributo`, `riservato`, `bug`, `idea`, `approvato`,
   `AA-Presa in carico` (per le issue e le pull request su cui un agente sta lavorando,
   vedi [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md)).
6. Installa l'app gratuita **DCO** di GitHub, che rifiuta i commit senza
   `Signed-off-by`.

## Aprire una tappa

1. Completa il documento in `docs/tappe/` partendo dal modello: sezioni 5-8.
2. Porta gli ADR richiesti in stato *Accettato*.
3. Crea il milestone `vX.Y`, con la descrizione che rimanda al documento.
4. Per ogni pacchetto crea una issue con il modello *Pacchetto di lavoro*:
   titolo `[X.Y-PN] Titolo`, etichette `tappa-X.Y`, `pacchetto`, difficoltà ed
   eventualmente `buon-primo-contributo` o `riservato`; milestone `vX.Y`.
5. Scrivi nel documento della tappa il numero di ogni issue.
6. Aggiorna la tabella in `docs/ROADMAP.md` (stato *aperta*) con una PR.

## Seguire una tappa

- Assegna ogni pacchetto a una sola persona o a un solo agente (etichetta
  `agente:…`); dopo 14 giorni senza notizie rimuovi l'assegnazione. Per più
  agenti in parallelo segui [LAVORO-IN-PARALLELO.md](LAVORO-IN-PARALLELO.md).
- Se un pacchetto si rivela sbagliato, correggi il piano e annota il motivo in
  "Modifiche al piano".
- Le PR fuori pacchetto, fuori tappa o con la CI rossa si chiudono con un
  rimando a `CONTRIBUTING.md`, senza discutere il codice.

## Chiudere una tappa e pubblicare la versione

1. Verifica e spunta ogni criterio di chiusura nel documento della tappa.
2. Esegui la prova manuale su Windows e Linux (o falla eseguire a revisori che
   usano quei sistemi) e annota l'esito. Su macOS falla eseguire se c'è un
   revisore che lo usa; altrimenti scrivi nelle note di rilascio che non è stata
   fatta (ROADMAP, "Decisioni").
3. Scrivi `docs/rilasci/X.Y.md` con novità e limiti noti.
4. Aggiorna `project(... VERSION X.Y.0)` nel `CMakeLists.txt` principale.
5. Dopo il merge: `git tag -a vX.Y.0 -m "Tecnigrafo X.Y.0"` e
   `git push origin vX.Y.0`; crea la *Release* su GitHub copiando le note.
6. Chiudi il milestone, aggiorna `docs/ROADMAP.md` e apri la tappa successiva.

## Versioni

- `X.Y.0`: chiusura di una tappa.
- `X.Y.Z`: solo correzioni di bug della tappa `X.Y`, su un ramo `release/X.Y` se
  nel frattempo `main` è già andato avanti.
