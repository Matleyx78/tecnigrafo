# [0.8-PT11] `core` e `io`: stili di testo nel modello e nel DXF

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** issue `[0.8-RT11]` (#…), agente claude
- **Dipende da:** PT4 (revisione delle interfacce), PT6, PT9, RT11
- **Modulo:** `core` (più la tabella in `io`)
- **Da leggere:** [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  punto 1; [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md), punto 2;
  [ADR 0023](../../adr/0023-layer-e-proprieta.md), punto 1 (handle dei record);
  `core/include/tg/core/layer.hpp` e il codec delle tabelle
  `io/src/dxf_tables.cpp` (come sono fatti `LAYER` e `LTYPE`);
  `io/src/dxf_skeleton.cpp` (lo `Standard` di un disegno nuovo); la scheda
  [PT6](PT6-scheletro-core.md), punto 5

## File

- `core/include/tg/core/text_style_table.hpp`, `core/src/text_style_table.cpp`
- `core/src/document.cpp` solo se `Document::newDrawing()` deve mettere
  `Standard` nella tabella (una riga)
- `io/src/dxf_tables.cpp`, `io/src/dxf_tables.hpp`, `io/src/dxf_header.cpp`
  (`$TEXTSTYLE`), `io/src/dxf_skeleton.cpp`
- `tests/core/text_style_table_test.cpp` (nuovo),
  `tests/io/dxf_tables_test.cpp`, `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. In `text_style_table.hpp`:

   ```cpp
   struct TextStyle {
       std::uint64_t id{0};            // handle of the STYLE record
       std::string name;
       std::string font;               // 3
       std::string bigFont;            // 4
       double fixedHeight{0.0};        // 40, 0 = none
       double widthFactor{1.0};        // 41, > 0
       double oblique{0.0};            // 50, radians
       bool backwards{false};          // 71 bit 2
       bool upsideDown{false};         // 71 bit 4
       double lastHeight{2.5};         // 42
       RawGroups extras;               // what the model does not use
   };
   class TextStyleTable {
   public:
       [[nodiscard]] std::span<const TextStyle> styles() const;  // table order
       [[nodiscard]] const TextStyle* find(std::string_view name) const;  // sameTableName
       /// Adds or replaces (same name) a style; returns its handle.
       /// Precondition: valid table name, widthFactor > 0.
       std::uint64_t put(TextStyle style, Document& owner);  // see note
       [[nodiscard]] const std::string& current() const;     // $TEXTSTYLE
       void setCurrent(std::string name);  // precondition: find(name)
       // the STYLE records the model does not handle (shape files), raw
       [[nodiscard]] std::span<const RawGroups> rawRecords() const;
       friend bool operator==(const TextStyleTable&, const TextStyleTable&) = default;
   };
   ```

   Nota sull'handle: un record nuovo prende un handle da `Document` come i
   layer (ADR 0023, punto 1). Se la firma con `Document&` crea un ciclo di
   dipendenze, l'handle lo assegna chi chiama (`edit`, con `Document::nextId`)
   e `put` lo riceve già fatto: scegli tu, purché un handle non si usi due volte
   e la scelta sia scritta nella pull request.
2. Un disegno nuovo ha `Standard` (font `arial.ttf`, come lo scheletro di oggi),
   corrente.
3. `io` legge la tabella `STYLE` nel modello e la riscrive dal modello,
   nell'ordine del file; i record con 70 bit 1 (file di forme) restano grezzi
   nel loro posto. `$TEXTSTYLE` diventa lo stile corrente; manca → `Standard`.
   Un file che nomina uno stile che non c'è nella tabella (in un `TEXT`) si
   legge come oggi: il nome resta nel testo.

## Regole della tappa che valgono qui

- Il disegno usa sempre il font incluso: `font` si conserva, non si usa.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `text_style_table_test.cpp`: `put`, `find` senza distinguere maiuscole,
  stile corrente.
- `dxf_tables_test.cpp`: giro di una tabella con `Standard`, uno stile con
  altezza fissa e uno con un file di forme (che resta grezzo); `$TEXTSTYLE`.

## Fatto quando

- I casi di RT11 passano; i casi di conservazione esistenti sono invariati.
- La CI è verde.
