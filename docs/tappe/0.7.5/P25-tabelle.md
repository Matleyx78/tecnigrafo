# [0.7.5-P25] `edit`: le operazioni sulle tabelle in un componente separato

- **Tappa:** 0.7.5 — [piano](../0.7.5-correzione-di-rotta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: il comportamento non cambia; i test di unità e i
  casi di verifica esistenti, invariati, sono la prova
- **Dipende da:** P14, P18, P24 (senza P18, se non si fa)
- **Modulo:** `edit` (più i chiamanti in `cmd`, `script` e `app`, con un cambio
  meccanico)
- **Da leggere:** [ADR 0041](../../adr/0041-documenti-e-ambienti.md), punto 4;
  [ADR 0005](../../adr/0005-comandi-e-annullamento.md);
  [ADR 0023](../../adr/0023-layer-e-proprieta.md); `edit/include/tg/edit/editor.hpp`
  e `edit/src/editor.cpp` (dopo P24); `edit/include/tg/edit/layer_rules.hpp`

## Perché

La 0.8 aggiunge tabelle (stili di testo e di quota, definizioni dei blocchi). Se
restano tutte in `Editor`, la parte «disegno» di `Editor` cresce ancora. Le
operazioni sulle tabelle vanno in un componente loro, ma nella **stessa
transazione** e nella **stessa cronologia** delle entità (ADR 0041, punto 4).

## File

- `edit/include/tg/edit/document_tables.hpp`, `edit/src/document_tables.cpp`
  (nuovi)
- `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`, `edit/CMakeLists.txt`
- i chiamanti delle operazioni spostate, trovati con una ricerca e elencati
  nella pull request (oggi almeno `cmd/src/commands/layer_commands.cpp`,
  `cmd/src/commands/property_commands.cpp`, `cmd/src/commands/file_commands.cpp`,
  `app/src/layer_panel.cpp`, `app/src/properties_panel.cpp`,
  `app/src/main_window.cpp`) e i loro test
- `tests/edit/document_tables_test.cpp` (nuovo), `tests/edit/editor_test.cpp`,
  `tests/CMakeLists.txt`

Il cambio nei chiamanti è solo `editor.addLayer(…)` → `editor.tables().addLayer(…)`:
se un chiamante chiede altro, fermati e scrivilo nella issue. Per la misura del
pacchetto, i chiamanti contano come un file solo.

## Cosa fare

```cpp
namespace tg::edit {

/// The operations on the tables and the header of a document (layers,
/// linetypes, current properties, INSUNITS, LTSCALE, LWDISPLAY), inside the
/// transactions and the history of its Editor (docs/adr/0041, point 4).
/// It belongs to an Editor and lives as long as it.
class DocumentTables {
public:
    // The same functions, with the same contracts, as today's Editor:
    bool setUnits(int units);
    std::optional<core::LayerId> addLayer(core::Layer layer);
    bool renameLayer(core::LayerId id, std::string name);
    bool removeLayer(core::LayerId id);
    bool changeLayer(const core::Layer& layer);
    std::optional<core::LinetypeId> addLinetype(core::Linetype linetype);
    bool setCurrent(const core::CurrentProperties& current);
    bool setLtscale(double scale);
    void setLwdisplay(bool on);
};

class Editor {
public:
    [[nodiscard]] DocumentTables& tables();
    // ...
};

}  // namespace tg::edit
```

- Ogni funzione ha il contratto di oggi, compresa la precondizione «una
  transazione è aperta» e il caso di `setLwdisplay`, che non è un passo di
  annulla.
- Le modifiche si registrano nella transazione aperta di `Editor`, con gli stessi
  tipi di `Change` di oggi: annulla e ripeti di un passo con entità e tabelle
  insieme funzionano come oggi.
- `setProperties` resta in `Editor`: cambia un'entità, non una tabella.
- Come `DocumentTables` raggiunge la transazione di `Editor` (amicizia, una
  piccola interfaccia interna) lo scegli tu, senza esporlo nelle intestazioni
  pubbliche oltre a `tables()`.

## Regole della tappa che valgono qui

- `editor.*`, `main_window.*`: prima di te P24. `layer_panel.*`,
  `properties_panel.*`: prima di te P14 e P18. `command.hpp`, `command_runner.*`:
  non li tocchi.
- I casi di verifica devono restare invariati.

## Test richiesti

- `document_tables_test.cpp`: un passo con un'entità e un layer aggiunti insieme
  si annulla e si ripete in un colpo; un layer aggiunto fuori da una transazione
  è un errore come oggi.
- `editor_test.cpp`: i test di oggi sulle tabelle, spostati su `tables()`, con gli
  stessi risultati.

## Fatto quando

- Le operazioni sulle tabelle stanno in `DocumentTables` e `Editor` non le ha più.
- I test qui sopra esistono e passano, e i casi di verifica sono invariati.
- La CI è verde.
