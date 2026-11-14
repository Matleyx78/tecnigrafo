# [0.8-PT8] `render` e `app`: geometria generata e riempimenti

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno medio
- **Riferimenti:** non servono: si vede solo nelle immagini; test con il pittore
  di prova e prova manuale
- **Dipende da:** PT4 (revisione delle interfacce), PT6
- **Modulo:** `render` (più l'implementazione del pittore in `app`)
- **Da leggere:** [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punti 2 e 7; la scheda [PT6](PT6-scheletro-core.md), punto 3;
  `render/include/tg/render/painter.hpp`, `render/src/scene_renderer.cpp`,
  `render/src/linework.cpp`, `app/src/qt_painter.hpp`, `app/src/qt_painter.cpp`

## File

- `render/include/tg/render/painter.hpp`
- `render/src/scene_renderer.cpp`, `render/src/linework.cpp`,
  `render/src/linework.hpp`
- `render/src/generated_drawing.cpp`, `render/src/generated_drawing.hpp` (nuovi)
- `app/src/qt_painter.hpp`, `app/src/qt_painter.cpp`
- `tests/render/…` (il pittore di prova e i test della scena), `tests/CMakeLists.txt`
  (file comune)

## Cosa fare

1. **Riempimento di più anelli.** `Painter` ha

   ```cpp
   /// Fills the area enclosed by `loops` (screen pixels) with the even-odd
   /// rule, without outline: islands stay empty, islands of islands are filled.
   virtual void fillLoops(std::span<const std::vector<core::Vec2>> loops,
                          const Color& color) = 0;
   ```

   `QtPainter` lo realizza con un `QPainterPath` con `Qt::OddEvenFill`; il
   pittore di prova dei test registra le chiamate.
2. **Disegno della geometria generata.** Per ogni entità visibile la scena
   chiama `core::generate` e disegna ogni elemento con le sue proprietà
   effettive:
   - `Line`, `Arc`, `Circle`, `Polyline` e `Text` come oggi (tipo di linea e
     spessore compresi), dopo la trasformazione dell'elemento;
   - `Solid` con `fillPolygon` sul quadrilatero `v0, v1, v3, v2` (ADR 0044,
     punto 7);
   - `Ellipse` come spezzata calcolata sullo schermo, con lo stesso passo
     angolare degli archi, tipo di linea compreso (ADR 0052, punto 3);
   - `FilledRegion` con `fillLoops`, gli anelli resi come spezzate dopo la sua
     trasformazione (un retino dentro un inserimento stirato).
   Le forme arrivano già nel sistema del disegno (PT6, punto 3): niente altre
   affinità da applicare. Limite della 0.8 (PT4, punto P8: [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT4»):
   in un inserimento stirato il tipo di linea si calcola in lunghezza su ogni
   forma generata e il motivo riparte da capo a ogni forma, anche dove una
   polilinea con `PLINEGEN` si divide in linee e archi d'ellisse; sotto una
   scala non uniforme le larghezze di una polilinea si perdono su tutti i
   pezzi, dritti ed ellittici ([DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Secondo giro delle revisioni PT4 e PT5»). L'impronta completa è nell'idea
   #762.
   Per le cinque entità di oggi il disegno non cambia: `generate` le dà così
   come sono.
3. Gli attributi invisibili e le entità sui layer spenti o congelati non
   arrivano dalla geometria generata (PT6): la scena non li filtra di nuovo.
4. Niente funzioni trigonometriche di `<cmath>` (ADR 0044, punto 8).

## Regole della tappa che valgono qui

- `scene_renderer.cpp` e `linework.*` dopo questo pacchetto non si toccano per
  un tipo nuovo: i tipi arrivano tutti dalla geometria generata.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- Pittore di prova: una `FilledRegion` di un quadrato con un quadrato dentro
  chiama `fillLoops` con due anelli; un `Solid` chiama `fillPolygon` con i
  quattro vertici nell'ordine `v0, v1, v3, v2`; un'`Ellipse` di rapporto 0,5 è
  una spezzata i cui punti stanno sull'ellisse; una `FilledRegion` con
  trasformazione `scaling(2, 1)` riempie anelli allargati in X.
- Evidenziazione e anteprima di un inserimento disegnano le stesse forme
  generate della scena, con il colore della selezione.
- Una polilinea con un arco e larghezza 2, in un inserimento con scala
  `(2, 1)`: tutti i pezzi, dritti ed ellittici, arrivano senza larghezza; il
  motivo tratteggiato riparte all'inizio di ogni pezzo.
- Il disegno di un documento con le cinque entità di oggi dà le stesse chiamate
  di prima.
- Un test Qt: `fillLoops` di due quadrati annidati lascia vuoto il centro in un
  `QImage`.

## Fatto quando

- La scena disegna la geometria generata, con riempimenti ed ellissi.
- I test qui sopra passano; la galleria di schermate non cambia; la CI è verde.
