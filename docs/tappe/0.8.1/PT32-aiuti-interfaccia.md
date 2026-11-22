# [0.8.1-PT32] `app`: griglia, snap alla griglia e snap nuovi nell'interfaccia

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: test Qt e prova manuale; il comportamento è
  già verificato in `cmd` e `core`
- **Dipende da:** PT27, PT28, PT29, PT30, PT31
- **Modulo:** `app`
- **Da leggere:** [ADR 0055](../../adr/0055-griglia-snap-alla-griglia-icona-ucs.md),
  punti 3 e 6; [ADR 0057](../../adr/0057-snap-vicino-prolungamento-parallelo.md),
  punto 1; [ADR 0040](../../adr/0040-icone-a-ruoli.md);
  `app/src/main_window.cpp` (barra di stato, aiuti), `app/src/builtin_actions.cpp`,
  `app/src/settings_dialog.cpp`, `app/src/theme.cpp` (`sceneStyle`)

## File

- `app/src/main_window.cpp`, `app/src/main_window.hpp`
- `app/src/builtin_actions.cpp`
- `app/src/settings_dialog.cpp`, `app/src/settings_dialog.hpp`
- `app/src/theme.cpp`
- `resources/icons/` (`aids-grid`, `aids-snapgrid`, `aids-snap-nearest`,
  `aids-snap-extension`, `aids-snap-parallel`, a coppie 16/32)
- `tests/app/drawing_aids_test.cpp`, `tests/app/settings_dialog_test.cpp`

## Cosa fare

1. Azioni `aids.grid` (`F7`) e `aids.snapgrid` (`F9`), con gli interruttori a
   icona nella barra di stato accanto all'orto e al polare: cambiano
   `GridSettings` del documento attivo con i messaggi dell'ADR 0055, punto 3,
   con `edit::DocumentTables::setGrid()` di PT27, che lo segna modificato
   senza un passo di annullamento (verbale della revisione PT5, [bloccante]
   1); lo stato degli interruttori segue il documento quando cambia la
   scheda.
2. *Vicino*, *Prolungamento* e *Parallelo* fra gli snap della barra e delle
   Impostazioni (`aids.snap.nearest`, `.extension`, `.parallel`), spenti per
   difetto.
3. Nelle Impostazioni: la **calamita** degli snap (accesa per difetto, bit 4 di
   `AUTOSNAP`, una preferenza del programma; la usa PT33) e gli **assi**
   (icona dell'UCS, rette, niente; `AxesStyle` di PT29).
4. `sceneStyle()` passa `AxesStyle` e la misura dell'icona a `render`.

## Regole della tappa che valgono qui

- Icone a ruoli, a coppie 16/32. `builtin_actions.cpp` lo cambia dopo PT37.

## Test richiesti

- `F7` e `F9` con due documenti aperti, appena salvati: `F7` su A segna
  modificata solo la scheda di A, senza un passo di annullamento; salvato e
  riaperto, A ha la griglia com'era al salvataggio; i tre snap nuovi accesi dalla barra
  arrivano a `DrawingAids`; calamita e assi salvati nelle Impostazioni.

## Fatto quando

- I test passano; la CI è verde.
