# [0.6-P17] `app`: temi, pannelli agganciabili e impostazioni salvate

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #250
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: interfaccia non pilotabile dagli script; test
  Qt e prova manuale (decisione 5 del piano)
- **Dipende da:** P15, P16
- **Modulo:** `app`
- **Da leggere:** [ADR 0009](../../adr/0009-interfaccia-grafica.md) punti 3 e
  5, [ADR 0011](../../adr/0011-modalita-di-controllo.md) punto 5 (tema chiaro
  in modalità di controllo), [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md)
  punto 6; `app/src/script_session.cpp` (`lightSceneStyle`),
  `app/src/main_window.cpp`, `app/src/cad_view.cpp`, `app/src/ui_builder.hpp`
  (P16)

## File

- `app/src/theme.hpp`, `app/src/theme.cpp` (nuovi)
- `app/src/settings_dialog.hpp`, `app/src/settings_dialog.cpp` (nuovi)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/src/script_session.hpp`, `app/src/script_session.cpp` (usa il tema
  chiaro di `theme.cpp`)
- `app/src/cad_view.cpp` (`devicePixelRatio` nello stile)
- `app/src/builtin_actions.cpp` (azioni `tools.settings`, `ui.theme`,
  `ui.interface`, `aids.lwdisplay`)
- `app/CMakeLists.txt`
- `tests/app/theme_test.cpp`, `tests/app/settings_test.cpp` (nuovi)
- `tests/CMakeLists.txt`

## Cosa fare

### Temi

```cpp
enum class Theme : std::uint8_t { Light, Dark };

/// Fusion style with our palette, the same on the three systems.
void applyTheme(QApplication& application, Theme theme);
/// The drawing area of each theme; Light is the control mode's.
[[nodiscard]] render::SceneStyle sceneStyle(Theme theme);
/// An icon from the resources, drawn in the palette's text colour
/// (docs/adr/0009, point 4). Empty icon if the name is unknown.
[[nodiscard]] QIcon themedIcon(const QString& name, const QPalette& palette);
```

- `sceneStyle(Light)` è l'attuale `lightSceneStyle` (che sparisce da
  `script_session.cpp`); `sceneStyle(Dark)` i colori attuali di `SceneStyle`.
  `entity` è nero sul chiaro e bianco sullo scuro (colore 7, ADR 0024).
- Il cambio di tema vale subito: tavolozza, vista, icone.
- `themedIcon` disegna l'SVG in una pixmap e la riempie con il colore del
  testo (`CompositionMode_SourceIn`). Le icone arrivano con P18–P19; qui basta
  che una icona di prova funzioni.

### Pannelli

- `MainWindow::addPanel(QDockWidget* panel, Qt::DockWidgetArea area)`: il
  pannello si aggancia, si raggruppa a schede, si stacca; il suo
  `objectName` è stabile (serve a `saveState`).
- Nessun pannello reale qui: arrivano con P22 e P24.

### Impostazioni salvate (`QSettings`)

- Chiavi: `ui/interface` (`ribbon`/`classic`), `ui/theme` (`light`/`dark`),
  `ui/geometry`, `ui/state` (`saveGeometry`, `saveState`). Si scrivono
  all'uscita e alla scelta; si leggono all'avvio.
- Finestra **Impostazioni** (`tools.settings`): interfaccia e tema.
- In modalità di controllo (`--script`) le impostazioni non si leggono né si
  scrivono: tema chiaro, ribbon, disposizione predefinita.

### `LWT`

Pulsante `LWT` (`lwdisplayButton`) nella barra di stato, legato all'azione
`aids.lwdisplay`, che chiama `Editor::setLwdisplay` e ridisegna.

## Regole della tappa che valgono qui

- `main_window.*`, `builtin_actions.cpp`, `app/CMakeLists.txt`: solo le tue
  righe.

## Test richiesti

- `applyTheme` e `sceneStyle` per i due temi; colore 7 coerente.
- `themedIcon` con un SVG di prova: colore uguale al testo della tavolozza.
- Riavvio simulato (nuova finestra con le stesse `QSettings` in una cartella
  temporanea): interfaccia, tema e disposizione di un pannello di prova
  ritrovati.
- `--script` non tocca le impostazioni.
- `LWT` cambia `lwdisplay()`.

## Fatto quando

- I test qui sopra esistono e passano; i casi di livello 2 esistenti passano.
- La CI è verde.
