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
- `app/src/builtin_actions.cpp` (le azioni dei file, se il punto 3 le tocca)
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
   le azioni che chiedono un documento sono disattivate.
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
4. **Chiudere una scheda** con la crocetta: se il documento è modificato, la
   richiesta di salvare di oggi (*Yes*, *No*, *Cancel*); *Cancel* lascia la scheda
   aperta. Chiusa l'ultima scheda di documento, resta attiva la pagina *Start*.
   Chiudere la finestra chiede, scheda per scheda, per ogni documento modificato;
   un *Cancel* ferma la chiusura.
5. Il titolo della finestra è il nome del documento attivo, o `Tecnigrafo` con la
   pagina *Start*.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P28b, dopo di te P30. `builtin_actions.cpp`: prima
  di te P20, dopo di te P30.
- Interfaccia in inglese.

## Test richiesti

`document_tabs_test.cpp` (test Qt):

- il programma si avvia sulla pagina *Start* senza documenti; due `+` danno
  `Drawing1.cadx` e `Drawing2.cadx`; cliccare una scheda attiva la sua sessione;
- una finestra con `InitialDrawing::Empty` (gli script) parte con un disegno
  vuoto, attivo, come oggi;
- una scheda modificata ha `*`; chiuderla chiede di salvare, *Cancel* la lascia
  aperta, *No* la chiude; chiusa l'ultima, è attiva la pagina *Start*;
- aprire due volte lo stesso file attiva la stessa scheda;
- `NEW` scritto nella riga di comando apre una scheda nuova; `OPEN` all'avvio,
  con il disegno appena creato, ne apre un'altra e il disegno resta.

## Fatto quando

- Si lavora su più documenti in schede, con la pagina *Start*.
- I test qui sopra esistono e passano.
- La CI è verde.
