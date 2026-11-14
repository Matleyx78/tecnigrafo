# [0.8-PT17] `core`: attributi (`ATTDEF`, `ATTRIB`)

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT17]` (#…), agente codex, fascia forte
- **Dipende da:** PT4 (revisione delle interfacce), PT16, RT17
- **Modulo:** `core`
- **Da leggere:** [ADR 0045](../../adr/0045-blocchi-e-attributi.md), punti 4 e
  6; [ADR 0031](../../adr/0031-modifiche-alle-entita.md), punti 3 e 6;
  [ADR 0022](../../adr/0022-allineamenti-obliquo-specchiatura-testo.md);
  `core/include/tg/core/text.hpp`, `similarity.hpp`; le schede
  [PT6](PT6-scheletro-core.md) e [PT16](PT16-blocchi-core.md) (sorella)

## File

- `core/include/tg/core/attribute_definition.hpp`, `core/src/attribute_definition.cpp`
- `core/include/tg/core/insert.hpp`, `core/src/insert.cpp` (la parte degli
  attributi)
- `core/src/generated.cpp` (la parte del testo di `transformedShape`, dopo PT16)
- `core/src/text_affine.cpp`, `core/include/tg/core/text_affine.hpp` (nuovi: un
  testo sotto un'affinità)
- `tests/core/attributes_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const AttributeDefinition&)` restituisce `true` se il testo è
   valido come un `Text` e l'etichetta non è vuota e non ha spazi.
   L'`ATTDEF` si comporta come un `TEXT` che mostra l'etichetta (ingombro,
   disegno, selezione, snap, grip, trasformazioni), tranne quando è dentro un
   blocco, dove non si disegna (punto 4).
2. **Testo sotto un'affinità** (ADR 0044, punto 2; è la parte del testo di
   `transformedShape`, PT6): `Text transformedText(const
   Text&, const Transform2D&)` scompone la parte lineare `A` in
   `R(θ)·[[a, b], [0, c]]` con `c > 0` (QR); se `det A < 0` prima si riflette in
   X (segnale all'indietro invertito, come `MIRRTEXT` = 1). La scomposizione
   si fa sulla base del testo, cioè sulla parte lineare `A·R(r)` con `r` la
   rotazione del testo, non su `A` da sola: il risultato è la geometria vera
   del testo trasformato (PT4, punto P20). Per un testo `Fit` o `Aligned`
   altezza e rotazione vengono dai due punti trasformati. Allora: rotazione
   `+θ`, altezza `·c`, fattore di larghezza `·a/c`, obliquo
   `atan2(b, c)` sommato con la regola delle tangenti
   (`tan(o') = tan(o) + b/c`, con `core::atan2`), punti trasformati con `A`.
   Con una similitudine coincide con `transformed` dell'ADR 0031 e
   `MIRRTEXT` = 1.
3. **`ATTRIB` all'inserimento** (ADR 0045, punto 6):

   ```cpp
   /// The ATTRIBs an insert of `definition` gets with ATTREQ = 0: one per
   /// non-constant ATTDEF, in order, with the default value, placed by the
   /// insert's transform (transformedText), with layer, color, linetype,
   /// lineweight and flags of its ATTDEF. Ids are 0: the editor assigns them.
   [[nodiscard]] std::vector<Attribute> defaultAttributes(const Document&,
                                                          const Insert& insert);
   ```

4. **Attributi nel disegno:**

   ```cpp
   struct PlacedAttribute { std::string tag; std::string value;
       AttributeFlags flags; Text text; bool fromDefinition; };
   /// Every attribute of an insert in drawing coordinates: its ATTRIBs, then
   /// the constant ATTDEFs of its definition (fromDefinition), in order.
   [[nodiscard]] std::vector<PlacedAttribute> attributesOf(const Document&,
                                                           const Insert& insert);
   ```

   È il modo in cui l'elettrico leggerà i pin. Le etichette si confrontano senza
   maiuscole e minuscole (`findAttribute(…, tag)`); con due etichette uguali
   (`PIN` e `pin`) `findAttribute` dà la prima nell'ordine di `attributesOf`.
   Un `ATTRIB` nuovo prende layer, colore, tipo di linea, spessore e flag dal
   suo `ATTDEF` (PT4, punto P21: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT4»).
5. **Geometria generata dell'inserimento** (completa PT16): gli `ATTDEF` della
   definizione **non** si disegnano; gli `ATTRIB` visibili si disegnano come
   `Text` con le loro proprietà (layer `0` e *DaBlocco* come le entità del
   blocco); gli invisibili no.
6. **Trasformazioni dell'inserimento:** ogni `ATTRIB` si trasforma come un
   testo, con `MIRRTEXT` per `MIRROR` (ADR 0031, punto 3), nello stesso passo;
   `transformedOf` di `Insert` (PT16) chiama questa parte.
7. **Copie:** una funzione che, per una copia dell'inserimento, toglie gli
   handle agli attributi (0), così l'editor li assegna nuovi (ADR 0031, punto 6,
   senza i gruppi 102 negli `extras`).

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `attributes_test.cpp`: `defaultAttributes` con un `ATTDEF` variabile, uno
  invisibile e uno costante (che non dà `ATTRIB`); `attributesOf` con il
  costante nelle coordinate del disegno per un inserimento ruotato di 90°;
  `transformedText` con una scala `(2, 1)`, con una riflessione, e con una
  similitudine (uguale a `transformed`); un testo ruotato di 90°, altezza 2 e
  larghezza 1, sotto `diag(2, 1)`: altezza 4, fattore di larghezza 0,5,
  rotazione 90°; `MIRROR` con `MIRRTEXT` 0; un `ATTDEF` sul layer `PINS` con
  colore 1 dà un `ATTRIB` sul layer `PINS` con colore 1; `findAttribute` con
  `PIN` e `pin` nella stessa istanza dà il primo.

## Fatto quando

- I casi di RT17 passano; la CI è verde.
