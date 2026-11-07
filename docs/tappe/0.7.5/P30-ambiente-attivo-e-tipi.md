# [0.7.5-P30] `app`: ambiente attivo e registro dei tipi di documento, con l'ambiente di prova

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: con il solo disegno non cambia niente; l'ambiente
  e il tipo di prova, solo nei test, lo dimostrano con test Qt
- **Dipende da:** P20, P27, P29
- **Corretta:** 2026-11-07, con le decisioni del responsabile su B6, B7, B8 e
  B9 del verbale della revisione delle interfacce (P23,
  [#563](https://github.com/Matleyx78/tecnigrafo/issues/563);
  [DECISIONI.md](../../DECISIONI.md)): aprire e salvare passano dal tipo, in
  [P30b](P30b-aprire-e-salvare-dal-tipo.md); azioni globali; l'ambiente è della
  sessione; l'indicatore anche con *Start*
- **Modulo:** `app`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 1, 6
  e 7; [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 5;
  [ADR 0009](../../adr/0009-interfaccia-grafica.md), punti 1 e 6;
  `app/src/action_registry.hpp`, `app/src/ui_builder.hpp` e `.cpp`,
  `app/src/shortcut_map.hpp` e `.cpp`, `app/src/builtin_actions.cpp`, `app/src/main_window.cpp` (dopo P29),
  `app/src/main.cpp`; `cmd/include/tg/cmd/command_runner.hpp`
  (`setEnvironment`, P27) e `command_registry.hpp` (`environmentsOf`)

## File

- `app/src/document_types.hpp`, `app/src/document_types.cpp` (nuovi)
- `app/src/environments.hpp`, `app/src/environments.cpp` (nuovi)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`, `app/src/main.cpp`
- `app/src/ui_builder.hpp`, `app/src/ui_builder.cpp`
- `app/src/action_registry.hpp`, `app/src/builtin_actions.cpp` (le azioni
  globali, punto 2), `app/src/shortcut_map.cpp` (i conflitti con le azioni
  globali)
- `app/src/document_session.hpp` (l'identificatore del tipo diventa quello del
  registro; l'ambiente della sessione)
- `resources/icons/env-drawing2d-16.svg`, `env-drawing2d-32.svg`, `doc-drawing-16.svg`,
  `doc-drawing-32.svg` (nuove)
- `app/CMakeLists.txt`
- `tests/app/environments_test.cpp` (nuovo), `tests/CMakeLists.txt`

## Cosa fare

### 1. I due registri

```cpp
namespace tg::app {

/// An environment (docs/adr/0041, point 1): the commands, ribbon and menus
/// that are valid while a document is edited in it.
struct Environment {
    QString id;                    ///< "drawing2d", as in ActionSpec::environments
    QString name;                  ///< "Drawing"
    QString icon;                  ///< icon name, docs/adr/0040
    QStringList contextualRibbon;  ///< ribbon tabs it adds, docs/adr/0039 point 5
};

/// A type of document (docs/adr/0041, point 7).
struct DocumentType {
    QString id;                     ///< "drawing"
    QString name;                   ///< "Drawing"
    QString icon;
    QStringList extensions;         ///< "cadx", "dxf"
    QStringList environments;       ///< the ones it can host, the first one by default
    std::function<std::unique_ptr<DocumentSession>()> create;  ///< a new document
};

class EnvironmentRegistry { /* add, find by id, list in order */ };
class DocumentTypeRegistry { /* add, find by id, by extension, list in order */ };

/// The types and environments of the program: today the drawing in drawing2d.
void registerBuiltinDocumentTypes(DocumentTypeRegistry&, EnvironmentRegistry&);

}  // namespace tg::app
```

Aprire e salvare passano anche loro dal tipo, come chiede l'ADR 0041, punto 7
(decisione B6): lo fa [P30b](P30b-aprire-e-salvare-dal-tipo.md), che aggiunge a
`DocumentType` le due funzioni. Qui il tipo ha `create`, le estensioni e gli
ambienti. Il montaggio (`main.cpp` o la costruzione di `MainWindow`) chiama
`registerBuiltinDocumentTypes`; il resto di `app` passa dai registri.

### 2. L'ambiente attivo

- È **della sessione attiva** (ADR 0041, punto 1; decisione B8): ogni
  `DocumentSession` ha il suo ambiente, che alla creazione è il predefinito del
  suo tipo e cambia solo con la scelta di P31 (il modo di modifica, per esempio
  lo schizzo in una parte, arriverà con il 3D). Tornando a una sessione torna il
  suo ambiente. `activate` lo dà al runner (`setEnvironment`).
- **Azioni globali** (decisione B7). `ActionSpec::environments` vuoto vuol dire
  «tutti gli ambienti», come la lista vuota dei comandi (P27). Sono globali
  `file.new`, `file.open`, `file.recent` (e le voci dei recenti), `file.quit`,
  `edit.undo`, `edit.redo`, `tools.settings`, `tools.shortcuts`,
  `ui.interface`, `ui.theme` e `help.about`; `file.save` e `file.saveas` lo
  diventano con P30b. Le altre restano di `drawing2d`.
- Ribbon, menu, barre e scorciatoie mostrano solo le azioni globali e quelle il
  cui `ActionSpec::environments` contiene l'ambiente attivo; con la pagina
  *Start*, che non ha un ambiente, solo le globali, e di queste quelle con
  `requiresDocument` (P29) sono disattivate. Le schede contestuali
  dell'ambiente si aggiungono a destra delle fisse. Il cambio avviene senza
  ricostruire la finestra, e la scheda del ribbon scelta resta se esiste ancora.
  Un'azione nascosta o disattivata non risponde nemmeno alla sua scorciatoia.
- Il controllo dei conflitti delle scorciatoie (`shortcut_map.cpp`) tratta
  un'azione globale come presente in tutti gli ambienti: la sua scorciatoia è
  in conflitto con quella di qualunque altra azione.
- L'**indicatore dell'ambiente** nella barra di stato, nel posto lasciato da P12,
  mostra icona e nome dell'ambiente attivo. **Con la pagina *Start* resta**
  (decisione B9) e dice che nessun ambiente è attivo; il testo, in inglese, lo
  sceglie chi implementa. È l'unico punto d'ingresso per scegliere un ambiente
  (P31), uguale ovunque.

### 3. L'ambiente di prova, solo nei test

Un test registra un ambiente `test` con una scheda contestuale e un'azione, un
comando di `cmd` con `environments{"test"}`, e un tipo `test` (estensione
`tgtest`) che ospita solo l'ambiente `test`. Il codice di `app` che gestisce
schede, sessioni e ribbon non sa niente di loro.

## Regole della tappa che valgono qui

- `main_window.*`, `builtin_actions.cpp`: prima di te P29, dopo di te P30b.
  `ui_builder.*`: prima di te P20.
- Con il solo disegno l'interfaccia è quella di prima.

## Test richiesti

`environments_test.cpp` (test Qt), con l'ambiente e il tipo di prova:

- una scheda del tipo `test` attiva l'ambiente `test`: il ribbon ha la scheda
  contestuale e non ha *Draw*; `LINE` scritto non parte (messaggio di P27); il
  comando di prova parte; l'indicatore dice l'ambiente;
- tornare alla scheda del disegno rimette ribbon, menu e comandi del disegno;
- una scorciatoia di un'azione dell'ambiente `test` non vale nel disegno;
- (B7) con la pagina *Start*: `Ctrl+N` e `Ctrl+O` funzionano, *Exit* e *Settings*
  ci sono; `Ctrl+Z` e *Draw* no. Con la scheda del tipo `test`: *New*, *Open*
  e *Undo* ci sono, e *Undo* annulla il comando di prova; *Draw* no (che *New*
  e *Open* funzionino anche lì lo prova P30b). Un'azione globale con la stessa
  scorciatoia di un'azione del disegno è segnalata come conflitto.
- (B8) una sessione del tipo `test` e una del disegno: l'ambiente segue la
  sessione attiva, e tornando alla scheda `test` torna `test`.
- (B9) con la pagina *Start* l'indicatore dell'ambiente c'è e non nomina un
  ambiente; con un documento nomina il suo.

## Fatto quando

- Passando da un documento all'altro cambiano ribbon, icone, menu e comandi
  digitati, e l'ambiente di prova lo dimostra senza toccare `app`.
- I test qui sopra esistono e passano.
- La CI è verde.
