# ADR 0051: Canale MCP per l'assistente dell'utente

- **Stato:** Proposto
- **Data:** 2026-11-11

## Contesto

L'idea [#224](https://github.com/Matleyx78/tecnigrafo/issues/224), strada A
(decisioni del 2026-10-25), mette nella tappa 0.8 un canale MCP del programma
distribuito, solo sul PC dell'utente, spento finché lui non lo accende, con gli
stessi strumenti di `tecnigrafo-mcp` ([ADR 0027](0027-server-mcp-di-sviluppo.md))
e la stessa libreria; le domande al modello non si espongono. L'[ADR 0011](0011-modalita-di-controllo.md),
punto 10, chiede un ADR per ogni canale dal vivo. L'[ADR 0041](0041-documenti-e-ambienti.md),
punto 2, dice già che il canale agisce sulla sessione attiva, con il suo runner
e il suo annulla, mai in parallelo con l'utente.

Gli assistenti che l'utente ha (Claude Desktop, Claude Code, Codex, un modello
locale) avviano un server MCP come **programma** e gli parlano su stdin e
stdout. Il disegno però è nel programma già aperto.

## Decisione

1. **Ponte che resta vivo** (decisione del 2026-11-11). L'assistente avvia
   `tecnigrafo --mcp`: lo stesso eseguibile, senza finestre, che parla MCP su
   stdin e stdout e **resta acceso** finché l'assistente non lo chiude, anche se
   il programma è chiuso o il canale spento. Risponde da solo a `initialize` e
   a `tools/list` (l'elenco dei sei strumenti del punto 4, che conosce perché è
   lo stesso eseguibile); a ogni `tools/call` prova a collegarsi al programma
   attraverso il canale locale e passa la richiesta così com'è. Se non ci
   riesce, quella chiamata risponde con un errore dello strumento,
   `Tecnigrafo is not running, or its assistant channel is off.`, e la
   successiva riprova. Così il canale si accende quando si vuole, e
   l'assistente funziona subito, senza riavviarlo.
2. **Canale locale.** `QLocalServer`, con un nome che contiene l'utente del
   sistema, accessibile solo a lui (`QLocalServer::UserAccessOption`): un socket
   di dominio Unix su Linux e macOS, una named pipe locale su Windows. Nessuna
   porta di rete. **Un client alla volta**: un secondo si rifiuta.
3. **Spento a ogni avvio** (decisione del 2026-11-11): l'utente lo accende con
   un interruttore del ribbon e della barra di stato con la sua icona. Per ora
   niente impostazione «accendi all'avvio»; se il clic a ogni sessione pesa, la
   si propone come `idea`.
4. **Protocollo e strumenti.** La libreria di `mcp` (JSON-RPC, revisione del
   protocollo e limiti dell'ADR 0027) gira dentro il programma. Gli strumenti:
   `run`, `report`, `list_commands`, `describe_command`, `open`, `save`, con gli
   stessi risultati di `tecnigrafo-mcp`. Non ci sono `ask`, `run_script`,
   `check_case` né `reset` (che butterebbe via il disegno dell'utente). `open` e
   `save` passano da `OPEN` e `SAVEAS` con i percorsi del PC, come farebbe
   l'utente, e `OPEN` apre una scheda nuova (decisione del 2026-11-07).
5. **Sessione.** Gli strumenti agiscono sulla sessione **attiva** (ADR 0041,
   punto 2); se l'utente cambia scheda, l'assistente segue. Senza documenti
   aperti (pagina *Start*), gli strumenti che ne chiedono uno rispondono
   `No drawing is open.`; `open` funziona.
6. **Coda comune.** Le richieste dell'assistente si eseguono una alla volta,
   nel thread dell'interfaccia, solo quando il runner della sessione attiva è
   fermo a `Command:`. Se l'utente è dentro un comando, la richiesta aspetta
   fino a 30 secondi, poi risponde `The user is running a command.` Mentre una
   richiesta gira, l'input dell'utente aspetta che finisca.
7. **Annullamento.** Ogni comando dell'assistente è un passo della cronologia
   come quelli dell'utente: `U` lo annulla.
8. **Visibile.** Le righe dell'assistente compaiono nella cronologia dei comandi
   precedute da `[AI] `; un pannello *Assistant* elenca le richieste con l'esito,
   e la barra di stato dice se un assistente è collegato.
9. **Prove:** test Qt con un client locale (collegamento, rifiuto del secondo
   client, coda con un comando dell'utente in corso, annulla, canale spento) e
   la prova manuale con un assistente vero. Il canale non è una prova del
   comportamento: valgono i casi di verifica (ADR 0027, punto 7).

## Alternative considerate

- **Porta TCP su `localhost`:** raggiungibile da ogni programma del PC, anche di
  altri utenti, e da un browser con una pagina malevola; la scheda chiede un
  canale non raggiungibile dalla rete.
- **Un eseguibile ponte a parte:** un file in più da installare e da tenere
  allineato; lo stesso eseguibile con un'opzione basta.
- **Ponte che esce se il programma non c'è:** gli assistenti avviano i server
  MCP una volta sola, al loro avvio; con il canale spento a ogni avvio
  l'utente dovrebbe riavviare l'assistente ogni volta.
- **Coda senza limite di tempo:** l'assistente resterebbe bloccato finché
  l'utente non finisce un comando, senza saperlo.
- **Esporre `reset` e le domande al modello:** il primo butterebbe via il
  lavoro dell'utente, le seconde creano entità senza comandi (ADR 0011,
  punto 10).

## Conseguenze

- `mcp` separa gli strumenti dalla sessione che li ospita: `tecnigrafo-mcp`
  tiene la sua sessione privata, il programma presta quella attiva.
- La guida per collegare un assistente sta nel manuale.
