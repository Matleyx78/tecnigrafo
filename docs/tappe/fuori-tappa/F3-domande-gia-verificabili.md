# [F3] `script`: la parte già verificabile delle domande in attesa

- **Tappa:** fuori tappa, miglioramento dei test ([ROADMAP.md](../../ROADMAP.md),
  "Cosa si può fare fuori dalla tappa aperta"); nasce dall'idea
  [#426](https://github.com/Matleyx78/tecnigrafo/issues/426)
- **Issue:** #… (da aprire)
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: il pacchetto cambia lo strumento di verifica,
  non il comportamento di Tecnigrafo. Lo provano i test di unità con testi
  costruiti apposta e l'esecuzione su tutti i casi in attesa
- **Dipende da:** [F2](F2-parte-gia-verificabile.md) (`Frontier`,
  `changesWith`, gli esiti di `check`)
- **Modulo:** `script`
- **Da leggere:** [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md)
  punti 4–7 e 9; `docs/verifica/domande.md` ("Il file", "Le risposte");
  `docs/verifica/casi.md` ("Controlli anticipati", scritto da F1 e F2);
  `script/src/questions.cpp`, `script/src/case_check.cpp`

## File

- `script/include/tg/script/questions.hpp`, `script/src/questions.cpp`
- `script/src/case_check.cpp`
- `tests/script/questions_test.cpp`, `tests/script/case_check_test.cpp`
- `tests/verifica/in-attesa.txt` (solo `esente`, sotto)
- `docs/verifica/casi.md`, `docs/verifica/domande.md`

## Cosa fare

Il pacchetto realizza i punti 4, 5 e 6 dell'ADR 0037 per i casi di
**domande**, esattamente come sono scritti lì. `answerQuestions()` resta
com'è: la funzione nuova si aggiunge accanto.

```cpp
// questions.hpp
struct Answer {
    // ... as today
    bool skipped{false};  ///< not compared: the program cannot answer it yet
};

/// The answers of a question file up to its frontier.
struct QuestionPrefix {
    std::vector<Answer> answers;       ///< above the frontier; some skipped
    std::optional<Frontier> frontier;  ///< none: every line was read
};

/// Reads a .tgq file as answerQuestions() does, for a pending case
/// (docs/adr/0037, point 4): a question the program cannot answer yet (a
/// question not implemented, an argument or entity type not implemented,
/// an unhandled entity of a file) is an Answer with `skipped`; a definition
/// or a directive not implemented yet, or a "; cambia con #<n>" comment
/// (changesWith()), is the frontier, and the lines below it are not read.
/// A malformed line above the frontier is an error.
[[nodiscard]] std::variant<QuestionPrefix, ParseError> answerQuestionsUntilFrontier(
    std::string_view text, io::FileAccess* files = nullptr);
```

In `check`, per un caso di domande con `--pending` e i controlli anticipati
accesi, dopo il confronto intero di oggi e se il caso non passa:

- ogni risposta non saltata, sopra il confine, deve coincidere con quella
  attesa; altrimenti esce con 1, con lo stesso messaggio di F2
  (`differs before the point the program cannot do yet`) e le differenze;
- un caso senza confine e senza domande saltate che non passa esce con 1, con
  il messaggio di F2 che rimanda a `; cambia con`;
- altrimenti l'esito di oggi (`pending:`, codice 0).

La prima riga `; oracle: ezdxf` resta un commento come oggi; il passo
`Oracolo ezdxf` della CI non cambia.

Il pacchetto esegue i casi di domande in attesa con il controllo nuovo; a
quelli che non lo superano aggiunge `esente` (ADR 0037, punto 9) e li elenca
nella pull request, con la prima differenza di ciascuno. Non tocca i casi.

Documenti: in `docs/verifica/domande.md` il commento `; cambia con #<numero>`
nella riga dei commenti, con il rimando all'ADR 0037; in
`docs/verifica/casi.md` le domande in "Controlli anticipati".

## Regole della tappa che valgono qui

- Il pacchetto non tocca `tests/verifica/casi/` né `sorgenti.json`; in
  `in-attesa.txt` aggiunge solo `esente`.
- Niente modifiche a `.github/` né a `cmake/`.
- Messaggi del programma in inglese; documenti in italiano.

## Test richiesti

- Una domanda non realizzata è saltata, e la domanda dopo si confronta.
- Un argomento non realizzato (`from=`) e un tipo di entità non realizzato in
  una domanda: saltate.
- Una definizione non realizzata ferma il resto del file, con la sua riga nel
  confine.
- `; cambia con #346` ferma il resto del file; una riga malformata sopra il
  confine è un errore, sotto no.
- `check` con `--pending`: una risposta sbagliata a una domanda realizzata
  prima del confine esce con 1 (come #295); una differenza solo in una domanda
  saltata esce con 0; il caso `esente` come oggi.

## Fatto quando

- Un caso di domande in attesa con una risposta sbagliata a una domanda già
  realizzata, sopra il confine, fa fallire la CI.
- Le domande non realizzate si saltano, e una definizione non realizzata o
  `; cambia con` fermano il resto del file.
- I casi di domande in attesa di `main` superano il controllo, tranne quelli
  marcati `esente` ed elencati nella pull request.
- `docs/verifica/domande.md` e `docs/verifica/casi.md` descrivono il controllo.
- La CI è verde.
