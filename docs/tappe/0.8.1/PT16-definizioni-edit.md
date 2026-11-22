# [0.8.1-PT16] `edit`: operazioni sulle definizioni e sincronizzazione degli inserimenti

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: le operazioni si vedono con `ATTSYNC`
  ([PT18](PT18-attsync-attdisp.md), che ha i riferimenti) e con `BATTMAN`
  ([PT20](PT20-battman.md), test Qt); test di unità
- **Dipende da:** PT5, PT15
- **Modulo:** `edit`
- **Da leggere:** [ADR 0056](../../adr/0056-comandi-degli-attributi.md),
  punto 5; [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punto 5;
  `edit/include/tg/edit/blocks.hpp`, `edit/src/blocks.cpp`,
  `edit/include/tg/edit/editor.hpp`; la scheda sorella
  [PT15](PT15-attributi-core.md)

## File

- `edit/include/tg/edit/blocks.hpp`, `edit/src/blocks.cpp`
- `tests/edit/blocks_test.cpp`

## Cosa fare

```cpp
// All four work inside the caller's transaction, like defineBlock(), and
// return false (changing nothing) when the block or the tag is missing.
bool reorderAttributeDefinitions(Editor& editor, std::string_view block,
                                 std::span<const std::string> tags);  // a permutation of all tags
bool replaceAttributeDefinition(Editor& editor, std::string_view block,
                                std::string_view tag, core::AttributeDefinition definition);
bool removeAttributeDefinition(Editor& editor, std::string_view block, std::string_view tag);
// Every insert of `block`, in id order, gets core::synchronizedAttributes();
// new ATTRIBs get fresh handles. Returns the number of inserts changed.
std::size_t synchronizeInserts(Editor& editor, std::string_view block);
```

- Le entità del blocco che non sono `ATTDEF` non cambiano posto; gli `ATTDEF`
  occupano gli stessi posti nell'ordine nuovo.
- Annullare la transazione rimette definizione, inserimenti e handle com'erano.

## Regole della tappa che valgono qui

- Etichette confrontate senza maiuscole (ADR 0045, punto 6); un'etichetta
  nuova uguale a un'altra della definizione si rifiuta.

## Test richiesti

- Riordino, modifica, rimozione e sincronizzazione, ciascuno con l'annulla;
  un blocco annidato in un altro, i cui inserimenti esterni non cambiano.

## Fatto quando

- I test passano; la CI è verde.
