# [0.5-P17] `app`: menu File, finestre di dialogo, file recenti, chiusura

- **Tappa:** 0.5 — [piano](../0.5-file-e-dxf.md)
- **Issue:** #177
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.5-R17]` (#176), agente gemini (livello 2)
- **Dipende da:** P16, R17
- **Modulo:** `app`
- **Da leggere:** [ADR 0021](../../adr/0021-comandi-dei-file.md) punti 2 e 5;
  `app/src/main_window.hpp/.cpp` (menu esistenti, nomi degli oggetti per i
  test), `app/src/script_session.cpp`, `cmd/include/tg/cmd/command.hpp`
  (`FilePrompt`, P15), `io/include/tg/io/file_access.hpp` (`DiskFiles`, P4);
  `tests/app/` per lo stile dei test

## File

- `app/src/main_window.hpp`
- `app/src/main_window.cpp`
- `app/src/recent_files.hpp` (nuovo)
- `app/src/recent_files.cpp` (nuovo)
- `app/CMakeLists.txt`
- `tests/app/file_menu_test.cpp` (nuovo)
- `tests/app/recent_files_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R17)

## Cosa fare

L'interfaccia porta i comandi dei file (P15, P16) a chi usa il mouse. Tutto
passa dalla riga di comando: il menu scrive il comando, la finestra di
dialogo scrive la risposta. Così il comportamento è uno solo, quello che la
modalità di controllo verifica.

### Accesso ai file

Fuori da `--script`, `MainWindow` dà al runner un `io::DiskFiles` (P4).

### Menu File

Nell'ordine, con nomi d'oggetto per i test:

| Voce | Scorciatoia | Nome | Scrive |
|---|---|---|---|
| New | `QKeySequence::New` (`Ctrl+N`) | `newAction` | `NEW` |
| Open… | `QKeySequence::Open` (`Ctrl+O`) | `openAction` | `OPEN` |
| Save | `QKeySequence::Save` (`Ctrl+S`) | `saveAction` | `QSAVE` |
| Save As… | `QKeySequence::SaveAs` (`Ctrl+Shift+S`) | `saveAsAction` | `SAVEAS` |
| Recent Files ▸ | | `recentMenu` | `OPEN` e il percorso |
| Quit | come oggi | | |

Le voci si disattivano mentre un altro comando è in corso, come quelle del
menu Modifica.

### Finestre di dialogo

Quando il runner fa una domanda con `FilePrompt::Open` o `FilePrompt::Save` e
**non** si è in `--script`, la finestra apre `QFileDialog` (filtri
`Tecnigrafo drawing (*.cadx)` e `DXF (*.dxf)`, cartella del file corrente o
l'ultima usata):

- un file scelto: il suo percorso assoluto si scrive come risposta;
- annullata: `Esc` al comando.

La domanda di sostituzione di un file esistente la fa già `QFileDialog`: la
risposta `Yes` alla domanda del comando si invia da sola. In `--script` non
si apre mai una finestra: le domande restano sulla riga di comando.

### Titolo

`<nome del file>[*] — Tecnigrafo`, con `setWindowModified(editor.isModified())`
aggiornato a ogni modifica.

### Chiusura

`closeEvent` con modifiche non salvate: `QMessageBox` con `Save`, `Discard`,
`Cancel` e il testo `Save changes to <nome>?`. `Save` esegue `QSAVE` (con la
finestra di dialogo se serve); se dopo il disegno è ancora modificato, la
chiusura si annulla. In `--script` la chiusura non chiede niente.

### File recenti (`recent_files.hpp/.cpp`)

```cpp
/// The last nine drawings opened or saved, most recent first, kept in
/// QSettings under "recentFiles" (docs/adr/0021, point 5).
class RecentFiles {
public:
    explicit RecentFiles(QSettings& settings);
    void add(const QString& absolutePath);   ///< moves it first, keeps 9
    void remove(const QString& absolutePath);
    [[nodiscard]] QStringList files() const;
};
```

- Si aggiunge un file dopo ogni `OPEN`, `QSAVE`, `SAVE`, `SAVEAS` riuscito
  (il runner lo dice con il cambio del nome dell'editor e `isModified()`
  falso).
- Una voce il cui file non esiste più: messaggio sulla riga di comando e
  voce tolta.
- In `--script` l'elenco non si legge e non si scrive (impostazioni non
  toccate).

## Regole della tappa che valgono qui

- Nessuna logica dei file nell'interfaccia: lettura e scrittura restano nei
  comandi e in `io`.
- Testi dell'interfaccia in inglese con `tr()`.

## Test richiesti

- Ogni voce del menu e ogni scorciatoia scrive il suo comando.
- Con una finestra di dialogo sostituita da una di prova (per esempio un
  oggetto che restituisce un percorso dato): apertura e salvataggio veri in
  una cartella temporanea; annullamento.
- Titolo con e senza `*`.
- Chiusura con modifiche: `Save`, `Discard`, `Cancel`.
- `RecentFiles`: ordine, limite di 9, duplicati, rimozione, impostazioni
  separate per i test.

## Fatto quando

- I casi di `[0.5-R17]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- I casi del livello 2 delle tappe precedenti passano ancora.
- La CI è verde sui tre sistemi.
