# [0.7.5-P37] CI: su Windows la build Debug usa le Qt ottimizzate

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura; test e casi invariati
- **Dipende da:** P36 (stesso `.github/workflows/ci.yml`); P4 viene dopo questo
  pacchetto
- **Nata:** 2026-11-08, dalla diagnostica della bozza
  [#599](https://github.com/Matleyx78/tecnigrafo/pull/599), chiusa senza merge.
  Su Windows la build Debug si collega alle Qt di debug (`Qt6Cored.dll`,
  `Qt6Guid.dll`, `Qt6Widgetsd.dll`…), e ogni caso di `app_widgets` è da 5 a 20
  volte più lento che su Linux, dove le Qt sono sempre quelle ottimizzate. Con
  le due opzioni del punto 1 `app_widgets` è sceso da 93,6 s a 8,17 s, il passo
  *Test* a 56 s (prima 172–361 s), e tutti i 1673 test e la verifica sono
  passati. Il responsabile accetta di perdere su Windows i controlli del
  runtime di debug di MSVC e delle Qt di debug
  ([DECISIONI.md](../../DECISIONI.md), 2026-11-08)
- **Modulo:** `.github/` (questa scheda lo autorizza), più il limite di
  `app_widgets` in `tests/CMakeLists.txt`
- **Da leggere:** il job `windows` di `.github/workflows/ci.yml`; il blocco
  `if(TG_BUILD_APP)` di `tests/CMakeLists.txt`

## File

- `.github/workflows/ci.yml`
- `tests/CMakeLists.txt` (limite e commento di `app_widgets`)

## Cosa fare

1. **Qt ottimizzate nel job Windows.** Nel passo «Build, warnings as errors»
   del job `windows`, la riga di `cmake --preset ci` riceve anche:

   ```powershell
   -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL "-DCMAKE_MAP_IMPORTED_CONFIG_DEBUG=Release;RelWithDebInfo"
   ```

   Il nostro codice resta Debug: senza ottimizzazioni, con le informazioni di
   debug e con le asserzioni attive (niente `NDEBUG`). Cambiano due cose: il
   runtime di MSVC, che è quello normale, e le librerie importate (Qt), che sono
   quelle ottimizzate. GoogleTest, scaricato con `FetchContent` e
   `gtest_force_shared_crt`, segue da solo. Un commento sopra il passo dice:
   - perché: i tempi della #599;
   - che cosa si perde: i controlli degli iteratori e della memoria del runtime
     di debug di MSVC e le asserzioni interne delle Qt;
   - perché si accetta: gli errori di memoria li cerca il job ASan su Linux, e
     le nostre asserzioni restano attive.

   Non cambiano `cmake/`, i preset, gli altri job e le build in locale.
2. **Limite.** Il `TIMEOUT` di `app_widgets` diventa tre volte la sua durata
   nella CI di Windows del ramo, arrotondata ai 30 s in su e mai sotto 120 s:
   con gli 8 s della #599, **120 s**. Se nel ramo la durata supera 40 s, il
   valore segue la stessa regola e la pull request spiega perché. Il commento
   sopra `add_test(NAME app_widgets …)` dice da dove viene il valore e che il
   limite ferma solo un blocco vero; quello di oggi, sui 300 s di P35, si
   toglie.
3. **Tempi.** La pull request riporta, prima e dopo, la durata di
   `app_widgets` e dei passi *Test* e *Verifica* del job Windows.

## Regole della tappa che valgono qui

- Nessun controllo si toglie e nessun avviso si disattiva: avvisi come errori,
  test, verifica e asserzioni restano come oggi.
- `.github/workflows/ci.yml` è in catena: P1 → P3 → P36 → P37 → P4 → P5 → P6.
- `tests/CMakeLists.txt` è un file comune: cambia solo le tue righe e, se un
  altro pacchetto è arrivato prima, aggiornati.

## Test richiesti

Non ci sono test nuovi: le prove sono l'esecuzione della CI del ramo, che la
pull request riporta.

- Nel log della build di Windows si vede che `tg_app_tests.exe` dipende da
  `Qt6Core.dll` e non da `Qt6Cored.dll`, per esempio con
  `dumpbin /dependents` in un passo che la pull request toglie prima del merge.
- `ActionRegistryDeathTest.AnIdCanBeAddedOnlyOnce`, che si basa su
  un'asserzione, passa su Windows.

## Fatto quando

- Il job Windows compila con avvisi come errori e passa test e verifica.
- `app_widgets` su Windows dura meno di 40 s, e il suo `TIMEOUT` segue il
  punto 2.
- La pull request riporta i tempi del punto 3 e la prova sulle librerie Qt.
- Nessun altro job cambia.
- CI verde.
