# [0.7.5-P29] `app`: documenti in schede e pagina iniziale

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: `NEW` e `OPEN` aprono già i documenti nuovi
  (P28b, con i suoi riferimenti); qui si vedono e si chiudono in schede; test Qt
  e prova manuale
- **Dipende da:** P28b
- **Corretta:** 2026-11-07, con le decisioni del responsabile su B4 e B5 del
  verbale della revisione delle interfacce (P23,
  [#563](https://github.com/Matleyx78/tecnigrafo/issues/563);
  [DECISIONI.md](../../DECISIONI.md)): la scheda attiva dopo una chiusura e la
  chiusura della finestra; con B7, il flag `requiresDocument` delle azioni
- **Modulo:** `app`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 2, 3
  e 9; [DECISIONI.md](../../DECISIONI.md), 2026-11-07, «`NEW` e `OPEN` con i
  documenti in schede»; la scheda [P28b](P28b-new-e-open.md); [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 1
  (area 4) e punto 5; [ADR 0021](../../adr/0021-comandi-dei-file.md);
  `app/src/main_window.hpp` e `.cpp` dopo P28 (sessioni, `activate`, chiusura),
  `app/src/recent_files.hpp`, `app/src/builtin_actions.cpp` (le azioni dei file)

## File

- `app/src/document_tabs.hpp`, `app/src/document_tabs.cpp` (nuovi: la riga delle
  schede)
- `app/src/start_page.hpp`, `app/src/start_page.cpp` (nuovi)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`, `app/src/main.cpp`
  (l'avvio sulla pagina *Start*)
- `app/src/action_registry.hpp` (il flag `requiresDocument`, punto 2)
- `app/src/builtin_actions.cpp` (le azioni dei file, se il punto 3 le tocca, e
  `requiresDocument`)
- `resources/icons/`: le icone a ruoli che servono (scheda nuova, menu dei
  documenti, tipo disegno), 16 e 32 px
- `app/CMakeLists.txt`
- `tests/app/document_tabs_test.cpp` (nuovo), `tests/app/file_menu_test.cpp`,
  `tests/CMakeLists.txt`

## Cosa fare

1. **Riga delle schede** sotto il ribbon (ADR 0039, area 4): il pulsante del menu
   dei documenti aperti, la scheda *Start*, una scheda per sessione con l'icona del
   tipo, il nome, l'indicatore di modifica (`*`) e la crocetta, e un pulsante `+`.
   Cliccare una scheda la attiva (`activate` di P28); la scheda attiva è
   evidenziata. Con l'interfaccia classica la riga c'è lo stesso.
2. **Pagina *Start*** (ADR 0041, punto 9): sempre la prima scheda, non è un
   documento e non ha sessione. Mostra *New* (un pulsante per ogni tipo di
   documento: oggi il disegno), *Open…* e l'elenco dei file recenti. Con la
   pagina attiva, vista, riga di comando e pannelli sono senza sessione (P28) e
   le azioni che chiedono un documento sono disattivate, scorciatoie comprese.
   Lo dice un flag nuovo di `ActionSpec` (decisione B7), separato dagli
   ambienti:

   ```cpp
   bool requiresDocument{true};  ///< disabled while no document is active (the Start page)
   ```

   È `false` per `file.new`, `file.open`, `file.recent` (e le voci dei
   recenti), `file.quit`, `tools.settings`, `tools.shortcuts`, `ui.interface`,
   `ui.theme` e `help.about`; `true` per tutte le altre, comprese `edit.undo` e
   `edit.redo`. Gli ambienti delle azioni li tratta P30.
   **Il programma si avvia sulla pagina *Start***, senza documenti aperti, come
   AutoCAD (DECISIONI.md, 2026-11-07): l'utente crea il documento che gli serve,
   oggi un disegno, domani anche gli altri tipi del registro (P30). Il disegno di
   esempio non si apre più all'avvio: `MainWindow::InitialDrawing` ha un valore
   nuovo, `None` (la pagina *Start*), predefinito per il programma; `Sample`
   resta per la galleria di schermate (P3) e per i test, `Empty` per gli script
   di livello 2, che partono come oggi da un disegno vuoto.
3. **Documenti nuovi e aperti:** il pulsante `+` e la pagina *Start* eseguono
   `NEW` e `OPEN`, come le azioni `file.new` e `file.open` e i comandi scritti;
   ogni documento che P28b aggiunge ha la sua scheda, nell'ordine di apertura, e
   la scheda attiva segue il documento attivo.
4. **Chiudere una scheda** con la crocetta: un comando in corso si chiude prima
   come con `Esc`, come fa oggi la chiusura della finestra; poi, se il documento
   è modificato, la richiesta di salvare di oggi (*Yes*, *No*, *Cancel*).
   *Cancel*, o un salvataggio annullato o non riuscito, lascia la scheda aperta.
   - **Quale scheda diventa attiva** (decisione B4): chiusa la scheda attiva,
     diventa attiva la scheda di documento alla sua **destra**; se non c'è,
     quella alla sua **sinistra**; se non resta nessun documento, la pagina
     *Start*. Chiudere una scheda non attiva non cambia l'attiva.
   - **Chiudere la finestra** (decisione B5; ADR 0041, punto 3) chiude i
     documenti **uno per uno, nell'ordine delle schede, come la crocetta di
     ciascuno**: un documento non modificato si chiude, uno modificato si chiude
     appena la sua domanda ha risposta (*No*, o *Yes* con il salvataggio
     riuscito). Un *Cancel*, o un salvataggio annullato o non riuscito, ferma la
     chiusura: quel documento e i seguenti restano aperti, quelli già chiusi
     restano chiusi, e la finestra resta aperta. L'attiva, dopo ogni chiusura,
     segue la regola di B4.
5. Il titolo della finestra è il nome del documento attivo, o `Tecnigrafo` con la
   pagina *Start*.
6. Con la pagina *Start* non c'è una sessione attiva: i chiamanti interni di
   `MainWindow` usano `activeSession()` e controllano `nullptr` (P28), e
   `document()`, `editor()` e `runner()` non si chiamano.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P28b, dopo di te P30. `builtin_actions.cpp`: prima
  di te P20, dopo di te P30.
- Interfaccia in inglese.

## Test richiesti

`document_tabs_test.cpp` (test Qt):

- il programma si avvia sulla pagina *Start* senza documenti; due `+` danno
  `Drawing1.cadx` e `Drawing2.cadx`; cliccare una scheda attiva la sua sessione;
- (B2, con P28) in A `LINE` aspetta il secondo punto; scrivo `10,20` nella riga
  di comando senza `Invio`, attivo la pagina *Start* e torno ad A: il campo ha
  `10,20`, e `Invio` dà il punto a `LINE` di A. Lo stesso con un valore in un
  campo dell'immissione dinamica;
- (B7) con la pagina *Start*, `Ctrl+N` apre un disegno e `Ctrl+O` il dialogo;
  *Undo*, *Save* e `Ctrl+Z` sono disattivati; con un documento attivo tornano;
- una finestra con `InitialDrawing::Empty` (gli script) parte con un disegno
  vuoto, attivo, come oggi;
- una scheda modificata ha `*`; chiuderla chiede di salvare, *Cancel* la lascia
  aperta, *No* la chiude; chiusa l'ultima, è attiva la pagina *Start*;
- (B4) schede A, B, C con B attiva: chiusa B, è attiva C; poi, chiusa C, è
  attiva A; chiusa A, è attiva *Start*. Con A, B, C e A attiva, chiudere C lascia
  attiva A;
- (B5) schede A (modificato), B (non modificato), C (modificato), con A attiva:
  chiudo la finestra e rispondo *No* per A e *Cancel* per C; A e B sono
  chiuse, C è aperta e attiva, modificata, e la finestra è aperta. Con *No* per
  A e per C la finestra si chiude. Con *Yes* per A e il salvataggio annullato
  nel dialogo, nessun documento si chiude e la finestra resta aperta;
- aprire due volte lo stesso file attiva la stessa scheda;
- `NEW` scritto nella riga di comando apre una scheda nuova; `OPEN` all'avvio,
  con il disegno appena creato, ne apre un'altra e il disegno resta.

## Fatto quando

- Si lavora su più documenti in schede, con la pagina *Start*.
- I test qui sopra esistono e passano.
- La CI è verde.
