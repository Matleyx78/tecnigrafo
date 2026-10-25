# [0.6-P25] `mcp`: modulo, JSON e protocollo

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: strumento di sviluppo; lo provano i test del
  protocollo
- **Dipende da:** P1, P3
- **Modulo:** `mcp` (nuovo) e `third_party/nlohmann_json`
- **Da leggere:** [ADR 0027](../../adr/0027-server-mcp-di-sviluppo.md) (tutto),
  [ADR 0026](../../adr/0026-descrizione-dei-comandi.md) punto 4,
  [ADR 0019](../../adr/0019-archivi-zip.md) (come si copia una dipendenza);
  `docs/ARCHITETTURA.md` (moduli); `script/CMakeLists.txt` e
  `script/tool/main.cpp` (uno strumento di sviluppo non installato);
  `cmd/include/tg/cmd/command_description.hpp` (P1)

## File

- `third_party/nlohmann_json/json.hpp`, `LICENSE`, `README.md` (nuovi: la
  versione 3.12.0, con hash SHA-256 e licenza; non contano nel limite)
- `third_party/CMakeLists.txt` (bersaglio d'interfaccia `tg_nlohmann_json`)
- `CMakeLists.txt` (`add_subdirectory(mcp)`, il commento sugli strati)
- `mcp/CMakeLists.txt` (nuovo)
- `mcp/include/tg/mcp/server.hpp`, `mcp/src/server.cpp` (nuovi: JSON-RPC e
  metodi)
- `mcp/include/tg/mcp/tool.hpp` (nuovo: l'interfaccia degli strumenti)
- `mcp/src/command_tools.cpp` (nuovo: `list_commands`, `describe_command`)
- `mcp/tool/main.cpp` (nuovo: `tecnigrafo-mcp`)
- `tests/mcp/server_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `.github/workflows/ci.yml` solo se serve (lo approva il responsabile)

## Cosa fare

### Dipendenza

`json.hpp` 3.12.0 copiato com'è, incluso come `SYSTEM`; il bersaglio
`tg_nlohmann_json` lo collega **solo** `tg_mcp` (ADR 0027, punto 2).

### Server

```cpp
/// One tool (docs/adr/0027, point 5).
class Tool {
public:
    virtual ~Tool() = default;
    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual std::string description() const = 0;
    [[nodiscard]] virtual nlohmann::json inputSchema() const = 0;  ///< JSON Schema object
    /// The result content; an error is {isError: true} with the message.
    [[nodiscard]] virtual ToolResult call(const nlohmann::json& arguments) = 0;
};

struct ServerOptions {
    std::size_t maxMessageBytes{4 * 1024 * 1024};
};

/// MCP over stdio: one JSON-RPC message per line on `in`, answers on `out`,
/// logs on `log` (docs/adr/0027, point 3). Returns at the end of `in`.
class Server {
public:
    Server(std::vector<std::unique_ptr<Tool>> tools, ServerOptions options = {});
    int run(std::istream& in, std::ostream& out, std::ostream& log);
};
```

- Metodi: `initialize` (risponde con la revisione del protocollo, il nome
  `tecnigrafo-mcp`, la versione del progetto e la capacità `tools`),
  `notifications/initialized`, `ping`, `tools/list`, `tools/call`. Ogni altro
  metodo: errore `-32601`. JSON non valido: `-32700`; richiesta non valida:
  `-32600`; argomenti non validi: `-32602`. Prima di `initialize` si risponde
  solo a `ping`.
- **Revisione del protocollo:** verifica sulla specifica pubblica di MCP
  (<https://modelcontextprotocol.io/specification>) qual è la più recente
  stabile; l'ADR 0027 indica `2025-11-25`. Se è un'altra, usala e scrivilo
  nella pull request. Se il client chiede la revisione precedente, la si
  accetta; un'altra si rifiuta con l'errore della specifica.
- Un messaggio oltre il limite non si analizza: errore `-32600` e il server
  continua.
- **Strumenti di questo pacchetto:** `list_commands` e `describe_command`,
  dalle descrizioni del `CommandRegistry` con i comandi di `cmd` (ADR 0026),
  in JSON con gli stessi campi di `CommandDescription`.

### `tecnigrafo-mcp`

`tecnigrafo-mcp [--root <cartella>] [--sources <cartella>] [--questions]`:
le opzioni si leggono e si convalidano qui (cartelle esistenti); le usa P26.
Senza stdin interattivo esce alla fine dell'input. Come `tecnigrafo-script`:
compilato con i test, **mai installato**.

## Regole della tappa che valgono qui

- `mcp` non usa Qt; dipende da `script`, `cmd`, `io` e da nlohmann/json.
- Nessuna porta di rete, nessun socket (ADR 0011, punto 10).
- `CMakeLists.txt` e `third_party/CMakeLists.txt`: solo le tue righe (P3 è
  già unito).

## Test richiesti

Con flussi in memoria:

- sequenza `initialize` → `notifications/initialized` → `tools/list` →
  `tools/call`;
- ogni errore JSON-RPC; messaggio troppo grande; richiesta prima di
  `initialize`; notifiche senza risposta;
- `list_commands` con tutti i comandi; `describe_command` per nome, alias,
  comando sconosciuto (`isError`).

## Fatto quando

- I test qui sopra esistono e passano sui tre sistemi.
- `tecnigrafo-mcp` si compila e non si installa.
- La CI è verde.
