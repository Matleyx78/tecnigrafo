# [0.8-PT2] `app`: traccia dello stack su Windows e avviso al riavvio dopo un crash

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: un crash non si scrive in uno script; test con
  un processo che va in crash e prova manuale
- **Dipende da:** —
- **Modulo:** `app`
- **Da leggere:** [#62](https://github.com/Matleyx78/tecnigrafo/issues/62)
  (commento del 2026-11-11: che cosa entra nella 0.8 e che cosa resta alla 0.9);
  [DECISIONI.md](../../DECISIONI.md), 2026-11-11, «Errori e crash»;
  `app/src/log_file.hpp`, `app/src/log_file.cpp`, `app/src/main.cpp`;
  [ADR 0010](../../adr/0010-segnalazione-errori.md) solo per sapere che cosa
  **non** fare ora (finestre di segnalazione, issue precompilata, pulizia dei
  dati personali)

## File

- `app/src/log_file.hpp`, `app/src/log_file.cpp`
- `app/src/crash_notice.hpp`, `app/src/crash_notice.cpp` (nuovi)
- `app/src/main.cpp`
- `app/CMakeLists.txt` (file comune)
- `tests/app/log_file_test.cpp`, `tests/app/crash_child.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Windows.** Il registro `tecnigrafo.log` scrive anche su Windows la traccia
   di un crash, come fa già su Linux e macOS: un filtro delle eccezioni non
   gestite (`SetUnhandledExceptionFilter`) scrive la riga d'intestazione, il
   codice dell'eccezione e l'indirizzo, poi la pila con
   `CaptureStackBackTrace`, una riga per livello nella forma
   `<modulo>+0x<scostamento>` (nome del file del modulo con
   `GetModuleHandleExW` e `GetModuleFileNameW`, scostamento dall'inizio del
   modulo), al più 64 livelli. Nessuna libreria nuova: niente `DbgHelp` né
   simboli, che sono lavoro della 0.9 con gli installatori; chi sviluppa
   risolve gli scostamenti dopo. Sotto un sanitizer, nulla, come oggi.
2. **Riga d'intestazione comune.** Su tutti e tre i sistemi la traccia comincia
   con la stessa riga, `fatal signal; the stack of the crash follows` (quella
   di oggi): è il segno che il programma cerca al riavvio. Su Windows segue
   `exception 0x<codice> at 0x<indirizzo>`.
3. **Avviso al riavvio.** `startLogFile()` restituisce anche se il registro
   della sessione prima (`tecnigrafo.log.old`, dopo la rotazione) contiene la
   riga d'intestazione:

   ```cpp
   struct LogStart {
       QString path;          ///< the log of this session, empty if it cannot be written
       QString previousPath;  ///< tecnigrafo.log.old, empty if absent
       bool previousCrashed{false};  ///< it holds the crash header line
   };
   LogStart startLogFile();
   ```

   Con `previousCrashed`, quando la finestra principale è visibile, una finestra
   di messaggio (`crash_notice.*`) dice: `Tecnigrafo closed unexpectedly last
   time. The log of that session, with the stack of the crash, is in:` seguita
   dal percorso di `tecnigrafo.log.old`, con i pulsanti *Open folder* (apre la
   cartella con `QDesktopServices`) e *OK*. Il testo passa da `tr()`.
   Niente invio di dati, niente issue precompilata (0.9).
4. Gli script (`--script`) e il server di sviluppo non mostrano l'avviso.

## Regole della tappa che valgono qui

- File comuni: `app/CMakeLists.txt`, `tests/CMakeLists.txt`.
- `app/src/main.cpp` e i file di `app` non toccano `builtin_actions.cpp`,
  `ui_builder.cpp` né `main_window.*`: l'avviso è una finestra a sé.

## Test richiesti

- `crash_child.cpp`: un piccolo eseguibile di test che avvia il registro in una
  cartella temporanea e poi va in crash (accesso a un puntatore nullo); il test
  lo lancia con `QProcess`, aspetta che finisca e controlla che il registro
  contenga la riga d'intestazione e almeno una riga della pila, su tutti e tre i
  sistemi (salvo sotto i sanitizer, dove il test si salta).
- `log_file_test.cpp`: con un `tecnigrafo.log` di prova che contiene la riga
  d'intestazione, il riavvio dà `previousCrashed` vero e `previousPath`
  giusto; senza, falso.

## Fatto quando

- Un crash su Windows scrive la traccia nel registro, e al riavvio compare
  l'avviso con il percorso; su Linux e macOS lo stesso avviso.
- I test qui sopra esistono e passano; la CI è verde.
