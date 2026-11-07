# [0.7.5-P26] `cmd`: il protocollo dei comandi separato dal contesto di disegno

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: il comportamento non cambia; i test di unità e i
  casi di verifica esistenti, invariati, sono la prova
- **Dipende da:** P15, P25
- **Corretta:** 2026-11-07, con il verbale della revisione delle interfacce
  (P23, [#563](https://github.com/Matleyx78/tecnigrafo/issues/563)): N2
  (inizializzatori dei test)
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punto 5;
  [ADR 0005](../../adr/0005-comandi-e-annullamento.md);
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md);
  [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md),
  punti 1–4; `cmd/include/tg/cmd/command.hpp`,
  `cmd/include/tg/cmd/command_description.hpp`,
  `cmd/include/tg/cmd/command_registry.hpp`,
  `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`

## Perché

Nel meccanismo dei comandi (richieste, parole chiave, stato, `Esc`, descrizioni)
non c'è niente di 2D, ma sta nello stesso file e nella stessa struttura del
contesto del disegno (`edit::Editor&`, valori predefiniti, file). Un ambiente
futuro deve poter usare lo stesso protocollo con il suo contesto (ADR 0041,
punto 5). Selezione, snap, grip, anteprima e coordinate restano 2D e restano nel
runner del disegno: non si inventa un ingresso universale.

## File

- `cmd/include/tg/cmd/command_protocol.hpp` (nuovo)
- `cmd/include/tg/cmd/command.hpp`
- `cmd/include/tg/cmd/command_description.hpp`, solo gli `#include`
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
- `cmd/src/command_protocol.cpp` (nuovo), solo se il punto 3 sposta funzioni
- `cmd/CMakeLists.txt`
- `tests/cmd/command_protocol_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `tests/cmd/arc_command_test.cpp`, `tests/cmd/circle_command_test.cpp`,
  `tests/cmd/text_command_test.cpp`, `tests/cmd/file_commands_test.cpp`: solo
  gli inizializzatori diretti di `CommandContext` (punto 2)

## Cosa fare

1. **`command_protocol.hpp`** contiene ciò che non dipende dal disegno,
   spostato da `command.hpp` senza cambiare nomi né comportamenti: `Keyword`,
   `PromptKind`, `FilePrompt`, `Prompt`, `Status`, e

   ```cpp
   /// What every command needs, whatever it works on (docs/adr/0041, point 5).
   struct ProtocolContext {
       std::function<void(std::string)> message;  ///< writes to the history
       bool scriptSequences{false};  ///< AutoCAD script sequences (ADR 0032, points 6-8).
   };
   ```

   `command_protocol.hpp` non include niente di `core`, `edit` né `io`.
2. **`command.hpp`** resta il protocollo dei comandi del disegno e include
   `command_protocol.hpp`: `Input` e i suoi tipi, `CommandDefaults`, `Command` e

   ```cpp
   /// What a command of the drawing works with.
   struct CommandContext : ProtocolContext {
       edit::Editor& editor;
       edit::DocumentFile& file;                  ///< P24
       std::optional<core::Vec2> lastPoint;       ///< updated by the command
       CommandDefaults defaults{};
       io::FileAccess* files{nullptr};
       bool documentReplaced{false};
   };
   ```

   I comandi continuano a scrivere `context.editor`, `context.message(…)`:
   nessun comando cambia. I test che costruiscono un `CommandContext` con gli
   inizializzatori diretti (`CommandContext context{editor, …}`, già adeguati
   da P24) non compilano più, perché il primo sotto-oggetto è ora
   `ProtocolContext`: si adeguano in modo meccanico, senza toccare asserzioni né
   risultati.
3. **Runner:** resta il runner del disegno. Le funzioni che non usano niente di
   2D (per esempio riconoscere una parola chiave in un testo scritto) passano in
   `command_protocol.*` come funzioni libere, con test loro; le altre restano.
   Elenca nella pull request che cosa hai spostato e che cosa no, e perché.
4. **Descrizioni** (ADR 0026): restano dove sono; `command_description.hpp`
   include `command_protocol.hpp` invece di `command.hpp` se gli basta.

## Regole della tappa che valgono qui

- `command.hpp`, `command_runner.*`: prima di te P24 e P15, dopo di te P27.
- Nessun comando cambia comportamento; i casi di verifica restano invariati.

## Test richiesti

- `command_protocol_test.cpp`: si compila includendo solo `command_protocol.hpp`
  (nessuna dipendenza da `core`, `edit`, `io`); le funzioni spostate al punto 3
  con i loro casi.
- I test di oggi di `cmd` passano senza modifiche, salvo gli `#include` e gli
  inizializzatori diretti di `CommandContext` (punto 2).

## Fatto quando

- Il protocollo sta in `command_protocol.hpp`, senza dipendenze dal disegno, e
  `CommandContext` ne deriva.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
