# ADR 0027: Server MCP di sviluppo e libreria JSON

- **Stato:** Accettato (2026-10-25)
- **Data:** 2026-10-25

## Contesto

Gli agenti che lavorano a Tecnigrafo verificano il programma solo attraverso la
CI e `tecnigrafo-script`: scrivono uno script, lo eseguono, leggono il
rapporto. Un canale dal vivo, in cui l'agente esegue un comando e vede subito
il risultato, accorcia il lavoro. L'ADR 0011 (punto 10) lo ammette solo con un
nuovo ADR, sopra lo stesso linguaggio, senza porte di rete.

Il protocollo che gli agenti già parlano è **MCP** (Model Context Protocol):
JSON-RPC 2.0 su stdio. Il responsabile ha scelto il 2026-10-25 (idea #224): un
eseguibile di sviluppo `tecnigrafo-mcp`, senza Qt e non installato, su stdio;
una libreria JSON già pronta; le domande al modello esposte solo con
un'opzione; una libreria degli strumenti che il programma distribuito riuserà
nella tappa 0.8, dove le domande non si esporranno mai.

## Decisione

### 1. Modulo e eseguibile

- Nuovo modulo **`mcp`** (`tg::mcp`, cartella `mcp/`, libreria `tg_mcp`): dipende
  da `script`, `cmd` e `io`, e dalla libreria JSON; **non** da Qt.
- `tecnigrafo-mcp` (in `mcp/tool/`) è uno strumento di sviluppo come
  `tecnigrafo-script`: si compila con i test, **non si installa** e non entra
  nei pacchetti di distribuzione.

### 2. Libreria JSON

- **nlohmann/json 3.12.0** (licenza MIT, compatibile con la GPLv3 per l'ADR
  0003), solo l'intestazione unica `json.hpp`, copiata in
  `third_party/nlohmann_json/` con versione, hash SHA-256 e licenza nel suo
  `README.md`, come zlib (ADR 0019).
- La usa **solo** `mcp`. È la nuova dipendenza ammessa da questo ADR (ADR
  0002).
- Il lettore JSON di `io` (`io/src/json.cpp`, per `manifest.json` del
  `.cadx`) resta com'è: è piccolo, con limiti stretti, e non è un'interfaccia
  pubblica.

### 3. Trasporto e protocollo

- Solo **stdio**: un messaggio JSON-RPC per riga su stdin e stdout, i log su
  stderr. Nessuna porta di rete, nessun socket, nessun file di scambio.
- Revisione del protocollo: la più recente stabile al momento del pacchetto
  0.6-P25 (oggi `2025-11-25`), scritta nella sua scheda; il server accetta
  anche la revisione precedente se il client la chiede.
- Metodi: `initialize`, `notifications/initialized`, `ping`, `tools/list`,
  `tools/call`. Nient'altro: niente risorse, prompt, campionamento,
  notifiche di modifica.
- Limiti: un messaggio in entrata oltre 4 MiB o una richiesta con più di 10 000
  righe di script si rifiutano con un errore, senza eseguire nulla.

### 4. La sessione

- Un processo tiene **una sessione dal vivo**: un documento, l'editor e un
  `CommandRunner` con i comandi di `cmd`, come il livello 1 (ADR 0011), con il
  controllo delle descrizioni acceso (ADR 0026) e le stesse regole di
  determinismo (UTC, formattazione inglese europea, numerazione).
- `--root <cartella>` è la sola cartella in cui i comandi dei file leggono e
  scrivono: solo percorsi relativi, niente `..`, niente percorsi assoluti
  (come `io::FileAccess` in modalità di controllo, ADR 0021). Senza `--root`
  i comandi dei file rispondono `File access is not available.`
- `--sources <cartella>` dà accesso in sola lettura ai file pubblici come
  `@sources/<nome>`, come in `tecnigrafo-script`.

### 5. Strumenti

| Strumento | Fa | Restituisce |
|---|---|---|
| `run` | esegue righe di script (`.tgs`, solo direttive del livello 1) nella sessione | le righe nuove della cronologia e la domanda attuale |
| `report` | — | lo stato attuale nel formato del rapporto (`docs/verifica/rapporto.md`) |
| `reset` | ricomincia da un documento vuoto | la domanda attuale |
| `list_commands` | — | nome, alias e sintesi di ogni comando (ADR 0026) |
| `describe_command` | — | la descrizione completa di un comando |
| `open`, `save` | eseguono `OPEN` e `SAVEAS` con il nome dato, dentro `--root` | come `run` |
| `run_script` | esegue un intero script in una sessione nuova, separata da quella dal vivo | il rapporto completo, identico a `tecnigrafo-script run` |
| `check_case` | esegue un caso di `tests/verifica/casi/` dentro `--root` | l'esito e le differenze, come `tecnigrafo-script check` |
| `ask` | solo con `--questions`: esegue un file di domande al modello (`.tgq`) | le risposte, come `tecnigrafo-script` |

- `open` e `save` passano dai comandi, non da `io` direttamente, così il
  risultato è quello di uno script.
- Un errore di uno strumento (riga non valida, file assente) è un risultato con
  `isError` e il messaggio; un errore di protocollo è un errore JSON-RPC.
- Senza `--questions`, `ask` non compare in `tools/list` e una chiamata
  risponde come a uno strumento sconosciuto.

### 6. Equivalenza con `tecnigrafo-script`

Un test esegue ogni caso di verifica di livello 1 attraverso `run_script` e
`check_case` e confronta il risultato con `tecnigrafo-script`: devono essere
identici. È il riferimento del modulo: `mcp` non ha casi propri.

### 7. Regole per gli agenti

- Chi scrive riferimenti non usa `tecnigrafo-mcp` né `tecnigrafo-script` per
  ricavare un valore atteso (ADR 0011, punto 1): registrerebbe il
  comportamento del programma. La regola entra in AGENTS.md.
- `tecnigrafo-mcp` non sostituisce la CI: fa fede solo la CI.

### 8. Il canale dell'utente (tappa 0.8)

La libreria degli strumenti è pensata per essere riusata dal programma
distribuito, sul disegno aperto, solo in locale e con il canale acceso
dall'utente. Lì `ask`, `run_script` e `check_case` non esistono. I dettagli si
decidono con un ADR della tappa 0.8.

## Alternative considerate

- **Usare il JSON di `io`:** è minimo, pensato per un file piccolo e fidato, e
  renderlo pubblico allargherebbe `io` a un uso che non è il suo.
- **Scrivere un JSON nostro per `mcp`:** lavoro su un componente che non è il
  cuore del CAD, e con più rischi di una libreria molto usata e provata.
- **Altre librerie** (RapidJSON, simdjson, Boost.JSON): più veloci o più
  grandi, ma la velocità qui non conta; nlohmann/json è un solo file, MIT,
  con un'interfaccia semplice.
- **Trasporto HTTP:** apre una porta, contro l'ADR 0011 punto 10.
- **Server dentro il programma con l'interfaccia:** utile per l'utente, è la
  tappa 0.8; per gli agenti il livello 1 basta e gira ovunque.

## Conseguenze

- La CI compila e prova `tecnigrafo-mcp` sui tre sistemi, come
  `tecnigrafo-script`.
- `docs/AMBIENTE-SVILUPPO.md` spiega come collegarlo a Claude Code, Codex e
  Antigravity (pacchetto 0.6-P28).
- nlohmann/json va seguita come ogni dipendenza copiata: si aggiorna con un
  pacchetto, cambiando versione e hash.
