# [0.8.1-PT22] `core`: nomi dei tipi e modello delle proprietà proprie; `ELLIPSE` e `SOLID`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT22]` (#…), agente codex, fascia forte
  (raggi e rapporto dell'ellisse)
- **Dipende da:** PT2, PT4, RT22
- **Modulo:** `core` (più `quick_select.cpp` di `cmd` e le domande
  `properties` e `set-property` di `script`)
- **Da leggere:** [#913](https://github.com/Matleyx78/tecnigrafo/issues/913);
  [ADR 0052](../../adr/0052-ellisse.md); [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punto 1 (le visite comuni); `core/include/tg/core/ellipse.hpp`,
  `core/include/tg/core/solid.hpp`, `cmd/src/quick_select.cpp`; le schede
  sorelle [PT23](PT23-proprieta-retini-quote.md),
  [PT24](PT24-proprieta-testi-inserimenti.md) e [PT25](PT25-pannello.md)

## Perché in `core`

Il pannello (PT25) è solo una vista: i nomi, i valori e le regole dei
cambiamenti stanno in `core`, così si verificano con le domande e
`-HATCHEDIT` (PT26) riusa le stesse funzioni. Il modello si divide in tre file
per gruppo di tipi, così PT23 e PT24 lavorano in parallelo.

## File

- `core/include/tg/core/entity_properties.hpp`,
  `core/src/entity_properties.cpp` (nuovi: nomi dei tipi, smistamento)
- `core/src/entity_properties_shapes.cpp` (nuovo: `ELLIPSE`, `SOLID`)
- `core/src/entity_properties_annotations.cpp`,
  `core/src/entity_properties_texts.cpp` (nuovi, con le funzioni di `HATCH` e
  `DIMENSION`, e di `MTEXT`, `ATTDEF` e `INSERT`, che per ora non danno
  proprietà: le scrivono PT23 e PT24)
- `core/CMakeLists.txt` (file comune)
- `cmd/src/quick_select.cpp` (usa `entityTypeName`)
- `script/src/property_questions.cpp`, `script/src/question_table.cpp` (file
  comune: le due righe)
- `tests/core/entity_properties_test.cpp` (nuovo), `tests/cmd/quick_select_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. Interfaccia:

   ```cpp
   // The DXF name of every alternative of core::Entity; a static_assert or a
   // test fails when an alternative has no name (#913).
   [[nodiscard]] std::string_view entityTypeName(const Entity& entity);

   enum class PropertyKind : std::uint8_t { Number, Distance, Angle, Point, Text, Choice, Flag };
   using PropertyValue = std::variant<double, Vec2, std::string, bool>;
   struct OwnProperty {
       std::string name;              // the names of docs/verifica/domande.md
       PropertyKind kind;
       PropertyValue value;           // angles in radians
       std::vector<std::string> choices;  // for Choice
       bool readOnly{false};
   };
   [[nodiscard]] std::vector<OwnProperty> ownProperties(const Document&, const EntityRecord&);
   struct PropertyChange {
       std::optional<Entity> entity;
       std::optional<EntityExtras> extras;  // when the change touches them (the hatch origin, PT23)
       std::string refusal;                 // empty when entity has a value
   };
   [[nodiscard]] PropertyChange withOwnProperty(const Document&, const EntityRecord&,
                                                std::string_view name, const PropertyValue&);
   ```

   Nessuna proprietà per i tipi di prima della 0.8: il pannello tiene per loro
   il gruppo *Geometry* di oggi.
2. **`ELLIPSE`** (`center`, `major-radius`, `minor-radius`, `ratio`,
   `start-angle`, `end-angle`, `~area`): `center` sposta; `major-radius` cambia
   la lunghezza dell'asse maggiore tenendo direzione e raggio minore;
   `minor-radius` il contrario; `ratio` (0 < r ≤ 1) tiene il maggiore; se il
   minore supera il maggiore gli assi si scambiano (REGOLE-NUMERICHE.md, sezione
   2, punto 7). Gli angoli sono quelli del palette di AutoCAD. L'area è quella
   dell'ellisse intera o del settore. **Da registrare** prima del pacchetto, in
   una seduta in AutoCAD: che cosa tiene il palette quando si cambia il raggio
   maggiore, e se gli angoli iniziale e finale sono parametri o angoli veri.
3. **`SOLID`** (`vertex1`…`vertex4`): ogni vertice si sposta da solo; il quarto
   uguale al terzo resta un triangolo.
4. Un valore non valido (raggio ≤ 0, rapporto fuori da (0, 1], numero non
   finito) dà `refusal` con il messaggio `Invalid value.` e nessuna entità.
5. `quickselect type=` e il filtro di `QSELECT` usano `entityTypeName`: i tipi
   della 0.8 si filtrano (#913).
6. `properties` e `set-property` (annunciate da PT2), con i valori nel formato
   del rapporto, angoli in gradi.

## Regole della tappa che valgono qui

- File comuni: `core/CMakeLists.txt`, `script/src/question_table.cpp`,
  `tests/CMakeLists.txt`.
- Gli angoli scritti in gradi passano da `reducedDegreesToRadians`
  ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 2).

## Test richiesti

- `entityTypeName` per ogni alternativa; le proprietà dell'ellisse intera e di
  un arco d'ellisse; il cambio del raggio maggiore sotto il minore; un rapporto
  0 rifiutato; i quattro vertici del `SOLID`.

## Fatto quando

- I casi di RT22 passano; la CI è verde.

## Prova a secco dei riferimenti

```text
# casi/proprieta-proprie/ellisse-solid/domande.tgq
E = ELLIPSE center=10,10 major-axis=20,0 ratio=0.5 start=0 end=360
? properties E -> …
? set-property E major-radius=30 -> …
? set-property E minor-radius=25 -> …
? set-property E ratio=0 -> invalid
S = SOLID vertex=0,0 vertex=10,0 vertex=0,10 vertex=10,10
? properties S -> …
? set-property S vertex2=20,0 -> …
? quickselect type=ELLIPSE -> E
```

- **Strumenti:** `properties`, `set-property` dell'onda 0 (PT2);
  `quickselect` su `main`. La sintassi delle definizioni di `ELLIPSE` e
  `SOLID` è quella del [rapporto](../../verifica/rapporto.md).
- **Dipendenze:** nessun comando.
- **Fonte:** la seduta «da registrare» del punto 2 per il raggio maggiore e
  gli angoli.
- **Risposte condivise:** `quickselect type=` con un tipo della 0.8 cambia
  risposta (oggi nessuna entità); nessun caso di oggi lo chiede (controllato:
  solo `type=LINE`, `LWPOLYLINE`, `CIRCLE`, `ARC`, `TEXT`).
