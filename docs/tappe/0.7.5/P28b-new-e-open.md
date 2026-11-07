# [0.7.5-P28b] `cmd`, `script` e `app`: `NEW` e `OPEN` aprono un documento nuovo

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7.5-R28b]` (#…), agente gemini
- **Dipende da:** P27, P28, R28b
- **Modulo:** `cmd` (più l'ospite dei documenti in `script` e in `app`)
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 2 e
  3; [ADR 0021](../../adr/0021-comandi-dei-file.md);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-07, «`NEW` e `OPEN` con i
  documenti in schede»; [verifica/rapporto.md](../../verifica/rapporto.md),
  `documents:` e `drawing:`; `cmd/src/commands/file_commands.cpp`,
  `cmd/include/tg/cmd/drawing_session.hpp` (P28), `command.hpp`,
  `command_runner.hpp`; `script/include/tg/script/level1.hpp` e
  `script/src/level1.cpp`, `script/src/report.cpp`; `app/src/main_window.cpp`
  (sessioni e `activate`, P28), `app/src/script_session.cpp`

## Perché

Con i documenti in schede, `NEW` e `OPEN` fanno come in AutoCAD: aprono un
documento nuovo, che diventa attivo, e lasciano aperto quello di prima, senza
chiedere di salvarlo (decisione del 2026-11-07, strada B). Deve valere nel
programma e negli script dei due livelli, perché i due livelli scrivono lo
stesso rapporto. Le schede che si vedono arrivano con P29; qui la finestra apre
già le sessioni nuove.

## File

- `cmd/include/tg/cmd/document_host.hpp` (nuovo)
- `cmd/include/tg/cmd/command.hpp`, `cmd/include/tg/cmd/command_runner.hpp`,
  `cmd/src/command_runner.cpp` (l'ospite nel contesto)
- `cmd/src/commands/file_commands.cpp`
- `script/include/tg/script/level1.hpp`, `script/src/level1.cpp`,
  `script/src/report.cpp`
- `app/src/main_window.hpp`, `app/src/main_window.cpp`,
  `app/src/script_session.cpp`
- `tests/cmd/file_commands_test.cpp`, `tests/script/level1_test.cpp`,
  `tests/script/report_test.cpp`, `tests/app/file_menu_test.cpp`

## Cosa fare

### 1. L'ospite dei documenti

```cpp
namespace tg::cmd {

/// Who keeps the open documents, for NEW and OPEN (docs/adr/0041, point 2).
class DocumentHost {
public:
    virtual ~DocumentHost() = default;
    /// Adds `document`, with its file, after the others and makes it active.
    virtual DrawingSession& addDocument(core::Document document, edit::DocumentFile file) = 0;
    /// The open document whose file is `name` (the same text as
    /// DocumentFile::name()), or nullptr.
    [[nodiscard]] virtual DrawingSession* findOpen(std::string_view name) = 0;
    /// Makes `session`, one of the open documents, active.
    virtual void activate(DrawingSession& session) = 0;
    /// The names of the documents not saved yet ("Drawing<N>.cadx"), for the
    /// next untitled name.
    [[nodiscard]] virtual std::vector<std::string> openNames() const = 0;
};

}  // namespace tg::cmd
```

`CommandContext` ha `DocumentHost* host{nullptr}`; il runner ha
`setDocumentHost(DocumentHost*)`. Un runner senza ospite (i test di unità che lo
costruiscono da solo) fa `NEW` e `OPEN` come oggi, sostituendo il documento:
nessun uso nel programma, nei due livelli di script o nel server MCP resta senza
ospite.

### 2. `NEW` e `OPEN` con l'ospite

- **`NEW`:** niente richiesta di salvare; un documento nuovo vuoto, chiamato
  `Drawing<N>.cadx` con il primo `N` da 1 non tra `openNames()`, aggiunto e
  attivo.
- **`OPEN`:** chiede il file come oggi; niente richiesta di salvare.
  - Se il file è già aperto (`findOpen`), lo attiva e scrive
    `<name> is already open.` nella cronologia.
  - Se il documento attivo **non ha file e non ha passi di annulla né di
    ripeti** (il disegno appena creato, come il `Drawing1` iniziale di AutoCAD),
    il file aperto lo **sostituisce**, come oggi.
  - Altrimenti il file aperto si aggiunge come documento nuovo, attivo.
  - Un errore di lettura lascia tutto com'è, con i messaggi di oggi.
- Le richieste e i messaggi che `NEW` e `OPEN` scrivono oggi restano, tranne la
  domanda `Save changes to …? [Yes/No/Cancel] <Yes>:`, che non c'è più.
- La cronologia è una sola per la riga di comando e per lo script: le righe di
  `NEW` e `OPEN` non si perdono quando il documento attivo cambia.

### 3. Gli ospiti

- **Livello 1** (`Level1Session`, anche del server MCP di sviluppo): tiene un
  elenco di `DrawingSession`, in ordine di apertura, con una attiva; le azioni
  dello script vanno alla sessione attiva.
- **Livello 2 e programma** (`MainWindow`): aggiunge una `DocumentSession` per
  ogni documento nuovo e la attiva con `activate` (P28). Le schede per
  vederle e chiuderle le fa P29.
- **Rapporto:** il campo `documents:` quando i documenti aperti sono più di uno
  ([rapporto.md](../../verifica/rapporto.md)); gli altri campi descrivono il
  documento attivo, come prima.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P28, dopo di te P29. `command.hpp`,
  `command_runner.*`: prima di te P27.
- I casi in attesa di R28b sono i riferimenti di questo pacchetto: togli il loro
  file da `tests/verifica/in-attesa/` (AGENTS.md, «Prima di dichiarare il lavoro
  finito»). Gli altri casi restano invariati.

## Test richiesti

- `file_commands_test.cpp`, con un ospite di prova: `NEW` con un documento
  modificato aggiunge `Drawing2.cadx`, attivo, senza domanda; `OPEN` sul disegno
  appena creato lo sostituisce; `OPEN` con un documento modificato ne aggiunge
  uno; `OPEN` di un file già aperto lo attiva e scrive il messaggio; senza ospite,
  `NEW` sostituisce come oggi.
- `level1_test.cpp`, `report_test.cpp`: due documenti aperti danno `documents:`
  con l'attivo segnato; uno solo non lo scrive.
- `file_menu_test.cpp`: `Ctrl+N` con un disegno modificato apre una sessione
  nuova senza domanda.

## Fatto quando

- `NEW` e `OPEN` aprono un documento nuovo nei due livelli e nel programma, con
  l'eccezione del disegno appena creato.
- I casi di R28b passano e sono tolti dall'elenco in attesa; i test qui sopra
  esistono e passano.
- La CI è verde.
