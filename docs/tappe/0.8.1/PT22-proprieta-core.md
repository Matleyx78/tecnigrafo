# [0.8.1-PT22] `core`: nomi dei tipi e modello delle proprietà proprie; `ELLIPSE` e `SOLID`

- **Tappa:** 0.8.1 — [piano](../0.8.1-completamenti.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8.1-RT22]` (#…), agente codex, fascia forte
  (raggi e rapporto dell'ellisse)
- **Dipende da:** PT2, PT4, RT22
- **Modulo:** `core` (più `quick_select.cpp` di `cmd`, le domande
  `properties` e `set-property` di `script` e la sostituzione con gli extras
  in `edit`)
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
- `edit/include/tg/edit/editor.hpp`, `edit/src/editor.cpp` (`replace` con gli
  extras)
- `script/src/property_questions.cpp`, `script/src/question_table.cpp` (file
  comune: le due righe)
- `tests/core/entity_properties_test.cpp` (nuovo), `tests/cmd/quick_select_test.cpp`,
  `tests/edit/editor_test.cpp`, `tests/CMakeLists.txt` (file comune)

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
   2, punto 7), anche se il palette di AutoCAD lo rifiuta
   ([DECISIONI.md](../../DECISIONI.md), 2026-11-22, «Esiti della seconda seduta
   in AutoCAD della 0.8.1», punto 2). `start-angle` ed `end-angle` sono
   **angoli veri**, come nel palette di AutoCAD, misurati dall'asse maggiore
   come il comando `ELLIPSE`: un angolo θ è il parametro
   `t = atan2(a · sin θ, b · cos θ)` (`a`, `b` i raggi), e scriverne uno cambia
   solo quel parametro. `~area` è quella dell'ellisse intera (`π · a · b`) o,
   per un arco, del **segmento** chiuso dalla corda, `a · b / 2 · (Δt − sin Δt)`
   con `Δt` l'ampiezza in parametro (punto 3 della stessa voce). Valori della
   seduta del 2026-11-22 (`autocad-proprieta`, [dxf-di-prova.md](dxf-di-prova.md)):
   raggio maggiore da 20 a 30 con il minore 10 → rapporto 0,3333; minore da 10
   a 5 → 0,25; l'arco 45°–135° di un'ellisse 20 × 10 ha i parametri 63,43° e
   116,57° e l'area 12,7295; con l'angolo iniziale a 30 il parametro diventa
   49,11° e l'area 25,377.
3. **`SOLID`** (`vertex1`…`vertex4`): ogni vertice si sposta da solo; il quarto
   uguale al terzo resta un triangolo.
4. Un valore non valido (raggio ≤ 0, rapporto fuori da (0, 1], numero non
   finito) dà `refusal` con il messaggio `Invalid value.` e nessuna entità.
5. `quickselect type=` e il filtro di `QSELECT` usano `entityTypeName`: i tipi
   della 0.8 si filtrano (#913).
6. `properties` e `set-property` (annunciate da PT2), con i valori nel formato
   del rapporto, angoli in gradi. Le due domande **non modificano** il
   documento delle domande, che è in sola lettura: `set-property` risponde con
   l'entità e le proprietà dopo la modifica, ma la domanda seguente sulla
   stessa entità vede quella di prima. Ciò che si verifica di una modifica sta
   nella risposta di `set-property`; il salvataggio e la riapertura del DXF
   modificato si provano con i test di unità (#927, [bloccante] 6).
7. **Applicare una `PropertyChange`** (verbale della revisione delle interfacce
   PT4, #927, [bloccante] 6). Oggi `Editor::replace` riceve solo l'entità e dà
   `false` se è identica: un cambiamento che tocca solo gli extras (`scale-z` da
   3 a 2, il gruppo 43 di PT24) o entità ed extras insieme (l'origine di un
   retino, famiglie e XDATA, PT23) non avrebbe un passo. In `edit`:

   ```cpp
   /// Replaces geometry and extras together, as one change of the open
   /// transaction: undo and redo restore both. Keeps identifier, handle, order
   /// and properties. False when the entity does not exist, when both are
   /// equal to the current ones, or when the type would change (as replace()).
   bool replace(core::EntityId id, core::Entity entity, core::EntityExtras extras);
   ```

   Si fa in `edit` con `Document::remove()` e `Document::restore()`, che tengono
   identificativo, extras e chiave d'ordine, **senza** toccare
   `core/include/tg/core/document.hpp`, che cambia PT27. Il pannello (PT25) e
   `-HATCHEDIT` (PT26) applicano una `PropertyChange` così: con `extras` usano
   questa funzione, senza la `replace` di oggi.

## Regole della tappa che valgono qui

- File comuni: `core/CMakeLists.txt`, `script/src/question_table.cpp`,
  `tests/CMakeLists.txt`.
- Gli angoli scritti in gradi passano da `reducedDegreesToRadians`
  ([REGOLE-NUMERICHE.md](../../REGOLE-NUMERICHE.md), sezione 2, punto 2).

## Test richiesti

- `entityTypeName` per ogni alternativa; le cinque ellissi di
  `autocad-proprieta` (raggio maggiore, raggio minore, angoli e area dell'arco,
  angolo iniziale cambiato); il cambio del raggio maggiore sotto il minore
  (assi scambiati, non rifiutato); un rapporto 0 rifiutato; i quattro vertici
  del `SOLID`.
- `Editor::replace` con gli extras: un cambiamento dei soli extras (il gruppo
  43 da 3 a 2) dà un passo, e `U` rimette il 3; entità ed extras cambiati
  insieme tornano insieme con `U` e con la ripetizione; handle, ordine e gli
  altri gruppi degli extras restano; entità ed extras uguali danno `false` e
  nessun passo.

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
- **Fonte:** `autocad-proprieta.dxf` e la sua cronologia (seduta del
  2026-11-22, righe `[a mano nel palette Proprietà]`): i valori dopo le
  modifiche si leggono con l'oracolo; il cambio del maggiore sotto il minore è
  una regola nostra (lo scambio), non di AutoCAD. RT22 aggiunge
  `autocad-proprieta` a `sorgenti.json`.
- **Risposte condivise:** `quickselect type=` con un tipo della 0.8 cambia
  risposta (oggi nessuna entità); nessun caso di oggi lo chiede (controllato:
  solo `type=LINE`, `LWPOLYLINE`, `CIRCLE`, `ARC`, `TEXT`).
