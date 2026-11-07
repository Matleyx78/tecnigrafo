# [0.7.5-P3] CI: galleria di schermate dell'interfaccia

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: è uno strumento per il responsabile, non cambia
  il comportamento; test Qt
- **Dipende da:** P1
- **Modulo:** `app` (un programma di prova) e `.github/`
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punti 1 e 7; [ADR 0009](../../adr/0009-interfaccia-grafica.md), punti 2, 3 e 5;
  `app/src/main_window.hpp`, `app/src/theme.hpp`, `app/src/sample_drawing.hpp`,
  `app/CMakeLists.txt`; in `.github/workflows/ci.yml` il lavoro «Linux (GCC,
  clang-tidy)» e i passi che caricano gli artefatti

## Perché

La CI non misura la bellezza. Perché il responsabile confronti l'aspetto con le
immagini di riferimento dell'ADR 0039 senza compilare, ogni pull request che
tocca l'interfaccia produce le stesse schermate, nei due temi.

## File

- `app/tool/gallery.cpp` (nuovo): il programma `tecnigrafo-galleria`
- `app/CMakeLists.txt` (il bersaglio nuovo)
- `tests/app/gallery_test.cpp` (nuovo), `tests/CMakeLists.txt`
- `.github/workflows/ci.yml`
- `docs/GUIDA-RESPONSABILE.md`, A3 (come si guarda la galleria prima di unire)

## Cosa fare

1. `tecnigrafo-galleria <cartella>` apre `MainWindow` con il disegno di esempio,
   a 1600×1000 pixel logici e scala 1, e salva in PNG, per ciascuno dei due temi:
   - `finestra-<tema>.png`: la finestra intera con la ribbon;
   - `finestra-classica-<tema>.png`: la finestra con l'interfaccia classica;
   - `ribbon-<tema>.png`, `proprieta-<tema>.png`, `layer-<tema>.png`,
     `stato-<tema>.png`: il ribbon, i due pannelli (con una polilinea
     selezionata, perché il pannello *Properties* mostri la geometria) e la
     barra di stato, ritagliati dal loro widget.
   I nomi dei file non cambiano tra le pull request, così si confrontano. Le
   schermate non dipendono da impostazioni salvate: il programma usa una
   cartella di impostazioni vuota e temporanea.
2. Il programma si esegue con `QT_QPA_PLATFORM=offscreen`; il font è quello del
   programma, non quello del sistema, dove l'interfaccia lo permette.
3. In CI, nel lavoro Linux, un passo esegue il programma quando la pull
   request tocca `app/` o `resources/`, e carica la cartella come artefatto
   `galleria`. Il passo non fallisce per l'aspetto: fallisce solo se il
   programma non parte o non scrive tutte le immagini.
4. Il riepilogo del lavoro (`$GITHUB_STEP_SUMMARY`) elenca le immagini e dice
   dove scaricarle.

## Regole della tappa che valgono qui

- `ci.yml`: prima di te P1, dopo di te P4.
- `app/CMakeLists.txt`, `tests/CMakeLists.txt`: file comuni, aggiungi solo le
  tue righe, in ordine alfabetico.
- Le schermate le guarda il responsabile; nessun test confronta i pixel.

## Test richiesti

- Test Qt: il programma, eseguito in una cartella temporanea, scrive tutte le
  immagini dei due temi, non vuote e della misura attesa.

## Fatto quando

- Una pull request che tocca `app/` ha l'artefatto `galleria` con le dodici
  immagini.
- Il test qui sopra esiste e passa.
- La CI è verde.
