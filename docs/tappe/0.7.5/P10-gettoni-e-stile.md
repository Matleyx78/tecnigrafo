# [0.7.5-P10] `app`: gettoni del tema, foglio di stile e `QProxyStyle`

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: aspetto dell'interfaccia; test Qt, galleria di
  schermate e prova manuale (DECISIONI.md, 2026-10-25)
- **Dipende da:** —
- **Modulo:** `app`
- **Da leggere:** [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md),
  punti 2, 3 e 4; [ADR 0009](../../adr/0009-interfaccia-grafica.md), punto 5;
  `app/src/theme.hpp`, `app/src/theme.cpp`, `app/src/main_window.cpp`
  (`setTheme`), `render/include/tg/render/scene_renderer.hpp` (`SceneStyle`)

## File

- `app/src/theme.hpp`, `app/src/theme.cpp`
- `app/src/style.hpp`, `app/src/style.cpp` (nuovi)
- `app/src/main_window.cpp` (solo `setTheme`)
- `app/CMakeLists.txt` (i sorgenti nuovi)
- `tests/app/theme_test.cpp`, `tests/app/style_test.cpp` (nuovo),
  `tests/CMakeLists.txt`

## Cosa fare

**Gettoni** (ADR 0039, punto 2), in `theme.hpp`:

```cpp
/// The colours of the interface, one set per theme (docs/adr/0039, point 2).
struct ThemeTokens {
    QColor chrome, ribbon, group, tabs, panel, field, canvas;  ///< surfaces
    QColor border, text, textSecondary, accent, selection;
    QColor iconGeo, iconPt, iconCost, iconEsist;  ///< docs/adr/0040, point 1
};
[[nodiscard]] const ThemeTokens& themeTokens(Theme theme);
```

con i valori delle tabelle degli ADR 0039 e 0040 (`iconEsist` è `text`).

**Applicazione:**

- `applyTheme` costruisce la `QPalette` dai gettoni (non più dai colori scritti
  nella funzione), installa lo stile del punto sotto e il foglio di stile;
- `QString styleSheet(const ThemeTokens&)` in `style.hpp`, generato dai gettoni:
  superfici di ribbon, schede, pannelli e intestazioni dei pannelli, campi e
  combo, voci selezionate, righe da 20–22 px nelle viste a elenco, nessun bordo
  ai pulsanti a riposo (ADR 0039, punto 3);
- `class Style final : public QProxyStyle` in `style.hpp`, sopra Fusion, per ciò
  che il foglio di stile non controlla bene: barre di scorrimento sottili,
  schede, frecce delle combo, intestazioni dei `QDockWidget`;
- `sceneStyle` prende lo sfondo da `canvas`; gli altri colori del disegno non
  cambiano.

Nessun altro file di `app` scrive colori dell'interfaccia: quelli che ne
scrivono oggi li elenchi nella pull request, e li cambiano i pacchetti dei loro
file (P12–P14).

## Regole della tappa che valgono qui

- Il ribbon di SARibbon e la cornice non si toccano qui: li fa P12.
- `theme.cpp`: dopo di te P11. `main_window.cpp`: dopo di te P12.
- I testi dell'interfaccia restano in inglese.

## Test richiesti

- `theme_test.cpp`: per i due temi, la palette usa i gettoni (testo, fondo,
  evidenziazione); lo sfondo del disegno è `canvas`.
- `style_test.cpp`: il foglio di stile contiene i colori dei gettoni del tema
  e cambia quando cambia il tema; lo stile installato è `Style`.

## Fatto quando

- I colori dell'interfaccia vengono dai gettoni, nei due temi.
- La galleria di schermate mostra superfici, bordi e selezione dell'ADR 0039.
- I test qui sopra esistono e passano.
- La CI è verde.
