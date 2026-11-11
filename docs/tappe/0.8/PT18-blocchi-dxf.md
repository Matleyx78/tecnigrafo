# [0.8-PT18] `io`: blocchi, inserimenti e attributi nel DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** impegnativa
- **Agente:** claude
- **Modello e impegno:** forte; impegno alto
- **Riferimenti:** issue `[0.8-RT18]` (#…), agente codex, fascia forte
- **Dipende da:** PT3, PT10, PT17, RT18
- **Modulo:** `io`
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punti 1, 2,
  6; [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md) per intero;
  [ADR 0017](../../adr/0017-modulo-io-e-dxf.md); [ADR 0023](../../adr/0023-layer-e-proprieta.md),
  punto 1; `io/src/dxf_read.cpp`, `dxf_write.cpp`, `dxf_skeleton.cpp`,
  `dxf_tables.cpp`, `dxf_entities.cpp`, `dxf_text.cpp`; le schede
  [PT16](PT16-blocchi-core.md) e [PT17](PT17-attributi-core.md)

## Perché resta impegnativa

Scrivere la sezione `BLOCKS` dal modello tiene coerenti handle, proprietari
(330) e `BLOCK_RECORD` di ogni file, con casi che nessun documento fissa del
tutto: blocchi anonimi, spazio carta, oggetti che nominano entità dei blocchi.
Un errore rompe i file in AutoCAD (piano, «Motivi»).

## File

- `io/src/dxf_blocks.cpp`, `io/src/dxf_blocks.hpp` (nuovi: sezione `BLOCKS` e
  `BLOCK_RECORD`)
- `io/src/dxf_insert.cpp`, `io/src/dxf_attdef.cpp` (nuovi: codec di `INSERT`
  con `ATTRIB` e `SEQEND`, di `ATTDEF`)
- `io/src/dxf_read.cpp`, `io/src/dxf_write.cpp`, `io/src/dxf_tables.cpp`
- `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`, `io/src/dxf_entity_common.hpp`
  (file comuni)
- `tests/io/dxf_blocks_test.cpp`, `tests/io/dxf_insert_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Lettura di `BLOCKS`.** Ogni `BLOCK … ENDBLK` diventa una
   `BlockDefinition` (PT16), tranne `*Model_Space`, `*Paper_Space…`, i blocchi
   esterni (70 bit 4 o 1 xref-dipendente) e i `*D…` (che legge PT25), che
   restano grezzi come oggi, al loro posto. Le entità del blocco passano dai
   codec come quelle del disegno: gestite o grezze, con il loro handle e il
   loro ordine. Il `BLOCK_RECORD` con lo stesso nome dà `recordId`, unità (70),
   esplodibile (280), scala uniforme (281) e `recordExtras`.
2. **`INSERT`.** Codec con 2 (nome), 10/20, 41/42 (43 in `extras`), 50, 66 e gli
   `ATTRIB` che seguono fino a `SEQEND`, ognuno con handle, proprietà ed
   `extras`; l'handle del `SEQEND` e i suoi gruppi si conservano. Resta grezzo
   (ADR 0044, punto 4; ADR 0045, punto 2): `MINSERT` (70 o 71 > 1), blocco che
   non c'è o esterno, scala 0, un `ATTRIB` non gestito (multiriga, 101), un
   inserimento che crea un ciclo.
3. **`ATTDEF`** nel disegno e nei blocchi: i gruppi di `TEXT` più 3, 2, 70, 73,
   74 (allineamento verticale degli attributi), 280; multiriga (101) resta
   grezzo.
4. **Ordine.** Le definizioni si leggono prima delle entità, così il codec di
   `INSERT` conosce la tabella; i cicli si scoprono alla fine e quegli
   inserimenti tornano grezzi.
5. **Scrittura.** La sezione `BLOCKS` si scrive dal modello nell'ordine della
   tabella, con i blocchi grezzi al loro posto; la tabella `BLOCK_RECORD` ha un
   record per ogni definizione, con i proprietari (330) delle entità uguali al
   `recordId`. Una definizione nuova (`recordId` 0) prende handle nuovi per
   `BLOCK_RECORD`, `BLOCK` ed `ENDBLK`. Un file aperto e salvato senza
   modifiche resta identico secondo l'ADR 0018, punto 6.
6. **`$HANDSEED`** oltre l'handle più alto, entità dei blocchi e attributi
   compresi.

## Regole della tappa che valgono qui

- I casi di conservazione di oggi (`conservazione-*`, `dxf/*`) sono la rete:
  devono restare invariati.
- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `dxf_blocks_test.cpp`: giro di un file con un blocco semplice, uno annidato,
  uno anonimo `*U`, lo spazio carta con entità; un blocco nuovo riceve
  `BLOCK_RECORD` e handle coerenti; un ciclo torna grezzo.
- `dxf_insert_test.cpp`: `INSERT` con due `ATTRIB` e `SEQEND`; `MINSERT` e
  blocco mancante grezzi; `ATTDEF` costante e invisibile.

## Fatto quando

- I casi di RT18 passano, compresi quelli di `autocad-blocchi.dxf` e della
  libreria dell'elettrico se il responsabile l'ha pubblicata.
- La CI è verde.
