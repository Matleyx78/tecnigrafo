# [0.7.5-P24] `edit`: il ciclo di vita del file fuori da `Editor`

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: il comportamento non cambia; i test di unità e i
  casi di verifica esistenti, invariati, sono la prova
- **Dipende da:** P18, P23
- **Corretta:** 2026-11-07, con il verbale della revisione delle interfacce
  (P23, [#563](https://github.com/Matleyx78/tecnigrafo/issues/563)): R1 (il file
  si aggiorna prima della notifica), N1 (il file dato al runner è un prestito),
  N2 (inizializzatori dei test), N3 (il rapporto legge il file dal runner)
- **Modulo:** `edit` (più i chiamanti in `cmd`, `script` e `app`, con un cambio
  meccanico)
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 2 e
  3; [ADR 0021](../../adr/0021-comandi-dei-file.md);
  `edit/include/tg/edit/editor.hpp` e `edit/src/editor.cpp`;
  `cmd/src/commands/file_commands.cpp`; `cmd/include/tg/cmd/command.hpp`
  (`CommandContext`); `cmd/include/tg/cmd/command_runner.hpp`
  (`setFileAccess`, come esempio); `script/src/report.cpp` (il campo `drawing`);
  `app/src/main_window.cpp` (titolo, percorso dei dialoghi, chiusura)

## Perché

`Editor` tiene oggi anche il nome del file e se il disegno ne ha uno. Con più
documenti aperti, nome e file sono della sessione (ADR 0041, punto 3); la
cronologia resta di `Editor`, e «modificato» resta calcolato da lei.

## File

- `edit/include/tg/edit/document_file.hpp`, `edit/src/document_file.cpp` (nuovi)
- `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`, `edit/CMakeLists.txt`
- `cmd/include/tg/cmd/command.hpp`, `cmd/include/tg/cmd/command_runner.hpp`,
  `cmd/src/command_runner.cpp`, `cmd/src/commands/file_commands.cpp`
- `script/src/report.cpp`
- `app/src/main_window.cpp`
- `tests/edit/document_file_test.cpp` (nuovo), `tests/edit/editor_test.cpp`,
  `tests/cmd/file_commands_test.cpp`, `tests/app/file_menu_test.cpp`,
  `tests/CMakeLists.txt`
- `tests/cmd/arc_command_test.cpp`, `tests/cmd/circle_command_test.cpp`,
  `tests/cmd/text_command_test.cpp`: solo gli inizializzatori diretti di
  `CommandContext` (punto 3)

## Cosa fare

### 1. `edit::DocumentFile`

```cpp
namespace tg::edit {

/// The file of an open document (docs/adr/0041, point 3): its name and
/// whether it has one. Whether it is modified is the Editor's: it depends on
/// the history.
class DocumentFile {
public:
    DocumentFile() = default;  ///< "Drawing1.cadx", no file

    /// The name, UTF-8: "Drawing1.cadx" until the first save or load.
    [[nodiscard]] const std::string& name() const { return m_name; }

    /// True once the document has been saved or loaded under a name.
    [[nodiscard]] bool hasFile() const { return m_hasFile; }

    /// After a save or a load: the document is now the file `name`.
    void setSaved(std::string name);

    /// A new document without a file, named `untitled` ("Drawing1.cadx").
    void reset(std::string untitled);

private:
    std::string m_name{"Drawing1.cadx"};
    bool m_hasFile{false};
};

}  // namespace tg::edit
```

### 2. `Editor` senza nome e file

- `fileName()` e `hasFile()` spariscono.
- `markSaved(std::string name)` diventa `markSaved()`: segna la posizione
  salvata della cronologia e `LWDISPLAY` salvato, come oggi, senza il nome.
- `replaceDocument(core::Document, std::string name)` diventa
  `replaceDocument(core::Document)`: sostituisce il documento, svuota la
  cronologia, segna la posizione salvata e notifica una volta, come oggi.
- `isModified()` resta com'è, calcolato dalla cronologia.

### 3. Chi tiene il file

- `CommandContext` ha `edit::DocumentFile& file`, accanto a `editor`.
- `CommandRunner` ha `void setDocumentFile(edit::DocumentFile* file)`, come
  `setFileAccess`; finché non gliene si dà uno, usa un `DocumentFile` suo. Così
  i costruttori e i test di oggi non cambiano. Il `DocumentFile` dato è un
  prestito: vive più a lungo del runner, e lo dice il commento di
  `setDocumentFile`, come per `setFileAccess`.
- `NEW`, `OPEN`, `SAVE`, `SAVEAS` e `QSAVE` leggono e scrivono il nome e il file
  dal contesto, con gli stessi messaggi di oggi: dove oggi chiamano
  `markSaved(name)`, chiamano `markSaved()` e `file.setSaved(name)`; dove
  chiamano `replaceDocument(doc, name)`, chiamano **prima**
  `file.setSaved(name)`, o `file.reset("Drawing1.cadx")` per `NEW`, e **poi**
  `replaceDocument(doc)`. L'ordine conta: `replaceDocument` notifica dentro la
  chiamata, e chi ascolta (il listener di `MainWindow`, che riconosce un `OPEN`
  riuscito e lo mette nei file recenti) deve trovare documento e file già
  coerenti, come oggi. Un errore di lettura non tocca né il file né il
  documento.
- `CommandRunner` ha anche
  `[[nodiscard]] const edit::DocumentFile& documentFile() const`, il file in uso
  (il suo o quello dato con `setDocumentFile`).
- `MainWindow` tiene un `DocumentFile`, lo dà al runner e lo legge per il titolo,
  i dialoghi e la chiusura; il rapporto degli script lo legge dal runner, con
  `documentFile()` (`ReportInput` ha già il runner: non cambia).
- I test che costruiscono un `CommandContext` con gli inizializzatori diretti
  (`CommandContext context{editor, {}, …}` in `arc_command_test.cpp`,
  `circle_command_test.cpp`, `text_command_test.cpp` e
  `file_commands_test.cpp`) non compilano più, perché `file` è un riferimento
  senza valore predefinito: si adeguano in modo meccanico, con un
  `DocumentFile` locale, senza toccare asserzioni né risultati.

### 4. Che cosa non cambia

Messaggi, richieste, cronologia, rapporto degli script e casi di verifica.

## Regole della tappa che valgono qui

- `command.hpp`, `command_runner.*`: dopo di te P25 (solo i chiamanti) e P26.
- `main_window.*`: prima di te P16 e P18, dopo di te P25 e P28. `editor.*`:
  dopo di te P25.
- Un caso di verifica che cambia ti ferma (AGENTS.md, «Quando fermarsi»).

## Test richiesti

- `document_file_test.cpp`: il nome iniziale; `setSaved`; `reset`.
- `editor_test.cpp`: i test di oggi su `markSaved`, `replaceDocument` e
  `isModified`, riscritti con le firme nuove, con gli stessi risultati.
- `file_commands_test.cpp`: `SAVEAS` poi `QSAVE` scrivono nel file del contesto;
  un `DocumentFile` dato con `setDocumentFile` riceve il nome.
- `file_commands_test.cpp`, con un listener installato sull'editor: da
  `Drawing1.cadx` senza file, `OPEN plan.dxf`; **dentro** la notifica il file
  dice `plan.dxf`, `hasFile` vero e `isModified` falso. Da un documento aperto
  come `plan.dxf`, `NEW`: dentro la notifica il file dice `Drawing1.cadx` e
  `hasFile` falso. Un `OPEN` che non legge il file non notifica e lascia nome e
  file com'erano (R1 del verbale di P23).
- `file_menu_test.cpp`: il test di oggi sui file recenti dopo `OPEN`
  (`SavesOpensCopiesAndUpdatesTitleAndRecents`) passa invariato.

## Fatto quando

- `Editor` non ha più nome né file, e i comandi dei file usano `DocumentFile`.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
