# [0.7.5-P5] CI: confronto dei risultati tra sistemi e controllo incrociato simmetrico

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: infrastruttura; il confronto legge i rapporti
  dei casi esistenti, senza cambiarli
- **Dipende da:** P2, P4
- **Modulo:** `.github/` (più un piccolo script di confronto in `scripts/`)
- **Da leggere:** [ADR 0038](../../adr/0038-ci-dei-due-progetti.md), punti 5, 6 e
  7; [ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md), punto 9;
  [verifica/rapporto.md](../../verifica/rapporto.md), «Struttura»;
  `.github/workflows/ci.yml`, `.github/workflows/solid-space-main.yml`; in
  solid-space, in sola lettura, il lavoro che confronta i risultati tra sistemi
  nel suo `ci.yml` e `.github/workflows/tecnigrafo-core.yml`

## File

- `.github/workflows/ci.yml`
- `.github/workflows/solid-space-main.yml`
- `scripts/confronta_sistemi.py`, `scripts/confronta_sistemi_test.py` (nuovi)
- `docs/verifica/sistemi.md` (nuovo): che cosa si confronta e l'elenco delle
  eccezioni, vuoto

## Cosa fare

### 1. Confronto tra sistemi (ADR 0038, punto 5)

- I lavori `ci-release` di Linux, Windows e macOS caricano i rapporti dei casi
  di verifica (quelli che la verifica scrive in `build/<preset>/tests/verifica/`,
  già caricati oggi come artefatto) in un artefatto per sistema.
- Un lavoro «Cross-platform comparison», dopo i tre, li scarica e li confronta
  con `scripts/confronta_sistemi.py`: per ogni caso, i tre rapporti devono
  essere uguali riga per riga, salvo le righe che variano per costruzione e non
  sono risultati (percorsi, tempi: lo script le elenca in testa, e
  `sistemi.md` le spiega). Le immagini non si confrontano qui.
- Il confronto **blocca**. Un caso nell'elenco delle eccezioni di `sistemi.md`
  si salta, con un avviso che lo nomina.
- Se il primo giro trova differenze, non le metti tra le eccezioni da solo:
  fermati e scrivile nella issue, caso per caso, con le righe dei tre sistemi.
  Decide il responsabile.

### 2. Controllo incrociato simmetrico (ADR 0038, punto 6)

`solid-space-main.yml` diventa la direzione Tecnigrafo → solid-space:

- gira ogni giorno su `main` (non più una volta a settimana) e su ogni pull
  request che tocca `core/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json` o
  `third_party/solid-space`;
- compila con `main` di solid-space, solo in locale nel lavoro, i preset `ci` e
  `ci-release` senza `app`, ed esegue i test di `core` e di solid-space;
- non blocca: sulla pull request è un avviso; su `main` apre o aggiorna la issue,
  come oggi; non sposta mai il puntatore del sottomodulo.

Nella pull request scrivi il testo per solid-space: `tecnigrafo-core.yml`
compila Tecnigrafo anche con `ci-release` (ADR 0038, punto 6). Lo fa una chat di
solid-space fuori tappa; le due pull request si citano.

## Regole della tappa che valgono qui

- `ci.yml`: prima di te P4, dopo di te P6. `solid-space-main.yml`: dopo di te P6.
- Prima di te P2: `ci-attendi.sh` aspetta solo `ci.yml`, quindi il workflow in
  più sulle pull request non lo confonde.

## Test richiesti

- `confronta_sistemi_test.py`: rapporti uguali; una riga diversa (il caso e la
  riga nell'errore); un caso che manca su un sistema; un caso tra le eccezioni.

## Fatto quando

- Una pull request che tocca il codice ha il confronto tra sistemi, che passa.
- `solid-space-main.yml` gira ogni giorno e sulle pull request di `core`, come
  avviso.
- La CI è verde.
