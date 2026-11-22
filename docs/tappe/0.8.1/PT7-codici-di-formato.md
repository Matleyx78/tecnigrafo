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
       bool bold{false}, italic{false};  // PT8 turns them into a FontFace (PT6)
       double height{};        // absolute, drawing units
       double widthFactor{1};
       double oblique{0};      // radians
       double tracking{1};
       std::optional<Color> color;  // nullopt: the colour of the entity
       bool underline{false}, overline{false}, strike{false};
   };
   struct MTextStack { std::u32string top, bottom; StackKind kind; };
   // The bytes of the value one editable unit comes from: a character with
   // its escape (\U+0042, %%c, \\, \~), or a whole stacked fraction.
   struct MTextUnit { std::size_t sourceBegin, sourceEnd; };
   struct MTextRun {
       std::u32string text;               // literal characters, codes resolved
       std::optional<MTextStack> stack;   // a stacked fraction, text empty
       MTextFormat format;
       std::size_t sourceBegin, sourceEnd;  // byte range in the value
       std::vector<MTextUnit> units;      // one per character of text; one for a stack
   };
   using MTextParagraph = std::vector<MTextRun>;
   [[nodiscard]] std::vector<MTextParagraph> formattedParagraphs(
       std::string_view value, const MTextFormat& base);
   ```

2. Le regole del punto 1 dell'ADR 0054: un gruppo `{…}` salva e ripristina il
   formato; `\H<n>x;` moltiplica l'altezza corrente; `\c<n>;` è BGR; `\f…;`
   accende o spegne `bold` e `italic` da `|b` e `|i` e ignora il nome; `\A`, `\p…;` e gli altri
   codici saltati restano fuori dal testo; `\S` con `/`, `#`, `^` (anche `^ `,
   come lo salva AutoCAD) e i caratteri di escape dell'ADR 0046. Pezzi uguali e
   contigui si fondono.
3. `sourceBegin` e `sourceEnd` coprono i byte del valore da cui nasce il pezzo,
   codici di apertura compresi. `units` dice da quali byte nasce **ogni**
   carattere del pezzo (o la frazione, un'unità sola): i byte fra due unità
   sono codici. Servono all'editor (PT10) per cancellare o formattare un
   carattere senza perdere i codici e senza ricavarne la posizione dagli
   avanzamenti o dal numero di caratteri (verbale della revisione delle
   interfacce PT4, #927, [bloccante] 4). Esempi: in `A\A1;B` i due caratteri
   hanno lo stesso formato e stanno in un pezzo solo, con le unità `[0,1)` e
   `[5,6)` (`\A1;` è fra le due); in `é\U+0042` le unità sono `[0,2)` (due byte
   UTF-8) e `[2,9)` (sette byte).

## Regole della tappa che valgono qui

- Un codice malformato si legge come testo, come nella 0.8 (ADR 0046): un
  codice sconosciuto (`\Zx;`) o senza il suo `;` è testo; un codice noto con il
  suo `;` è nascosto, anche con un argomento che non è un numero (`\Habc;`),
  come oggi (ADR 0054, punto 1; i paragrafi di `paragraphs()` non cambiano).
- Un argomento fuori dal dominio della sua grandezza (`\H0;`, `\H0x;`,
  `\W-1;`, `\T0;`, `\Q` o `\H` non finiti) è un ingresso fuori dall'uso reale,
  perché AutoCAD non lo produce ([DECISIONI.md](../../DECISIONI.md),
  2026-11-12, «Ingressi fuori dall'uso reale»): il codice resta nascosto e
  **senza effetto**, come nella 0.8, e il formato resta quello di prima; così
  nessuna altezza, larghezza o spaziatura nulla o negativa arriva a PT8
  (verbale della #927, [decisione] 1).
- `MTextFormat` non usa `FontFace`, che nasce in PT6: i due pacchetti partono
  insieme senza toccare gli stessi file (verbale della revisione del piano,
  [bloccante] 1).

## Test richiesti

- Ogni riga della tabella dell'ADR 0054, punto 1; gruppi annidati; `\H2x;`
  dentro `\H3;`; `\S1^ 2;`; un valore senza codici dà un pezzo Regular per
  paragrafo; i paragrafi di `paragraphs()` di oggi non cambiano.
- Le unità di `A\A1;B` (un pezzo, `[0,1)` e `[5,6)`) e di `é\U+0042` (`[0,2)`
  e `[2,9)`); una frazione `\S1/2;` è un'unità sola, con tutti i suoi byte.
- `\Zx;` e `\H2` senza `;` restano testo; `\Habc;`, `\H0;`, `\W-1;` e `\T0;`
  sono nascosti e lasciano il formato com'era (altezza, larghezza e
  spaziatura del pezzo dopo uguali a quelle del pezzo prima).

## Fatto quando

- I test passano e i casi di oggi restano verdi; la CI è verde.
