# [0.5-P8] `script`: i file nella modalità di controllo

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è il sistema di verifica, si prova con i suoi
  test (come 0.4-P1)
- **Dipende da:** P1, P4, P5
- **Modulo:** `script` (più le specifiche in `docs/verifica/`)
- **Da leggere:** [ADR 0011](../../adr/0011-modalita-di-controllo.md) punti
  3–7, [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punti 3,
  5 e 6, [ADR 0021](../../adr/0021-comandi-dei-file.md) punti 4 e 6,
  [ADR 0020](../../adr/0020-fuzzing-e-oracolo-ezdxf.md) punto 2; i quattro
  file di `docs/verifica/`; `script/src/questions.cpp`,
  `script/src/question_table.cpp`, `script/src/report.cpp`,
  `script/src/case_check.cpp`

## File

- `docs/verifica/domande.md`
- `docs/verifica/rapporto.md`
- `docs/verifica/script.md`
- `docs/verifica/casi.md`
- `script/src/file_questions.cpp` (nuovo)
- `script/src/question_table.cpp` (righe delle domande nuove)
- `script/src/questions.cpp` (definizioni `DXF` e `CADX`, nomi `D:<handle>`)
- `script/src/report.cpp` (campi `drawing:`, `units:`, `unhandled:`)
- `script/include/tg/script/case_files.hpp` (nuovo)
- `script/src/case_files.cpp` (nuovo)
- `script/include/tg/script/level1.hpp`
- `script/src/level1.cpp`
- `script/include/tg/script/case_check.hpp`
- `script/src/case_check.cpp`
- `script/tool/main.cpp` (opzione `--sources`)
- `script/CMakeLists.txt` (`tg_script` collega `Tecnigrafo::io`)
- `tests/verifica/CMakeLists.txt` (passa `--sources`)
- `tests/script/file_test.cpp` (nuovo)
- `tests/CMakeLists.txt` (una riga)

Sono 19 file, più della misura consigliata: quattro sono le specifiche e tre
sono righe di CMake, e dividerli separerebbe le specifiche dal codice che le
realizza. Il codice nuovo resta sotto le 600 righe; se le supera, fermati e
scrivilo nella issue.

## Cosa fare

Questo pacchetto fissa **prima del lavoro** come i casi di verifica parlano
di file, e realizza la parte che serve ai riferimenti di R10–R14. I comandi
dei file (P15, P16) useranno gli script; qui si scrive la loro specifica, non
il loro codice.

### 1. Specifiche (`docs/verifica/`)

**`script.md`**, nuova sezione "File":

- negli script i nomi di file sono relativi alla **cartella di uscita** del
  caso, senza `..` né percorsi assoluti; `@sources/<nome>` è il file `<nome>`
  di `sorgenti.json`, in sola lettura (ADR 0021, punto 4); ogni altro nome dà
  `Invalid file name.`;
- un esempio: `OPEN`, `@sources/librecad-sample`, `!report`.

**`rapporto.md`**:

- tre campi nuovi, scritti solo se diversi da una sessione nuova, dopo
  `ortho:`: `drawing: <nome>[, modified]` (diverso quando il nome non è
  `Drawing1.cadx` o ci sono modifiche), `units: <n>` (diverso da 4),
  `unhandled: <TIPO n>, …` (entità non gestite del disegno, per tipo in
  ordine alfabetico);
- proprietà nuove delle entità, scritte solo se diverse dal valore
  predefinito, in fondo alla riga prima delle derivate, nello stesso ordine
  per tutte le entità che le hanno:
  - `LWPOLYLINE`: `width=` (43) dopo `closed`; `start-width=` e
    `end-width=` (40, 41) dopo il `vertex` (e il suo `bulge`) a cui si
    riferiscono;
  - `LINE`: `start-z=` (30), `end-z=` (31);
  - `CIRCLE`, `ARC`, `TEXT`: `z=` (30); `TEXT` anche `alignment-z=` (31);
  - `LWPOLYLINE`: `elevation=` (38);
  - tutte: `thickness=` (39) e `extrusion=x,y,z` (210/220/230);
  - per un'entità con estrusione `(0,0,-1)` la geometria scritta è quella
    nel sistema globale (ADR 0018, punto 5), seguita da `extrusion=0,0,-1`.
- le stesse proprietà si accettano nelle definizioni, come le altre.

Le proprietà nuove le scrivono e le leggono P11 e P12, che realizzano le
entità; qui si fissa solo il formato.

**`domande.md`**, nuova sezione "File":

| Riga | Significato |
|---|---|
| `D = DXF source=<nome>` / `D = DXF file=<nome>` | legge un DXF di `sorgenti.json` o della cartella del caso con `io::readDxf`; un errore di lettura rende il file di domande non valido |
| `C = CADX source=<nome>` / `C = CADX file=<nome>` | lo stesso con `io::readCadx` |
| `D:<handle>` | come nome d'entità nelle domande su un oggetto: l'entità gestita di `D` con quell'handle esadecimale (`D:2F`) |

| Domanda | Risposta |
|---|---|
| `entity D <handle>` | l'entità gestita nel formato del rapporto, `unhandled <TIPO>`, o `none` |
| `count D` | `TIPO n` per tipo gestito, separati da `;`, in ordine alfabetico, o `none` |
| `unhandled D` | lo stesso per le entità non gestite |
| `version D` | `$ACADVER` letto (`AC1015`) |
| `units D` | `INSUNITS` del disegno |
| `warnings D` | gli avvisi della lettura separati da `;`, o `none` |
| `roundtrip D [as=dxf\|cadx]` | `identical`, oppure `different: <la prima differenza>`; scrive e rilegge con le regole dell'ADR 0018, punto 6; per difetto nello stesso formato e versione |
| `open-error source=<nome>` / `open-error file=<nome>` | il messaggio d'errore della lettura tra virgolette, o `none` |

`roundtrip` confronta il file **originale** con quello riscritto, a livello
di gruppi (`io::readGroups`): sezioni non gestite, entità non gestite e
gruppi in più uguali carattere per carattere; entità gestite uguali entro la
tolleranza; stesso ordine. Così non basta che il lettore sia coerente con sé
stesso.

**`casi.md`**: un caso può contenere file creati da noi (per esempio un DXF
scritto a mano) accanto a `script.tgs` o `domande.tgq`; `--sources`; la riga
`; oracle: ezdxf` che dichiara un caso controllato dall'oracolo (P9).

### 2. Realizzazione

- `file_questions.cpp`: le definizioni e le domande della tabella. Finché
  `io` restituisce `NotImplemented` (prima di P10 e P14), una definizione dà
  un errore con `notImplemented` vero: il caso risulta "non ancora
  realizzato", come per le domande della tappa 0.4.
- `report.cpp`: i tre campi nuovi, letti da `edit::Editor` (P5) e dal
  documento.
- `case_check` e `tecnigrafo-script` (`run` e `check`) ricevono
  `--sources <cartella>` (per difetto nessuna: `source=` e `@sources/` danno
  un errore che lo dice); `tests/verifica/CMakeLists.txt` passa
  `TG_VERIFICA_SOURCES_DIR`.
- `case_files.hpp/.cpp`: l'accesso ai file della modalità di controllo
  (ADR 0021, punto 4):

  ```cpp
  /// io::FileAccess for scripts: relative names inside `output` (read and
  /// write), "@sources/<name>" inside `sources` (read only), anything else
  /// rejected with "Invalid file name.".
  class CaseFiles final : public io::FileAccess {
  public:
      CaseFiles(std::filesystem::path output, std::optional<std::filesystem::path> sources);
      …
  };
  ```

  Un nome è rifiutato se è assoluto, ha una lettera di unità, contiene `..`
  come componente, `\`, o un carattere di controllo; `<name>` di `@sources/`
  segue la regola dei nomi di `sorgenti.json` (`^[a-z0-9-]+$`).
- `level1.hpp/.cpp`: `runLevel1(const Script&, const RunOptions&)` con
  `struct RunOptions { io::FileAccess* files{nullptr}; };`. `checkCase` e
  `runScriptFile` creano un `CaseFiles` (cartella d'uscita del caso, o quella
  corrente per `run`) e lo passano. In questo pacchetto `runLevel1` lo tiene
  e basta: il collegamento al `CommandRunner` lo fa P15, con una riga.

## Regole della tappa che valgono qui

- Linguaggio, catalogo e rapporto sono contratti (ADR 0011): si aggiunge, non
  si cambia. Se ti sembra di dover cambiare qualcosa che c'è, fermati.
- `docs/verifica/domande.md` lo tocca anche P2: chi arriva dopo si aggiorna.
- Le domande sui file esistono solo in `tecnigrafo-script`.

## Test richiesti

- In `tests/script/file_test.cpp`: ogni definizione e domanda nuova, con un
  DXF scritto nel test (le risposte che dipendono da `io` restano per R10:
  scrivilo nella pull request); `CaseFiles` con nomi validi e ognuno dei
  rifiuti, lettura da `@sources/`, scrittura rifiutata in `@sources/`.
- Errori: `source=` senza `--sources`, nome inesistente, `D:` con un handle
  che non c'è, nome definito due volte.
- Rapporto: i tre campi compaiono solo quando sono diversi da una sessione
  nuova; un rapporto della tappa 0.4 resta identico.

## Fatto quando

- Le quattro specifiche descrivono tutto quanto sopra, con esempi.
- I casi della tappa 0.4 passano ancora, identici.
- `tecnigrafo-script check --sources` funziona e la CI lo usa.
- La CI è verde.
