# [0.6-P16] `app`: ribbon e interfaccia classica

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #249
- **Difficoltà:** impegnativa
- **Agente:** codex
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.6-R16]` (#248), agente gemini
- **Dipende da:** P3, P7, R16
- **Modulo:** `app`
- **Da leggere:** [ADR 0009](../../adr/0009-interfaccia-grafica.md) punti 1 e
  2, [ADR 0026](../../adr/0026-descrizione-dei-comandi.md) punto 2;
  `app/src/action_registry.hpp` (P7), `app/src/builtin_actions.cpp`,
  `app/src/main_window.cpp`, `app/src/script_session.cpp`;
  `third_party/saribbon/README.md` (P3) e `tests/app/ribbon_smoke_test.cpp`

## File

- `app/src/ui_builder.hpp`, `app/src/ui_builder.cpp` (nuovi: ribbon e barre
  dal registro)
- `app/src/app_commands.hpp`, `app/src/app_commands.cpp` (nuovi: i comandi
  dell'applicazione)
- `app/src/action_registry.hpp`, `app/src/action_registry.cpp`
- `app/src/builtin_actions.cpp`
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/CMakeLists.txt`
- `tests/app/ui_builder_test.cpp` (nuovo)
- `tests/CMakeLists.txt`
- `tests/verifica/in-attesa.txt` (togliere le righe di R16)

## Cosa fare

### Due interfacce dallo stesso registro

```cpp
enum class InterfaceKind : std::uint8_t { Ribbon, Classic };

/// Builds and tears down the ribbon, or the menu bar and toolbars, from the
/// registry (docs/adr/0009, point 2). Switching keeps the same QAction
/// objects, so state (checked, enabled) and shortcuts never change.
class UiBuilder {
public:
    UiBuilder(QMainWindow& window, const ActionRegistry& registry);
    void show(InterfaceKind kind);
    [[nodiscard]] InterfaceKind current() const;
};
```

- **Ribbon** (predefinita): schede e gruppi da `ActionSpec::ribbon`
  (`home/draw`, `home/modify`, `home/layers`, `home/properties`,
  `view/navigate`, `view/aids`). Pulsanti grandi per i comandi di disegno,
  piccoli per gli altri. Il **menu dell'applicazione** di SARibbon contiene il
  menu File; i menu classici restano raggiungibili da un pulsante del menu
  dell'applicazione.
- **Classica:** barra dei menu (`buildMenus`) e barre degli strumenti
  `Standard` (file, annulla, ripeti), `Draw`, `Modify`, `Layers`,
  `Properties`, una per gruppo della ribbon.
- La riga di comando c'è sempre.
- Il cambio avviene subito, senza riavviare: con il pulsante della barra di
  stato `ribbonButton` (testo `RIBBON`/`MENU`), dalle impostazioni (P17) e con
  i comandi qui sotto.
- Un'azione senza `ribbon` vale solo nei menu; il test verifica che ogni
  azione sia raggiungibile **in entrambe** le interfacce (ribbon più menu
  dell'applicazione, o menu più barre).

### Comandi dell'applicazione

`app_commands.cpp` registra nel `CommandRegistry` della finestra, con la loro
descrizione (ADR 0026), i comandi che esistono solo nel programma:

| Comando | Domanda | Effetto |
|---|---|---|
| `RIBBON` | nessuna | interfaccia ribbon |
| `RIBBONCLOSE` | nessuna | interfaccia classica |

Non scrivono messaggi. Al livello 1 non esistono (`Unknown command`): i loro
casi sono di livello 2. Lo stesso file accoglierà `LAYER`, `PROPERTIES`,
`QSELECT` (P22, P24).

## Regole della tappa che valgono qui

- Nessun `QAction` fuori dal registro.
- SARibbon si usa solo qui e in `main_window.cpp`.
- `main_window.cpp/.hpp`, `builtin_actions.cpp`, `app/CMakeLists.txt`: solo
  le tue righe; li toccano poi P17–P24.

## Test richiesti

- Passaggio ribbon → classica → ribbon: stesse `QAction`, stato `checked`
  conservato, nessun widget orfano.
- Ogni azione raggiungibile in entrambe le interfacce.
- Le scorciatoie funzionano in entrambe.
- `RIBBON` e `RIBBONCLOSE` dalla riga di comando.

## Fatto quando

- I casi di `[0.6-R16]` passano e le loro righe sono tolte da
  `tests/verifica/in-attesa.txt`.
- La CI è verde.
