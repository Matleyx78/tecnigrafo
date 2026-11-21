# [0.8.1-PT7] `core`: i codici di formato di `MTEXT` in pezzi

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: nessun comportamento del programma cambia; i
  pezzi arrivano nel disegno con [PT8](PT8-impaginazione-formati.md), che ha i
  riferimenti. Test di unità
- **Dipende da:** PT4
- **Modulo:** `core`
- **Da leggere:** [ADR 0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md),
  punto 2; [ADR 0054](../../adr/0054-testo-formattato-ed-editor.md), punti 1 e
  3; `core/src/mtext_layout.cpp` (`paragraphs`, la lettura dei codici di oggi);
  le schede sorelle [PT8](PT8-impaginazione-formati.md) e
  [PT10](PT10-modello-di-modifica.md)

## File

- `core/include/tg/core/mtext_format.hpp`, `core/src/mtext_format.cpp` (nuovi)
- `core/src/mtext_layout.cpp` (solo: `paragraphs` passa dalla lettura nuova,
  senza cambiare risultato)
- `core/CMakeLists.txt` (file comune)
- `tests/core/mtext_format_test.cpp` (nuovo), `tests/CMakeLists.txt` (file
  comune)

## Cosa fare

1. Interfaccia:

   ```cpp
   enum class StackKind : std::uint8_t { Horizontal, Diagonal, Tolerance };  // / # ^
   struct MTextFormat {
       FontFace face{FontFace::Regular};
       double height{};        // absolute, drawing units
       double widthFactor{1};
       double oblique{0};      // radians
       double tracking{1};
       std::optional<Color> color;  // nullopt: the colour of the entity
       bool underline{false}, overline{false}, strike{false};
   };
   struct MTextStack { std::u32string top, bottom; StackKind kind; };
   struct MTextRun {
       std::u32string text;               // literal characters, codes resolved
       std::optional<MTextStack> stack;   // a stacked fraction, text empty
       MTextFormat format;
       std::size_t sourceBegin, sourceEnd;  // byte range in the value
   };
   using MTextParagraph = std::vector<MTextRun>;
   [[nodiscard]] std::vector<MTextParagraph> formattedParagraphs(
       std::string_view value, const MTextFormat& base);
   ```

2. Le regole del punto 1 dell'ADR 0054: un gruppo `{…}` salva e ripristina il
   formato; `\H<n>x;` moltiplica l'altezza corrente; `\c<n>;` è BGR; `\f…;`
   sceglie la variante da `|b` e `|i` e ignora il nome; `\A`, `\p…;` e gli altri
   codici saltati restano fuori dal testo; `\S` con `/`, `#`, `^` (anche `^ `,
   come lo salva AutoCAD) e i caratteri di escape dell'ADR 0046. Pezzi uguali e
   contigui si fondono.
3. `sourceBegin` e `sourceEnd` coprono i byte del valore da cui nasce il pezzo,
   codici di apertura compresi: servono all'editor (PT10) per non perdere i
   codici.

## Regole della tappa che valgono qui

- Un codice malformato si legge come testo, come nella 0.8 (ADR 0046).

## Test richiesti

- Ogni riga della tabella dell'ADR 0054, punto 1; gruppi annidati; `\H2x;`
  dentro `\H3;`; `\S1^ 2;`; un valore senza codici dà un pezzo Regular per
  paragrafo; i paragrafi di `paragraphs()` di oggi non cambiano.

## Fatto quando

- I test passano e i casi di oggi restano verdi; la CI è verde.
