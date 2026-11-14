# [0.8-PT22] `core` e `io`: stili di quota e variabili di quota

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT22]` (#…), agente codex
- **Dipende da:** PT5 (revisione delle interfacce), PT11, RT22
- **Modulo:** `core` (più la tabella e l'intestazione in `io`)
- **Da leggere:** [ADR 0047](../../adr/0047-quote-e-stili-di-quota.md), punti 2
  e 3; [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md), punto 2;
  `io/src/dxf_tables.cpp`, `io/src/dxf_header.cpp`, `io/src/dxf_skeleton.cpp`;
  le schede [PT6](PT6-scheletro-core.md), punto 5, e [PT11](PT11-stili-di-testo.md)
  (lo stesso schema per un'altra tabella); i valori di `Standard` e `ISO-25`
  confermati dal responsabile in AutoCAD LT ([dxf-di-prova.md](dxf-di-prova.md))

## File

- `core/include/tg/core/dim_style_table.hpp`, `core/src/dim_style_table.cpp`
- `io/src/dxf_dimstyle.cpp` (nuovo), `io/src/dxf_tables.cpp`,
  `io/src/dxf_header.cpp`, `io/src/dxf_skeleton.cpp`, `io/CMakeLists.txt`
  (file comune)
- `tests/core/dim_style_table_test.cpp`, `tests/io/dxf_dimstyle_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. In `dim_style_table.hpp`:

   ```cpp
   /// The essential variables of docs/adr/0047, point 2, with their DXF codes.
   struct DimVariables {
       double scale{1.0}; double arrowSize{2.5}; double extOffset{0.625};
       double extExtension{1.25}; double textHeight{2.5}; double gap{0.625};
       int textAbove{1}; bool textInsideHorizontal{false};
       bool textOutsideHorizontal{false}; int decimals{2}; int angleDecimals{0};
       char decimalSeparator{','}; int zeroSuppression{8}; int angleZeroSuppression{0};
       double linearFactor{1.0}; double round{0.0}; std::string post;
       std::string textStyle{"Standard"}; double centerMark{2.5};
       bool suppressExt1{false}; bool suppressExt2{false};
       bool suppressDim1{false}; bool suppressDim2{false};
       Color dimColor; Color extColor; Color textColor;      // ByBlock
       int dimLineweight{-2}; int extLineweight{-2};         // ByBlock
       friend bool operator==(const DimVariables&, const DimVariables&) = default;
   };
   struct DimStyle { std::uint64_t id{0}; std::string name; DimVariables vars;
                     RawGroups extras; };  // the other variables, as read
   class DimStyleTable {
   public:
       [[nodiscard]] std::span<const DimStyle> styles() const;
       [[nodiscard]] const DimStyle* find(std::string_view name) const;
       [[nodiscard]] const std::string& current() const;      // $DIMSTYLE
       [[nodiscard]] const DimVariables& currentVariables() const;  // header $DIM…
       [[nodiscard]] const RawGroups& currentExtras() const;  // other $DIM… as read
       /// True when every variable that blocks regeneration (docs/adr/0047,
       /// point 2) has the ISO-25 value in `extras` (absent counts as ISO-25).
       [[nodiscard]] static bool regenerable(const RawGroups& extras);
       /// True when the effective essential variables (style plus overrides)
       /// are in the ranges DIMSTYLE accepts and DIMTXSTY names a text style
       /// of `styles` (PT5, B07; checked after the overrides).
       [[nodiscard]] static bool inDomain(const DimVariables& effective,
                                          const TextStyleTable& styles);
       // put/setCurrent/setCurrentVariables like TextStyleTable (PT11)
   };
   ```

   I valori predefiniti qui sopra sono quelli di `ISO-25` attesi; si correggono
   con quelli confermati dal responsabile, e la pull request li cita.
2. Un disegno nuovo ha `Standard` e `ISO-25`, corrente `ISO-25`, con le variabili
   correnti uguali a `ISO-25`.
3. `io` legge la tabella `DIMSTYLE` nel modello (variabili essenziali per codice
   di gruppo; il resto in `extras`, compreso l'handle 105) e le variabili
   `$DIM…` dell'intestazione (le essenziali nel modello, le altre in
   `currentExtras`); riscrive tutto com'era. **Stile mancante** (PT5, punto
   B06: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT5»): come per gli stili di testo (PT11, punto 3),
   se `$DIMSTYLE` manca o nomina uno stile che non c'è, lo stile corrente è
   `Standard`, e se `Standard` non c'è si crea con i valori di AutoCAD; una
   quota che nomina (gruppo 3) uno stile che non c'è tiene quel nome e usa le
   variabili di `Standard`.
4. **Valori fuori dominio** (PT5, punto B07): una quota la cui variabile
   essenziale effettiva (stile più sovrascritture) è fuori dai limiti che
   `DIMSTYLE` accetta (PT28; per esempio `DIMDEC` −1 o `DIMTXT` 0), o il cui
   `DIMTXSTY` non si risolve in uno stile di testo, si conserva ma non è
   rigenerabile, come per le variabili dell'ADR 0047, punto 2.
5. **Sovrascritture di una quota** (XDATA `ACAD`, `DSTYLE`): una funzione legge
   dagli `extras` di una quota le variabili sovrascritte (essenziali e no) e
   le scrive; le userà PT23.

## Regole della tappa che valgono qui

- File comuni: `io/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `dim_style_table_test.cpp`: `find`, corrente, `regenerable` con `DIMTOL` 1 e
  con `DIMBLK` diverso; `inDomain` falso con `DIMDEC` −1 e con `DIMTXT` 0, e
  sulle variabili effettive nei due versi: stile valido con una
  sovrascrittura `DIMDEC` −1 (falso), stile con `DIMDEC` −1 e sovrascrittura
  2 (vero); `DIMTXSTY` che non c'è (falso).
- `dxf_dimstyle_test.cpp`: `$DIMSTYLE=PROVA` che non c'è: corrente
  `Standard`; una quota con gruppo 3 `PROVA` tiene il nome e usa `Standard`.
- `dxf_dimstyle_test.cpp`: giro della tabella e dell'intestazione di un file di
  AutoCAD (`autocad-quote.dxf` se pubblicato, altrimenti uno scritto dal test);
  sovrascritture `DSTYLE` lette e riscritte.

## Fatto quando

- I casi di RT22 passano; i casi di conservazione esistenti sono invariati.
- La CI è verde.
