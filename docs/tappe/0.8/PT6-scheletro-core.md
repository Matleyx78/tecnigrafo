# [0.8-PT6] `core`: scheletro delle entità composte e delle tabelle nuove

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** non servono: nessun lettore e nessun comando crea ancora i
  tipi nuovi; test di unità, casi di verifica invariati
- **Dipende da:** PT4 (revisione delle interfacce)
- **Modulo:** `core` (più le righe delle visite negli altri moduli, perché
  compilino)
- **Da leggere:** [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md)
  per intero; i punti dei campi in [0045](../../adr/0045-blocchi-e-attributi.md)
  (2, 6), [0046](../../adr/0046-stili-di-testo-e-testo-multiriga.md) (1, 2),
  [0047](../../adr/0047-quote-e-stili-di-quota.md) (1, 3),
  [0048](../../adr/0048-retini.md) (1); [ADR 0004](../../adr/0004-modello-geometrico-2d.md);
  [ADR 0031](../../adr/0031-modifiche-alle-entita.md), punti 1–2;
  `core/include/tg/core/document.hpp`, `similarity.hpp`, `snap_points.hpp`,
  `grips.hpp`, `proximity.hpp`, `dashes.hpp`, `effective_properties.hpp`,
  `transform2d.hpp`; le schede sorelle [PT7](PT7-tabelle-edit.md),
  [PT8](PT8-geometria-generata-render.md), [PT9](PT9-rapporto-entita-nuove.md)

## File

- nuovi in `core/include/tg/core/` e `core/src/`: `solid`, `ellipse`, `mtext`,
  `attribute_definition`, `insert`, `dimension`, `hatch`, `generated`,
  `text_style_table`, `dim_style_table`, `block_table` (`.hpp` e `.cpp`)
- `core/include/tg/core/document.hpp`, `core/src/document.cpp`
- le visite: `core/src/similarity.cpp`, `proximity.cpp`, `snap_points.cpp`,
  `grips.cpp`, `dashes.cpp`; `core/src/document_query.cpp` (scelta e snap
  passano dalle funzioni del punto 3)
- le righe che fanno compilare le altre visite, senza comportamento nuovo:
  `render/src/linework.cpp`, `render/src/scene_renderer.cpp`,
  `script/src/report.cpp`, `script/src/question_table.cpp`,
  `script/src/property_questions.cpp`, `script/src/level1.cpp`,
  `app/src/script_session.cpp`, `io/src/dxf_entities.cpp`
- `core/CMakeLists.txt` (file comune)
- `tests/core/new_entities_test.cpp`, `tests/core/generated_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

### 1. I tipi

`Entity` = `std::variant<Polyline, Line, Circle, Arc, Text, Solid, Ellipse, MText,
AttributeDefinition, Insert, Dimension, Hatch>`. Ogni tipo ha `operator==`
predefinito. Angoli in radianti, come `Arc` e `Text`.

```cpp
struct Solid { std::array<Vec2, 4> vertices; };  // DXF 10, 11, 12, 13

struct Ellipse {  // docs/adr/0052
    Vec2 center; Vec2 majorAxis;   // 10; 11, relative to the center
    double ratio{1.0};             // 40, in (0, 1]
    double startParameter{0.0};    // 41, radians
    double endParameter{0.0};      // 42, radians; 2*pi with 0 = full
};

enum class MTextAttachment : std::uint8_t { TopLeft = 1, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight, BottomLeft, BottomCenter, BottomRight };
struct MText {
    Vec2 insertion; double height{2.5}; double width{0.0};
    MTextAttachment attachment{MTextAttachment::TopLeft};
    double rotation{0.0}; std::string style{"Standard"};
    double lineSpacing{1.0}; bool exactSpacing{false}; bool directionByStyle{false};
    std::string value;  // groups 3 and 1 joined, codes kept
};

struct AttributeFlags { bool invisible{false}; bool constant{false};
                        bool verify{false}; bool preset{false}; };
struct AttributeDefinition { Text text; std::string tag; std::string prompt;
                             AttributeFlags flags; };  // text.value = default

struct Attribute {  // ATTRIB, owned by its Insert
    EntityId id; Text text; std::string tag; bool invisible{false};
    EntityExtras extras; EntityProperties properties;
};
struct Insert {
    std::string block; Vec2 insertion; double scaleX{1.0}; double scaleY{1.0};
    double rotation{0.0}; std::vector<Attribute> attributes;
};

enum class DimensionKind : std::uint8_t { Linear, Aligned, Angular, Diameter,
                                          Radius, Other };
