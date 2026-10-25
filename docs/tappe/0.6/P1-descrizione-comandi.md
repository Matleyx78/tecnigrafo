# [0.6-P1] `cmd`: descrizione dei comandi

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #227
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: metadati senza effetti sul disegno; il
  controllo di completezza gira su tutti i casi di verifica esistenti
- **Dipende da:** —
- **Modulo:** `cmd` (più i collegamenti in `script` e `app`)
- **Da leggere:** [ADR 0026](../../adr/0026-descrizione-dei-comandi.md) (tutto),
  [ADR 0005](../../adr/0005-comandi-e-annullamento.md) (domande e parole
  chiave); `cmd/include/tg/cmd/command.hpp`, `command_registry.hpp`,
  `command_runner.hpp`, `cmd/src/builtin_commands.cpp` e i file dei comandi
  in `cmd/src/commands/` (per trascrivere le domande)

## File

- `cmd/include/tg/cmd/command_description.hpp` (nuovo)
- `cmd/include/tg/cmd/command_registry.hpp`, `cmd/src/command_registry.cpp`
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
- `cmd/src/command_descriptions.hpp`, `cmd/src/command_descriptions.cpp`
  (nuovi: le descrizioni di tutti i comandi)
- `cmd/src/builtin_commands.cpp`, `cmd/CMakeLists.txt`
- `script/src/case_check.cpp` (controllo acceso; una domanda non descritta
  fa fallire il caso)
- `script/tool/main.cpp` (sottocomando `describe`)
- `app/src/main_window.cpp` (controllo spento fuori da `--script`)
- `tests/cmd/command_registry_test.cpp` (nuovo),
  `tests/cmd/command_runner_test.cpp`, `tests/app/command_line_test.cpp`
  (nuova firma di `add`)
- `tests/CMakeLists.txt`

## Cosa fare

### Tipi

In `command_description.hpp`, come nell'ADR 0026, punto 1:

```cpp
enum class ValueType : std::uint8_t {
    Point, Distance, Angle, Integer, Real, Text, Name, Color, Keyword,
    FileOpen, FileSave
};

struct PromptDescription {
    std::string id;            ///< stable, English, kebab-case: "next-point"
    std::string text;          ///< as shown, "{default}" for the value in < >
    ValueType value{ValueType::Point};
    std::vector<Keyword> keywords;
    bool acceptsEmpty{false};  ///< Enter alone is an answer

    friend bool operator==(const PromptDescription&, const PromptDescription&) = default;
};

struct CommandDescription {
    std::string name;
    std::vector<std::string> aliases;
    std::string summary;  ///< one English sentence, ending with a full stop
    bool selectsFirst{false};
    std::vector<PromptDescription> prompts;

    friend bool operator==(const CommandDescription&, const CommandDescription&) = default;
};

/// True if `prompt`, as a command shows it, matches `description`: the text
/// equal once each "{default}" is replaced by a non-empty text without '<'
/// or '>', the keywords equal and in the same order, and the kind
/// compatible with the value type (table below).
[[nodiscard]] bool matches(const PromptDescription& description, const Prompt& prompt);
```

| `PromptKind` | `ValueType` compatibili |
|---|---|
| `Point` | `Point`, `Keyword` |
| `Distance` | `Distance`, `Keyword` |
| `Angle` | `Angle`, `Keyword` |
| `Text` | `Text`, `Name`, `Integer`, `Real`, `Color`, `Keyword`, `FileOpen`, `FileSave` |

### Registro

```cpp
/// Registers a command. Preconditions: the description is valid (name not
/// empty, summary not empty, unique prompt ids, every keyword in square
/// brackets in its prompt's text; no prompts at all is valid, for commands
/// that ask nothing), `factory` is not
/// empty, and neither the name nor any alias is already registered.
void add(CommandDescription description, Factory factory);

/// Every description, in alphabetical order of name.
[[nodiscard]] std::vector<const CommandDescription*> descriptions() const;

/// The description of a command, or nullptr if the name is unknown.
[[nodiscard]] const CommandDescription* describe(std::string_view nameOrAlias) const;
```

La vecchia `add(name, aliases, factory)` sparisce. `canonicalName()` e
`create()` restano.

### Controllo nel runner

```cpp
/// Called with a message when a running command shows a prompt that its
/// description lacks (docs/adr/0026, point 3).
using DescriptionCheck = std::function<void(const std::string& message)>;

/// The default check writes the message to std::cerr and calls
/// std::abort(), so that any test fails. An empty function turns the check
/// off, as the application does outside --script.
void setDescriptionCheck(DescriptionCheck check);
```

- Il runner controlla ogni domanda che un comando mostra: all'avvio, dopo ogni
  input, e dopo `cancel()` se il comando continua. Le domande del runner
  (`Select objects:`, `Specify opposite corner:`, `Command:`) non si
  controllano.
- Il messaggio dice comando e testo:
  `PLINE: undescribed prompt "Specify next point or [Close/Undo]:"`.
- `tecnigrafo-script` (livello 1) e `app` con `--script` (livello 2)
  installano un controllo che registra il messaggio: il caso **non passa**, e
  il messaggio si stampa con le differenze del caso (codice d'uscita 1).

### Descrizioni

In `command_descriptions.cpp`, una funzione per comando, in ordine
alfabetico, che `builtin_commands.cpp` usa: `ARC`, `CIRCLE`, `ERASE`,
`INSUNITS`, `LINE`, `NEW`, `OPEN`, `PLINE`, `QSAVE`, `SAVE`, `SAVEAS`, `TEXT`.
Le domande si trascrivono dal codice dei comandi, una volta ciascuna,
nell'ordine dell'uso normale. Sintesi brevi e in inglese ("Draws a straight
line segment, or a chain of them.").

### `tecnigrafo-script describe [nome]`

Stampa le descrizioni (tutte, o una) in testo, un campo per riga:

```
command: PLINE
aliases: PL
summary: Draws a polyline of line and arc segments.
prompt start-point: Specify start point: (point)
prompt next-point: Specify next point or [Arc/Close/Undo]: (point; Arc=A Close=C Undo=U; Enter)
```

Un nome sconosciuto: `Unknown command "X".` e codice d'uscita 2.

## Regole della tappa che valgono qui

- Ogni comando nuovo della tappa (P12, P13, P14, P16, P22–P24) aggiunge la sua
  descrizione in `command_descriptions.cpp`, o nel suo file se è un comando
  dell'applicazione: solo le sue righe.
- Testi in inglese, interi, come li mostra il comando (ADR 0005).

## Test richiesti

- `matches()`: testo uguale, `{default}` all'inizio, in mezzo, due volte;
  testo con `<` nel valore (non deve corrispondere); parole chiave diverse o
  in ordine diverso; ogni riga della tabella dei tipi.
- Registro: precondizioni (descrizione non valida, nome o alias doppio),
  `descriptions()` in ordine, `describe()` per nome e alias, maiuscole.
- Runner: un comando di prova con una domanda non descritta fa scattare il
  controllo; con il controllo spento non succede nulla.
- Ogni comando di `registerBuiltinCommands` ha una descrizione valida.
- Tutti i test esistenti di `cmd`, `script` e `app` passano con il controllo
  acceso.

## Fatto quando

- Tutti i casi di verifica passano con il controllo acceso, ai due livelli.
- `tecnigrafo-script describe` stampa i dodici comandi.
- La CI è verde.
