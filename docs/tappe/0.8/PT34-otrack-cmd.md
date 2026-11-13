# [0.8-PT34] `cmd` e `script`: tracciamento degli snap a oggetto e `!hover`

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT34]` (#…), agente codex
- **Dipende da:** RT34
- **Modulo:** `cmd` (più la direttiva e le righe del rapporto in `script`)
- **Da leggere:** [ADR 0049](../../adr/0049-tracciamento-degli-snap-a-oggetto.md);
  [ADR 0033](../../adr/0033-nuovi-snap-e-tracciamento-polare.md), punti 8–12;
  [ADR 0014](../../adr/0014-richieste-di-valori-e-testo.md) (distanza scritta);
  `cmd/include/tg/cmd/drawing_aids.hpp`, `cmd/src/drawing_aids.cpp`,
  `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`;
  [verifica/script.md](../../verifica/script.md) e
  [verifica/rapporto.md](../../verifica/rapporto.md); `script/src/script.cpp`,
  `script/src/level1.cpp`, `script/src/report.cpp`; la scheda
  [PT35](PT35-otrack-app.md) (sorella)

## File

- `cmd/include/tg/cmd/drawing_aids.hpp`, `cmd/src/drawing_aids.cpp`
- `cmd/include/tg/cmd/command_runner.hpp`, `cmd/src/command_runner.cpp`
- `cmd/src/object_tracking.cpp`, `cmd/src/object_tracking.hpp` (nuovi)
- `script/src/script.cpp`, `script/src/level1.cpp`, `script/src/report.cpp`
- `docs/verifica/script.md`, `docs/verifica/rapporto.md`
- `tests/cmd/object_tracking_test.cpp` (nuovo), `tests/cmd/command_runner_test.cpp`,
  `tests/script/…`, `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Stato.** `DrawingAids::tracking` (`bool`, spento). Il runner:

   ```cpp
   /// The pointer stays on `point` long enough to acquire or release a snap
   /// point (docs/adr/0049, point 2). Same as pointerMoved() otherwise.
   void pointerHeld(const core::Vec2& point);
   struct TrackingState {
       std::vector<core::Vec2> acquired;   // oldest first, at most 7
       std::optional<core::Vec2> guidePoint;  // the point the guides give
       enum class Guide : std::uint8_t { None, Horizontal, Vertical, Crossing,
                                         WithPolar } guide{Guide::None};
       std::optional<core::Vec2> from;     // the acquired point of a single guide
   };
   [[nodiscard]] const TrackingState& tracking() const;
   ```

2. **Acquisizione** (ADR 0049, punto 2): `pointerHeld` durante una richiesta di
   punto, con tracciamento e snap a oggetto accesi, su uno snap attivo: lo
   aggiunge (l'ottavo toglie il più vecchio) o, se c'è già (stesso punto), lo
   toglie. I punti si azzerano quando la richiesta riceve un punto, finisce o
   cambia comando.
3. **Aggancio** (punti 3–5), con l'apertura in unità del disegno che il runner
   usa già per il polare (10 pixel): per ogni punto acquisito la guida
   orizzontale (`y = py`) e la verticale (`x = px`); il puntatore entro
   l'apertura da due guide di punti diversi va all'incrocio esatto
   (`x` dell'una, `y` dell'altra); entro una sola, si proietta su di essa; fra
   più guide vince la più vicina, a pari distanza la più recente; fra
   l'orizzontale e la verticale dello stesso punto acquisito, che hanno la
   stessa età, vince l'orizzontale, come l'orto a 45° esatti (ADR 0008,
   punto 5; [DECISIONI.md](../../DECISIONI.md), 2026-11-13, «Pareggio fra le
   guide dello stesso punto»). Due guide di punti diversi entro l'apertura
   danno l'incrocio, quindi non arrivano a questo pareggio. Con il polare
   acceso e un punto base, conta anche la semiretta polare, e il suo incrocio
   con una guida si chiede a solid-space con `core::intersections` sulle rette
   estese. Priorità: snap a oggetto, incrocio, guida, polare, orto, puntatore.
4. **Distanza scritta** (punto 6): con una guida singola attiva, una distanza
   scritta mette il punto a quella distanza da `from` lungo la guida, verso il
   puntatore.
5. **Script** (punto 7): `!hover x,y` (livelli 1 e 2) chiama `pointerHeld`;
   `!otrack on` / `!otrack off` cambiano lo stato senza scrivere nella
   cronologia. In [script.md](../../verifica/script.md) le due righe nuove.
6. **Rapporto** (punto 8), righe nuove dopo `active snap:`, solo quando diverse
   da una sessione nuova (rapporto.md, «Come cresce il formato»):
   - `tracking: on`;
   - `acquired:` con un punto per riga, dal più vecchio;
   - `active tracking:` `Horizontal x,y from x,y`, `Vertical …`,
     `Crossing x,y`, `Polar crossing x,y`.

## Regole della tappa che valgono qui

- Niente funzioni trigonometriche di `<cmath>`.
- `rapporto.md` lo toccano solo PT9 e PT34: chi arriva dopo si aggiorna.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `object_tracking_test.cpp`: acquisire e rilasciare; l'ottavo punto; guida
  orizzontale; incrocio di due guide; guida più vicina; incrocio con il polare;
  distanza scritta lungo una guida; azzeramento dopo un punto.
- Pareggio fra le guide dello stesso punto (punto 3): disegno con il solo
  segmento da `(-100,0)` a `(0,0)`, snap solo sugli estremi, orto e polare
  spenti, apertura di 10 unità; acquisito `(0,0)`, il puntatore in `(8,8)` dà
  `(8,0)`, con `active tracking: Horizontal 8,0 from 0,0`; negli altri tre
  quadranti, in `(-8,8)`, `(-8,-8)` e `(8,-8)`, dà `(-8,0)`, `(-8,0)` e
  `(8,0)`. Senza pareggio vince la più vicina: in `(7,8)` la verticale,
  `(0,8)` con `Vertical 0,8 from 0,0`.
- Test di `script`: `!hover` e `!otrack`, e le righe del rapporto.

## Fatto quando

- I casi di RT34 passano; i casi esistenti sono invariati; la CI è verde.
