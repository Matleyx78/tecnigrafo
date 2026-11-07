# [0.7.5-P27] `cmd`: gli ambienti nel registro dei comandi

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: oggi c'è un solo ambiente, quindi nessun comando
  cambia; un ambiente di prova nei test di unità
- **Dipende da:** P26
- **Modulo:** `cmd`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punto 6;
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md);
  [ADR 0009](../../adr/0009-interfaccia-grafica.md), punto 1 (l'ambiente
  `drawing2d` delle azioni); `cmd/include/tg/cmd/command_description.hpp`,
  `command_registry.hpp`, `command_runner.hpp` e i loro `.cpp`; l'esportazione
  delle descrizioni (`cmd/src/command_descriptions.cpp` e chi la usa in `mcp`)

## File

- `cmd/include/tg/cmd/command_description.hpp`
- `cmd/include/tg/cmd/command_registry.hpp`, `cmd/src/command_registry.cpp`
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
- `tests/cmd/command_registry_test.cpp`, `tests/cmd/command_runner_test.cpp`

## Cosa fare

1. `CommandDescription` ha
   `std::vector<std::string> environments{"drawing2d"};`: gli ambienti in cui il
   comando vale. Nessun comando di oggi lo cambia. Il confronto `==` lo
   include. L'esportazione delle descrizioni (ADR 0026, quella che legge il
   server MCP) **non** cambia in questa tappa: il campo non si esporta.
2. Il runner ha l'ambiente attivo:

   ```cpp
   /// The environment commands are typed in (docs/adr/0041, point 6):
   /// "drawing2d" until set.
   void setEnvironment(std::string id);
   [[nodiscard]] const std::string& environment() const;
   ```

   Cambiarlo con un comando in corso è una precondizione violata: si cambia solo
   con il runner fermo.
3. Un comando digitato (o un suo alias) che non vale nell'ambiente attivo non
   parte: il runner scrive nella cronologia `Command "<NAME>" is not available
   in this environment.` e resta a `Command:`, come per un comando sconosciuto,
   senza transazione. `Invio` dopo, che ripete l'ultimo comando, segue la stessa
   regola.
4. Il registro dice gli ambienti di un comando per nome
   (`environmentsOf(std::string_view name)`), per `app` (P30).

## Regole della tappa che valgono qui

- `command_runner.*`: prima di te P26.
- Con l'ambiente `drawing2d` niente cambia: i casi di verifica restano invariati.

## Test richiesti

- `command_registry_test.cpp`: un comando di prova con `environments{"test"}`;
  `environmentsOf` per un nome e per un alias.
- `command_runner_test.cpp`: nell'ambiente `drawing2d` il comando di prova non
  parte e la cronologia ha il messaggio; nell'ambiente `test` parte, e `LINE` non
  parte; `Invio` che ripete un comando di un altro ambiente non lo esegue.

## Fatto quando

- I comandi dichiarano i loro ambienti, e il runner rifiuta quelli di un altro.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
