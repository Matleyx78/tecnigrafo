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
  `coordinate_input.cpp`, `drawing_aids.cpp`, `dynamic_input.cpp`
- `render/src/scene_renderer.cpp`
- `docs/verifica/sistemi.md`
- `core/CMakeLists.txt` (file comune: una riga)
- `tests/core/math_test.cpp` (nuovo), `tests/CMakeLists.txt` (file comune)

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
4. **Eccezioni.** La CI dà errore per un'eccezione il cui caso è ormai uguale su
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
  `sin(0) == 0`, `cos(0) == 1`.

## Fatto quando

- In `core`, `cmd` e `render` non ci sono più chiamate a `std::sin`, `std::cos`,
  `std::atan2` e `std::atan` (`grep` nella pull request).
- I casi di verifica passano e il confronto tra sistemi non ha errori; l'elenco
  delle eccezioni ha solo le righe ancora diverse.
- La CI è verde.
