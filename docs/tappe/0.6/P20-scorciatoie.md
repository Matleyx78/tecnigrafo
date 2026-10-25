# [0.6-P20] `app`: scorciatoie personalizzabili

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #253
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: finestra non pilotabile dagli script; test Qt
  e prova manuale (decisione 5 del piano)
- **Dipende da:** P19
- **Modulo:** `app`
- **Da leggere:** [ADR 0009](../../adr/0009-interfaccia-grafica.md) punto 6;
  `app/src/action_registry.hpp` (P7), `app/src/settings_dialog.cpp` (P17)

## File

- `app/src/shortcut_map.hpp`, `app/src/shortcut_map.cpp` (nuovi)
- `app/src/shortcuts_dialog.hpp`, `app/src/shortcuts_dialog.cpp` (nuovi)
- `app/src/main_window.cpp` (caricamento all'avvio, azione `tools.shortcuts`)
- `app/src/builtin_actions.cpp`
- `app/CMakeLists.txt`
- `tests/app/shortcut_map_test.cpp`, `tests/app/shortcuts_dialog_test.cpp` (nuovi)
- `tests/CMakeLists.txt`

## Cosa fare

```cpp
/// The shortcuts of the registry, with the user's changes (docs/adr/0009,
/// point 6). Only differences from the defaults are stored.
class ShortcutMap {
public:
    explicit ShortcutMap(const ActionRegistry& registry);
    [[nodiscard]] QKeySequence shortcut(const QString& id) const;
    /// The id of another action of a shared environment with `sequence`, or
    /// an empty string. An empty sequence never conflicts.
    [[nodiscard]] QString conflict(const QString& id, const QKeySequence& sequence) const;
    /// False, changing nothing, on a conflict.
    bool set(const QString& id, const QKeySequence& sequence);
    void reset(const QString& id);
    void resetAll();
    void apply(ActionRegistry& registry) const;  ///< sets every QAction now
    void save(QSettings& settings) const;        ///< group "shortcuts", differences only
    void load(QSettings& settings);              ///< ignores unknown ids and conflicts
    /// One line per changed action: "<id>\t<sequence in PortableText>"; an
    /// empty sequence means "removed".
    [[nodiscard]] QString exportText() const;
    /// Errors (unknown id, invalid sequence, conflict) list the line numbers;
    /// nothing is applied if there is one.
    [[nodiscard]] QStringList importText(const QString& text);
};
```

- **Finestra:** tabella con azione, descrizione, scorciatoia attuale e
  predefinita; filtro di ricerca; modifica con `QKeySequenceEdit`; pulsanti
  Togli, Ripristina, Ripristina tutto, Esporta, Importa. Un conflitto si
  segnala con il nome dell'altra azione e non si accetta.
- Il cambio vale subito (`apply`) e si salva con **OK**.
- Le parole della riga di comando non sono scorciatoie e non compaiono.

## Regole della tappa che valgono qui

- `main_window.cpp`, `builtin_actions.cpp`, `app/CMakeLists.txt`: solo le tue
  righe.

## Test richiesti

- `ShortcutMap`: imposta, togli, conflitto rifiutato, ripristino, salvataggio
  e caricamento con `QSettings` in una cartella temporanea, esportazione e
  importazione (con errori).
- Finestra: una scorciatoia cambiata vale subito; un conflitto mostra
  l'avviso; riavvio simulato con la scorciatoia ritrovata.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
