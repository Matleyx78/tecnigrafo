# [F10] `ci`: nessun indirizzo personale nei commit di una pull request

- **Tappa:** fuori tappa: migliora i controlli
  ([ROADMAP.md](../../ROADMAP.md), "Cosa si può fare fuori dalla tappa
  aperta"; [DECISIONI.md](../../DECISIONI.md), 2026-11-15, «Solo indirizzi
  noreply nei commit»)
- **Issue:** da aprire
- **Difficoltà:** facile
- **Agente:** da assegnare dal responsabile
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: è un controllo della CI, lo provano i suoi test
  di unità
- **Dipende da:** —
- **Modulo:** `scripts/` e `.github/workflows/ci.yml`: questa scheda permette
  di modificare `.github/` (AGENTS.md, "Regole che non si possono violare"),
  solo nel punto 3 di «Cosa fare»
- **Da leggere:** [AGENTS.md](../../AGENTS.md), "Commit e pull request";
  l'intestazione di `.github/workflows/ci.yml`; `scripts/sentinella.py` e il
  suo test, come esempio di script Python della CI con i test accanto

## Perché

Lo squash and merge di GitHub copia l'autore di ogni commit della pull request
in una riga `Co-authored-by` del commit su `main`. Se un agente firma con un
indirizzo personale, quell'indirizzo finisce nella storia pubblica e non si
toglie più senza riscriverla. Gli strumenti prendono l'identità dall'account e
non leggono la regola di AGENTS.md quando firmano: serve un controllo che fermi
la pull request prima del merge.

## File

- `scripts/indirizzi_commit.py` (nuovo)
- `scripts/indirizzi_commit_test.py` (nuovo)
- `.github/workflows/ci.yml` (solo il job nuovo)

## Cosa fare

1. `scripts/indirizzi_commit.py <base> <head>` legge con `git log` i commit
   in `<base>..<head>` e, per ciascuno, l'indirizzo dell'autore, quello del
   committer e quelli delle righe `Co-authored-by`. Sono ammessi solo:
   - `…@users.noreply.github.com` (il responsabile, i bot di GitHub);
   - `noreply@anthropic.com`, `codex@openai.com`, `noreply@github.com`.
2. Per ogni indirizzo non ammesso scrive una riga con il commit abbreviato, il
   campo (autore, committer o `Co-authored-by`) e l'indirizzo **mascherato**
   (solo il primo carattere e il dominio: `l…@example.com`), perché il log
   della CI è pubblico; esce con 1. Senza indirizzi non ammessi esce con 0.
   Il messaggio finale dice come rimediare: impostare l'indirizzo noreply e
   riscrivere l'autore dei commit del ramo
   (`git rebase --exec 'git commit --amend --no-edit --reset-author' <base>`),
   poi pubblicare con `--force-with-lease`.
3. In `ci.yml` un job «Commit addresses», solo su `pull_request`, che fa il
   checkout con la storia del ramo (`fetch-depth: 0`) e lancia lo script con
   la base e la testa della pull request. Gira anche quando la pull request
   tocca solo documenti: non dipende dalla build.

## Test richiesti

- Un repository temporaneo con tre commit: uno del responsabile (noreply), uno
  con `Co-authored-by: Claude <noreply@anthropic.com>` e uno con autore
  `someone@example.com`: lo script esce con 1 e segnala solo il terzo.
- Lo stesso con l'indirizzo non ammesso solo nel committer, e solo in una riga
  `Co-authored-by`: entrambi segnalati.
- Tutti i commit ammessi: esce con 0 e non scrive righe di errore.
- L'indirizzo nell'output è mascherato.

## Fatto quando

- Una pull request di prova con un commit firmato da un indirizzo non ammesso
  ha il job «Commit addresses» rosso, con la riga mascherata; la stessa con
  l'autore riscritto lo ha verde; la CI è verde.