struct DimensionDefinition {      // what it measures (DXF definition points)
    DimensionKind kind{DimensionKind::Linear}; int dxfType{0};  // 70 & 7
    Vec2 linePoint;   // 10
    Vec2 point13; Vec2 point14; Vec2 point15; Vec2 point16;
    double rotation{0.0};  // 50, linear only
};
struct DimensionPresentation {    // how it shows
    std::string style{"ISO-25"}; Vec2 textPoint;  // 11
    std::string textOverride;     // 1
    double textRotation{0.0};     // 53
    bool userTextPosition{false}; // 70 bit 128
    // essential overrides, by variable name (PT22 fixes the value type)
    std::vector<std::pair<std::string, std::string>> overrides;
};
enum class ValueSource : std::uint8_t { Measured };
struct DimensionValue { ValueSource source{ValueSource::Measured};
                        double measurement{0.0}; };  // 42
struct Dimension {
    DimensionDefinition definition; DimensionPresentation presentation;
    DimensionValue value;
    std::vector<EntityRecord> graphics;  // the *D block, in order
    std::vector<RawEntity> rawGraphics;  // what of it the model does not handle
};

enum class IslandStyle : std::uint8_t { Normal, Outer, Ignore };
struct HatchEdge { std::variant<Line, Arc> curve; };   // a full circle is an Arc
struct HatchLoop {
    std::uint32_t flags{0};  // 92
    std::optional<Polyline> polyline;  // polyline loop, otherwise edges
    std::vector<HatchEdge> edges;
};
struct PatternLine { double angle{0.0}; Vec2 origin; Vec2 offset;
                     std::vector<double> dashes; };  // 53, 43/44, 45/46, 49
struct Hatch {
    std::string pattern; bool solid{false}; IslandStyle islands{IslandStyle::Normal};
    int patternType{1}; double angle{0.0}; double scale{1.0}; bool doubled{false};
    std::vector<PatternLine> patternLines; std::vector<Vec2> seeds;
    std::vector<HatchLoop> loops;
};
```

`EntityRecord` sta in `document.hpp`: `Dimension` lo usa, quindi
`dimension.hpp` include una dichiarazione anticipata o l'ordine dei file va
sistemato senza cicli; la scelta è libera purché le intestazioni compilino da
sole.

### 2. Tipi non ancora realizzati

```cpp
/// False for a type whose package is not merged yet (docs/adr/0044, point 1).
[[nodiscard]] bool isSupported(const Entity& entity);
```

Ogni tipo nuovo ha nel suo `.cpp` `bool supported(const T&)` che qui restituisce
`false`; il pacchetto del tipo la cambia in `true`. `Document::add`, `insert`,
`restore` e `replaceEntity` hanno la precondizione `isSupported`. Le funzioni
per tipo (punto 4) di un tipo non realizzato hanno la stessa precondizione.

### 3. Geometria generata

`generated.hpp`:

```cpp
struct FilledRegion {
    std::vector<Polyline> loops;   // closed, even-odd
    Transform2D transform;         // identity unless inside a non-uniform insert
};
using GeneratedShape = std::variant<Line, Arc, Circle, Ellipse, Polyline, Text,
                                    Solid, FilledRegion>;
struct GeneratedItem {
    GeneratedShape shape;             // in drawing coordinates (docs/adr/0044, point 2)
    EffectiveProperties properties;   // resolved
};
/// The exact image of a shape under an affine map (docs/adr/0044, point 2;
/// docs/adr/0052, point 2): circles and arcs become ellipses, a polyline with
/// arcs splits into lines and elliptical arcs, text is decomposed.
[[nodiscard]] std::vector<GeneratedShape> transformedShape(const GeneratedShape&,
                                                           const Transform2D&);
[[nodiscard]] std::vector<GeneratedItem> generate(const Document&, const EntityRecord&);
[[nodiscard]] BBox extent(const Document&, const EntityRecord&);
[[nodiscard]] double distance(const Document&, const EntityRecord&, const Vec2&);
[[nodiscard]] bool isInside(const Document&, const EntityRecord&, const BBox&);
[[nodiscard]] bool intersects(const Document&, const EntityRecord&, const BBox&);
void appendSnapPoints(const Document&, const EntityRecord&, const SnapModes&,
                      std::vector<SnapPoint>& out, const std::optional<Vec2>& from);
struct TransformOutcome { std::optional<Entity> entity; std::string refusal; };
[[nodiscard]] TransformOutcome transformedIn(const Document&, const Entity&,
                                             const Similarity&, MirrorText);
[[nodiscard]] GripResult moveGripIn(const Document&, const Entity&, std::size_t index,
                                    const Vec2& target);
