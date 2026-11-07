# [0.7.5-P28] `cmd` e `app`: una sessione per documento e il contratto di attivazione

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: con un documento il comportamento non cambia;
  test di unità della sessione e test Qt dell'attivazione
- **Dipende da:** P14, P16, P18, P24, P26
- **Modulo:** `app` (più la sessione del disegno in `cmd`)
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 2 e
  3; `app/src/main_window.hpp` e `.cpp` (i membri in ordine di dipendenza, il
  listener di `Editor`, `refresh`), `app/src/cad_view.hpp`,
  `app/src/command_line.hpp`, `app/src/layer_panel.hpp`,
  `app/src/properties_panel.hpp`, `app/src/script_session.hpp`;
  `cmd/include/tg/cmd/command_runner.hpp` (costruzione, `setFileAccess`,
  `setDocumentFile` di P24)

## Perché

`MainWindow` possiede oggi l'unico documento, il suo `Editor` e il suo runner, e
vista, riga di comando e pannelli li tengono per riferimento dalla costruzione.
Le schede dei documenti (P29) chiedono che tutto questo stia in una **sessione**
per documento e che l'interfaccia segua la sessione attiva (ADR 0041, punto 2).
Questo pacchetto fa il cambio con **una sola** sessione: niente cambia per chi
usa il programma.

## File

- `cmd/include/tg/cmd/drawing_session.hpp`, `cmd/src/drawing_session.cpp` (nuovi)
- `cmd/CMakeLists.txt`
- `app/src/document_session.hpp`, `app/src/document_session.cpp` (nuovi)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/cad_view.hpp`, `app/src/cad_view.cpp`
- `app/src/command_line.hpp`, `app/src/command_line.cpp`
- `app/src/layer_panel.hpp`, `app/src/layer_panel.cpp`,
  `app/src/properties_panel.hpp`, `app/src/properties_panel.cpp`: solo il
  cambio di sessione
- `app/CMakeLists.txt`
- `tests/cmd/drawing_session_test.cpp` (nuovo), `tests/app/session_test.cpp`
  (nuovo), `tests/CMakeLists.txt`

## Cosa fare

### 1. La sessione del disegno, senza Qt

```cpp
namespace tg::cmd {

/// One open drawing (docs/adr/0041, point 2): the document, its history, its
/// file and its runner, built in this order and destroyed in reverse.
class DrawingSession {
public:
    /// `registry` and `files` are not owned and must outlive the session;
    /// `files` may be nullptr (no file commands).
    DrawingSession(const CommandRegistry& registry, io::FileAccess* files,
                   core::Document document = core::Document::newDrawing());
    DrawingSession(const DrawingSession&) = delete;
    DrawingSession& operator=(const DrawingSession&) = delete;

    [[nodiscard]] const core::Document& document() const;
    [[nodiscard]] edit::Editor& editor();
    [[nodiscard]] edit::DocumentFile& file();
    [[nodiscard]] CommandRunner& runner();
    // and the const versions
};

}  // namespace tg::cmd
```

In `cmd` perché script e canale MCP della 0.8 la possano usare senza Qt; in
questa tappa la usa solo `app`, e il server MCP di sviluppo tiene la sua
`Level1Session` (ADR 0041, punto 2).

### 2. La sessione dell'interfaccia

`app::DocumentSession` possiede una `cmd::DrawingSession` e lo stato della vista
(centro e scala), e ha l'identificatore del suo tipo di documento (oggi sempre il
disegno; il registro è di P30). `MainWindow` possiede un elenco di sessioni e ne
ha una **attiva**; in questo pacchetto l'elenco ne ha una sola, creata
all'avvio con il disegno di esempio o vuoto, come oggi.

### 3. Il contratto di attivazione

`MainWindow::activate(DocumentSession&)`, con il runner della sessione che si
lascia fermo o a metà di un comando:

- salva lo stato della vista della sessione che lascia;
- collega alla nuova sessione la vista (`CadView`), la riga di comando, i pannelli
  *Layers* e *Properties*, le azioni di annulla e ripeti, gli aiuti al disegno
  (orto, polare, snap restano impostazioni della finestra e si danno al runner
  nuovo) e il listener dei cambiamenti; ripristina la sua vista;
- aggiorna titolo e stato.

Vista, riga di comando e pannelli non tengono più riferimenti presi alla
costruzione: ricevono la sessione con una funzione (`setSession`), e senza
sessione non disegnano e non accettano immissioni. Il listener di ogni sessione
aggiorna l'interfaccia solo se la sessione è quella attiva.

`document()`, `editor()` e `runner()` di `MainWindow` restituiscono quelli della
sessione attiva: script di livello 2, rapporto e test non cambiano.

## Regole della tappa che valgono qui

- `main_window.*`, `cad_view.*`, pannelli: prima di te P14, P16 e P18 (se c'è);
  dopo di te P29. `command_runner.*`: non lo tocchi.
- Con una sessione il programma si comporta come oggi; i casi di verifica di
  livello 2 restano invariati.

## Test richiesti

- `drawing_session_test.cpp`: costruzione e distruzione; il runner della sessione
  lavora sul suo documento; due sessioni hanno cronologie separate.
- `session_test.cpp` (test Qt): con una seconda sessione creata dal test,
  `activate` fa disegnare alla vista l'altro documento, i pannelli mostrano i
  suoi layer, `U` annulla nella sessione attiva e non nell'altra; un comando
  lasciato a metà in una sessione riprende quando torna attiva.

## Fatto quando

- `MainWindow` non possiede più documento, editor e runner, ma sessioni, e
  l'interfaccia segue quella attiva.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
