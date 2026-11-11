# [0.8-PT36] `mcp`: strumenti sopra una sessione prestata

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: `tecnigrafo-mcp` non cambia; il confronto con
  `tecnigrafo-script` resta il riferimento (ADR 0027, punto 6)
- **Dipende da:** —
- **Modulo:** `mcp`
- **Da leggere:** [ADR 0051](../../adr/0051-canale-mcp-dell-assistente.md);
  [ADR 0027](../../adr/0027-server-mcp-di-sviluppo.md);
  [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punto 2;
  `mcp/include/tg/mcp/*.hpp`, `mcp/src/*.cpp`, `mcp/tool/main.cpp`;
  `cmd/include/tg/cmd/drawing_session.hpp`; la scheda
  [PT37](PT37-mcp-canale.md) (sorella)

## File

- `mcp/include/tg/mcp/tool_host.hpp` (nuovo), `mcp/include/tg/mcp/server.hpp`,
  `mcp/src/server.cpp`, `mcp/src/command_tools.cpp`, `mcp/src/session_tools.cpp`,
  `mcp/src/case_tools.cpp`, `mcp/include/tg/mcp/session.hpp`, `mcp/src/session.cpp`
- `tests/mcp/…`, `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Ospite degli strumenti.** Gli strumenti `run`, `report`,
   `list_commands`, `describe_command`, `open` e `save` lavorano su
   un'interfaccia, non su `Session`:

   ```cpp
   /// What the tools act on (docs/adr/0051, point 4): the private session of
   /// tecnigrafo-mcp, or the active session lent by the program.
   class ToolHost {
   public:
       virtual ~ToolHost() = default;
       [[nodiscard]] virtual ToolResult run(std::span<const std::string> lines) = 0;
       [[nodiscard]] virtual std::string report() = 0;
       [[nodiscard]] virtual ToolResult open(std::string_view path) = 0;
       [[nodiscard]] virtual ToolResult save(std::string_view path, bool replace) = 0;
       [[nodiscard]] virtual const cmd::CommandRegistry& commands() const = 0;
   };
   ```

   `Session` di oggi la realizza, senza cambiare comportamento.
2. **Insiemi di strumenti.** Il server si crea con l'elenco degli strumenti da
   offrire: `tecnigrafo-mcp` quelli di oggi (con `ask` solo con `--questions`);
   il programma (PT37) `run`, `report`, `list_commands`, `describe_command`,
   `open`, `save`, senza `reset`, `run_script`, `check_case`, `ask`. Uno
   strumento non offerto risponde come uno sconosciuto.
3. **Trasporto separato.** Il server legge e scrive messaggi JSON-RPC da
   un'interfaccia di righe (`readLine`/`writeLine`), così PT37 gli dà un socket
   locale al posto di stdin e stdout. Limiti e protocollo dell'ADR 0027 non
   cambiano.
4. Nessuna dipendenza nuova; `mcp` resta senza Qt.

## Regole della tappa che valgono qui

- Il test di equivalenza con `tecnigrafo-script` deve restare verde senza
  cambiare i casi.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- Un `ToolHost` di prova: il server con l'insieme del programma elenca solo i
  sei strumenti e rifiuta `reset`; `run` arriva all'ospite.
- Il trasporto a righe in memoria: `initialize`, `tools/list`, `tools/call`.

## Fatto quando

- `tecnigrafo-mcp` si comporta come prima; gli strumenti girano su un ospite
  qualunque e su un trasporto qualunque; la CI è verde.
