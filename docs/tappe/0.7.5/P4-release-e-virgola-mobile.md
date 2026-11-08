# [0.7.5-P4] CI: build Release con gli avvisi come errori e opzioni di virgola mobile

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: infrastruttura; i casi di verifica esistenti,
  invariati, girano anche in Release e sono la prova
- **Dipende da:** P3, P38 (stesso `ci.yml`; aggiunto il 2026-11-08)
- **Modulo:** `cmake/` e `.github/` (più le correzioni al codice che la build
  Release chiede)
- **Da leggere:** [ADR 0038](../../adr/0038-ci-dei-due-progetti.md), punti 3, 4 e
  7; `CMakeLists.txt`, `CMakePresets.json`, `cmake/TecnigrafoWarnings.cmake`,
  `.github/workflows/ci.yml`; in solid-space, in sola lettura,
  `cmake/SolidSpaceDeterminism.cmake`, `cmake/SolidSpaceWarnings.cmake` e
  `CMakePresets.json`

## File

- `CMakePresets.json` (il preset `ci-release`, con i suoi preset di build e di
  test)
- `cmake/TecnigrafoDeterminism.cmake` (nuovo), `CMakeLists.txt` (lo include e lo
  applica a ogni bersaglio di Tecnigrafo)
- `cmake/TecnigrafoWarnings.cmake`, solo se il confronto del punto 3 lo chiede
- `.github/workflows/ci.yml`
- `docs/adr/0038-ci-dei-due-progetti.md`: solo la riga «Insiemi di avvisi»
  della tabella del punto 7, con l'esito del confronto
- `docs/AMBIENTE-SVILUPPO.md` (il preset nuovo)
- i file di codice che la build Release rifiuta, elencati nella pull request con
  l'avviso che li ha toccati

## Cosa fare

1. **Preset `ci-release`:** Release, `TG_WARNINGS_AS_ERRORS=ON`, con build e
   test come `ci`. Il `release` di oggi resta com'è.
2. **Virgola mobile** (ADR 0038, punto 4): `cmake/TecnigrafoDeterminism.cmake`
   applica `-ffp-contract=off` con GCC e Clang e `/fp:precise` con MSVC a ogni
   bersaglio di Tecnigrafo (librerie, programmi, test), e la configurazione si
   ferma con un errore se le opzioni di compilazione contengono `-ffast-math`,
   `-Ofast`, `-funsafe-math-optimizations`, `/fp:fast` o `/fp:contract`. Stessa
   regola e stessi nomi di quella di solid-space, scritta da capo per Tecnigrafo.
3. **Avvisi:** confronta `TecnigrafoWarnings.cmake` con
   `SolidSpaceWarnings.cmake`, avviso per avviso. Gli avvisi di solid-space che
   Tecnigrafo non ha si aggiungono se il codice li regge con correzioni piccole;
   gli altri si scrivono nella riga della tabella dell'ADR con il motivo.
4. **CI** (ADR 0038, punto 3): un lavoro «Linux Release» (GCC, `ci-release`,
   test e verifica, senza `clang-tidy`); i lavori Windows e macOS passano da
   `ci` a `ci-release`. I nomi dei lavori dicono la configurazione.
5. Un risultato di un caso di verifica che cambia in Release, o con le opzioni
   di virgola mobile, non si corregge toccando il caso: fermati e scrivilo nella
   issue, con il caso, il sistema e i due valori.
6. Nella pull request scrivi che cosa cambia per solid-space (il controllo
   incrociato userà `ci-release`, ADR 0038, punto 6): lo fa P5 dal lato di
   Tecnigrafo e una chat di solid-space dal suo.

## Regole della tappa che valgono qui

- `ci.yml`: prima di te P3, dopo di te P5.
- Nessun avviso disattivato, nessun test tolto (AGENTS.md).

## Test richiesti

- La configurazione con `-DCMAKE_CXX_FLAGS=-ffast-math` si ferma con il
  messaggio dell'ADR: una prova in CI (un passo che si aspetta l'errore) o un
  test di CMake.
- Tutti i test e i casi di verifica passano in `ci-release` sui tre sistemi.

## Fatto quando

- Linux ha `ci` e `ci-release`; Windows e macOS usano `ci-release`; tutti verdi
  con gli avvisi come errori.
- Ogni bersaglio di Tecnigrafo si compila con le opzioni di virgola mobile
  dell'ADR, e le opzioni vietate fermano la configurazione.
- La riga degli avvisi dell'ADR 0038 dice l'esito del confronto.
- La CI è verde.
