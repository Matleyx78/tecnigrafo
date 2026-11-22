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
  occupano gli stessi posti nell'ordine nuovo. **Il posto è
  `EntityRecord::order`** (verbale della revisione PT5, [bloccante] 6): il
  codec scrive gestite e grezze fuse per `order`, quindi riordinare scambia
  gli `ATTDEF` fra i valori di `order` che avevano, e ognuno tiene il suo
  handle; le entità grezze (`rawEntities`) non si toccano. Esempio: `ATTDEF A`
  con `order` 10, un'entità grezza con 20, `ATTDEF B` con 30; riordinato in
  `B, A`, il DXF scritto ha `B` (con il suo handle) al posto 10, la grezza al
  20 e `A` al 30.
- Rimuovere l'ultimo `ATTDEF` spegne `hasAttributes` della definizione, come
  `defineBlock()`.
- **Quali inserimenti** ([bloccante] 12): `synchronizeInserts` aggiorna ogni
  record `INSERT` del blocco, nel disegno e dentro le definizioni di altri
  blocchi (esplodibili o no), **una volta per record**: un `INSERT A` dentro
  la definizione di `B` si aggiorna una volta sola, anche se `B` è inserito
  più volte, e gli `INSERT B` del disegno non cambiano (i loro dati restano;
  la loro geometria generata segue la definizione di `B` aggiornata). Il
  numero restituito conta i record cambiati.
- **Handle** ([decisione] 1 della revisione PT5): gli `ATTRIB` che restano
  tengono il loro, una volta sola (la regola delle etichette doppie di PT15);
  i nuovi ne hanno uno nuovo, anche dentro le definizioni (la riserva degli
  handle annidati di `Document`).
- Annullare la transazione rimette definizione, inserimenti e handle com'erano.

## Regole della tappa che valgono qui

- Etichette confrontate senza maiuscole (ADR 0045, punto 6); un'etichetta
  nuova uguale a un'altra della definizione si rifiuta.

## Test richiesti

- Riordino, modifica, rimozione e sincronizzazione, ciascuno con l'annulla;
  un blocco annidato in un altro, i cui inserimenti esterni non cambiano.
- Revisione PT5: il riordino dell'esempio qui sopra con l'entità grezza in
  mezzo, scritto in DXF e annullato ([bloccante] 6); la rimozione dell'ultimo
  `ATTDEF` (`hasAttributes` spento); un `INSERT A` nel disegno e uno dentro la
  definizione di `B`, inserita due volte: `synchronizeInserts(A)` restituisce
  2, gli `ATTRIB` nuovi hanno handle nuovi e unici, quelli tenuti lo stesso
  handle, l'ordine è quello della definizione, gli `INSERT B` hanno i dati di
  prima e un `U` rimette tutto in un passo ([bloccante] 12); due `ATTRIB` con
  etichette `A` e `a` e un solo `ATTDEF` `A` ([decisione] 1).

## Fatto quando

- I test passano; la CI è verde.
