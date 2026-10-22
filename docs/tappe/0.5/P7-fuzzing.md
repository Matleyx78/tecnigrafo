# [0.5-P7] Fuzzing del lettore DXF in CI

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #160
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura, non cambia il comportamento del
  programma
- **Dipende da:** P4
- **Modulo:** infrastruttura (`tests/fuzz/`, CMake, CI)
- **Da leggere:** [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md) punto 1,
  [ADR 0016](../../adr/0016-sanitizer-in-ci.md) (stesse opzioni dei
  sanitizer); `CMakeLists.txt` (opzione `TG_SANITIZE`), `CMakePresets.json`
  (preset `asan`), `.github/workflows/ci.yml` (job `changes` e `sanitizers`),
  `io/include/tg/io/io.hpp` (da P4)

## File

- `CMakeLists.txt` (opzione `TG_FUZZ`)
- `CMakePresets.json` (preset `fuzz`)
- `tests/CMakeLists.txt` (`add_subdirectory(fuzz)` solo con `TG_FUZZ`)
- `tests/fuzz/CMakeLists.txt` (nuovo)
- `tests/fuzz/fuzz_dxf.cpp` (nuovo)
- `tests/fuzz/corpus/dxf/` (nuova: 4–6 DXF minuscoli scritti a mano)
- `tests/fuzz/README.md` (nuovo: come eseguirlo in locale)
- `.github/workflows/ci.yml` (job nuovo e uscita nuova di `changes`)

Le modifiche a `.github/` e a `CMakePresets.json` le approva il responsabile
(AGENTS.md): scrivilo nella pull request.

## Cosa fare

### CMake

- Opzione `TG_FUZZ` (spenta per difetto). Accesa con un compilatore diverso da
  Clang dà un errore di configurazione, come `TG_SANITIZE` con MSVC.
- Con `TG_FUZZ` tutto il codice si compila con
  `-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=all
  -fno-omit-frame-pointer`, e i bersagli di `tests/fuzz/` si collegano con
  `-fsanitize=fuzzer,address,undefined`.
- Preset `fuzz`: Clang 18, `Debug` con ottimizzazione `-O1`, `TG_BUILD_APP`
  spento, `TG_FUZZ` acceso, cartella `build${TG_BUILD_SUFFIX}/fuzz` come gli
  altri preset.

### Il bersaglio

```cpp
// tests/fuzz/fuzz_dxf.cpp
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    const std::string_view bytes{reinterpret_cast<const char*>(data), size};
    (void)tg::io::readDxf(bytes);
    return 0;
}
```

Con P4 `readDxf` legge solo i gruppi; quando P10–P12 lo completano, il
bersaglio prova tutto il lettore senza cambiare.

### Corpus

- `tests/fuzz/corpus/dxf/`: pochi DXF scritti a mano, di qualche riga: vuoto
  ma valido, con una entità per tipo, con una code page `ANSI_1252`, con un
  `\U+` e con una sezione sconosciuta.
- In CI si aggiungono i file di `tests/verifica/sorgenti.json` già scaricati
  dalla configurazione (`TG_VERIFICA_SOURCES_DIR`), se ce ne sono.

### Job della CI `Fuzzing (io)`

- Il job `changes` riceve una seconda uscita, `io`, vera quando la pull
  request (o il push) cambia `io/`, `third_party/` o `tests/fuzz/`.
- Il job `Fuzzing (io)` parte solo con `io` vera, su `ubuntu-24.04` con
  `clang-18`: configura e compila il preset `fuzz`, poi esegue ogni bersaglio
  di `tests/fuzz/` per 180 secondi (`-max_total_time=180 -timeout=10
  -rss_limit_mb=2048`) sul corpus.
- Un input che fa fallire il bersaglio: il job fallisce, stampa l'input in
  esadecimale nel log (al massimo 4 KiB) e lo allega come artefatto
  `fuzz-crash`.
- Le opzioni a tempo di esecuzione dei sanitizer sono quelle del preset
  `asan` (ADR 0016).

### `tests/fuzz/README.md`

Come compilare il preset `fuzz` ed eseguire un bersaglio in locale, come
riprodurre un input allegato dalla CI, e la regola dell'ADR 0020: un crash
trovato si corregge e diventa un caso di verifica scritto da un altro agente.

## Regole della tappa che valgono qui

- I bersagli nuovi (`fuzz_zip` in P13, `fuzz_cadx` in P14) si aggiungono a
  `tests/fuzz/CMakeLists.txt`: il job li deve trovare da solo (per esempio
  tutti i file `fuzz_*.cpp`), senza modificare `ci.yml` di nuovo.
- `ci.yml` lo tocca anche P9, che dipende da questo pacchetto.

## Test richiesti

- La pull request stessa: il job `Fuzzing (io)` parte (cambia `tests/fuzz/`)
  e passa.
- Prova in locale, descritta nella pull request: un bersaglio con un errore
  provocato apposta (per esempio un accesso fuori dai limiti in una copia
  temporanea) fa fallire il fuzzer in pochi secondi. La copia non si
  pubblica.

## Fatto quando

- `cmake --preset fuzz` e la compilazione funzionano con Clang 18; con GCC
  la configurazione si ferma con un messaggio chiaro.
- Il job `Fuzzing (io)` parte solo quando cambiano `io/`, `third_party/` o
  `tests/fuzz/`, gira 3 minuti per bersaglio e passa.
- Un fallimento stampa l'input e lo allega.
- Gli altri job non cambiano comportamento.
