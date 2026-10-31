# [0.7-P21] `cmd`: `CHAMFER`, con lo smusso in `core`

- **Tappa:** 0.7 — [piano](../0.7-modifiche.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** codex
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.7-R21]` (#…), agente gemini (valori da
  `autocad-raccordi.dxf`; lo smusso è un calcolo di Tecnigrafo)
- **Dipende da:** P20, R21
- **Modulo:** `cmd` (più lo smusso in `core` e la domanda `chamfer` in `script`)
- **Da leggere:** [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md)
  punti 1, 15, 16, 21 e 22; [ADR 0032](../../adr/0032-richieste-di-oggetti-e-sequenze-degli-script.md)
  punto 8; `core/include/tg/core/fillet.hpp` (P19: `CornerResult`),
  `core/include/tg/core/intersections.hpp` (P14),
  `core/include/tg/core/curve_parts.hpp` (P15);
  `cmd/src/commands/fillet_command.cpp` (P20, lo stesso schema)

## File

- `core/include/tg/core/chamfer.hpp`, `core/src/chamfer.cpp` (nuovi)
- `core/CMakeLists.txt`
- `cmd/include/tg/cmd/commands/chamfer_command.hpp`,
  `cmd/src/commands/chamfer_command.cpp` (nuovi)
- `cmd/include/tg/cmd/command.hpp` (`CommandDefaults`: distanze, lunghezza,
  angolo, metodo)
- `cmd/src/builtin_commands.cpp`, `cmd/src/command_descriptions.cpp`,
  `cmd/CMakeLists.txt`
- `script/src/curve_questions.cpp`, `script/src/question_table.cpp`
- `tests/core/chamfer_test.cpp`, `tests/cmd/chamfer_command_test.cpp` (nuovi),
  `tests/CMakeLists.txt`
- `tests/verifica/in-attesa/0.7-R21.txt` (da cancellare)

## Cosa fare

### Lo smusso (`chamfer.hpp`)

```cpp
struct ChamferDistances { double first{0.0}; double second{0.0}; };
struct ChamferAngle { double length{0.0}; double angle{0.0}; };  ///< on the first; radians
using ChamferSpec = std::variant<ChamferDistances, ChamferAngle>;

/// CHAMFER of two lines, or two straight segments of one polyline, picked at
/// the given points (docs/adr/0035, point 15). An arc, a circle or a text:
/// NotAllowed. Parallel or collinear lines: None.
[[nodiscard]] CornerResult chamfer(const Entity& first, const Vec2& pickFirst,
                                   const Entity& second, const Vec2& pickSecond,
                                   const ChamferSpec& spec, bool trim);
/// The same on two segments of one polyline (as filletSegments).
[[nodiscard]] CornerResult chamferSegments(const Polyline& polyline, const Vec2& pickFirst,
                                           const Vec2& pickSecond, const ChamferSpec& spec);
/// CHAMFER Polyline: every vertex between two straight segments.
[[nodiscard]] PolylineCorners chamferPolyline(const Polyline& polyline, const ChamferSpec& spec);
```

- **Lo spigolo** è il punto comune delle due rette: `intersect` con
  `Extension::Extended` sulle due linee (P14). Nessun punto, o un tratto in
  comune: `None`.
- **La parte da tenere** di ciascuna linea è quella dalla parte dello spigolo
  dove cade la proiezione del punto indicato (`nearestPosition` sulla retta):
  è la scelta dell'utente, non una decisione geometrica.
- **Gli estremi dello smusso** stanno alle distanze date dallo spigolo, lungo
  la parte tenuta; con *Angle*, il primo alla lunghezza data, il secondo dove
  la retta che parte dal primo con quell'angolo dalla prima linea incontra la
  seconda. È una costruzione di Tecnigrafo (ADR 0035, punto 15).
- Una distanza che supera la parte tenuta: `None`. Distanze entrambe 0: lo
  spigolo, senza linea nuova.
- Le linee si portano agli estremi dello smusso con `partOf` (P15); con `trim`
  falso non cambiano. La linea nuova va in `added`.
- La domanda `chamfer` del catalogo, con le risposte scritte lì.

### `CHAMFER` (`CHA`)

Prima il messaggio `(TRIM mode) Current chamfer Dist1 = <d1>, Dist2 = <d2>` (o
`Length = <l>, Angle = <a>` con il metodo dell'angolo), poi:

```
Select first line or [Undo/Polyline/Distance/Angle/Trim/mEthod/Multiple]:
Select second line or shift-select to apply corner or [Distance/Angle/Method]:
Specify first chamfer distance <{default}>:
Specify second chamfer distance <{default}>:
Specify chamfer length on the first line <{default}>:
Specify chamfer angle from the first line <{default}>:
Enter trim method [Distance/Angle] <{default}>:
Select 2D polyline or [Distance/Angle/Method]:
```

- Stesso schema di `FILLET` (P20): richieste di oggetto, `Shift` per lo
  spigolo, *Multiple*, *Undo*, le due sequenze (negli script il comando finisce
  dopo *Distance*, *Angle*, *mEthod* e *Trim*), le proprietà della linea nuova
  (ADR 0035, punto 16).
- `NotAllowed` (un arco) e `None` danno il messaggio inglese di AutoCAD (da
  controllare, ADR 0035 punto 22), e la domanda resta.
- Un passo di annulla `Chamfer`.

## Regole della tappa che valgono qui

- `builtin_commands.cpp`, `command_descriptions.cpp`, `cmd/CMakeLists.txt`:
  solo le tue righe; prima di te P20, dopo di te P23.
- `core/CMakeLists.txt`, `question_table.cpp`, `curve_questions.cpp`: file
  comuni, solo le tue righe (li toccano anche P14, P17, P19 e P22).
- Lo spigolo e la sua esistenza vengono da `intersections.hpp`: nessuna soglia
  propria per "parallele".

## Test richiesti

- I casi di `autocad-raccordi.dxf` per `CHAMFER`, nelle due sequenze.
- Distanze, angolo, *mEthod*, *Polyline*, *Trim No*, `Shift`, un arco,
  parallele, distanza troppo lunga, *Multiple*, *Undo*; annulla e ripeti.

## Fatto quando

- I casi di `[0.7-R21]` passano e il loro file è cancellato da
  `tests/verifica/in-attesa/`.
- La CI è verde.
