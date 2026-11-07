# [0.7.5-P30] `app`: ambiente attivo e registro dei tipi di documento, con l'ambiente di prova

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: con il solo disegno non cambia niente; l'ambiente
  e il tipo di prova, solo nei test, lo dimostrano con test Qt
- **Dipende da:** P20, P27, P29
- **Modulo:** `app`
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punti 1, 6
  e 7; [ADR 0039](../../adr/0039-decisione-visiva-dell-interfaccia.md), punto 5;
  [ADR 0009](../../adr/0009-interfaccia-grafica.md), punti 1 e 6;
  `app/src/action_registry.hpp`, `app/src/ui_builder.hpp` e `.cpp`,
  `app/src/shortcut_map.hpp`, `app/src/main_window.cpp` (dopo P29),
  `app/src/main.cpp`; `cmd/include/tg/cmd/command_runner.hpp`
  (`setEnvironment`, P27) e `command_registry.hpp` (`environmentsOf`)

## File

- `app/src/document_types.hpp`, `app/src/document_types.cpp` (nuovi)
- `app/src/environments.hpp`, `app/src/environments.cpp` (nuovi)
- `app/src/main_window.hpp`, `app/src/main_window.cpp`, `app/src/main.cpp`
- `app/src/ui_builder.hpp`, `app/src/ui_builder.cpp`
- `app/src/document_session.hpp` (l'identificatore del tipo diventa quello del
  registro)
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

Aprire e salvare restano dei comandi dei file (P24): il tipo dice le estensioni,
e la pagina *Start* e i dialoghi le usano. Il montaggio (`main.cpp` o la
costruzione di `MainWindow`) chiama `registerBuiltinDocumentTypes`; il resto di
`app` passa dai registri.

### 2. L'ambiente attivo

- È funzione della sessione attiva: l'ambiente predefinito del suo tipo (il modo
  di modifica, per esempio lo schizzo in una parte, arriverà con il 3D).
  `activate` lo dà al runner (`setEnvironment`).
- Ribbon, menu, barre e scorciatoie mostrano solo le azioni il cui
  `ActionSpec::environments` contiene l'ambiente attivo; le schede contestuali
  dell'ambiente si aggiungono a destra delle fisse. Il cambio avviene senza
  ricostruire la finestra, e la scheda del ribbon scelta resta se esiste ancora.
- L'**indicatore dell'ambiente** nella barra di stato, nel posto lasciato da P12,
  mostra icona e nome dell'ambiente attivo. Con la pagina *Start* non c'è.

### 3. L'ambiente di prova, solo nei test

Un test registra un ambiente `test` con una scheda contestuale e un'azione, un
comando di `cmd` con `environments{"test"}`, e un tipo `test` (estensione
`tgtest`) che ospita solo l'ambiente `test`. Il codice di `app` che gestisce
schede, sessioni e ribbon non sa niente di loro.

## Regole della tappa che valgono qui

- `main_window.*`, `builtin_actions.cpp`: prima di te P29, dopo di te P31.
  `ui_builder.*`: prima di te P20.
- Con il solo disegno l'interfaccia è quella di prima.

## Test richiesti

`environments_test.cpp` (test Qt), con l'ambiente e il tipo di prova:

- una scheda del tipo `test` attiva l'ambiente `test`: il ribbon ha la scheda
  contestuale e non ha *Draw*; `LINE` scritto non parte (messaggio di P27); il
  comando di prova parte; l'indicatore dice l'ambiente;
- tornare alla scheda del disegno rimette ribbon, menu e comandi del disegno;
- una scorciatoia di un'azione dell'ambiente `test` non vale nel disegno.

## Fatto quando

- Passando da un documento all'altro cambiano ribbon, icone, menu e comandi
  digitati, e l'ambiente di prova lo dimostra senza toccare `app`.
- I test qui sopra esistono e passano.
- La CI è verde.
