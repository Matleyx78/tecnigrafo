# [F2] `script`: la parte già verificabile degli script in attesa

- **Tappa:** fuori tappa, miglioramento dei test ([ROADMAP.md](../../ROADMAP.md),
  "Cosa si può fare fuori dalla tappa aperta"); nasce dall'idea
  [#426](https://github.com/Matleyx78/tecnigrafo/issues/426)
- **Issue:** #431
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: il pacchetto cambia lo strumento di verifica,
  non il comportamento di Tecnigrafo. Lo provano i test di unità con testi
  costruiti apposta e l'esecuzione su tutti i casi in attesa
- **Dipende da:** [F1](F1-forma-dei-casi.md) (#430)
- **Modulo:** `script`
- **Da leggere:** [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md)
  punti 4–7 e 9; [ADR 0011](../../adr/0011-modalita-di-controllo.md) punto 7;
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md) punto 2 (il registro);
  `docs/verifica/casi.md`, `docs/verifica/script.md`;
  `script/src/script.cpp`, `script/src/level1.cpp`, `script/src/compare.cpp`,
  `script/src/case_check.cpp`; in `cmd`, solo `CommandRegistry::canonicalName`
  e `CommandRunner::isActive`

## File

- `script/include/tg/script/script.hpp`, `script/src/script.cpp`
- `script/include/tg/script/level1.hpp`, `script/src/level1.cpp`
- `script/include/tg/script/compare.hpp`, `script/src/compare.cpp`
- `script/include/tg/script/case_check.hpp`, `script/src/case_check.cpp`
- `tests/script/script_test.cpp`, `tests/script/level1_test.cpp`,
  `tests/script/compare_test.cpp`, `tests/script/case_check_test.cpp`
- `tests/verifica/in-attesa.txt` (solo `esente`, sotto)
- `docs/verifica/casi.md`, `docs/verifica/script.md`
- `AGENTS.md`

## Cosa fare

Il pacchetto realizza i punti 4, 5 e 6 dell'ADR 0037 per gli **script**,
esattamente come sono scritti lì: il **confine** di un caso in attesa, il
commento `; cambia con #<numero>` e il confronto della parte prima del confine.
Le domande vengono con [F3](F3-domande-gia-verificabili.md), che usa
`Frontier` e `changesWith` di questo pacchetto. Le funzioni di oggi restano
come sono: quelle nuove si aggiungono accanto.

### 1. Il commento e il confine

```cpp
// script.hpp
/// Where a pending case stops being compared (docs/adr/0037, point 4).
struct Frontier {
    int line;            ///< 1-based, in script.tgs or domande.tgq
    std::string reason;  ///< "not implemented yet: ...", "unknown command PLINE",
                         ///< "changes with #346"

    friend bool operator==(const Frontier&, const Frontier&) = default;
};

struct Script {
    // ... as today
    std::vector<int> lines;  ///< 1-based line of each action, as many as actions
};

/// The number of a comment "; cambia con #<number>" (docs/adr/0037,
/// point 5): ';' in the first column, then optional spaces, "cambia con #"
/// and the digits; anything after them is ignored. Nothing for any other
/// line. Shared by the scripts and the questions.
[[nodiscard]] std::optional<int> changesWith(std::string_view line);

/// A script above its frontier.
struct ScriptPrefix {
    Script script;                     ///< the actions above the frontier
    std::optional<Frontier> frontier;  ///< none: the same actions as parseScript()
};

/// Reads the whole script as parseScript() does, and stops the actions at
/// the first line of points 4.1 or 4.3 of docs/adr/0037: a line that uses
/// what the catalogue describes but the program does not have yet (a
/// ParseError with notImplemented), or a "; cambia con #<n>" comment. A
/// malformed line anywhere, also below the frontier, is still an error.
[[nodiscard]] std::variant<ScriptPrefix, ParseError> parseScriptPrefix(std::string_view text);
```

`parseScript()` riempie anche `Script::lines`.

### 2. L'esecuzione fino al confine (script)

```cpp
// level1.hpp
/// A level 1 run up to the frontier.
struct PrefixRun {
    std::string report;                ///< as runLevel1() writes it, [final] at the frontier
    std::optional<Frontier> frontier;  ///< the line typed with an unknown command name
};

/// Runs `script` as runLevel1() does, but stops before the first normal
/// line typed while no command is running (CommandRunner::isActive() is
/// false) whose text the runner would answer with "Unknown command":
/// not U, not REDO, and not a command or alias of the registry
/// (CommandRegistry::canonicalName), read as startCommand() reads it
/// (docs/adr/0037, point 4.2). The report ends with the section [final]
/// at that point. Precondition: script.level == 1, script.lines filled.
[[nodiscard]] PrefixRun runLevel1UntilUnknownCommand(const Script& script,
                                                     const RunOptions& options = {});
```

Il confine di uno script è il primo dei due: quello di `parseScriptPrefix()` e
quello di `runLevel1UntilUnknownCommand()` sulle azioni sopra il primo.

### 3. Il confronto prima del confine

```cpp
// compare.hpp
/// Compares a report obtained up to the frontier with the expected one
/// (docs/adr/0037, point 6). The sections of `actual` before its last one
/// ([report 1] ... [report k]) are compared line by line with the expected
/// sections of the same titles; the lines of the "messages:" list of the
/// last section of `actual` must be the first lines of the "messages:" list
/// of the expected section after [report k]. Nothing else is compared. The
/// header line is compared. Returns the differences, with the lines of the
/// expected file; an expected section missing is one difference.
[[nodiscard]] std::vector<Difference> compareReportPrefix(std::string_view expected,
                                                          std::string_view actual,
                                                          const Tolerances& tolerances);
```

### 4. `check`

Per un caso a script, con `--pending` e i controlli anticipati accesi (non
`esente`), dopo la forma (F1) e il confronto intero di oggi:

- se il caso passa per intero, resta l'esito di oggi (codice 1: va tolto
  dall'elenco);
- altrimenti `check` esegue la parte prima del confine e la confronta;
  scrive il rapporto ottenuto in `ottenuto-confine.txt` accanto a
  `ottenuto.txt`;
- con differenze prima del confine esce con **1**:
  `FAILED: <caso>: differs before the point the program cannot do yet (<file>, line <n>: <reason>)`,
  seguito dalle differenze come oggi; senza confine:
  `FAILED: <caso>: fails, but uses nothing the program does not have yet: fix the reference, or write "; cambia con #<issue>" before the line where the behaviour changes (docs/adr/0037)`;
- senza differenze prima del confine, l'esito di oggi (`pending:`, codice 0),
  con il confine nell'avviso.

Gli script di livello 2 (codice 77), i casi di domande e i casi non in attesa
non cambiano.

### 5. `esente` e documenti

- Il pacchetto esegue i casi a script in attesa con il controllo nuovo; a
  quelli che non lo superano aggiunge `esente` (ADR 0037, punto 9) e li
  elenca nella pull request, con la prima differenza di ciascuno. Non tocca i
  casi.
- `docs/verifica/casi.md`: il confine, il confronto prima del confine e i nuovi
  esiti in "Controlli anticipati" e in "Codici di uscita".
- `docs/verifica/script.md`: il commento `; cambia con #<numero>`, nella riga
  dei commenti, con il rimando all'ADR 0037.
- `AGENTS.md`, "In un pacchetto di riferimenti": il commento `; cambia con` per
  un comportamento che il lavoro cambia apposta; se una differenza prima del
  confine resiste al ricalcolo dalle fonti, fermarsi e scriverlo nella issue
  (ADR 0037, punto 7).

## Regole della tappa che valgono qui

- Il pacchetto non tocca `tests/verifica/casi/` né `sorgenti.json`; in
  `in-attesa.txt` aggiunge solo `esente`.
- Niente modifiche a `.github/`, `cmake/` né a `cmd`: il registro e il runner
  si leggono con le funzioni che esistono.
- Le funzioni di oggi (`parseScript`, `runLevel1`, `compareReports`) non
  cambiano comportamento, tranne `Script::lines`.
- Messaggi del programma in inglese; documenti in italiano.

## Test richiesti

- `changesWith`: `; cambia con #346`, con spazi dopo `;`, con testo dopo il
  numero; `;cambia con 346` e ` ; cambia con #346` (con lo spazio davanti è
  testo) no.
- `parseScriptPrefix`: confine su una direttiva annunciata, su
  `; cambia con`, il primo dei due; una riga malformata sotto il confine resta
  un errore; senza confine, le stesse azioni di `parseScript`.
- `runLevel1UntilUnknownCommand`: un comando sconosciuto a `Command:` è il
  confine, con la sua riga; lo stesso testo come risposta a una domanda di un
  comando in corso no; `U` e `REDO` no; un alias registrato no.
- `compareReportPrefix`: sezioni prima del confine uguali (nessuna differenza)
  e diverse (una differenza, con la riga del file atteso); messaggi della
  sezione finale che non sono l'inizio di quelli attesi; un handle sbagliato
  in `[report 1]` prima di un comando sconosciuto (#296).
- `check` con `--pending`: differenza prima del confine (codice 1), caso senza
  confine che non passa (codice 1), differenza solo dopo il confine (codice 0),
  caso che passa (codice 1 come oggi), caso `esente` (come oggi); un caso come
  #301, `Specify start point` invece di `Specify first point` prima di un
  comando sconosciuto, esce con 1.

## Fatto quando

- Un caso a script in attesa con una differenza prima del confine fa fallire
  la CI, con il confine e la differenza nel messaggio.
- Un caso a script in attesa senza confine che non passa fa fallire la CI,
  con il messaggio che rimanda a `; cambia con`.
- `; cambia con #<numero>` ferma il confronto dello script, e non cambia nulla
  per un caso non in attesa.
- I casi a script in attesa di `main` superano il controllo, tranne quelli
  marcati `esente` ed elencati nella pull request.
- `docs/verifica/casi.md`, `script.md` e `AGENTS.md` descrivono il confine e
  il commento.
- La CI è verde.
