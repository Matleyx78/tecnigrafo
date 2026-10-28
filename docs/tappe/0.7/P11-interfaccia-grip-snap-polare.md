# [0.7-P11] `app` e `render`: grip disegnati, azioni dei nuovi snap e del polare

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nell'interfaccia, sopra
  comportamenti già verificati da R7–R10; painter di prova (come 0.6-P15), test Qt
  e prova manuale
- **Dipende da:** P10
- **Modulo:** `app` (più il disegno dei grip in `render`)
- **Da leggere:** [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md)
  punti 1, 6, 9 e 12; [ADR 0034](../../adr/0034-grip.md) punto 1;
  [ADR 0009](../../adr/0009-interfaccia-grafica.md); `docs/ICONE.md`;
  `render/include/tg/render/scene_renderer.hpp`; `app/src/cad_view.cpp`,
  `app/src/main_window.cpp`, `app/src/builtin_actions.cpp`

## File

- `render/include/tg/render/scene_renderer.hpp`, `render/src/scene_renderer.cpp`
- `app/src/cad_view.cpp`, `app/src/main_window.cpp`, `app/src/main_window.hpp`,
  `app/src/builtin_actions.cpp`
- `resources/icons/aids-snap-intersection.svg`,
  `aids-snap-perpendicular.svg`, `aids-snap-tangent.svg`,
  `aids-snap-quadrant.svg`, `aids-polar.svg` (nuovi), e il file delle risorse se
  le elenca
- `tests/render/scene_renderer_test.cpp`, `tests/app/drawing_aids_test.cpp`

## Cosa fare

**`render`:**

```cpp
struct SceneStyle {
    // ...
    Color grip{0, 0, 255};     ///< like AutoCAD GRIPCOLOR
    Color hotGrip{255, 0, 0};  ///< like AutoCAD GRIPHOT
    double gripSizePx{10.0};   ///< cmd::CommandRunner::kGripSizePixels
};
/// Draws filled grip squares in screen space; the hot one in hotGrip.
void renderGrips(std::span<const core::Vec2> grips, std::optional<core::Vec2> hot,
                 const Viewport& viewport, const SceneStyle& style, Painter& painter);
```

**`app`:**

- la vista disegna `runner.grips()` e `runner.hotGrip()` sopra la selezione;
- le azioni `aids.snap.intersection`, `aids.snap.perpendicular`,
  `aids.snap.tangent` e `aids.snap.quadrant` tra i modi dello snap, con le loro
  icone e i valori predefiniti dell'ADR 0033, punto 6;
- l'azione `aids.polar` sulla barra di stato e con `F10`:
  - scrive `<Polar on>` e `<Polar off>` come l'orto;
  - accenderla spegne l'orto, e accendere l'orto la spegne;
- mentre il polare aggancia, la vista disegna la semiretta tratteggiata dal punto
  base e il suggerimento `Polar: <distanza> < <angolo>°`, con i numeri formattati
  come nella riga di comando.

Il runner deve dire quando il polare aggancia: se serve, aggiungi in
`command_runner.hpp` una funzione costante che lo restituisca, e scrivilo nella
pull request.

## Regole della tappa che valgono qui

- Icone nello stile di `docs/ICONE.md`, leggibili nei due temi; ogni azione ha
  la sua icona (il test delle icone lo controlla).
- `scene_renderer.*`: prima di te P7. `main_window.*`, `builtin_actions.cpp`,
  `cad_view.cpp`: dopo di te P12.

## Test richiesti

- Painter di prova: grip e grip caldo, nella posizione e della misura giusta.
- Test Qt: le quattro azioni degli snap cambiano i modi del runner; `F10` accende
  il polare e spegne l'orto, e viceversa con `F8`; i messaggi nella cronologia.

## Fatto quando

- I test qui sopra esistono e passano; il test delle icone passa.
- La CI è verde.
