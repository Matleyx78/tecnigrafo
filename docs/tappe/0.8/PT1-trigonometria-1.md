# [0.8-PT1] `core`, `cmd`, `render`: funzioni trigonometriche di `robust`, prima parte

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** codex
- **Modello e impegno:** leggero; impegno medio
- **Riferimenti:** non servono: le funzioni nuove sbagliano al più di 2 ulp,
  quindi i casi restano uguali entro la tolleranza; la verifica è il confronto
  tra sistemi della CI
- **Dipende da:** —
- **Modulo:** `core` (più le righe che lo usano in `cmd` e `render`)
- **Da leggere:** [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
  (aggiornamento in cima); [verifica/sistemi.md](../../verifica/sistemi.md),
  «Eccezioni»; [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punto 8; `third_party/solid-space/robust/include/solid_space/robust/trigonometry.hpp`;
  `core/include/tg/core/math.hpp`; l'uso che ne fanno già `core/src/fillet.cpp`
  e `core/src/curve_parts.cpp`; la scheda sorella, che avrà la seconda parte
  dopo il contratto (PT41 nel piano)

## File

- `core/include/tg/core/math.hpp`, `core/src/math.cpp` (nuovo)
- in `core/src`: `arc.cpp`, `arc_geometry.cpp`, `chamfer.cpp`, `dashes.cpp`,
  `intersections.cpp`, `polyline.cpp`, `proximity.cpp`, `similarity.cpp`,
  `snap_points.cpp`, `text.cpp`, `transform2d.cpp`
- in `cmd/src`: `commands/arc_command.cpp`, `commands/pline_command.cpp`,
  `commands/rotate_scale_commands.cpp`, `commands/text_command.cpp`,
  `command_runner.cpp`, `coordinate_input.cpp`, `drawing_aids.cpp`,
  `dynamic_input.cpp`
- `render/src/scene_renderer.cpp`
- `docs/verifica/sistemi.md`
- `core/CMakeLists.txt` (file comune: una riga)
- `tests/core/math_test.cpp` (nuovo), `tests/core/similarity_test.cpp`,
  `tests/CMakeLists.txt` (file comune)
- in `tests/cmd`: `coordinate_input_test.cpp`, `rotate_scale_commands_test.cpp`,
  `text_command_test.cpp`

Più di una quindicina di file, ma ogni modifica è di una riga (piano, «Motivi»).

## Cosa fare

1. `core` collega `solid_space::robust` in privato, quindi `cmd` e `render` non
   lo vedono: `core` offre le funzioni in `tg/core/math.hpp`.

   ```cpp
   /// Sine and cosine of an angle in radians, the same bits on every
   /// platform (solid_space::robust::sinCos, docs/adr/0044, point 8).
   /// Angles beyond robust's range are first reduced with std::fmod by 2*pi.
   struct SinCos { double sine{0.0}; double cosine{1.0}; };
   [[nodiscard]] SinCos sinCos(double angle);
   [[nodiscard]] double sin(double angle);   ///< sinCos(angle).sine
   [[nodiscard]] double cos(double angle);   ///< sinCos(angle).cosine
   /// robust::atan2; precondition: finite arguments.
   [[nodiscard]] double atan2(double y, double x);
   /// atan2(x, 1.0): the same bits on every platform.
   [[nodiscard]] double atan(double x);
   /// Below this magnitude a component of cardinalSinCos is taken as 0:
   /// above the rounding left by a quarter turn given as a double (at most
   /// about 2^-50 for angles up to a few turns), far below the relative
   /// tolerance 2^-42 of solid_space::robust::TolerancePolicy.
   inline constexpr double kCardinalSnap = 0x1p-48;
   /// sinCos(angle), except that a component below kCardinalSnap in
   /// magnitude becomes exactly 0 and the other exactly +1 or -1, with its
   /// sign: quarter turns are exact (docs/DECISIONI.md, 2026-11-12, "Quarti
   /// di giro esatti"). For rotations and directions, not for evaluating
   /// sine and cosine as numbers.
   ///
   /// Band (solid-space ADR 0013, point 4): an angle within kCardinalSnap of
   /// a quarter turn is taken as that quarter turn. A point at distance r
   /// from the centre then moves by at most about r * 2^-48, so the matrix
   /// and the angle kept by a Similarity, a composition and its two
   /// applications, a reflection and the axis given by two points agree
   /// within the tolerance, not bit for bit. Beyond a few tens of radians a
   /// quarter turn may not be recognised: the result is then sinCos. Angles
   /// given in degrees stay within that range when they enter through
   /// reducedDegreesToRadians.
   [[nodiscard]] SinCos cardinalSinCos(double angle);
   /// An angle given in degrees, reduced to [0, 360) with std::fmod (exact;
   /// a negative remainder gets 360 added, and a result that rounds to 360
   /// or is -0 becomes +0), then converted with degreesToRadians: whatever
   /// its number of turns, a multiple of 90 degrees stays a quarter turn for
   /// cardinalSinCos (docs/DECISIONI.md, 2026-11-12, "Angoli in gradi
   /// ridotti a un giro").
   [[nodiscard]] double reducedDegreesToRadians(double degrees);
   ```

   Precondizione: argomenti finiti (come oggi: un angolo non finito è già un
   errore a monte).
2. In `core`, `cmd` e `render` ogni chiamata a `std::sin`, `std::cos`,
   `std::atan2` e `std::atan` passa a quelle di `tg::core` (in `core` si può
   chiamare `robust` direttamente, come fanno `fillet.cpp` e `curve_parts.cpp`).
   `std::tan`, `std::acos`, `std::hypot` e `std::log10` restano: le prime tre
   cambiano con la seconda parte, dopo il contratto di solid-space; `log10`
   non è trigonometrica.
3. Dove serviva `sin` e `cos` dello stesso angolo, una sola chiamata a
   `sinCos`.
4. **Quarti di giro esatti** ([DECISIONI.md](../../DECISIONI.md), 2026-11-12,
   «Quarti di giro esatti»): `Similarity::matrix` (`similarity.cpp`) prende
   coseno e seno con `cardinalSinCos(m_angle)`, per la rotazione e per la
   riflessione, così `ROTATE` e `MIRROR` di qualunque entità di un multiplo
   di 90° (riflessione su un asse a un multiplo di 45°) danno coordinate
   esatte, per gli angoli digitati (punto 5) e per quelli che arrivano entro
   la fascia dichiarata nel commento di `cardinalSinCos`; un angolo calcolato
   oltre quella fascia dà il risultato di `sinCos`, come prima di questa
   tappa. Il resto del punto 2 usa `sinCos`. I casi di verifica non cambiano:
   la differenza è sotto ogni tolleranza.
5. **Angoli digitati in gradi** ([DECISIONI.md](../../DECISIONI.md),
   2026-11-12, «Angoli digitati ridotti a un giro»): entrano con
   `reducedDegreesToRadians` il valore di una richiesta `PromptKind::Angle`
   (`command_runner.cpp`, `typedPoint`: oggi `spec->a * (kPi / 180)`; vale
   per l'angolo di `ROTATE`, quelli della sua opzione *Reference* e la
   rotazione di `TEXT`) e l'angolo di una coordinata polare `d<a`
   (`coordinate_input.cpp`, `polar`: oggi `degreesToRadians`, che con
   `1<1e308` dà infinito e romperebbe la precondizione di `sinCos`). Così un
   multiplo di 90° digitato resta un quarto di giro esatto qualunque sia il
   numero di giri, e nessun angolo finito digitato diventa infinito.
   **Rotazione di `TEXT`** ([DECISIONI.md](../../DECISIONI.md), 2026-11-13,
   «Rotazione di TEXT in [0°, 360°)»): il valore che `TEXT` propone alla
   domanda successiva e quello che salva nell'entità (gruppo DXF 50) stanno
   sempre in [0°, 360°), qualunque sia il percorso d'ingresso, come AutoCAD.
   Il numero digitato ci arriva già dal runner; l'angolo dato con un punto
   (`text_command.cpp`, `Stage::Rotation`: oggi `atan2`, in (−180°, 180°])
   diventa `reducedDegreesToRadians(radiansToDegrees(atan2(…)))`, così `-90`
   digitato e il punto a −90° danno gli stessi bit di `270`, e un angolo che
   arrotonda al giro intero, o −0, dà +0. Le entità lette da un DXF tengono la
   loro rotazione (ADR 0006): la regola vale per il comando.
6. **Eccezioni.** La CI dà errore per un'eccezione il cui caso è ormai uguale su
   tutti i sistemi. Togli da [verifica/sistemi.md](../../verifica/sistemi.md)
   le righe che il job del confronto segnala così; lascia le altre, che
   aspettano la seconda parte, e aggiorna il paragrafo sopra l'elenco: la prima
   parte è fatta, la seconda è PT41.

## Regole della tappa che valgono qui

- Codice nuovo di `core`, `cmd` e `render`: niente funzioni trigonometriche di
  `<cmath>` (ADR 0044, punto 8).
- File comuni: `core/CMakeLists.txt`, `tests/CMakeLists.txt` (solo le tue righe,
  in ordine alfabetico; chi arriva dopo si aggiorna).

## Test richiesti

- `math_test.cpp`: `sinCos` e `atan2` di `tg::core` danno gli stessi bit di
  `robust` per una decina di angoli; un angolo oltre `2^20` dà lo stesso
  risultato dell'angolo ridotto; `atan(x) == atan2(x, 1)` bit per bit;
  `sin(0) == 0`, `cos(0) == 1`;
- `math_test.cpp`, `cardinalSinCos`: a `π/2` (il `double`), `π`, `3π/2`,
  `−π/2` e `π/2 + 2π` dà esattamente `(0, 1)`, `(−1, 0)`, `(0, −1)`,
  `(0, −1)`, `(0, 1)` come `(cosine, sine)`; a 0 dà `(1, 0)`; a `π/2 − 2^-50`
  dà `(0, 1)`; a `π/4`, a `π/2 − 2^-30` e a `π/2 − 2^-46` dà gli stessi bit di
  `sinCos`;
- `math_test.cpp`, `reducedDegreesToRadians`: 3600, 360, 0 e −0 danno `+0`;
  3690, −270 e 90 danno gli stessi bit di `degreesToRadians(90)`; −3600 dà
  `+0`; `−1e-20` dà `+0` (il resto più 360 arrotonda a 360);
  `cardinalSinCos(reducedDegreesToRadians(3690))` è esattamente `(0, 1)`;
- `rotate_scale_commands_test.cpp`, digitando come testo attraverso il runner:
  `ROTATE` di una linea da `(0, 0)` a `(16777216, 0)` attorno a `(0, 0)` con
  90, 3600090 e −270: l'estremo va esattamente in `(0, 16777216)` nei tre
  casi; con 3600000: l'estremo resta esattamente in `(16777216, 0)`; con
  *Reference* 3600000 e nuovo angolo 3600090: lo stesso quarto di giro;
- `text_command_test.cpp`, digitando come testo attraverso il runner, con
  inizio `0,0` e altezza `2.5`: alla rotazione `-90`, e in un secondo `TEXT`
  il punto `0,-10`; in tutti e due i casi l'entità creata ha rotazione con
  gli stessi bit di `reducedDegreesToRadians(270)` e il `TEXT` successivo
  chiede `Specify rotation angle of text <270>:`; con `360` digitato, e con i
  punti `10,-0` e `10,-1e-20`, la rotazione salvata è `+0` e la domanda
  successiva propone `<0>`;
- `coordinate_input_test.cpp`: `1<1e308` dà un punto finito (nessuna
  asserzione), uguale a `1<` dell'angolo `fmod(1e308, 360)`; `100<3690` dà lo
  stesso punto di `100<90`;
- `similarity_test.cpp`: rotazione di
  +90° attorno a `(0, 0)`: `(0, 10)` va esattamente in `(−10, 0)` e `(10, 0)`
  in `(0, 10)`; lo stesso con −90° e 180°, e con +90° attorno a `(5, 5)`;
  riflessione sull'asse a 45°: `(1, 0)` va esattamente in `(0, 1)`;
- `similarity_test.cpp`, la fascia (confronti con
  `TolerancePolicy::coincident`, disegno in millimetri, `P = (2^24, 0)`):
  due rotazioni di `3 · 2^-50` attorno all'origine, ciascuna dentro la soglia
  e la somma fuori, e due rotazioni di `π/2 − 2^-47` e `2^-47`, ciascuna fuori
  e la somma dentro: `(a * b).apply(P)` e `a.apply(b.apply(P))` coincidono;
  lo stesso con gli angoli di segno opposto; una linea e un arco di raggio
  `2^24` centrato nell'origine con estremo comune in `P`, ruotati insieme di
  `3 · 2^-50`: gli estremi restano coincidenti; `MIRROR` sull'asse da
  `(0, 0)` a `(2^24, 2^24 + 2^-27)`, data nei due ordini: i due punti
  dell'asse restano coincidenti con se stessi e le due riflessioni danno
  punti coincidenti.

## Fatto quando

- In `core`, `cmd` e `render` non ci sono più chiamate a `std::sin`, `std::cos`,
  `std::atan2` e `std::atan` (`grep` nella pull request).
- I casi di verifica passano e il confronto tra sistemi non ha errori; l'elenco
  delle eccezioni ha solo le righe ancora diverse.
- La CI è verde.
