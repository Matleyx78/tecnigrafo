# [0.6-P2] `core`: colori, spessori e definizioni dei tipi di linea

- **Tappa:** 0.6 — [piano](../0.6-layer-e-proprieta.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: tipi di valore senza effetti osservabili; la
  tabella dei colori la verificano R9 e i tipi ISO R13, con le risposte di ezdxf
- **Dipende da:** —
- **Modulo:** `core`
- **Da leggere:** [ADR 0024](../../adr/0024-colori.md) (tutto),
  [ADR 0025](../../adr/0025-tipi-di-linea-e-spessori.md) punti 1, 2 e 6;
  `core/include/tg/core/vec2.hpp` (stile dei tipi di valore)

## File

- `core/include/tg/core/color.hpp`, `core/src/color.cpp` (nuovi)
- `core/include/tg/core/lineweight.hpp`, `core/src/lineweight.cpp` (nuovi)
- `core/include/tg/core/linetype_pattern.hpp`, `core/src/linetype_pattern.cpp` (nuovi)
- `core/CMakeLists.txt`
- `tests/core/color_test.cpp`, `tests/core/lineweight_test.cpp`,
  `tests/core/linetype_pattern_test.cpp` (nuovi)
- `tests/CMakeLists.txt`

## Cosa fare

### Colori (`color.hpp`)

```cpp
struct Rgb {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    friend constexpr bool operator==(const Rgb&, const Rgb&) = default;
};

/// A colour of an entity or a layer (docs/adr/0024, point 1).
class Color {
public:
    enum class Kind : std::uint8_t { ByLayer, ByBlock, Index, Rgb };

    [[nodiscard]] static constexpr Color byLayer();
    [[nodiscard]] static constexpr Color byBlock();
    [[nodiscard]] static Color fromIndex(int index);  ///< precondition: 1..255
    [[nodiscard]] static constexpr Color fromRgb(Rgb rgb);

    [[nodiscard]] Kind kind() const;
    [[nodiscard]] int index() const;  ///< precondition: kind() == Index
    [[nodiscard]] Rgb rgb() const;    ///< precondition: kind() == Rgb

    friend constexpr bool operator==(const Color&, const Color&) = default;
};

/// The RGB of an AutoCAD Color Index, 1..255 (docs/adr/0024, point 3).
[[nodiscard]] Rgb aciToRgb(int index);

/// The index 1..255, 7 excluded, nearest to `rgb` (docs/adr/0024, point 4).
[[nodiscard]] int nearestAci(Rgb rgb);

/// "bylayer", "byblock", "1".."255", "255,128,0" (docs/adr/0024, point 6).
[[nodiscard]] std::string formatColor(const Color& color);

/// Reads every spelling of docs/adr/0024, point 6, ignoring case and the
/// blanks around the text; std::nullopt for anything else ("0,0", "256,0,0",
/// "300", "rosso").
[[nodiscard]] std::optional<Color> parseColor(std::string_view text);
```

La tabella si scrive come dati (255 righe), con un commento che cita l'ADR
0024: colori 1–9, tinte 10–249 (24 tinte ogni 15°, cinque luminosità e due
saturazioni), grigi 250–255. I valori li controlla R9 con le risposte di
ezdxf: se non coincidono, si correggono qui in un pacchetto successivo.

### Spessori (`lineweight.hpp`)

```cpp
namespace lineweight {
inline constexpr int kByLayer = -1;
inline constexpr int kByBlock = -2;
inline constexpr int kDefault = -3;
/// The values allowed by DXF group 370, in hundredths of a millimetre.
inline constexpr std::array<int, 24> kAllowed{0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50,
                                              53, 60, 70, 80, 90, 100, 106, 120, 140, 158,
                                              200, 211};
}  // namespace lineweight

[[nodiscard]] bool isAllowedLineweight(int value);  ///< kAllowed or one of the three specials
/// The allowed value nearest to `value` (>= 0); on a tie the smaller one.
[[nodiscard]] int nearestLineweight(int value);
/// "bylayer", "byblock", "default", or millimetres: "0.25", "0", "2.11".
[[nodiscard]] std::string formatLineweight(int value);
/// "ByLayer", "ByBlock", "Default" (any case) or millimetres ("0.25", ".25",
/// "0.25mm"), rounded to the nearest allowed value only if within 0.005 mm;
/// std::nullopt otherwise.
[[nodiscard]] std::optional<int> parseLineweight(std::string_view text);
```

### Motivi dei tipi di linea (`linetype_pattern.hpp`)

```cpp
/// docs/adr/0025, point 1: > 0 dash, < 0 gap, 0 dot, in drawing units.
struct LinetypePattern {
    std::vector<double> elements;

    [[nodiscard]] bool continuous() const { return elements.empty(); }
    /// Sum of the absolute values.
    [[nodiscard]] double length() const;

    friend bool operator==(const LinetypePattern&, const LinetypePattern&) = default;
};

struct LinetypeDefinition {
    std::string name;
    std::string description;
    LinetypePattern pattern;
};

/// The definitions Tecnigrafo can load (docs/adr/0025, point 2), by name,
/// ignoring case; std::nullopt for any other name.
[[nodiscard]] std::optional<LinetypeDefinition> standardLinetype(std::string_view name);

/// The names of the standard definitions, in alphabetical order.
[[nodiscard]] std::span<const std::string_view> standardLinetypeNames();
```

Definizioni (le verifica R13 con quelle di ezdxf):

| Nome | Descrizione | Motivo |
|---|---|---|
| `ACAD_ISO02W100` | `ISO dash __ __ __ __ __ __ __ __ __ __ __ __ __` | 12, −3 |
| `ACAD_ISO10W100` | `ISO dash dot __ . __ . __ . __ . __ . __ . __` | 12, −3, 0, −3 |

## Regole della tappa che valgono qui

- `core` non usa Qt né librerie esterne.
- `core/CMakeLists.txt` lo toccano anche P4, P9 e P11, dopo: solo le tue righe.

## Test richiesti

- `Color`: le quattro specie, uguaglianza, precondizioni.
- `aciToRgb`: 1, 7, 10, 11, 249, 250, 255; nessun valore uguale tra 1 e 9.
- `nearestAci`: un RGB esatto di ogni gruppo dà il suo indice (salvo 7, che dà
  255 per il bianco); a parità l'indice minore.
- `formatColor` e `parseColor`: ogni scrittura, maiuscole, spazi, errori.
- Spessori: ogni valore ammesso, arrotondamenti e parità, testo nei due sensi.
- Motivi: lunghezza, continuo, i due tipi standard, nomi in minuscolo.

## Fatto quando

- I test qui sopra esistono e passano.
- La CI è verde.
