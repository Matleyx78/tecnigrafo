# [0.8-PT48] `core` e `io`: entità `ELLIPSE`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT48]` (#…), agente codex, fascia forte
- **Dipende da:** PT4 (revisione delle interfacce), PT8, PT9, RT48
- **Modulo:** `core` (più il codec in `io`)
- **Da leggere:** [ADR 0052](../../adr/0052-ellisse.md) per intero;
  [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md), punto 2;
  [ADR 0018](../../adr/0018-conservazione-dati-non-gestiti.md), punti 3–5;
  `core/include/tg/core/arc.hpp`, `core/src/arc.cpp` (come è fatto l'arco);
  `io/src/dxf_arc.cpp`; la scheda [PT6](PT6-scheletro-core.md), punti 1, 3 e 4

## File

- `core/include/tg/core/ellipse.hpp`, `core/src/ellipse.cpp`
- `core/src/generated.cpp` (la parte di `transformedShape` per cerchi, archi ed
  ellissi)
- `io/src/dxf_ellipse.cpp` (nuovo), `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp` (file comuni)
- `tests/core/ellipse_test.cpp`, `tests/io/dxf_ellipse_test.cpp` (nuovi),
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. `supported(const Ellipse&)` restituisce `true` se l'asse maggiore non è
   nullo, il rapporto è in (0, 1] e i parametri sono finiti.
2. **Punti:** `Vec2 pointAt(const Ellipse&, double t)` = centro `+ cos t · a +
   sin t · b`, con `a` = asse maggiore e `b` = `a` ruotato di +90° per il
   rapporto (ADR 0052, punto 1), con `core::sinCos`. Ellisse intera: parametri
   0 e 2π.
3. **Trasformazioni esatte** (ADR 0052, punto 2), una sola funzione per tutti:

   ```cpp
   /// The ellipse (or elliptical arc) that the affine map `m` makes of `e`,
   /// exactly up to rounding: new axes from the singular value decomposition
   /// of m·[a b], start and end parameters shifted to the new major axis; a
   /// reflection reverses the direction of the parameters (start and end swap).
   [[nodiscard]] Ellipse transformed(const Ellipse& e, const Transform2D& m);
   /// A circle or an arc as an ellipse with ratio 1 (start/end = arc angles).
   [[nodiscard]] Ellipse asEllipse(const Circle&);
   [[nodiscard]] Ellipse asEllipse(const Arc&);
   ```

   Con rapporto 1 dopo la trasformazione (una similitudine), il risultato resta
   un'`Ellipse`: `transformedShape` (PT6) dà invece un `Circle` o un `Arc` per
   cerchi e archi sotto una similitudine, come oggi. Se dopo una trasformazione
   il rapporto uscisse di poco sopra 1 per l'arrotondamento, gli assi si
   scambiano. Gli angoli con `core::atan2` e `std::sqrt`.
4. **Ingombro** esatto con i valori estremi in X e in Y (gli angoli dove la
   derivata si annulla, compresi solo se stanno nell'arco, più gli estremi).
5. **Snap** *Centro*, *Quadrante* (i quattro estremi degli assi che stanno
   sull'arco), *Estremo* (per un arco); **grip** sul centro e sugli estremi
   degli assi (uno sposta, gli altri cambiano assi e rapporto come AutoCAD).
   *Vicino* e *Intersezione* arrivano dopo il contratto (PT50).
6. **Selezione** fino a PT50: con la finestra se l'ingombro ci sta tutto
   dentro; con un clic e con l'attraversamento della finestra no (la distanza
   restituisce l'infinito). È il limite scritto nell'ADR 0052, punto 4.
7. **Codec DXF** `ELLIPSE`: 10/20/30, 11/21/31, 40, 41, 42, estrusione come
   l'ADR 0018, punto 5 (con `(0,0,-1)` centro e asse si specchiano in X e i
   parametri si invertono), il resto in `extras`; un'ellisse non supportata
   (punto 1) resta grezza.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- File comuni: `io/CMakeLists.txt`, `io/src/dxf_entities.cpp`,
  `io/src/dxf_entity_common.hpp`, `tests/CMakeLists.txt`.

## Test richiesti

- `ellipse_test.cpp`: `pointAt` ai quattro quadranti; `transformed` con una
  rotazione, una scala `(2, 1)` di un cerchio (ellisse di rapporto 0,5), una
  scala `(1, 2)` (assi scambiati), una riflessione di un arco d'ellisse
  (parametri), una deformazione con taglio; ingombro di un arco che passa e non
  passa per un estremo; snap e grip.
- `dxf_ellipse_test.cpp`: giro di un'ellisse intera e di un arco, con
  estrusione `(0,0,-1)`.

## Fatto quando

- I casi di RT48 passano; la CI è verde.
