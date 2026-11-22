# [0.8.1-PT15] `core` e `io`: posizione bloccata, sincronizzazione e scelta di un attributo

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT15]` (#…), agente codex
- **Dipende da:** PT1, PT2, PT5, PT27, RT15
- **Modulo:** `core` (più il codec di `io`, il campo del rapporto in `script`
  e la risposta dell'oracolo)
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punti 5 e
  6; [ADR 0056](../../adr/0056-comandi-degli-attributi.md), punti 1, 4 e 5;
  `core/include/tg/core/attribute_definition.hpp`, `core/include/tg/core/insert.hpp`,
  `io/src/dxf_attdef.cpp`, `io/src/dxf_insert.cpp`; la scheda sorella
  [PT16](PT16-definizioni-edit.md)

## File

- `core/include/tg/core/attribute_definition.hpp`,
  `core/src/attribute_definition.cpp`
- `core/include/tg/core/insert.hpp`, `core/src/insert.cpp`
- `io/src/dxf_attdef.cpp`, `io/src/dxf_insert.cpp`
- `script/src/new_entities_format.cpp` (`lock-position=yes`)
- `tests/verifica/oracolo/ezdxf_attesi.py` (lo stesso campo)
- `tests/core/attribute_definition_test.cpp`, `tests/core/insert_test.cpp`,
  `tests/io/dxf_blocks_test.cpp`

## Cosa fare

1. `bool lockPosition{false}` in `AttributeDefinition` e in `Attribute`: il
   **secondo** gruppo 280 (ADR 0056, punto 1); il primo 280 resta dov'è, intatto;
   la riscrittura è identica a quella di oggi. `defaultAttributes` lo copia
   dall'`ATTDEF` come campo, non copiando i 280 grezzi. **Un record nuovo**
   (`-ATTDEF`, l'`ATTRIB` che ne nasce, uno senza gruppi letti) scrive prima
   il 280 della versione (0) e poi, con `lockPosition`, il 280 della posizione
   bloccata (1): così un solo `280=1` non si rilegge mai come versione. Un
   record letto con il solo 280 della versione ha `lockPosition` falso, e
   accendendolo il secondo 280 si aggiunge subito dopo il primo, con il resto
   invariato (verbale della revisione PT5, [bloccante] 4).
2. ```cpp
   // ADR 0056, point 5: the attributes of `insert` after synchronising with
   // its definition, in the order of the definition.
   [[nodiscard]] std::vector<Attribute> synchronizedAttributes(
       const Document& document, const Insert& insert);
   struct AttributePick { EntityId insert; std::size_t index; };
   // The visible attribute nearest to `point` within `tolerance` among those
   // `accept` admits (none: all), with the rules of pickEntity: a tie goes to
   // the higher insert id, then the lower index. Only the ATTRIBs of the
   // inserts of the drawing: one of a nested insert lives in a block
   // definition and is never picked.
   [[nodiscard]] std::optional<AttributePick> pickAttribute(
       const Document& document, const Vec2& point, double tolerance,
       const std::function<bool(const Insert&, const Attribute&)>& accept = {});
   ```
   La sincronizzazione tiene valore **e handle** di chi ha la stessa etichetta
   (confronto senza maiuscole), e riprende il resto dall'`ATTDEF` trasformato
   dall'inserimento; quelli nuovi hanno handle `0`, che il chiamante assegna.
   **Etichette doppie** (revisione PT5, [decisione] 1, regola esistente: fra
   etichette uguali vale la prima, [DECISIONI.md](../../DECISIONI.md),
   2026-11-14, «Revisione delle interfacce PT4», P21): ogni `ATTDEF` non
   costante, nell'ordine della definizione, prende il **primo** `ATTRIB`
   ancora libero con la sua etichetta, nell'ordine del DXF; gli `ATTRIB` che
   restano senza `ATTDEF` spariscono; un `ATTDEF` omonimo che non trova un
   `ATTRIB` libero nasce con il predefinito e handle `0`. Così ogni handle si
   usa una volta sola. L'`ATTRIB` che resta prende dall'`ATTDEF` le
   proprietà modellate (ADR 0056, punto 5) e **conserva** i suoi dati non
   gestiti, XDATA e dizionari compresi (ADR 0006).
   **La scelta col clic** ([bloccante] 5): i filtri di chi chiama (il nome
   del blocco, l'etichetta e il valore di `-ATTEDIT`, ADR 0056, punto 4;
   PT19) stanno in `accept`, che riceve anche l'`Insert`,
   e si applicano **prima** del confronto delle distanze, così un attributo
   ammesso sotto uno escluso si trova. «Visibile» è ciò che si disegna, con la
   regola di `ATTDISP` (PT27) e dei layer della geometria generata
   dell'inserimento (0.8-PT16 e PT17): `pickAttribute` usa quella, non una
   propria.
3. Il campo `lock-position=yes` nel rapporto e nella risposta di ezdxf (campo
   annunciato da PT2).

## Regole della tappa che valgono qui

- `core/src/insert.cpp` lo cambia prima PT27 (gli attributi secondo
  `ATTDISP`): per questo PT15 lo aspetta.
- File comuni: `script/src/new_entities_format.cpp` e
  `tests/verifica/oracolo/ezdxf_attesi.py`, dove questo pacchetto aggiunge solo
  il campo `lock-position` (gli altri campi della tappa li aggiungono PT8 e
  PT23).
- Pareggi di `pickAttribute`: quelli di `pickEntity`
  ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 4, punto 1), poi
  l'indice più basso.

## Test richiesti

- Andata e ritorno di un `ATTDEF` e di un `ATTRIB` con *Lock position*; la
  sincronizzazione dei casi di `autocad-attsync.dxf` (tolto, nuovo, spostato,
  alto 3,5) e con un inserimento ruotato e scalato.
- Revisione PT5:
  - un `ATTDEF` nuovo con `lockPosition` e il suo `ATTRIB`, scritti e riletti
    (versione 0, poi 1); un record letto con il solo 280 della versione
    (`lockPosition` falso); lo stesso record con `lockPosition` acceso e
    riscritto, uguale per il resto ([bloccante] 4);
  - due `ATTRIB` `A` e `a` (handle 31 e 32, valori diversi) con un solo
    `ATTDEF` `A`: resta il 31 con il suo valore, il 32 sparisce; due `ATTDEF`
    `A` e `a` con un solo `ATTRIB` `A`: il primo tiene handle e valore, il
    secondo ha il predefinito e handle `0`; un `ATTRIB` con XDATA propria che
    resta: la XDATA c'è ancora ([decisione] 1);
  - due attributi visibili sovrapposti, `SIGLA` (primo per il pareggio) e
    `CODICE`, con `accept` che ammette solo `CODICE`: il clic dà `CODICE`;
    due inserimenti sovrapposti dei blocchi `A` e `B`, tutti e due con
    `CODICE` e lo stesso valore, con `accept` che ammette solo il blocco `A`:
    il clic dà l'attributo di `A` anche se `B` vincerebbe il pareggio; un
    attributo di un inserimento annidato sotto il clic: nessuna scelta
    ([bloccante] 5; verbale della revisione della correzione #992,
    [bloccante] 3).

## Fatto quando

- I casi di RT15 passano; la CI è verde, con il passo dell'oracolo.

## Prova a secco dei riferimenti

```text
; oracle: ezdxf
# casi/dxf-blocchi/posizione-bloccata/domande.tgq
D = DXF source=autocad-attributi
? count D -> …
? entity D <handle dell'ATTDEF SIGLA> -> …
? entity D <handle dell'ATTRIB SIGLA del primo inserimento> -> …
? roundtrip D -> identical
```

- **Strumenti:** `count`, `entity`, `roundtrip` su `main`; il campo
  `lock-position` annunciato da PT2. RT15 aggiunge `autocad-attributi` a
  `sorgenti.json`.
- **Dipendenze:** nessun comando.
- **Risposte condivise:** l'oracolo e il rapporto scrivono `lock-position=yes`
  solo per i gruppi 280 secondi uguali a 1: fra i DXF di `dxf-test-file` ce
  l'hanno solo `autocad-attributi.dxf` (nuovo) e `non-gestiti.dxf`, che nessun
  caso usa (controllato). Nessun caso già unito cambia.
