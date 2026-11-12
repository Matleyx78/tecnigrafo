# [0.8-PT41] `core` e `cmd`: funzioni trigonometriche di `robust`, seconda parte, ed eccezioni vuote

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** facile
- **Agente:** codex
- **Modello e impegno:** leggero; impegno medio
- **Riferimenti:** non servono: le funzioni nuove sbagliano al più di 2 ulp,
  quindi i casi restano uguali entro la tolleranza; la verifica è il confronto
  tra sistemi della CI
- **Dipende da:** PT1, PT40 (revisione delle interfacce),
  Matleyx78/solid-space#216. La richiesta diventa il pacchetto di solid-space
  che realizza `elementary.hpp`, quando la tappa `ss-0.3` di solid-space avrà i
  suoi pacchetti ([ADR 0030](../../adr/0030-solid-space-kernel-di-tecnigrafo.md),
  punto 13)
- **Modulo:** `core` (più le righe che lo usano in `cmd` e `render`)
- **Da leggere:** la scheda sorella [PT1](PT1-trigonometria-1.md) (la prima
  parte, `tg/core/math.hpp`); [#620](https://github.com/Matleyx78/tecnigrafo/issues/620)
  (aggiornamento in cima); [verifica/sistemi.md](../../verifica/sistemi.md),
  «Eccezioni»; [ADR 0044](../../adr/0044-entita-composte-e-geometria-generata.md),
  punto 8; l'[ADR 0016 di solid-space](https://github.com/Matleyx78/solid-space/blob/main/docs/adr/0016-tangente-arcocoseno-e-lunghezza-deterministici.md)
  (Decisione); in `third_party/solid-space` l'intestazione qui sotto; il [verbale della revisione PT40](https://github.com/Matleyx78/tecnigrafo/issues/735)
  (note non bloccanti per chi realizza)

## Intestazioni di solid-space che usa

Il contratto accettato con l'ADR 0016 di solid-space e rivisto in
[`[0.3-CS]` #222](https://github.com/Matleyx78/solid-space/issues/222), al
commit [`fd69e80`](https://github.com/Matleyx78/solid-space/commit/fd69e807d4ebafe12f73f46ba26072a46ff72809)
di `main` di solid-space (2026-11-12, #231):

- `solid_space/robust/elementary.hpp` (`tan`, `acos`, `hypot`,
  `kElementaryMaxErrorUlps`, `kMaxHypotArgument`), con `trigonometry.hpp`
  (`kMaxTrigArgument`) che già usa PT1.

I commenti di `elementary.hpp` dicono precondizioni, errori e simmetrie: la
scheda li cita e non li ripete.

## Prima di cominciare

- **Sottomodulo.** Il pacchetto sposta `third_party/solid-space` al commit
  della riga «Sottomodulo di solid-space» della issue: quello che chiude il
  pacchetto di solid-space che realizza `elementary.hpp` (oggi la richiesta
  Matleyx78/solid-space#216; il piano lo sostituisce con il pacchetto quando
  la tappa `ss-0.3` lo avrà). Le funzioni di `elementary.hpp` sono dichiarate e
  non ancora realizzate: si usano solo dopo che quel pacchetto è unito
  ([contratto.md](../../agenti/contratto.md)). Se la riga manca, fermati.
- Prima della pull request confronta `elementary.hpp` al commit di destinazione
  con quello di `fd69e80` ([contratto.md](../../agenti/contratto.md), «Prima
  della pull request, se sposti il sottomodulo»).

## File

- `third_party/solid-space` (il sottomodulo, spostato al commit della issue)
- `core/include/tg/core/math.hpp`, `core/src/math.cpp` (di PT1)
- `core/include/tg/core/vec2.hpp` (`Vec2::length`)
- in `core/src`: `arc_geometry.cpp`, `curve_parts.cpp`, `grips.cpp`,
  `join.cpp`, `text.cpp`
- in `cmd/src`: `command_runner.cpp`, `commands/pline_command.cpp`
- ogni altro file di `core`, `cmd` e `render` in cui, quando il pacchetto parte,
  c'è ancora una chiamata a `std::tan`, `std::acos` o `std::hypot` (anche nel
  codice arrivato con gli altri pacchetti della tappa); ogni modifica è di una
  riga
- `docs/verifica/sistemi.md`
- `tests/core/math_test.cpp` (di PT1), `tests/CMakeLists.txt` (file comune, se
  serve)

## Cosa fare

1. In `tg/core/math.hpp`, accanto alle funzioni di PT1:

   ```cpp
   /// Tangent of an angle in radians, the same bits on every platform
   /// (solid_space::robust::tan, docs/adr/0044, point 8). Angles beyond
   /// robust's range are first reduced as sinCos() reduces them (std::fmod by
   /// 2*pi). Precondition: finite angle.
   [[nodiscard]] double tan(double angle);
   /// Arccosine in [0, pi], robust::acos. Precondition: -1 <= x <= 1.
   [[nodiscard]] double acos(double x);
   /// Length of (x, y), robust::hypot: exact when the length is a double,
   /// never below max(|x|, |y|), the same bits for (x, y), (y, x), (-x, y).
   /// Beyond robust's range (an argument above kMaxHypotArgument; at the
   /// limit robust answers, with a finite result) both arguments are halved
   /// and the result doubled, which may give +inf (halving a subnormal may
   /// round it, far below the error of the result); with an infinite
   /// argument the result is +inf, with a NaN and no infinity it is NaN, as
   /// std::hypot.
   [[nodiscard]] double hypot(double x, double y);
   ```

   La riduzione dell'angolo di `tan` è la stessa di `sinCos` di PT1, in una
   funzione interna a `math.cpp` che usano tutte e due.
2. `Vec2::length()` chiama `tg::core::hypot(x, y)`; `vec2.hpp` include
   `tg/core/math.hpp`, che non dipende da `Vec2`.
3. In `core`, `cmd` e `render` ogni chiamata a `std::tan`, `std::acos` e
   `std::hypot` passa a quelle di `tg::core` (in `core` si può chiamare
   `robust` direttamente, come fanno `fillet.cpp` e `curve_parts.cpp`, ma per
   `tan` l'angolo deve stare nella precondizione). `std::log10`, `std::fmod`,
   `std::remainder` e `std::sqrt` restano: non sono trigonometriche, o IEEE 754
   le arrotonda in modo esatto (#620, aggiornamento in cima).
4. **Precondizioni dei chiamanti.** L'argomento di `acos` in
   `arc_geometry.cpp` (`tessellate`) sta già in [0, 1): la tolleranza è
   limitata al raggio prima della divisione. Se un'altra chiamata può dare un
   argomento fuori da [−1, 1] o non finito, il chiamante lo porta
   nell'intervallo prima (come fa oggi `tessellate`) e la pull request lo
   dice; il comportamento non cambia per i dati che oggi danno un numero.
5. **Eccezioni.** Il job del confronto dà errore per un'eccezione il cui caso è
   ormai uguale su tutti i sistemi. Togli da
   [verifica/sistemi.md](../../verifica/sistemi.md) ogni riga che il job
   segnala così e riscrivi il paragrafo sopra l'elenco: le due parti di #620
   sono fatte, l'elenco è vuoto. Se un caso resta diverso anche dopo questo
   pacchetto, la causa non è la trigonometria (#620, punto 4): lascia la sua
   riga con la causa che hai trovato, scrivila nella pull request e
   **fermati** prima del verde come per un buco visibile della scheda
   ([pacchetto.md](../../agenti/pacchetto.md), «Buchi visibili e no»): il
   criterio di chiusura della tappa vuole l'elenco vuoto, e come procedere lo
   decide il responsabile.

## Regole della tappa che valgono qui

- Dopo questo pacchetto, in `core`, `cmd` e `render` nessuna funzione
  trigonometrica di `<cmath>` (ADR 0044, punto 8): chi arriva dopo usa quelle
  di `tg/core/math.hpp`.
- File comune: `tests/CMakeLists.txt` (solo le tue righe, in ordine
  alfabetico; chi arriva dopo si aggiorna).

## Test richiesti

- `math_test.cpp`: `tan`, `acos` e `hypot` di `tg::core` danno gli stessi bit
  di `robust` per una decina di argomenti; `tan` di un angolo oltre `2^20` dà
  lo stesso risultato dell'angolo ridotto; `acos(1) == 0`, `acos(-1)` è il
  `double` più vicino a π; `hypot(3, 4) == 5`, `hypot(x, 0) == |x|`,
  `hypot(x, y) == hypot(y, -x)` bit per bit; `hypot(2^1023, 2^1023)`, al
  limite di `kMaxHypotArgument`, è finito (circa `1.2711610061536464e308`) e
  ha gli stessi bit di `robust::hypot`; `hypot(DBL_MAX, DBL_MAX)`, oltre il
  limite, è `+inf`; `hypot(inf, NaN)` è `+inf`, `hypot(1, NaN)` è NaN;
  `Vec2{3, 4}.length() == 5`.

## Fatto quando

- In `core`, `cmd` e `render` non ci sono più chiamate a `std::tan`,
  `std::acos`, `std::hypot`, né a `std::sin`, `std::cos`, `std::atan2` e
  `std::atan` (`grep` nella pull request).
- I casi di verifica passano, il confronto tra sistemi non ha errori e
  l'elenco delle eccezioni di [verifica/sistemi.md](../../verifica/sistemi.md)
  è vuoto.
- La CI è verde.
