# [0.7-P0] Sottomodulo solid-space

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura, non cambia il comportamento del
  programma; un test di unità controlla che le intestazioni si includano
- **Dipende da:** —
- **Modulo:** build (sottomodulo, CMake, CI), più un test
- **Da leggere:** [ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md)
  punti 4–10; l'[ADR 0008 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0008-componente-geom2d.md)
  punti 1, 4 e 5; il `CMakeLists.txt` principale di solid-space (opzioni
  `SOLID_SPACE_BUILD_KERNEL` e `SOLID_SPACE_WARNINGS_AS_ERRORS`);
  `CMakeLists.txt`, `third_party/CMakeLists.txt`, `core/CMakeLists.txt`,
  `cmake/TecnigrafoWarnings.cmake`, `.github/workflows/ci.yml`,
  `scripts/controlla.sh`; in `AGENTS.md` la sezione "Comandi"; in
  `docs/GUIDA-RESPONSABILE.md` le parti B2, B3 e B4

## File

- `.gitmodules` (nuovo), `third_party/solid-space` (sottomodulo)
- `third_party/CMakeLists.txt`
- `core/CMakeLists.txt` (il collegamento)
- `cmake/` se serve (per esempio un controllo del sottomodulo)
- `.github/workflows/ci.yml`
- `scripts/controlla.sh` se serve (il tidy)
- `tests/core/solid_space_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `docs/AMBIENTE-SVILUPPO.md` (clonare e aggiornare con il sottomodulo)
- `AGENTS.md` (sezione "Comandi")
- `docs/GUIDA-RESPONSABILE.md` (B2, B3, B4)

## Cosa fare

È il pacchetto dell'ADR 0030, punto 8: porta solid-space dentro Tecnigrafo
all'inizio della tappa, quando i suoi componenti hanno solo il contratto (le
intestazioni accettate con gli ADR 0008–0012 di solid-space) e nessuna funzione
realizzata. I pacchetti che poi usano i componenti non toccano più `cmake/` né
`.github/`.

**Sottomodulo** (ADR 0030, punto 6):

- `third_party/solid-space`, da `https://github.com/Matleyx78/solid-space.git`,
  fissato all'ultimo commit di `main` di solid-space nel giorno in cui lavori;
  scrivi il commit nella pull request;
- si inizializza **senza** `--recursive`: i sottomoduli di solid-space (OCCT,
  SolveSpace) non servono al 2D e non si scaricano.

**CMake** (ADR 0030, punti 5 e 7):

- `add_subdirectory(solid-space EXCLUDE_FROM_ALL)` in `third_party/`, con
  `SOLID_SPACE_BUILD_KERNEL` spento prima, così si compila solo ciò che `core`
  collega e il 2D non richiede OCCT;
- `SOLID_SPACE_WARNINGS_AS_ERRORS` con lo stesso valore di
  `TG_WARNINGS_AS_ERRORS`: stesse regole di avvisi del nostro codice;
- le intestazioni di solid-space **non** come `SYSTEM`;
- i test di solid-space non si compilano (solid-space li compila solo quando è
  il progetto principale; controllalo);
- nessuna opzione `-ffast-math` o `/fp:fast` arriva ai target di solid-space
  (ADR 0030, punto 7): controlla `cmake/` e i preset, e scrivi nella pull
  request che cosa hai controllato;
- il clang-tidy di Tecnigrafo non analizza i file di solid-space, che hanno il
  loro controllo nella CI di solid-space: se `TG_ENABLE_CLANG_TIDY` o il tidy
  della CI e di `scripts/controlla.sh` li raggiungono, escludili;
- se il sottomodulo non è inizializzato, la configurazione si ferma con un
  messaggio chiaro, che dà il comando
  `git submodule update --init third_party/solid-space`.

**`core`** collega `solid_space::robust` e `solid_space::geom2d` in modo
**privato**: le facciate di `core` che verranno parlano con i tipi di
Tecnigrafo (ADR 0035, punto 1), e nessun altro modulo vede le intestazioni di
solid-space. Oggi nessun file di `core` le include.

**Test** `tests/core/solid_space_test.cpp`, collegato a `solid_space::geom2d`:

- include `solid_space/robust/result.hpp`, `solid_space/robust/point.hpp`,
  `solid_space/geom2d/curves.hpp` e `solid_space/geom2d/diagnosis.hpp`;
- costruisce un `solid_space::geom2d::Result<int>` con un valore e uno con una
  diagnosi, e controlla `hasValue()`, `value()` e `diagnosis().code`;
- costruisce un `Segment`, un `Arc` e un `PolylineVertex` e ne legge i campi.

Non chiama nessuna funzione dichiarata nelle intestazioni: `Result` è l'unica
definizione del contratto (ADR 0008 di solid-space, punto 7), e le funzioni non
si usano prima che il loro pacchetto di solid-space sia unito (AGENTS.md, "Se
serve qualcosa a solid-space").

**CI:** `submodules: true` in ogni `actions/checkout` dei job che compilano,
**non** `recursive`. I job che non compilano (per esempio il filtro dei file
cambiati) restano come sono.

**`docs/AMBIENTE-SVILUPPO.md`:** come clonare (`git clone`, poi
`git submodule update --init third_party/solid-space`) e come aggiornare dopo
un `git pull` che sposta il sottomodulo (lo stesso comando). Niente
`--recursive` e niente `--recurse-submodules`.

**`AGENTS.md`, sezione "Comandi":** una regola per gli agenti: dopo il clone e
dopo ogni `git pull` si esegue `git submodule update --init third_party/solid-space`,
senza `--recursive`, così il sottomodulo è sempre al commit che `main` fissa.
Il resto di `AGENTS.md` non cambia.

**`docs/GUIDA-RESPONSABILE.md`:**

- in B2, B3 (anche nel lotto di Gemini 4 Argon) e B4, dopo ogni `git pull`,
  lo stesso comando `git submodule update --init third_party/solid-space`
  (senza `--recursive`);
- nella lista dei controlli di B3 (punto 3), la voce "i valori presi
  dall'archivio di solid-space citano il caso, percorso e commit", e il numero
  delle voci che la introduce aggiornato.

## Regole della tappa che valgono qui

- Le modifiche a `.gitmodules`, `third_party/`, `cmake/` e `.github/` sono
  autorizzate da questa scheda (ADR 0030, punto 8) e da nessun'altra della
  tappa; quelle ad `AGENTS.md` e a `docs/GUIDA-RESPONSABILE.md` sono solo
  quelle scritte qui.
- `core/CMakeLists.txt`, `tests/CMakeLists.txt`: solo le tue righe; dopo di te
  li toccano altri pacchetti.
- Il sottomodulo lo spostano poi i pacchetti che usano le funzioni nuove, al
  commit che la loro scheda indica (ADR 0030, punto 9).

## Test richiesti

- Il test qui sopra, sui tre sistemi operativi.
- La CI compila senza il kernel di solid-space: nessun job scarica OCCT o
  SolveSpace.

## Fatto quando

- Un clone nuovo con `git submodule update --init third_party/solid-space`
  compila e passa i test; senza, la configurazione dà il messaggio con il
  comando.
- Il test di unità esiste e passa.
- `AGENTS.md`, `docs/AMBIENTE-SVILUPPO.md` e `docs/GUIDA-RESPONSABILE.md`
  dicono come aggiornare il sottomodulo, con lo stesso comando.
- La CI è verde su tutti i sistemi, con il sottomodulo.
