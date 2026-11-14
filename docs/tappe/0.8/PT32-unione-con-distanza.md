# [0.8-PT32] `core`: unione con distanza e inversione delle polilinee

- **Tappa:** 0.8 — [piano](../0.8-quote-retini-blocchi.md)
- **Issue:** #…
- **Difficoltà:** media
- **Agente:** claude
- **Modello e impegno:** medio; impegno alto
- **Riferimenti:** issue `[0.8-RT32]` (#…), agente codex, fascia forte
- **Dipende da:** PT5 (revisione delle interfacce), RT32
- **Modulo:** `core`
- **Da leggere:** [ADR 0050](../../adr/0050-pedit.md);
  [ADR 0035](../../adr/0035-taglio-offset-raccordi-unione.md) (unione e
  prolungamento); [ADR 0031](../../adr/0031-modifiche-alle-entita.md), punto 7;
  `core/include/tg/core/join.hpp`, `core/src/join.cpp`,
  `core/include/tg/core/intersections.hpp`, `core/include/tg/core/tolerance.hpp`,
  `core/src/solid_space_bridge.hpp`; la scheda [PT33](PT33-comando-pedit.md)
  (sorella)

## File

- `core/include/tg/core/pedit_join.hpp`, `core/src/pedit_join.cpp` (nuovi)
- `core/include/tg/core/polyline.hpp`, `core/src/polyline.cpp` (`reversed`,
  `toPolyline`)
- `tests/core/pedit_join_test.cpp` (nuovo), `tests/core/polyline_test.cpp`,
  `tests/CMakeLists.txt` (file comune)

## Cosa fare

1. **Conversione:** `Polyline toPolyline(const Line&)`, `Polyline
   toPolyline(const Arc&)` (un segmento, il bulge dell'arco).
2. **Inversione:** `Polyline reversed(const Polyline&)` (ADR 0050, punto 3):
   vertici in ordine opposto; il bulge del segmento `i → i+1` diventa quello
   del segmento opposto, con segno cambiato; larghezze iniziale e finale di
   ogni segmento scambiate e spostate allo stesso modo; una polilinea chiusa
   resta chiusa con lo stesso primo vertice. In una polilinea aperta i dati
   dell'ultimo vertice (bulge e larghezze, che non hanno segmento) passano al
   nuovo ultimo vertice, così due inversioni ridanno la polilinea di partenza
   (ADR 0006; PT5, punto B26).
3. **Unione con distanza:**

   ```cpp
   enum class JoinType : std::uint8_t { Extend, Add, Both };
   struct GapJoinResult {
       Polyline polyline;               // the joined source
       std::vector<std::size_t> consumed;  // indices of `others` used
       std::size_t added{0};            // segments added, bridges included
   };
   /// Joins `others` (lines, arcs, open polylines) to the ends of `source`
   /// (docs/adr/0050, point 4). fuzz == 0: the rule of join() (docs/adr/0035).
   [[nodiscard]] GapJoinResult joinWithGaps(const Polyline& source,
                                            std::span<const Entity> others,
                                            double fuzz, JoinType type,
                                            const ToleranceRule& rule);
   ```

   - Si parte dalla fine della polilinea, poi dall'inizio: a ogni passo, fra gli
     oggetti non usati, quello con un estremo alla distanza minima ≤ `fuzz` da
     quell'estremità (a pari distanza il primo in `others`; un oggetto con i
     due estremi alla stessa distanza si attacca dal suo inizio); lo si gira se
     serve; si continua finché nessuno è entro `fuzz`. Un oggetto che coincide
     secondo `rule` (distanza zero) passa prima della graduatoria per `fuzz`,
     anche quando `fuzz` è più piccolo della tolleranza (PT5, punto B21:
     [DECISIONI.md](../../DECISIONI.md), 2026-11-14, «Revisione delle interfacce PT5»).
   - **Distanza zero** fra gli estremi (secondo `rule`): si attacca come `join`.
   - ***Extend*:** l'ultimo segmento della polilinea e il primo dell'oggetto si
     prolungano o si accorciano fino alla loro intersezione sulle curve estese
     (`geom2d::intersect` con `Extension::extended`, attraverso
     `solid_space_bridge`), la più vicina ai due estremi (somma delle distanze;
     a pari somma la prima che solid-space dà). Il punto d'intersezione arriva
     da solid-space e si usa così com'è, come nuovo estremo dei due segmenti
     (ADR 0031, punto 10). Un arco che si prolunga cambia angolo, non centro né
     raggio; il suo bulge si ricalcola. Senza un punto d'intersezione isolato
     (nessuna intersezione, o le due curve si sovrappongono in un tratto) la
     coppia non si unisce e si prova l'oggetto seguente nella graduatoria. Un
     segmento che si prolunga tiene la larghezza del suo estremo, senza
     prolungarne la variazione (larghezze 1→3 prolungate: 3) (PT5, punti B22 e
     B24).
   - ***Add*:** un segmento dritto fra i due estremi, larghezza zero.
   - ***Both*:** *Extend* se c'è un punto d'intersezione isolato, altrimenti
     *Add* (anche per due curve che si sovrappongono).
   - `added` conta i segmenti che entrano nella polilinea, ponti compresi: una
     sorgente più un oggetto di due segmenti e un ponte dà 3 (PT5, punto B25);
     `consumed` è nell'ordine di aggancio.
   - Se dopo un'unione l'estremità libera della polilinea coincide con l'altra
     (secondo `rule`), la polilinea diventa chiusa e l'unione finisce.
4. Le polilinee chiuse fra gli `others` non si uniscono; neanche quelle con
   quota diversa dalla sorgente.

## Regole della tappa che valgono qui

- Le decisioni geometriche sono di solid-space; nessuna soglia propria tranne
  `fuzz`, che è un valore dell'utente.
- Niente funzioni trigonometriche di `<cmath>`.
- File comune: `tests/CMakeLists.txt`.

## Test richiesti

- `polyline_test.cpp`: `reversed` con archi e larghezze, aperta e chiusa.
- `pedit_join_test.cpp`: due linee a 1 di distanza con `fuzz` 2 nei tre tipi;
  linee che si sovrappongono (*Extend* accorcia); arco seguito da una linea;
  linee parallele (con *Extend* nessuna unione, con *Both* un segmento);
  chiusura; `fuzz` 0 come `join`; sorgente `(-4,0)→(0,0)` e candidato
  `(1,0)→(3,0)` con `fuzz` 2: con *Extend* nessuna unione, con *Both* un
  ponte; un primo candidato parallelo e un secondo che si unisce con *Extend*:
  si unisce il secondo; un candidato `(-1,0)→(1,0)` con i due estremi alla
  stessa distanza: si attacca dal suo inizio; larghezze 1→3 prolungate di una
  lunghezza: larghezza finale 3; `added` 3 con un ponte e un oggetto di due
  segmenti; doppia inversione di una polilinea aperta con dati sull'ultimo
  vertice.

## Fatto quando

- I casi di RT32 passano; la CI è verde.
