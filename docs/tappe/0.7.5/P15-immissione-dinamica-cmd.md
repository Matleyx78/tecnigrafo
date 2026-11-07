# [0.7.5-P15] `cmd`: il modello dell'immissione dinamica

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: l'immissione dinamica produce lo stesso testo
  della riga di comando, e i test del runner confrontano le due immissioni
  (ADR 0042, punto 1); script e casi di verifica non cambiano
- **Dipende da:** —
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0042](../../adr/0042-immissione-dinamica.md) per intero;
  [ADR 0005](../../adr/0005-comandi-e-annullamento.md) (coordinate scritte:
  assolute, `@dx,dy`, `@L<A`); [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md);
  [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md), punto 9
  (orto e polare); `cmd/include/tg/cmd/command.hpp` (`Prompt`, `PromptKind`,
  `Command::basePoint()`), `cmd/include/tg/cmd/command_runner.hpp`,
  `cmd/include/tg/cmd/value_format.hpp`

## File

- `cmd/include/tg/cmd/dynamic_input.hpp`, `cmd/src/dynamic_input.cpp` (nuovi)
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`: solo le
  funzioni costanti del punto 2, se mancano
- `cmd/CMakeLists.txt`
- `tests/cmd/dynamic_input_test.cpp` (nuovo), `tests/CMakeLists.txt`

## Cosa fare

### 1. Il modello

```cpp
namespace tg::cmd {

/// One field of the dynamic input (docs/adr/0042, point 2).
struct DynamicField {
    enum class Kind : std::uint8_t { X, Y, Length, Angle, Value };
    Kind kind{Kind::X};
    std::string typed;  ///< what the user typed; empty: follows the pointer
};

/// What the dynamic input shows for the current prompt of a runner.
struct DynamicInput {
    std::string prompt;                ///< Prompt::text, as on the command line
    std::vector<DynamicField> fields;  ///< empty for Text, Object, Selection
    std::size_t active{0};             ///< the field that receives the keys
};

/// The fields for the runner's current prompt: X and Y without a base point,
/// Length and Angle with one, Value for Distance, Angle and Number prompts.
[[nodiscard]] DynamicInput dynamicInputFor(const CommandRunner& runner);

/// The values shown in the fields that the user has not typed, from
/// `pointer`, the point the runner would use for a click there (after ortho,
/// polar and snap), formatted as the command line formats them.
[[nodiscard]] std::vector<std::string> dynamicInputShown(const DynamicInput& input,
                                                         const CommandRunner& runner,
                                                         core::Vec2 pointer);

/// The line Enter sends with submitText: empty when no field is typed;
/// otherwise "<x>,<y>", "@<length><<angle>", "@<dx>,<dy>" (a comma typed in
/// the Length field) or the value, taking the fields not typed from
/// `pointer` as dynamicInputShown does.
[[nodiscard]] std::string dynamicInputText(const DynamicInput& input,
                                           const CommandRunner& runner,
                                           core::Vec2 pointer);

}  // namespace tg::cmd
```

### 2. Che cosa chiede al runner

In sola lettura: il punto base della richiesta in corso
(`std::optional<core::Vec2> basePoint() const`, da `Command::basePoint()`) e il
punto che userebbe per un clic in una posizione data, dopo orto, polare e snap.
Se esistono già, usale; se no, aggiungile come funzioni costanti del runner,
che non cambiano niente di ciò che fa oggi.

### 3. Regole

- I numeri sono formattati con `formatLength` e `formatAngle`, come nella riga
  di comando; la composizione del testo non arrotonda oltre ciò che quelle
  funzioni scrivono.
- `Tab` non è affare di `cmd`: il campo attivo lo cambia `app` (P16).
- Una parola chiave o un comando non passano di qui: vanno alla riga di comando
  (ADR 0042, punto 3).

## Regole della tappa che valgono qui

- Nessun tipo nuovo in `core`.
- `command_runner.*`: dopo di te P26.

## Test richiesti

`dynamic_input_test.cpp`, ognuno confrontando lo stato del documento e la
cronologia dopo l'immissione dinamica con quelli dopo il testo scritto:

- `LINE` primo punto con X e Y scritti; secondo punto con lunghezza e angolo
  scritti (`@L<A`); con la sola lunghezza scritta e l'angolo dal puntatore; con
  una virgola nella lunghezza (`@dx,dy`);
- `CIRCLE` con il raggio scritto (`Value`);
- `Invio` senza campi scritti: termina `LINE` come `Invio` nella riga di comando;
- con orto acceso, il campo libero dell'angolo segue il punto corretto
  dall'orto;
- `TEXT` e `ERASE` (richieste di testo e di selezione): nessun campo.

## Fatto quando

- Le tre funzioni esistono con queste firme e le due immissioni danno gli stessi
  risultati nei test qui sopra.
- La CI è verde.
