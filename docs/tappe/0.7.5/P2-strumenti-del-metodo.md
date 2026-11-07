# [0.7.5-P2] `scripts`: `ci-attendi.sh` su `ci.yml`, prefissi `PT`/`PS`, modello nelle pull request

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: strumenti del metodo, non cambiano il
  comportamento di Tecnigrafo; test di unità degli script
- **Dipende da:** —
- **Modulo:** `scripts` (più l'elenco dei casi in attesa e i modelli di
  `.github/`)
- **Da leggere:** [LAVORO-IN-PARALLELO.md](../../LAVORO-IN-PARALLELO.md), «Come
  leggere i riferimenti» (i prefissi) e «Chi fa cosa» (il modello);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-07, «Proposte di metodo»;
  [tappe/0000-modello.md](../0000-modello.md), sezione 6; `scripts/ci-attendi.sh`,
  `scripts/apri-tappa.sh`, `scripts/dipendenze.py`,
  `tests/verifica/pending_list.cmake`; in solid-space, in sola lettura,
  `scripts/ci-attendi.sh` (che filtra già su `ci.yml`)

## File

- `scripts/ci-attendi.sh`
- `scripts/apri-tappa.sh`
- `scripts/dipendenze.py`, `scripts/dipendenze_test.py`
- `tests/verifica/pending_list.cmake`
- `.github/pull_request_template.md`
- `docs/tappe/0000-modello.md`, `docs/tappe/0000-pacchetto.md` (gli esempi dei
  nomi), `docs/agenti/riferimenti.md` e `docs/agenti/pacchetto.md` dove nominano
  `RN`, `PN` o i file `<X.Y>-R<N>.txt`

## Cosa fare

### 1. `ci-attendi.sh` aspetta la run di `ci.yml`

Senza argomenti, o con un commit, lo script prende oggi la run più recente del
commit di **qualunque** workflow (`actions/runs?head_sha=…&per_page=1`): con due
workflow sulla stessa pull request può aspettare quello sbagliato. Deve
prendere la run di `ci.yml` per quel commit
(`actions/workflows/ci.yml/runs?head_sha=…`), come fa già solid-space. Con un
identificatore di run, lo script aspetta quella run, come oggi.
`TG_ATTESA_WORKFLOW` (predefinito `ci.yml`) permette di aspettarne un altro.

### 2. Prefissi dalla tappa 0.8

Dalla tappa 0.8 i pacchetti di Tecnigrafo si scrivono `PT`, `RT`, `CT` e quelli
di solid-space `PS`, `RS`, `CS`; le tappe fino alla 0.7.5 restano con `P`, `R`,
`C`, e nulla si rinomina (LAVORO-IN-PARALLELO.md). Quindi:

- `apri-tappa.sh` accetta nella tabella `PTN`/`RTN` per le tappe dalla 0.8 e
  `PN`/`RN` per quelle prima, e rifiuta la forma sbagliata per la tappa con un
  messaggio che lo dice; il testo delle issue nomina il file dei casi in attesa
  con la forma giusta (`0.8-RT5.txt`);
- `dipendenze.py` legge le due forme nei piani di Tecnigrafo (`P3` e `PT3`) e
  in quelli di solid-space (`P11` e `PS11`), anche con la lettera (`RT14a`) e
  negli intervalli (`PS4–PS7`);
- `pending_list.cmake` accetta `<tappa>-RT<N>.txt` dalla 0.8, oltre a
  `<tappa>-R<N>.txt`;
- i rami: `pt0.8-05-…` e `rt0.8-05-…` nei testi che li citano.

### 3. Strumento e modello nelle pull request

Il modello della pull request chiede, nella sezione sull'uso dell'IA, lo
strumento **e** il modello (per esempio «Claude Code, Claude Opus 5.5»), come
dice già AGENTS.md; e l'esempio di «Tappa e pacchetto» mostra la forma con i
prefissi (`0.8 / PT5`).

## Regole della tappa che valgono qui

- Prima di te F5 (le tappe con tre numeri), negli stessi file.
- `dipendenze.py`: dopo di te P6.
- In solid-space i prefissi toccano anche `tests/corpus/valida_corpus.py` e il
  suo modello delle pull request: non si fanno qui, li fa una chat di
  solid-space fuori tappa. Scrivi nella pull request che cosa deve cambiare lì.

## Test richiesti

- `dipendenze_test.py`: un piano della 0.8 con `PT`/`RT`, uno della 0.7 con
  `P`/`R`, un piano di solid-space con `PS`; le dipendenze e gli intervalli con
  le due forme; un piano della 0.8 con `P3` rifiutato.
- `ci-attendi.sh`: la pull request dice come l'hai provato (per esempio su un
  commit con due workflow) e con quale risultato.

## Fatto quando

- `ci-attendi.sh` senza argomenti aspetta la run di `ci.yml`.
- I test qui sopra esistono e passano.
- Il modello della pull request chiede strumento e modello.
- La CI è verde.
