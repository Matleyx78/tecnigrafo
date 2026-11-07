# [0.7.5-P28b] `cmd`, `script` e `app`: `NEW` e `OPEN` aprono un documento nuovo

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7.5-R28b]` (#…), agente gemini
- **Dipende da:** P27, P28, R28b
- **Corretta:** 2026-11-07, con la decisione del responsabile su B3 del verbale
  della revisione delle interfacce (P23,
  [#563](https://github.com/Matleyx78/tecnigrafo/issues/563);
  [DECISIONI.md](../../DECISIONI.md)): un file aperto in un solo documento; più
  R1 (i file recenti con l'ospite), N1 (durata dell'ospite) e N3 (che cosa
  riceve il rapporto)
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
  `script/include/tg/script/report.hpp`, `script/src/report.cpp`, e i chiamanti
  di `writeState` se `ReportInput` cambia (punto 3)
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

L'ospite è un prestito: vive più a lungo di tutti i runner a cui è dato, e lo
dice il commento di `setDocumentHost`. Quando `NEW` o `OPEN` aggiunge e attiva
un documento, il runner da cui il comando parte non si distrugge prima che il
comando finisca, e un comando sospeso nel documento che torna attivo resta
sospeso.

**Un file è aperto in un solo documento** (decisione B3). `findOpen` dà al più
un documento, perché nessun comando ne crea un secondo con lo stesso file:
`SAVEAS`, `SAVE` e `QSAVE`, con un ospite, rifiutano un nome che `findOpen` dà
per un **altro** documento. Il rifiuto arriva prima della domanda di
sostituzione e niente si scrive: `Cannot save <name>: it is open in another
document.`, con la forma di oggi per un salvataggio non riuscito. Il confronto
è quello di `findOpen`, sullo stesso testo di `DocumentFile::name()`, senza
normalizzare i percorsi. Salvare il documento sul suo stesso file resta
permesso, come oggi.

### 2. `NEW` e `OPEN` con l'ospite

- **`NEW`:** niente richiesta di salvare; un documento nuovo vuoto, chiamato
  `Drawing<N>.cadx` con il primo `N` da 1 non tra `openNames()`, aggiunto e
  attivo.
- **`OPEN`:** chiede il file come oggi; niente richiesta di salvare.
  - Se il file è già aperto (`findOpen`), lo attiva e scrive
    `<name> is already open.` nella cronologia.
  - Altrimenti il file aperto si aggiunge come documento nuovo, attivo, anche
    se il documento attivo è un disegno appena creato e mai toccato (come AutoCAD
    LT: DECISIONI.md, 2026-11-07).
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
  documento attivo, come prima. `ReportInput` riceve **esplicitamente** il file
  dell'attivo e l'elenco dei documenti aperti, in ordine di apertura, con quale
  è attivo: non si deducono dai nomi né dagli editor. La forma (campi nuovi di
  `ReportInput` o un accessor dell'ospite) la sceglie chi implementa, e la
  scrive nella pull request.
- **File recenti** (R1 del verbale di P23). Con l'ospite, `OPEN` aggiunge una
  sessione con `addDocument` e non chiama più `replaceDocument`: il listener
  dell'editor nuovo non scatta per l'apertura, e il percorso di oggi
  (`m_pendingOpen` nel listener di `MainWindow`) non registrerebbe più il file.
  Un file aperto con `OPEN`, dal menu, dalla riga di comando o dai recenti,
  entra nei file recenti come oggi. Se attivare un file già aperto lo porti in
  cima ai recenti lo sceglie chi implementa (non cambia esito né messaggi), con
  un test e una riga nella pull request.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P28, dopo di te P29. `command.hpp`,
  `command_runner.*`: prima di te P27.
- I casi in attesa di R28b sono i riferimenti di questo pacchetto: togli il
  loro file da `tests/verifica/in-attesa/` (AGENTS.md, «Prima di dichiarare il lavoro
  finito»). Gli altri casi restano invariati.

## Test richiesti

- `file_commands_test.cpp`, con un ospite di prova: `NEW` con un documento
  modificato aggiunge `Drawing2.cadx`, attivo, senza domanda; `OPEN` con il
  disegno appena creato ne aggiunge uno, e il disegno resta; `OPEN` con un
  documento modificato ne aggiunge uno; `OPEN` di un file già aperto lo attiva e scrive il messaggio; senza ospite,
  `NEW` sostituisce come oggi.
- `file_commands_test.cpp` (B3), con un ospite di prova: A aperto come
  `a.cadx`, B come `b.cadx`; in B `SAVEAS a.cadx` scrive `Cannot save a.cadx: it
  is open in another document.`, non chiede di sostituire, non scrive il file,
  e B resta `b.cadx`, con la stessa cronologia. Lo stesso con `SAVE a.cadx` (la
  copia). In A `SAVEAS a.cadx` salva come oggi. Senza ospite, `SAVEAS a.cadx`
  chiede di sostituire come oggi.
- `level1_test.cpp`, `report_test.cpp`: due documenti aperti danno `documents:`
  con l'attivo segnato; uno solo non lo scrive.
- `file_menu_test.cpp`: `Ctrl+N` con un disegno modificato apre una sessione
  nuova senza domanda.
- `file_menu_test.cpp` (R1): `OPEN plan.dxf` dal menu con un disegno aperto
  aggiunge una sessione, e `plan.dxf` è in cima ai file recenti; lo stesso con
  `OPEN` scritto nella riga di comando.

## Fatto quando

- `NEW` e `OPEN` aprono un documento nuovo nei due livelli e nel programma.
- I casi di R28b passano e sono tolti dall'elenco in attesa; i test qui sopra
  esistono e passano.
- La CI è verde.
