# [0.6-P7] `app`: registro delle azioni

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #233
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** non servono: riorganizzazione senza cambi di comportamento;
  la provano i casi di livello 2 esistenti e il test che conta i `QAction`
- **Dipende da:** P3
- **Modulo:** `app`
- **Da leggere:** [ADR 0009](../../adr/0009-interfaccia-grafica.md) punti 1, 2,
  6, 7; `app/src/main_window.hpp`, `app/src/main_window.cpp`,
  `app/src/command_line.hpp`; `tests/app/file_menu_test.cpp` e
  `drawing_aids_test.cpp` (come si provano menu e scorciatoie)

## File

- `app/src/action_registry.hpp`, `app/src/action_registry.cpp` (nuovi)
- `app/src/builtin_actions.cpp`, `app/src/builtin_actions.hpp` (nuovi: le
  azioni esistenti)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`
- `app/CMakeLists.txt`
- `tests/app/action_registry_test.cpp` (nuovo)
- `tests/app/file_menu_test.cpp`, `tests/app/drawing_aids_test.cpp` (solo se
  cercano le azioni per nome)
- `tests/CMakeLists.txt`

## Cosa fare

### Il registro

```cpp
/// One action of the interface, declared once (docs/adr/0009, point 1).
struct ActionSpec {
    QString id;               ///< stable, English: "draw.line", "edit.undo"
    QString text;             ///< menu text, with & for the mnemonic
    QString description;      ///< tooltip and shortcuts window
    QString icon;             ///< icon name, "draw-line"; empty until P18-P19
    QKeySequence shortcut;    ///< default; empty for none
    QString command;          ///< cmd command it runs ("LINE"), or empty
    bool checkable{false};
    QStringList environments{QStringLiteral("drawing2d")};
    QString menu;             ///< classic menu path: "&Edit", "&Tools/Object Snap &Modes"
    QString ribbon;           ///< ribbon place "home/draw"; empty: menu only (P16)
};

class ActionRegistry final : public QObject {
    Q_OBJECT
public:
    explicit ActionRegistry(QObject* parent = nullptr);
    /// Precondition: the id is new. The QAction is parented to the registry,
    /// with objectName == id.
    QAction* add(ActionSpec spec);
    [[nodiscard]] QAction* action(const QString& id) const;  ///< nullptr if unknown
    [[nodiscard]] const ActionSpec* spec(const QString& id) const;
    [[nodiscard]] QList<QAction*> actions() const;           ///< in order of addition
    /// Actions created at run time by the registry itself, for lists such
    /// as the recent files: ids "<prefix>.1"... "<prefix>.<count>".
    QList<QAction*> dynamicActions(const QString& prefix, int count);
    /// Builds the classic menus into `bar` from ActionSpec::menu, in order.
    void buildMenus(QMenuBar* bar) const;
};
```

- Le azioni con `command` lo eseguono come se l'utente lo scrivesse nella riga
  di comando (come oggi `Erase`): la riga di comando lo mostra.
- `builtin_actions.cpp` dichiara tutte le azioni esistenti, con gli stessi
  testi, scorciatoie e comportamenti di oggi: file (`file.new`, `file.open`,
  `file.save`, `file.saveas`, `file.recent.*`, `file.quit`), modifica
  (`edit.undo`, `edit.redo`, `edit.erase`), vista (`view.zoom-extents`),
  aiuti (`aids.osnap`, `aids.ortho`, `aids.snap.endpoint`, `.midpoint`,
  `.center`, `.insertion`), aiuto (`help.about`). Aggiungi i comandi di
  disegno come azioni (`draw.line`, `draw.pline`, `draw.circle`, `draw.arc`,
  `draw.text`), nel menu nuovo `&Draw`.
- `MainWindow` costruisce menu e barra di stato dal registro e non crea più
  `QAction` da sé. Gli `objectName` usati dai test (`saveAction`, …) si
  sostituiscono con gli id: aggiorna solo le righe dei test che li cercano.

## Regole della tappa che valgono qui

- Nessun `QAction` fuori dal registro (criterio di chiusura). Fanno eccezione
  solo le `menuAction()` dei sottomenu, che Qt crea da sé.
- Testi dell'interfaccia in inglese con `tr()`; la traduzione è della tappa
  0.9.
- `main_window.cpp/.hpp` e `app/CMakeLists.txt` li toccano poi P16–P24: solo
  le tue righe.

## Test richiesti

- Registro: id doppio (precondizione), `action`, `spec`, ordine, azioni
  dinamiche.
- **Conteggio:** nella finestra principale, ogni `QAction` trovato con
  `findChildren<QAction*>()`, tolte le `menuAction()` dei sottomenu, è nel
  registro.
- Ogni azione con `command` esegue il comando; ogni scorciatoia di oggi
  funziona ancora.

## Fatto quando

- Tutti i test di `app` e tutti i casi di verifica di livello 2 passano.
- Il test di conteggio esiste e passa.
- La CI è verde.
