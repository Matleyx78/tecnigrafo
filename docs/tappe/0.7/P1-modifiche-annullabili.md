# [0.7-P1] `edit`: modifiche annullabili delle entità

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo con i comandi (R5 in poi); test di
  unità
- **Dipende da:** —
- **Modulo:** `edit` (più l'operazione del documento in `core`)
- **Da leggere:** [ADR 0031](../../adr/0031-modifiche-alle-entita.md) punti 5, 6
  e 11; [ADR 0005](../../adr/0005-comandi-e-annullamento.md);
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) punto 4;
  `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`;
  `core/include/tg/core/document.hpp`, `core/include/tg/core/entity_extras.hpp`

## File

- `core/include/tg/core/document.hpp`, `core/src/document.cpp`
- `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp`
- `tests/core/document_test.cpp`, `tests/edit/editor_test.cpp`

## Cosa fare

Nel documento, l'operazione che `replace` usa (pubblica per lo stesso motivo
delle altre: solo `edit` e `io` la chiamano):

```cpp
/// Replaces the entity of record `id`, keeping identifier, order key, extras
/// and properties, and updates the spatial index. Returns the entity it
/// replaced. Precondition: the record exists.
Entity replaceEntity(EntityId id, Entity entity);
```

Nell'editor:

```cpp
/// Replaces the geometry of entity `id` (docs/adr/0031, point 5). The new
/// entity has the same type, or is an Arc replacing a Circle. Returns false,
/// changing nothing, if there is no such entity, the type change is not
/// allowed, or the entity is unchanged. Precondition: a transaction is open.
bool replace(core::EntityId id, core::Entity entity);

/// Adds `entity` derived from `source` (docs/adr/0031, point 6): a new
/// identifier, last in order, the properties and extras of `source` except
/// the 102 groups of extras.beforeEntity, each with everything between its
/// "{..." and "}". std::nullopt, changing nothing, if `source` does not exist.
/// Precondition: a transaction is open.
std::optional<core::EntityId> addCopy(core::EntityId source, core::Entity entity);

/// A position in the open transaction (docs/adr/0031, point 11).
struct Savepoint {
    std::size_t changes{0};
};
/// Precondition: a transaction is open.
[[nodiscard]] Savepoint savepoint() const;
/// Undoes, in reverse order, the changes made after `mark`; the transaction
/// stays open. Precondition: `mark` comes from the open transaction.
void rollbackTo(Savepoint mark);
```

- `replace` è un nuovo tipo di `Change` (`Replaced`), che ricorda l'entità di
  prima e la rimette con annulla. Annulla e ripeti tengono identificativo,
  ordine, proprietà e dati.
- `addCopy` non copia i gruppi 102 di `beforeEntity` (reattori, dizionario di
  estensione); copia tutto il resto, anche il gruppo 330.
- Dopo `rollbackTo` un `commit` registra solo le modifiche rimaste; se non ne
  resta nessuna, la transazione è vuota come oggi.

## Regole della tappa che valgono qui

- `document.hpp` e `document.cpp` li tocca solo questo pacchetto nella tappa.
- `tests/CMakeLists.txt`: non serve, i file di test esistono già.

## Test richiesti

- `replace`: stessa entità (false), tipo diverso (false), cerchio → arco (true),
  identificativo assente (false); annulla e ripeti; indice spaziale aggiornato
  (`query` trova l'entità nella nuova posizione e non nella vecchia).
- `addCopy`: proprietà e dati copiati, gruppi 102 tolti (con `{ACAD_REACTORS` e
  `{ACAD_XDICTIONARY`), 330 conservato, sorgente assente; annulla.
- `savepoint`/`rollbackTo`: aggiunte, rimozioni e sostituzioni dopo il punto
  annullate in ordine inverso, quelle prima intatte; `rollbackTo` all'inizio
  lascia una transazione vuota.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
