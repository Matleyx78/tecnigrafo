# [F1] `script`: la forma dei casi e l'eco delle righe dello script

- **Tappa:** fuori tappa, miglioramento dei test ([ROADMAP.md](../../ROADMAP.md),
  "Cosa si può fare fuori dalla tappa aperta"); nasce dall'idea
  [#426](https://github.com/Matleyx78/tecnigrafo/issues/426)
- **Issue:** #… (da aprire)
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: il pacchetto cambia lo strumento di verifica,
  non il comportamento di Tecnigrafo. Lo provano i test di unità con testi
  costruiti apposta e l'esecuzione su tutti i casi di `tests/verifica/casi/`
- **Dipende da:** [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md)
  accettato
- **Modulo:** `script` (più la lettura di `in-attesa.txt` in
  `tests/verifica/CMakeLists.txt` e le regole per gli agenti)
- **Da leggere:** [ADR 0037](../../adr/0037-controllo-anticipato-dei-riferimenti.md)
  punti 2, 3, 7 e 9; [ADR 0011](../../adr/0011-modalita-di-controllo.md) punto 7;
  `docs/verifica/casi.md`, `docs/verifica/script.md` (righe, commenti,
  direttive), `docs/verifica/rapporto.md` ("Struttura", "La cronologia");
  `script/src/case_check.cpp`, `script/tool/main.cpp`,
  `tests/verifica/CMakeLists.txt`

## File

- `script/include/tg/script/case_form.hpp`, `script/src/case_form.cpp` (nuovi)
- `script/CMakeLists.txt` (il sorgente nuovo)
- `script/include/tg/script/case_check.hpp`, `script/src/case_check.cpp`
- `script/tool/main.cpp`
- `tests/script/case_form_test.cpp` (nuovo), `tests/script/case_check_test.cpp`,
  `tests/CMakeLists.txt` (il test nuovo)
- `tests/verifica/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (il commento in testa e `esente`, sotto)
- `docs/verifica/casi.md`
- `AGENTS.md`
- `scripts/apri-tappa.sh` (il testo delle issue di riferimenti)

## Cosa fare

### 1. Le regole

Il pacchetto realizza i punti 2 e 3 dell'ADR 0037, esattamente come sono
scritti lì. In breve:

- `script.tgs`, `atteso.txt`, `domande.tgq`: niente `CR`, niente tabulazioni;
- `atteso.txt`, `domande.tgq`: niente righe vuote in fondo; in `atteso.txt` una
  riga vuota solo prima di una riga che inizia con `[`;
- spazi in fondo: mai in `domande.tgq`; in `script.tgs` solo sulle righe normali
  (non su `;…` né su `!…` in prima colonna); in `atteso.txt` solo su una riga
  che finisce con `: ` seguito dal testo intero di una riga normale dello script
  che ha quegli spazi in fondo;
- l'eco (solo se lo script non ha una riga `!level 2`): per ogni riga normale,
  nell'ordine, una riga di un elenco `messages:` di `atteso.txt` dopo quella
  della riga precedente; per una riga non vuota `t` una riga che finisce con
  `: ` + `t`; per una riga vuota una riga che finisce con `:` e che viene prima
  dell'eco della prossima riga non vuota.

Il BOM UTF-8 iniziale resta ammesso e non conta. I file si leggono così come
sono, senza togliere gli spazi.

### 2. L'interfaccia

```cpp
// case_form.hpp
namespace tg::script {

/// The first violation of the form of a case (docs/adr/0037, points 2 and 3).
struct FormError {
    std::string file;     ///< "script.tgs", "atteso.txt" or "domande.tgq"
    int line;             ///< 1-based, in `file`
    std::string message;  ///< English, one line

    friend bool operator==(const FormError&, const FormError&) = default;
};

/// Checks script.tgs and atteso.txt of a script case: the rules of point 2
/// on both files, then the echo of point 3 unless the script has a line
/// "!level 2". Reads the texts as they are; a UTF-8 BOM does not count.
[[nodiscard]] std::optional<FormError> checkScriptForm(std::string_view script,
                                                       std::string_view expected);

/// Checks domande.tgq (point 2).
[[nodiscard]] std::optional<FormError> checkQuestionsForm(std::string_view questions);

}  // namespace tg::script
```

I messaggi, nell'ordine in cui si controllano:

| Regola | `message` |
|---|---|
| `CR` | `carriage return: lines end with LF only` |
| tabulazione | `tab character` |
| riga vuota in fondo | `empty line at the end of the file` |
| riga vuota fuori posto in `atteso.txt` | `empty line not followed by a section` |
| spazi in fondo | `trailing spaces` |
| eco mancante | `no echo of this line in the messages of atteso.txt (an empty line is Enter: at Command: it repeats the last command)` |

Per l'eco mancante `file` è `script.tgs` e `line` la riga dello script.

```cpp
// case_check.hpp
struct CheckOptions {
    bool pending{false};
    /// The checks of docs/adr/0037 run; false only for a pending case
    /// marked "esente" in tests/verifica/in-attesa.txt (point 9).
    bool earlyChecks{true};
    std::filesystem::path output{"."};
};
```

- `checkCase()`, con `earlyChecks`, controlla la forma **prima** di eseguire il
  caso; una violazione è un caso non valido, codice 2 anche con `--pending`:
  `INVALID: <caso>: <file>, line <n>: <message>`.
- `tecnigrafo-script check` accetta `--no-early-checks`, solo insieme a
  `--pending` (da solo è un errore di argomenti, codice 2); il testo di aiuto
  lo descrive.

### 3. `in-attesa.txt` ed `esente`

- `tests/verifica/CMakeLists.txt` accetta una riga
  `<area>/<nome> #<numero> esente`, e solo quella parola dopo il numero; per
  quei casi aggiunge `--no-early-checks` a `--pending`. Una riga `esente` di un
  caso che non c'è resta un errore come oggi.
- Il commento in testa a `in-attesa.txt` spiega `esente` in due righe, con il
  rimando all'ADR 0037, punto 9.
- Il pacchetto esegue i controlli su tutti i casi; ai casi **in attesa** che
  non li superano aggiunge `esente`, e li elenca nella pull request con il
  messaggio di ciascuno. Il 2026-10-31 una prova dell'eco ne trovava cinque,
  ciascuno con una riga vuota dopo un comando già finito:
  `chamfer/casi-limite-errori` (riga 68), `fillet/casi-limite-proprieta`
  (riga 57), `grip/cerchio-e-modifica-non-valida` (riga 5),
  `join/arco-close-e-annulla` (riga 11), `offset/through-e-lati` (riga 11).
  L'elenco giusto è quello del giorno della pull request.
- Se un caso **non** in attesa non li supera, fermati e scrivilo nella issue
  con il messaggio: o la regola è sbagliata o il caso, e lo decide il
  responsabile. Non toccare il caso e non allentare la regola.

### 4. Le regole per gli agenti

- `AGENTS.md`, "In un pacchetto di riferimenti": la pull request è finita con
  la CI verde, controlli anticipati compresi (ADR 0037); i casi si possono
  eseguire per sapere se sono validi, mai per ricavare un valore atteso; non si
  aggiunge `esente`, e lo si toglie dai casi che si correggono.
- `AGENTS.md`, "In un pacchetto di lavoro": `esente` lo aggiunge solo un
  pacchetto che porta un controllo anticipato, se la sua scheda lo dice.
- `docs/verifica/casi.md`: una sezione "Controlli anticipati" con le regole
  della forma e dell'eco, una riga in "Codici di uscita" e `esente` in
  "`tests/verifica/in-attesa.txt`".
- `scripts/apri-tappa.sh`: nel testo delle issue di riferimenti, la stessa
  regola di `AGENTS.md`.

## Regole della tappa che valgono qui

- Il pacchetto non tocca `tests/verifica/casi/` né `sorgenti.json`: in
  `in-attesa.txt` aggiunge solo il commento e la parola `esente`.
- Niente modifiche a `.github/` né a `cmake/`: i controlli girano dentro
  `tecnigrafo-script check`, che `ctest` già esegue.
- Messaggi del programma in inglese; documenti in italiano.

## Test richiesti

In `tests/script/case_form_test.cpp`, per ogni regola un testo che la viola,
con file, riga e messaggio attesi, e uno che la rispetta:

- `CR` e tabulazione in ciascuno dei tre file;
- `atteso.txt` e `domande.tgq` che finiscono con una riga vuota; `atteso.txt`
  con una riga vuota prima di `messages:`;
- spazi in fondo a un commento e a una direttiva dello script (errore), a una
  riga normale (ammessi) e alla sua eco in `atteso.txt` (ammessi); gli stessi
  spazi su un'altra riga di `atteso.txt` (errore); in `domande.tgq` (errore);
- eco: `LINE`, due punti e una riga vuota che chiude il comando (valido);
  `CIRCLE`, centro, raggio e una riga vuota senza eco (errore sulla riga
  vuota, come in #417); una riga non vuota senza eco; le eco divise fra due
  sezioni; uno script con `!level 2` e una riga `PL 0,0 50,0` senza eco
  (valido: niente controllo dell'eco);
- un BOM all'inizio non conta.

In `tests/script/case_check_test.cpp`: un caso con la forma sbagliata esce con
2 con e senza `--pending`, e con `--pending` più `--no-early-checks` esce come
oggi; `--no-early-checks` senza `--pending` è un errore di argomenti.

## Fatto quando

- Ogni regola dei punti 2 e 3 dell'ADR 0037 ha un test che la viola e uno che
  la rispetta, con il messaggio della tabella.
- Una riga vuota dopo un comando già finito, senza eco, rende il caso non
  valido con il numero della riga dello script, anche se è in attesa.
- Le righe vuote che chiudono un comando e gli spazi scritti apposta in un
  testo (`text/contenuto-speciale`, `interfaccia/testo-caratteri-speciali`)
  restano validi.
- Una riga `esente` esegue il caso come oggi; una parola diversa dopo il numero
  è un errore di configurazione.
- Tutti i casi di `main` superano i controlli, tranne quelli in attesa marcati
  `esente` e elencati nella pull request.
- `docs/verifica/casi.md`, `AGENTS.md` e `scripts/apri-tappa.sh` descrivono le
  regole.
- La CI è verde.
