# [0.7.5-P38] CI: i test si eseguono in parallelo

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: infrastruttura; nessun caso cambia le sue
  verifiche
- **Dipende da:** P37 (stesso `.github/workflows/ci.yml`); P4 viene dopo questo
  pacchetto
- **Nata:** 2026-11-08, dai tempi della CI dopo P36 e P37 (esecuzione della
  #605). Con ccache la build ASan dura 18 s, e il job ASan resta a 648 s quasi
  solo per *Test* (423 s) e *Verifica* (146 s). I casi vanno uno alla volta su
  macchine con 3 o 4 processori. `gtest_discover_tests` fa di ogni caso di
  `tg_tests` un processo: sono 1673, che sotto ASan costano in media 0,21 s
  l'uno, e i 726 casi di verifica avviano ognuno il programma. Decisione del
  responsabile ([DECISIONI.md](../../DECISIONI.md), 2026-11-08)
- **Modulo:** `.github/` (questa scheda lo autorizza), `scripts/controlla.sh` e
  i test che condividono una cartella fissa
- **Da leggere:** i passi `ctest` di `.github/workflows/ci.yml`;
  `scripts/controlla.sh` (passo dei test); `scripts/ci-annota.sh` (come legge
  l'uscita di `ctest`); `tests/script/file_test.cpp` (`emptyFolder`,
  `sourcesFolder`); `tests/io/file_access_test.cpp` (cartella per caso, con il
  nome del caso)

## File

- `.github/workflows/ci.yml`
- `scripts/controlla.sh`
- `tests/script/file_test.cpp`
- gli altri file di test in cui il punto 2 trova una cartella condivisa: la pull
  request li elenca, con il motivo

## Cosa fare

1. **Parallelo nella CI.** Ogni chiamata a `ctest` di `ci.yml` riceve il
   numero di processori della macchina: per i test e per la verifica, nei job
   Linux GCC, Verifica, ASan, Windows e macOS.
   - Linux: `-j "$(nproc)"`;
   - macOS: `-j "$(sysctl -n hw.ncpu)"`;
   - Windows: `-j $env:NUMBER_OF_PROCESSORS`.

   Un commento in cima al file dice perché, con i tempi di questa scheda.
   `scripts/ci-annota.sh` deve trovare ancora i casi falliti: le righe
   `Test #N: … ***Failed` non cambiano forma, ma escono in ordine diverso.
   Va provato con un caso fatto fallire apposta in un commit che poi si toglie.
2. **Nessuna cartella condivisa.** Con i casi in processi diversi e
   contemporanei, due casi che svuotano la stessa cartella fissa si cancellano i
   file a vicenda.
   - **Il caso noto.** In `tests/script/file_test.cpp`, `sourcesFolder()` svuota
     `files/sorgenti` per 9 casi. Ogni cartella creata da `emptyFolder` prende
     il nome del caso in corso, come fa `tests/io/file_access_test.cpp` con
     `::testing::UnitTest::GetInstance()->current_test_info()`.
   - **Cercare gli altri.** Si cercano in tutti i test (`tests/**/*_test.cpp`):
     ogni `remove_all`, ogni cartella sotto `TG_*_TEST_OUTPUT` o `kOutput` con
     un nome fisso usato da più di un caso. Si correggono allo stesso modo.
   - Niente `RUN_SERIAL` né `RESOURCE_LOCK` per aggirare una collisione che si
     può togliere. Se un caso ha davvero bisogno di una risorsa unica, usa
     `RESOURCE_LOCK` con un commento che dice quale risorsa e perché.
3. **In locale.** Il passo dei test di `scripts/controlla.sh` usa
   `-j "$(nproc)"` per i test e per la verifica.
4. **Prova delle collisioni.** In locale, su Linux, e prima della pull
   request:

   ```sh
   ctest --preset ci -LE verifica -j 8 --schedule-random --repeat until-fail:3
   ctest --preset ci -L verifica -j 8 --schedule-random
   ```

   Si eseguono tre volte di seguito; la pull request riporta l'esito.

Fuori dal pacchetto: raggruppare i casi di `tg_tests` in meno processi, cioè
cambiare `gtest_discover_tests`, e i `TIMEOUT`, che restano come sono.

## Regole della tappa che valgono qui

- Nessun caso si toglie, si salta o cambia le sue verifiche: nei test cambiano
  solo i nomi delle cartelle di lavoro.
- `.github/workflows/ci.yml` è in catena: P1 → P3 → P36 → P37 → P38 → P4 → P5
  → P6.

## Test richiesti

- I test esistenti passano in parallelo sui tre sistemi, in CI e con la prova
  del punto 4.
- La pull request riporta la prova di `ci-annota.sh` con un caso che fallisce
  in parallelo, fatta in un commit che si toglie prima del merge.

## Fatto quando

- Ogni `ctest` di `ci.yml` e di `scripts/controlla.sh` gira in parallelo, e il
  commento in `ci.yml` dice perché.
- Nessun test svuota una cartella con un nome fisso usato da più di un caso.
  La pull request elenca i file corretti, oppure dice che oltre a
  `tests/script/file_test.cpp` non ce n'erano.
- La prova del punto 4 passa tre volte di seguito.
- Nel job ASan del ramo *Test* e *Verifica* durano insieme al più la metà dei
  569 s di oggi (esecuzione della #605). La pull request riporta, prima e dopo,
  i tempi di *Test* e *Verifica* di ogni job.
- CI verde sui tre sistemi.
