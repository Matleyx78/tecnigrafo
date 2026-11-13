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
   danno l'incrocio, quindi non arrivano a questo pareggio. Fra due incroci
   alla stessa distanza vince quello che usa la guida orizzontale del punto
   più recente; se l'orizzontale è dello stesso punto, quello che usa la
   verticale del punto più recente ([DECISIONI.md](../../DECISIONI.md),
   2026-11-13, «Pareggio fra due incroci»). Con il polare
   acceso e un punto base, conta anche la semiretta polare, e il suo incrocio
   con una guida si chiede a solid-space con `core::intersections` sulle rette
   estese. Priorità: snap a oggetto, incrocio, guida, polare, orto, puntatore.
   A pari distanza fra l'incrocio di due guide acquisite e l'incrocio di una
   guida con la semiretta polare vince il primo, come nella priorità
   ([DECISIONI.md](../../DECISIONI.md), 2026-11-13, «Pareggio fra un incrocio
   di guide e uno con il polare»). Le distanze si confrontano come le calcola
   il programma in `double`: un pareggio che nasce da una differenza di un ulp
   è un dato senza senso pratico ([revisione.md](../../agenti/revisione.md),
   «Che cosa blocca una revisione») e vale la regola del pareggio.
4. **Distanza scritta** (punto 6): con una guida singola attiva, una distanza
   scritta mette il punto a quella distanza da `from` lungo la guida, verso il
   puntatore.
5. **Script** (punto 7): `!hover x,y` (livelli 1 e 2) chiama `pointerHeld`;
   `!otrack on` / `!otrack off` cambiano lo stato senza scrivere nella
   cronologia. Le loro righe in [script.md](../../verifica/script.md) e
   l'annuncio in `kAnnouncedDirectives` li porta prima
   [F8](../fuori-tappa/F8-annuncio-hover-otrack.md) (#760), perché RT34 possa
   usarle: PT34 toglie `hover` e `otrack` da `kAnnouncedDirectives` e dal
   paragrafo di script.md sulle direttive non ancora realizzate.
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
- Pareggio fra due incroci (punto 3): disegno con il solo segmento da `(0,0)`
  a `(18,18)`, snap solo sugli estremi, orto e polare spenti, apertura di 10
  unità; acquisiti `(0,0)` e poi `(18,18)`, il puntatore in `(9,9)` è a 9 da
  tutte e quattro le guide e dà `(0,18)`, con `active tracking: Crossing
  0,18`; acquisiti nell'ordine inverso, dà `(18,0)`. Stessa orizzontale:
  segmenti da `(50,0)` a `(50,-20)` e da `(14,30)` a `(26,30)`, acquisiti
  nell'ordine `(50,0)`, `(14,30)`, `(26,30)`, il puntatore in `(20,5)` ha gli
  incroci `(14,0)` e `(26,0)` a `√61` e dà `(26,0)`; scambiando le ultime
  due acquisizioni, `(14,0)`.
- Pareggio fra un incrocio di guide e uno con il polare (punto 3): segmento
  da `(40,0)` a `(0,40)`, snap solo sugli estremi, polare acceso con
  `POLARANG` 90°, apertura di 10 unità; `LINE` con il primo punto
  `(-20,10)`, acquisiti `(40,0)` e `(0,40)`, il puntatore in `(2,5)` è entro
  l'apertura da `y=0`, da `x=0` e dalla semiretta polare `y=10`; gli incroci
  `(0,0)` e `(0,10)` sono tutti e due a `√29` e vince `(0,0)`, con
  `active tracking: Crossing 0,0`.
- Test di `script`: `!hover` e `!otrack`, e le righe del rapporto.

## Fatto quando

- I casi di RT34 passano; i casi esistenti sono invariati; la CI è verde.
