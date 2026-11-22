# [0.8.1-PT12] `app`: editor del testo nella finestra e doppio clic

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: è interazione nella finestra; test Qt e prova
  manuale
- **Dipende da:** PT9, PT10, PT11
- **Modulo:** `app`
- **Da leggere:** [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md),
  punto 5; [ADR 0042](../../adr/0042-immissione-dinamica.md) (come la vista
  ospita un campo sopra il disegno); `app/src/cad_view.cpp`,
  `app/src/dynamic_input_box.cpp`; le schede [PT10](PT10-modello-di-modifica.md)
  e [PT11](PT11-textedit.md)

## File

- `app/src/text_editor_overlay.hpp`, `app/src/text_editor_overlay.cpp` (nuovi)
- `app/src/cad_view.hpp`, `app/src/cad_view.cpp` (l'editor sopra la vista; il
  doppio clic con il tasto sinistro)
- `app/CMakeLists.txt` (file comune)
- `tests/app/text_editor_overlay_test.cpp` (nuovo), `tests/CMakeLists.txt`
  (file comune)

## Cosa fare

1. Quando il runner dà `textEditing()` (PT11) con un `MText` in `shown`, la
   vista apre l'editor su quel testo, con `value` come valore da modificare
   (per una quota il testo sostitutivo con `<>`, non la misura che si vede):
   disegna il testo con l'impaginazione di `core` (le
   stesse posizioni del disegno, in scala e ruotate con la vista e con il testo),
   il cursore e la scelta dalle posizioni di `MTextEditModel` (PT10), un bordo
   largo quanto la colonna. Tastiera: caratteri, frecce, `Home`/`End`,
   `Backspace`, `Canc`, `Shift` per scegliere, `Invio` nuovo paragrafo,
   `Ctrl+Invio` conferma, `Esc` annulla; il mouse sposta il cursore e sceglie.
   Un clic fuori dal bordo conferma.
2. Confermare consegna al runner il valore con i codici, su una riga, come la
   riga di comando (con una riga vuota dopo, per `MTEXT` nuovo); su un oggetto
   esistente lo consegna con `CommandRunner::submit(TextInput{…})`, **anche
   vuoto**: come AutoCAD, un `TEXT` o un `MTEXT` svuotato e confermato torna
   com'era, e una quota svuotata torna alla misura (prova in AutoCAD del
   responsabile, 2026-11-22; PT11). Se il valore alla conferma è **uguale** a
   quello di partenza (nessuna modifica, o modifiche che lo riportano
   com'era), l'editor consegna `ConfirmInput`, come Invio, che tiene il valore
   (verbale della rilettura della #993, [bloccante] 1). **Svuotato** è un
   valore senza niente di visibile, anche con codici nascosti rimasti
   (`MTextEditModel::isEmpty()` di PT10; decisione del responsabile sulla
   rilettura della #994): su un `TEXT` o un `MTEXT` l'editor consegna allora
   `ConfirmInput`, che lo lascia com'era con i suoi codici; su una quota
   consegna `TextInput{""}`, che torna alla misura, e i codici rimasti, che
   non hanno effetto, si perdono. Annullare dà `Esc`.
3. Su un `TEXT` (`shown` è un `Text`, con la sua posizione e il suo stile),
   l'editor è una riga sola sul testo, senza formati; `Invio` conferma.
4. **Doppio clic** con il tasto sinistro su `TEXT`, `MTEXT`, quota, `ATTDEF` o
   `INSERT` con attributi, a `Command:`: sceglie l'oggetto e avvia `TEXTEDIT`.
   Il doppio clic con il tasto centrale resta lo zoom di oggi.
5. Il formato lo aggiunge la barra di [PT13](PT13-barra-dei-formati.md):
   l'editor offre `applyFormat(...)` e il segnale della scelta.

## Perché resta impegnativa

L'editor non ha un modello in Qt da riusare: il testo ruotato e in scala, il
cursore sulle posizioni di `core` e le interazioni con la vista (zoom e
spostamento mentre si scrive, una richiesta che cambia, un altro documento)
mettono casi limite che nessun documento fissa e che emergono scrivendo.

## Regole della tappa che valgono qui

- File comuni: `app/CMakeLists.txt`, `tests/CMakeLists.txt`.
- Dopo PT12 `cad_view.cpp` lo cambia anche [PT33](PT33-segni-nella-vista.md),
  che dipende da questo pacchetto.

## Test richiesti

- Scrivere, cancellare, andare a capo e confermare dà il valore atteso; `Esc`
  non cambia il disegno; doppio clic su un `TEXT` avvia `TEXTEDIT` con
  l'oggetto scelto; zoom con l'editor aperto lo lascia sopra il testo.
- Una quota con `<> mm`: l'editor riceve `<> mm`, mostra la misura come nel
  disegno (un'unità sola, PT10, con il testo di `measurement`) e, confermato
  senza modifiche, ridà `<> mm`, e svuotato torna alla misura; cancellare tutto
  e confermare su un `MTEXT` lo lascia com'era, anche con `A\A1;B` di cui si
  cancellano le due lettere (resterebbe `\A1;`); una quota con `<>\A1; mm`
  svuotata torna alla misura (testo sostitutivo `""`); un `MTEXT` già vuoto
  aperto e confermato senza modifiche consegna `ConfirmInput` e resta; una
  frazione si salta e si cancella intera (PT10).

## Fatto quando

- I test passano; la CI è verde; la prova manuale (sezione 7 del piano) lo
  conferma.
