# [0.6-P3] Prova di SARibbon

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #229
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: infrastruttura
- **Dipende da:** —
- **Modulo:** `third_party` (più la libreria collegata da `app`)
- **Da leggere:** [ADR 0009](../../adr/0009-interfaccia-grafica.md) punto 2,
  [ADR 0019](../../adr/0019-archivi-zip.md) (come si è copiata zlib), il
  piano della tappa, sezione "SARibbon"; `third_party/CMakeLists.txt`,
  `third_party/zlib/README.md`, `app/CMakeLists.txt`

## File

- `third_party/saribbon/` (nuovo): i sorgenti di SARibbon, `LICENSE`,
  `README.md` con versione, indirizzo, hash SHA-256 dell'archivio e licenza.
  I file copiati non contano nel limite dei quindici
- `third_party/CMakeLists.txt` (bersaglio `tg_saribbon`, statico)
- `app/CMakeLists.txt` (collegamento a `tg_app_ui`)
- `tests/app/ribbon_smoke_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `.github/workflows/ci.yml` solo se serve (lo approva il responsabile)

## Cosa fare

1. Scegli l'**ultima versione rilasciata** di SARibbon
   (<https://github.com/czyt1988/SARibbon>, MIT) e scarica l'archivio del suo
   tag. Se offre i due file riuniti (`SARibbon.h`, `SARibbon.cpp`) copia quelli
   e le risorse che servono; altrimenti copia la libreria `SARibbonBar` e
   compilala con un bersaglio scritto da noi, senza usare il suo CMake.
2. Bersaglio `tg_saribbon` (statico, Qt6::Widgets) come `tg_zlib`: avvisi del
   codice copiato non trattati come errori e fuori da `clang-tidy`;
   intestazioni incluse come `SYSTEM`, così non generano avvisi nel nostro
   codice.
3. `tg_app_ui` lo collega; nessun file di `app/src` lo usa ancora (lo farà
   P16).
4. `ribbon_smoke_test.cpp`: con la piattaforma `offscreen`, crea una
   `SARibbonMainWindow` (o una `SARibbonBar` in una `QMainWindow`) con una
   scheda, un gruppo e un pulsante legato a un `QAction`; mostra la finestra;
   verifica che il pulsante esista e che premerlo attivi l'azione.

Se non compila senza avvisi nel nostro codice con Qt 6.4 su Linux o con 6.8
su Windows e macOS, **fermati** e scrivilo nella issue: serve un ADR che
cambi libreria o versione minima (ADR 0009, punto 2).

## Regole della tappa che valgono qui

- È l'unica dipendenza nuova di `app` (ADR 0009). Nessun download durante la
  configurazione di CMake.
- `third_party/CMakeLists.txt` e `CMakeLists.txt` li tocca anche P25, dopo.
- `app/CMakeLists.txt` lo toccano poi P7 e i pacchetti di `app`.

## Test richiesti

- Il test di prova qui sopra, su Linux, Windows e macOS.

## Fatto quando

- SARibbon è in `third_party/saribbon/` con versione, hash e licenza.
- Il test passa sui tre sistemi e la build resta senza avvisi.
- La CI è verde.