```

- Per le cinque entità di oggi, ognuna di queste fa ciò che fanno le funzioni
  di oggi (`boundingBox`, `distance`, …), e `generate` dà l'entità stessa con
  `effectiveProperties`, `transformedIn` dà `transformed` e nessun rifiuto.
- Per i tipi nuovi chiamano le funzioni per tipo (punto 4). Le forme generate
  sono già nel sistema del disegno: `distance`, `isInside`, `intersects` e lo
  snap le trattano come le entità dello stesso tipo. `transformedShape` qui
  si dichiara e tratta le similitudini con `transformed`; la parte affine la
  realizzano PT48 (ellissi e archi), PT17 (testo) e PT16 (il resto).
- `extent` è l'ingombro di tutta la geometria dell'entità, compresa quella
  sui layer spenti (non quella sui congelati), come lo zoom estensioni di
  AutoCAD; `generate` invece dà solo ciò che si disegna. Così riaccendere un
  layer non chiede di ricalcolare l'indice (PT4, punto P2: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT4»).
- `Document` registra nell'indice spaziale `extent()` invece di
  `boundingBox()`, e `pickEntity`, `selectInRectangle` e `findSnap`
  (`document_query.cpp`) usano `distance`, `isInside`, `intersects` e
  `appendSnapPoints` del documento.

### 4. Funzioni per tipo

Ogni tipo nuovo `T` ha, nel suo file, le funzioni che le visite chiamano, qui
con un corpo che non è mai raggiunto (precondizione del punto 2):
`BBox boundingBox(const T&)` (solo per `Solid`, `Ellipse` e `AttributeDefinition`, che non
dipendono dal documento), `std::vector<GeneratedItem> generateOf(const
Document&, const EntityRecord&, const T&)`, `TransformOutcome transformedOf(const
Document&, const T&, const Similarity&, MirrorText)`, `std::vector<Vec2>
gripPointsOf(const T&)`, `GripResult moveGripOf(const Document&, const T&,
std::size_t, const Vec2&)`, `bool supported(const T&)`. Le visite comuni chiamano
solo queste, una riga per tipo.

### 5. Tabelle

- `TextStyleTable`, `DimStyleTable` e `BlockTable` nei loro file, vuote
  (`operator==` predefinito); `BlockDefinition` con il solo nome. I pacchetti
  PT11, PT22 e PT16 aggiungono i campi nei loro file.
- `Document`:

  ```cpp
  [[nodiscard]] const TextStyleTable& textStyles() const;
  [[nodiscard]] const DimStyleTable& dimStyles() const;
  [[nodiscard]] const BlockTable& blocks() const;
  void replaceTextStyles(TextStyleTable table);
  void replaceDimStyles(DimStyleTable table);
  /// Puts, replaces or (nullopt) removes the definition `name`; then
  /// re-indexes every entity for which dependsOnBlock() is true.
  void replaceBlock(std::string name, std::optional<BlockDefinition> definition);
  ```

  `bool dependsOnBlock(const Document&, const EntityRecord&, std::string_view
  name)` sta in `insert.cpp` e qui restituisce `false`.

### 6. Le altre visite

In `render`, `script`, `app` e `io` ogni visita ha la riga dei tipi nuovi:
`render` non disegna nulla (PT8), `script` scrive solo il tipo DXF (PT9), `io` non
li scrive mai (non ci sono). Nessun comportamento cambia.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>` nel codice nuovo (ADR 0044,
  punto 8).
- Dopo questo pacchetto le visite comuni e `document.*` non si toccano più per
  un tipo: chi realizza un tipo cambia solo i suoi file.
- File comuni: `core/CMakeLists.txt`, `tests/CMakeLists.txt`.

## Test richiesti

- `new_entities_test.cpp`: i sette tipi si costruiscono e si confrontano;
  `isSupported` è falso per loro e vero per i cinque di oggi.
- `generated_test.cpp`: per `Line`, `Arc`, `Circle`, `Polyline` e `Text`,
  `generate` dà un elemento con l'entità e le proprietà effettive; `extent`,
  `distance`, `isInside`, `intersects` e `appendSnapPoints` danno gli stessi
  risultati delle funzioni di oggi; `transformedIn` dà `transformed`.
- `generated_test.cpp`: per un inserimento (con PT16) la cui linea interna,
  da X=0 a X=10, sta su un layer spento, `extent` va da 0 a 10 e `generate`
  non la dà; con il layer congelato non entra neanche nell'ingombro.
- `document_test.cpp` (esistente): `replaceTextStyles`, `replaceDimStyles` e
  `replaceBlock` cambiano le tabelle; nessuna entità cambia.

## Fatto quando

- I sette tipi, le funzioni per tipo, la geometria generata e le tre tabelle
  esistono, e tutte le visite li trattano.
- I casi di verifica sono invariati; la CI è verde.
