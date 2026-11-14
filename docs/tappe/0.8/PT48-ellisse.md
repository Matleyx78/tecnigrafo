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
   scambiano. Gli angoli con `core::atan2` e `std::sqrt`. **Verso degli
   assi** (PT4, punto P12: [DECISIONI.md](../../DECISIONI.md), 2026-11-14,
   «Revisione delle interfacce PT4»): quando il risultato è un cerchio
   (rapporto 1 esatto) l'asse maggiore va lungo X positivo; negli altri casi si
   sceglie il verso con l'angolo dell'asse maggiore in [0°, 180°); i
   parametri si riportano su quell'asse.
4. **Ingombro** esatto con i valori estremi in X e in Y (gli angoli dove la
   derivata si annulla, compresi solo se stanno nell'arco, più gli estremi).
5. **Snap** *Centro*, *Quadrante* (i quattro estremi degli assi che stanno
   sull'arco), *Estremo* (per un arco); **grip** sul centro (sposta) e sugli
   estremi degli assi: il grip di un asse cambia solo la lunghezza di quel
   semiasse, uguale alla distanza dal centro al punto, e l'orientamento resta;
   se il minore supera il maggiore, o il maggiore scende sotto il minore, gli
   assi si scambiano (il rapporto resta in (0, 1]); a rapporto 1 esatto vale
   il verso del punto 3 (asse su X positivo); un arco tiene i suoi estremi
   negli stessi punti angolari, con i parametri riportati sui nuovi assi; un
   semiasse nullo si rifiuta (PT4, punto P14;
   [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Secondo giro delle
   revisioni PT4 e PT5»).
   *Intersezione* arriva dopo il contratto (PT50); *Vicino* resta fuori dalla
   0.8 per tutte le entità (idea [#665](https://github.com/Matleyx78/tecnigrafo/issues/665)).
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
  passa per un estremo; snap e grip: asse `(4,0)`, rapporto 0,5, il grip del
  minore portato in `(1,5)` dà un minore di lunghezza `√26` lungo Y, quindi
  assi scambiati (maggiore `(0,√26)`, rapporto `4/√26`); il grip del maggiore
  portato da `(4,0)` a `(1,0)` dà maggiore `(0,2)` e rapporto 0,5, a `(2,0)`
  un cerchio con l'asse `(2,0)`, anche partendo da un'ellisse inclinata; un'ellisse d'asse
  `(0,2)` e rapporto 0,5 sotto scala `(2, 1)` dà un cerchio con l'asse
  maggiore `(2,0)`; un'ellisse con asse maggiore a 200° dopo una rotazione si
  scrive con l'asse a 20°.
- `dxf_ellipse_test.cpp`: giro di un'ellisse intera e di un arco, con
  estrusione `(0,0,-1)`.

## Fatto quando

- I casi di RT48 passano; la CI è verde.
