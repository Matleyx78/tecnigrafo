# [0.7.5-P6] `scripts` e CI: il commit del sottomodulo, scritto e controllato da solo

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: strumento del metodo, non cambia il
  comportamento di Tecnigrafo; test di unità con risposte dell'API costruite
  apposta
- **Dipende da:** P2, P5
- **Modulo:** `scripts` e `.github/`
- **Da leggere:** [ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md),
  punto 9; [DECISIONI.md](../../DECISIONI.md), 2026-11-07 («il commit del
  sottomodulo automatico e sicuro») e la proposta del 2026-11-06 nella
  [#486](https://github.com/Matleyx78/tecnigrafo/issues/486);
  [GUIDA-RESPONSABILE.md](../../GUIDA-RESPONSABILE.md), A4 e B11;
  [fuori-tappa/F4](../fuori-tappa/F4-dipendenze.md); `scripts/dipendenze.py` (in
  testa, e le verifiche 5 e 6), `scripts/dipendenze_test.py`,
  `.github/workflows/solid-space-main.yml` (come esempio di workflow che scrive
  issue in sola lettura su solid-space)

## Perché

Quando solid-space chiude un pacchetto, il commit a cui Tecnigrafo deve puntare
lo scrive oggi a mano chi se ne ricorda; la #367 (0.7-P19) è rimasta ferma
perché la issue non lo diceva. Il responsabile non vuole dipendere dalle sue
dimenticanze, ma il puntatore non deve mai muoversi da solo (ADR 0030, punto 9).

## File

- `scripts/dipendenze.py`, `scripts/dipendenze_test.py`
- `.github/workflows/sottomodulo.yml` (nuovo)
- `.github/workflows/ci.yml` (il controllo sulle pull request)
- `docs/GUIDA-RESPONSABILE.md` (A4 e B11), `docs/agenti/contratto.md`
  («Prima della pull request, se sposti il sottomodulo»)

## Cosa fare

### 1. Avvisare da soli

`dipendenze.py` ha un modo `--sottomodulo` che, per ogni issue aperta di
Tecnigrafo le cui dipendenze in solid-space sono tutte chiuse:

- trova la pull request di solid-space che ha chiuso ciascuna e ne prende il
  commit di unione su `main`; il più recente è il commit da puntare;
- se nel testo della issue manca la riga
  `**Sottomodulo di solid-space:** <commit> (chiude <pacchetto> con la pull request <numero>)`,
  la scrive;
- se c'è con un altro commit, **non la cambia**: lo dice in un commento della
  issue, con i due commit e il perché.

Il workflow `sottomodulo.yml` lo esegue ogni giorno e a mano (`workflow_dispatch`),
con `issues: write` solo su Tecnigrafo e niente su solid-space, che legge come
repository pubblico. Non tocca mai il puntatore.

### 2. Impedire l'errore

Un lavoro di `ci.yml` sulle pull request che cambiano
`third_party/solid-space`: legge la issue che la pull request chiude (`Closes
#N`) e fallisce se il nuovo puntatore

- non è un commit di `main` di solid-space, o
- è precedente al commit scritto nella riga della issue, o
- la issue non ha la riga (allora dice di lanciare il workflow del punto 1).

Senza una issue da chiudere (una pull request del responsabile), il lavoro
controlla solo che il commit sia su `main` di solid-space.

## Regole della tappa che valgono qui

- `ci.yml` e `solid-space-main.yml`: prima di te P5. `dipendenze.py`: prima di
  te P2.
- Mai scrivere in solid-space, nemmeno dal workflow (AGENTS.md, «Dove puoi
  scrivere»).

## Test richiesti

- `dipendenze_test.py`: riga assente (scritta), riga uguale (niente), riga con
  un altro commit (commento, riga invariata); dipendenze non tutte chiuse
  (niente); due pacchetti chiusi da due pull request (il commit più recente).
- Il controllo della pull request: puntatore giusto, su un commit precedente, su
  un commit fuori da `main`, issue senza riga. Se il controllo è uno script, i
  suoi test; altrimenti la pull request dice come l'hai provato.

## Fatto quando

- Il workflow del punto 1 scrive la riga mancante e segnala quella diversa senza
  cambiarla.
- Una pull request che sposta il sottomodulo su un commit sbagliato fallisce
  con un messaggio che dice perché.
- I test qui sopra esistono e passano.
- La CI è verde.
