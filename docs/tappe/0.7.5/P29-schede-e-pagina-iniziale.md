# [0.7.5-P29] `app`: documenti in schede e pagina iniziale

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: i comandi `NEW` e `OPEN` non cambiano (sotto,
  punto 3), quindi i casi di verifica restano invariati; test Qt e prova manuale
- **Dipende da:** P28
- **Modulo:** `app`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 2, 3
  e 9; [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 1
  (area 4) e punto 5; [ADR 0021](../../adr/0021-comandi-dei-file.md);
  `app/src/main_window.hpp` e `.cpp` dopo P28 (sessioni, `activate`, chiusura),
  `app/src/recent_files.hpp`, `app/src/builtin_actions.cpp` (le azioni dei file)

## File

- `app/src/document_tabs.hpp`, `app/src/document_tabs.cpp` (nuovi: la riga delle
  schede)
- `app/src/start_page.hpp`, `app/src/start_page.cpp` (nuovi)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/builtin_actions.cpp` (le azioni nuove del punto 3)
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
   le azioni che chiedono un documento sono disattivate. All'avvio la scheda
   attiva è il disegno, come oggi; la pagina si raggiunge con un clic.
3. **Documenti nuovi e aperti in una scheda nuova** dal pulsante `+`, dalla
   pagina *Start* e da due azioni nuove, `file.newtab` (*New Tab*) e
   `file.opentab` (*Open in New Tab…*), nel menu *File* e nella scheda *File*. Un
   documento nuovo si chiama `Drawing<N>.cadx`, con il primo `N` non usato da
   un'altra scheda. Aprire un file già aperto in una scheda attiva quella.
   I comandi `NEW` e `OPEN`, scritti o dalle azioni di oggi (`file.new`,
   `file.open`), **restano come oggi**: lavorano sul documento della scheda
   attiva, con la richiesta di salvare se è modificato.
4. **Chiudere una scheda** con la crocetta: se il documento è modificato, la
   richiesta di salvare di oggi (*Yes*, *No*, *Cancel*); *Cancel* lascia la scheda
   aperta. Chiusa l'ultima scheda di documento, resta attiva la pagina *Start*.
   Chiudere la finestra chiede, scheda per scheda, per ogni documento modificato;
   un *Cancel* ferma la chiusura.
5. Il titolo della finestra è il nome del documento attivo, o `Tecnigrafo` con la
   pagina *Start*.

## Regole della tappa che valgono qui

- `main_window.*`: prima di te P28, dopo di te P30. `builtin_actions.cpp`: prima
  di te P20, dopo di te P30.
- Interfaccia in inglese.

## Test richiesti

`document_tabs_test.cpp` (test Qt):

- due documenti nuovi con `+` si chiamano `Drawing1.cadx` e `Drawing2.cadx`;
  cliccare la scheda attiva la sessione;
- una scheda modificata ha `*`; chiuderla chiede di salvare, *Cancel* la lascia
  aperta, *No* la chiude; chiusa l'ultima, è attiva la pagina *Start*;
- aprire due volte lo stesso file attiva la stessa scheda;
- `NEW` scritto nella riga di comando lavora sulla scheda attiva e non apre una
  scheda nuova.

## Fatto quando

- Si lavora su più documenti in schede, con la pagina *Start*.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
